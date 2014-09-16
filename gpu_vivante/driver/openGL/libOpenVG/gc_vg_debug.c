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
#if vgvENABLE_ENTRY_SCAN
#include <string.h>
#endif
/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

#if vgvENABLE_ENTRY_SCAN

#	define vgmAPINAME(Name) \
		# Name

	typedef enum _vgeAPITYPE
	{
		vgvAPITYPE_GENERIC     = (1 <<  0),
		vgvAPITYPE_DRAW        = (1 <<  1),
		vgvAPITYPE_CONSTRUCTOR = (1 <<  2),
		vgvAPITYPE_DESTRUCTOR  = (1 <<  3),
		vgvAPITYPE_STATE       = (1 <<  4),
		vgvAPITYPE_GETTER      = (1 <<  5),
		vgvAPITYPE_SETTER      = (1 <<  6),
		vgvAPITYPE_MATRIX      = (1 <<  7),
		vgvAPITYPE_MASK        = (1 <<  8),
		vgvAPITYPE_PATH        = (1 <<  9),
		vgvAPITYPE_PATH_MOD    = (1 << 10),
		vgvAPITYPE_PAINT       = (1 << 11),
		vgvAPITYPE_IMAGE       = (1 << 12),
		vgvAPITYPE_TEXT        = (1 << 13),
		vgvAPITYPE_FILTER      = (1 << 14),
	}
	vgeAPITYPE;

	typedef struct _vgsAPI_INFO * vgsAPI_INFO_PTR;
	typedef struct _vgsAPI_INFO
	{
		gctSTRING	name;
		vgeAPITYPE	type;
	}
	vgsAPI_INFO;

	static vgsAPI_INFO _apiInfo[] =
	{
		/* Generic calls. */
		{ vgmAPINAME(vgGetError),               vgvAPITYPE_GENERIC },
		{ vgmAPINAME(vgFlush),                  vgvAPITYPE_GENERIC },
		{ vgmAPINAME(vgFinish),                 vgvAPITYPE_GENERIC },
		{ vgmAPINAME(vgHardwareQuery),          vgvAPITYPE_GENERIC },
		{ vgmAPINAME(vgGetString),              vgvAPITYPE_GENERIC },

		/* Getters and setters. */
		{ vgmAPINAME(vgSetf),                   vgvAPITYPE_STATE | vgvAPITYPE_SETTER },
		{ vgmAPINAME(vgSeti),                   vgvAPITYPE_STATE | vgvAPITYPE_SETTER },
		{ vgmAPINAME(vgSetfv),                  vgvAPITYPE_STATE | vgvAPITYPE_SETTER },
		{ vgmAPINAME(vgSetiv),                  vgvAPITYPE_STATE | vgvAPITYPE_SETTER },

		{ vgmAPINAME(vgGetf),                   vgvAPITYPE_STATE | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgGeti),                   vgvAPITYPE_STATE | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgGetVectorSize),          vgvAPITYPE_STATE | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgGetfv),                  vgvAPITYPE_STATE | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgGetiv),                  vgvAPITYPE_STATE | vgvAPITYPE_GETTER },

		{ vgmAPINAME(vgSetParameterf),          vgvAPITYPE_STATE | vgvAPITYPE_SETTER },
		{ vgmAPINAME(vgSetParameteri),          vgvAPITYPE_STATE | vgvAPITYPE_SETTER },
		{ vgmAPINAME(vgSetParameterfv),         vgvAPITYPE_STATE | vgvAPITYPE_SETTER },
		{ vgmAPINAME(vgSetParameteriv),         vgvAPITYPE_STATE | vgvAPITYPE_SETTER },

		{ vgmAPINAME(vgGetParameterf),          vgvAPITYPE_STATE | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgGetParameteri),          vgvAPITYPE_STATE | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgGetParameterVectorSize), vgvAPITYPE_STATE | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgGetParameterfv),         vgvAPITYPE_STATE | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgGetParameteriv),         vgvAPITYPE_STATE | vgvAPITYPE_GETTER },

		/* Matrix manipulation. */
		{ vgmAPINAME(vgLoadIdentity),           vgvAPITYPE_MATRIX },
		{ vgmAPINAME(vgLoadMatrix),             vgvAPITYPE_MATRIX },
		{ vgmAPINAME(vgGetMatrix),              vgvAPITYPE_MATRIX },
		{ vgmAPINAME(vgMultMatrix),             vgvAPITYPE_MATRIX },
		{ vgmAPINAME(vgTranslate),              vgvAPITYPE_MATRIX },
		{ vgmAPINAME(vgScale),                  vgvAPITYPE_MATRIX },
		{ vgmAPINAME(vgShear),                  vgvAPITYPE_MATRIX },
		{ vgmAPINAME(vgRotate),                 vgvAPITYPE_MATRIX },

		/* Masking and clearing. */
		{ vgmAPINAME(vgCreateMaskLayer),        vgvAPITYPE_MASK | vgvAPITYPE_DRAW | vgvAPITYPE_CONSTRUCTOR },
		{ vgmAPINAME(vgDestroyMaskLayer),       vgvAPITYPE_MASK                   | vgvAPITYPE_DESTRUCTOR  },
		{ vgmAPINAME(vgMask),                   vgvAPITYPE_MASK | vgvAPITYPE_DRAW },
		{ vgmAPINAME(vgRenderToMask),           vgvAPITYPE_MASK | vgvAPITYPE_DRAW },
		{ vgmAPINAME(vgFillMaskLayer),          vgvAPITYPE_MASK | vgvAPITYPE_DRAW },
		{ vgmAPINAME(vgCopyMask),               vgvAPITYPE_MASK | vgvAPITYPE_DRAW },
		{ vgmAPINAME(vgClear),                  vgvAPITYPE_MASK | vgvAPITYPE_DRAW },

		/* Paths. */
		{ vgmAPINAME(vgCreatePath),             vgvAPITYPE_PATH                       | vgvAPITYPE_CONSTRUCTOR },
		{ vgmAPINAME(vgDestroyPath),            vgvAPITYPE_PATH                       | vgvAPITYPE_DESTRUCTOR  },
		{ vgmAPINAME(vgClearPath),              vgvAPITYPE_PATH | vgvAPITYPE_PATH_MOD | vgvAPITYPE_DESTRUCTOR  },
		{ vgmAPINAME(vgRemovePathCapabilities), vgvAPITYPE_PATH | vgvAPITYPE_SETTER },
		{ vgmAPINAME(vgGetPathCapabilities),    vgvAPITYPE_PATH | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgAppendPath),             vgvAPITYPE_PATH | vgvAPITYPE_PATH_MOD },
		{ vgmAPINAME(vgAppendPathData),         vgvAPITYPE_PATH | vgvAPITYPE_PATH_MOD },
		{ vgmAPINAME(vgModifyPathCoords),       vgvAPITYPE_PATH | vgvAPITYPE_PATH_MOD },
		{ vgmAPINAME(vgTransformPath),          vgvAPITYPE_PATH | vgvAPITYPE_PATH_MOD },
		{ vgmAPINAME(vgInterpolatePath),        vgvAPITYPE_PATH | vgvAPITYPE_PATH_MOD },
		{ vgmAPINAME(vgPathLength),             vgvAPITYPE_PATH | vgvAPITYPE_PATH_MOD },
		{ vgmAPINAME(vgPointAlongPath),         vgvAPITYPE_PATH | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgPathBounds),             vgvAPITYPE_PATH | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgPathTransformedBounds),  vgvAPITYPE_PATH | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgDrawPath),               vgvAPITYPE_PATH | vgvAPITYPE_DRAW },

		/* Paint. */
		{ vgmAPINAME(vgCreatePaint),            vgvAPITYPE_PAINT | vgvAPITYPE_CONSTRUCTOR },
		{ vgmAPINAME(vgDestroyPaint),           vgvAPITYPE_PAINT | vgvAPITYPE_DESTRUCTOR  },
		{ vgmAPINAME(vgPaintPattern),           vgvAPITYPE_PAINT | vgvAPITYPE_DESTRUCTOR  },
		{ vgmAPINAME(vgSetPaint),               vgvAPITYPE_PAINT | vgvAPITYPE_SETTER },
		{ vgmAPINAME(vgGetPaint),               vgvAPITYPE_PAINT | vgvAPITYPE_GETTER },
		{ vgmAPINAME(vgSetColor),               vgvAPITYPE_PAINT | vgvAPITYPE_SETTER },
		{ vgmAPINAME(vgGetColor),               vgvAPITYPE_PAINT | vgvAPITYPE_GETTER },

		/* Images. */
		{ vgmAPINAME(vgCreateImage),            vgvAPITYPE_IMAGE | vgvAPITYPE_DRAW | vgvAPITYPE_CONSTRUCTOR },
		{ vgmAPINAME(vgChildImage),             vgvAPITYPE_IMAGE                   | vgvAPITYPE_CONSTRUCTOR },
		{ vgmAPINAME(vgDestroyImage),           vgvAPITYPE_IMAGE                   | vgvAPITYPE_DESTRUCTOR  },
		{ vgmAPINAME(vgClearImage),             vgvAPITYPE_IMAGE | vgvAPITYPE_DRAW },
		{ vgmAPINAME(vgCopyImage),              vgvAPITYPE_IMAGE | vgvAPITYPE_DRAW },
		{ vgmAPINAME(vgDrawImage),              vgvAPITYPE_IMAGE | vgvAPITYPE_DRAW },
		{ vgmAPINAME(vgSetPixels),              vgvAPITYPE_IMAGE | vgvAPITYPE_DRAW },
		{ vgmAPINAME(vgWritePixels),            vgvAPITYPE_IMAGE | vgvAPITYPE_DRAW },
		{ vgmAPINAME(vgCopyPixels),             vgvAPITYPE_IMAGE | vgvAPITYPE_DRAW },
		{ vgmAPINAME(vgImageSubData),           vgvAPITYPE_IMAGE },
		{ vgmAPINAME(vgGetImageSubData),        vgvAPITYPE_IMAGE },
		{ vgmAPINAME(vgGetParent),              vgvAPITYPE_IMAGE },
		{ vgmAPINAME(vgGetPixels),              vgvAPITYPE_IMAGE },
		{ vgmAPINAME(vgReadPixels),             vgvAPITYPE_IMAGE },

		/* Text. */
		{ vgmAPINAME(vgCreateFont),             vgvAPITYPE_TEXT | vgvAPITYPE_CONSTRUCTOR },
		{ vgmAPINAME(vgDestroyFont),            vgvAPITYPE_TEXT | vgvAPITYPE_DESTRUCTOR  },
		{ vgmAPINAME(vgClearGlyph),             vgvAPITYPE_TEXT | vgvAPITYPE_DESTRUCTOR  },
		{ vgmAPINAME(vgSetGlyphToPath),         vgvAPITYPE_TEXT | vgvAPITYPE_DESTRUCTOR  },
		{ vgmAPINAME(vgSetGlyphToImage),        vgvAPITYPE_TEXT | vgvAPITYPE_DESTRUCTOR  },
		{ vgmAPINAME(vgDrawGlyph),              vgvAPITYPE_TEXT | vgvAPITYPE_DRAW },
		{ vgmAPINAME(vgDrawGlyphs),             vgvAPITYPE_TEXT | vgvAPITYPE_DRAW },

		/* Image filters. */
		{ vgmAPINAME(vgColorMatrix),            vgvAPITYPE_FILTER },
		{ vgmAPINAME(vgConvolve),               vgvAPITYPE_FILTER },
		{ vgmAPINAME(vgSeparableConvolve),      vgvAPITYPE_FILTER },
		{ vgmAPINAME(vgGaussianBlur),           vgvAPITYPE_FILTER },
		{ vgmAPINAME(vgLookup),                 vgvAPITYPE_FILTER },
		{ vgmAPINAME(vgLookupSingle),           vgvAPITYPE_FILTER }
	};

#endif	/* vgvENABLE_ENTRY_SCAN */


/*******************************************************************************
**
** _GetAPIInfo
**
** Locate VG API information by its name.
**
** INPUT:
**
**    FunctionName
**       Name of the API to locate.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvENABLE_ENTRY_SCAN

static gctUINT _GetAPIInfo(
	gctSTRING FunctionName
	)
{
	gctUINT i;

	for (i = 0; i < gcmCOUNTOF(_apiInfo); i++)
	{
		if (strcmp(FunctionName, _apiInfo[i].name) == 0)
		{
			return _apiInfo[i].type;
		}
	}

	return 0;
}

#endif	/* vgvENABLE_ENTRY_SCAN */


/******************************************************************************\
**************************** Internal Debugging API ****************************
\******************************************************************************/

/*******************************************************************************
**
** Context count management.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvCOUNT_CONTEXTS

static gctUINT _contextCount;
static vgsCONTEXT_PTR _contextArray[10];

void vgfReportNewContext(
	vgsCONTEXT_PTR Context
	)
{
	if (_contextCount < gcmCOUNTOF(_contextArray))
	{
		_contextArray[_contextCount] = Context;
		_contextCount += 1;
	}
}

gctBOOL vgfVerifyContext(
	vgsCONTEXT_PTR Context,
	gctUINT Index
	)
{
	return
	(
		(Index < gcmCOUNTOF(_contextArray)) &&
		(_contextArray[Index] == Context)
	);
}

void vgfAdvanceFrame(
	void
	)
{
	gctUINT i;
	vgsCONTEXT_PTR context;

	for (i = 0; i < gcmCOUNTOF(_contextArray); i += 1)
	{
		context = _contextArray[i];

		if (context == gcvNULL)
		{
			break;
		}

		vgmADVANCE_FRAME_COUNT(context);

		if (i == 0)
		{
			gcmTRACE(
				gcvLEVEL_INFO,
				"Frame #%d\n",
				context->__frameCount
				);
		}

		vgmRESET_CLEAR_COUNT(context);
		vgmRESET_DRAWPATH_COUNT(context);
		vgmRESET_DRAWIMAGE_COUNT(context);
		vgmRESET_COPYIMAGE_COUNT(context);
		vgmRESET_GAUSSIAN_COUNT(context);
	}
}

#endif	/* vgvCOUNT_CONTEXTS */


/*******************************************************************************
**
** Frame count management.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvCOUNT_FRAMES

void vgfAdvanceFrameCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__frameCount += 1;
}

#endif	/* vgvCOUNT_FRAMES */


/*******************************************************************************
**
** Clear count management.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvCOUNT_SET_CONTEXT_CALLS

void vgfResetSetContextCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__setContextCount = 0;
}

void vgfAdvanceSetContextCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__setContextCount += 1;
}

#endif	/* vgvCOUNT_SET_CONTEXT_CALLS */


/*******************************************************************************
**
** Clear count management.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvCOUNT_CLEAR_CALLS

void vgfResetClearCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__clearCount = 0;
}

void vgfAdvanceClearCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__clearCount += 1;
}

#endif	/* vgvCOUNT_CLEAR_CALLS */


/*******************************************************************************
**
** Draw path count management.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvCOUNT_PATH_DRAW_CALLS

void vgfResetPathDrawCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__drawPathCount = 0;
}

void vgfAdvancePathDrawCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__drawPathCount += 1;
}

#endif	/* vgvCOUNT_PATH_DRAW_CALLS */


/*******************************************************************************
**
** Draw image count management.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvCOUNT_IMAGE_DRAW_CALLS

void vgfResetDrawImageCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__drawImageCount = 0;
}

void vgfAdvanceImageDrawCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__drawImageCount += 1;
}

#endif	/* vgvCOUNT_IMAGE_DRAW_CALLS */


/*******************************************************************************
**
** Copy image count management.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvCOUNT_IMAGE_COPY_CALLS

void vgfResetCopyImageCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__copyImageCount = 0;
}

void vgfAdvanceImageCopyCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__copyImageCount += 1;
}

#endif	/* vgvCOUNT_IMAGE_COPY_CALLS */


/*******************************************************************************
**
** Gaussian Blur count management.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvCOUNT_GAUSSIAN_CALLS

void vgfResetGaussianCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__drawGaussianCount = 0;
}

void vgfAdvanceGaussianCount(
	vgsCONTEXT_PTR Context
	)
{
	Context->__drawGaussianCount += 1;
}

#endif	/* vgvCOUNT_GAUSSIAN_CALLS */


/*******************************************************************************
**
** Path object count management.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvCOUNT_PATH_OBJECTS

void vgfSetPathNumber(
	vgsCONTEXT_PTR Context,
	vgsPATH_PTR Path
	)
{
	Context->__pathCount += 1;
	Path->__pathCount = Context->__pathCount;
}

#endif	/* vgvCOUNT_PATH_OBJECTS */


/*******************************************************************************
**
** Path container count management.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvCOUNT_PATH_CONTAINERS

void vgfSetContainerNumber(
	vgsCONTEXT_PTR Context,
	vgsCONTAINER_PTR Container
	)
{
	Context->__containerCount += 1;
	Container->__containerCount = Context->__containerCount;
}

#endif	/* vgvCOUNT_PATH_CONTAINERS */


/*******************************************************************************
**
** vgfDebugEntry
**
** Called in debug mode during the entry to a VG API.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    FunctionName
**       Pointer to the name of the API function.
**
** OUTPUT:
**
**    Nothing.
*/

#if gcvDEBUG

void vgfDebugEntry(
	vgsCONTEXT_PTR Context,
	gctSTRING FunctionName
	)
{
	/*gctUINT breakPoint;*/

#if vgvENABLE_ENTRY_SCAN
	{
		/* Get the API type. */
		gctUINT type = _GetAPIInfo(FunctionName);

		/* Verify the type. */
		if ((type & vgvAPITYPE_DESTRUCTOR) == vgvAPITYPE_DESTRUCTOR)
		{
			/*breakPoint = 0;*/
		}

		/* Verify the type. */
		if ((type & vgvAPITYPE_PATH) == vgvAPITYPE_PATH)
		{
			if (strcmp(FunctionName, "vgDrawPath") != 0)
			{
				/*breakPoint = 0;*/
			}
		}
	}
#endif	/* vgvENABLE_ENTRY_SCAN */

#if vgvDUMP_ENTRY_NAME
	gcmTRACE(0, "%s\n", FunctionName);
#endif

	/* Break point. */
	/*breakPoint = 0;*/
}

#endif	/* gcvDEBUG */


/*******************************************************************************
**
** vgfDebugExit
**
** Called in debug mode during the exit from a VG API.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    FunctionName
**       Pointer to the name of the API function.
**
** OUTPUT:
**
**    Nothing.
*/

#if gcvDEBUG

void vgfDebugExit(
	vgsCONTEXT_PTR Context,
	gctSTRING FunctionName
	)
{
	/*gctUINT breakPoint;*/

#if vgvFINISH_ON_EXIT
	/* Commit and stall after some functions. */
	{
		int i;

		static gctSTRING _finishList[] =
		{
			"vgClear",
			"vgCreateImage",
			"vgClearImage",
			"vgMask",
			"vgRenderToMask",
			"vgCopyMask",
			"vgCopyImage",
			"vgCopyPixels",
			"vgWritePixels",
			"vgSetPixels",
			"vgDrawImage",
			"vgDrawPath",
			"vgColorMatrix",
			"vgSeparableConvolve",
			"vgGaussianBlur"
		};

		if (Context != gcvNULL)
		{
			for (i = 0; i < gcmCOUNTOF(_finishList); i++)
			{
				if (strcmp(FunctionName, _finishList[i]) == 0)
				{
					vgfFlushPipe(Context, gcvTRUE);
				}
			}
		}
	}
#endif	/* vgvFINISH_ON_EXIT */

	/* Break point. */
	/*breakPoint = 0;*/
}

#endif	/* gcvDEBUG */


/*******************************************************************************
**
** vgfErrorCallback
**
** Called in debug mode during the entry into a VG API.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    FunctionName
**       Pointer to the name of the API function.
**
** OUTPUT:
**
**    Nothing.
*/

#if gcvDEBUG

void vgfErrorCallback(
	vgsCONTEXT_PTR Context,
	VGErrorCode Code
	)
{
	/* Break point. */
	/*gctUINT breakPoint = 0;*/
}

#endif	/* gcvDEBUG */


/*******************************************************************************
**
** vgfDumpMatrix
**
** Print a 3x3 matrix.
**
** INPUT:
**
**    Message
**       Message to print.
**
**    Matrix
**       Pointer to the matrix.
**
** OUTPUT:
**
**    Nothing.
*/

#if vgvENABLE_MATRIX_DUMP

void vgfDumpMatrix(
	IN gctSTRING Message,
	IN const vgsMATRIX_PTR Matrix
	)
{
	gctUINT row, col;

	gcmTRACE_ZONE(
		gcvLEVEL_INFO, gcvZONE_AUX,
		"%s @ %p, identity=%d (%s), determinant=%.4f (%s)\n",
		Message,
		Matrix,
		Matrix->identity,
		Matrix->identityDirty
			? "dirty"
			: "clean",
		Matrix->det,
		Matrix->detDirty
			? "dirty"
			: "clean"
		);

	for (row = 0; row < 3; row++)
	{
		for (col = 0; col < 3; col++)
		{
			VGfloat value = vgmMAT(Matrix, row, col);

			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_AUX,
				" %4.4f (%08X)",
				value,
				* (gctUINT32_PTR) &value
				);
		}

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_AUX,
			"\n"
			);
	}
}

#endif	/* vgvENABLE_MATRIX_DUMP */


/*******************************************************************************
**
** vgfDumpImage
**
** Dump contents of the specified image.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Image
**       Image object.
**
**    Message
**       Message to print with the image.
**
** OUTPUT:
**
**    Nothing.
*/

#if gcvDEBUG

void vgfDumpImage(
	IN vgsCONTEXT_PTR Context,
	IN gctSTRING Message,
	IN vgsIMAGE_PTR Image,
	IN gctINT32 X,
	IN gctINT32 Y,
	IN gctINT32 Width,
	IN gctINT32 Height
	)
{
	vgsFORMAT_PTR format;
	gctUINT8_PTR line, pixel;
	gctINT32 originX, originY;
	gctINT32 width, height;
	gctINT32 x0, y0, x1, y1;
	gctINT32 x, y;
	gctINT stride;
	gctUINT bpp;

	/* Get the format. */
	format = Image->surfaceFormat;

	/* Get the size of the image. */
	width  = Image->size.width;
	height = Image->size.height;

	/* Get the origin. */
	originX = Image->origin.x;
	originY = Image->origin.y;

	/* Get the pixel depth. */
	bpp = format->bitsPerPixel;

	/* Get the stride. */
	stride = Image->stride;

	/* Print image information. */
	gcmTRACE_ZONE(
		gcvLEVEL_INFO, gcvZONE_IMAGE,
		"%s:\n"
		"  %dx%d, bits per pixel: %d, %s\n"
		"  vg format: %d, hal format: %d\n",
		Message, width, height, bpp,
		Image->upsample ? "upsampled" : "not upsampled",
		Image->format, format->internalFormat
		);

	if (Image->parent == Image)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_IMAGE,
			"  root image\n"
			);
	}
	else
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_IMAGE,
			"  child image with origin %d, %d\n",
			originX, originY
			);
	}

	gcmTRACE_ZONE(
		gcvLEVEL_INFO, gcvZONE_IMAGE,
		"  format specifics: %s, %s\n",
		format->luminance ? "luminance" : "rgba",
		format->linear    ? "linear"    : "non-linear"
		);

	if (!format->luminance)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_IMAGE,
			"  components:\n"
			"    R: width=%d, offset=%d\n"
			"    G: width=%d, offset=%d\n"
			"    B: width=%d, offset=%d\n"
			"    A: width=%d, offset=%d\n",
			format->r.width, format->r.start,
			format->g.width, format->g.start,
			format->b.width, format->b.start,
			format->a.width, format->a.start
			);
	}

	/* Flush the image. */
	gcmVERIFY_OK(vgfFlushImage(Context, Image, gcvTRUE));

	/* Determine the origin. */
	if (X < 0)
	{
		X = 0;
	}

	if (Y < 0)
	{
		Y = 0;
	}

	if (X >= width)
	{
		X = width - 1;
	}

	if (Y >= height)
	{
		Y = height - 1;
	}

	if ((Width <= 0) || (X + Width > width))
	{
		Width = width - X;
	}

	if ((Height <= 0) || (Y + Height > height))
	{
		Height = height - Y;
	}

	if ((X != 0) || (Y != 0) || (Width != width) || (Height != height))
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_IMAGE,
			"  dumping subrectangle only: %d,%d %dx%d\n",
			X, Y, Width, Height
			);
	}

	/* Determine the coordinates. */
	x0 = originX + X;
	y0 = originY + Y;
	x1 = x0 + Width;
	y1 = y0 + Height;

	/* Determine the first line position. */
	line = Image->buffer + y0 * stride + (x0 * bpp) / 8;

	/* Loop through all pixels. */
	for (y = y0; y < y1; y += 1)
	{
		/* Set the position of the first pixel. */
		pixel = line;

		/* Print the prefix. */
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_IMAGE,
			"  0x%08X:", line
			);

		for (x = x0; x < x1; x += 1)
		{
			switch (bpp)
			{
			case 1:
				{
					gctINT shift = x & 7;
					gctUINT8 value = (*pixel >> shift) & 0x1;

					gcmTRACE_ZONE(
						gcvLEVEL_INFO, gcvZONE_IMAGE,
						" %d", value
						);

					if (shift == 7)
					{
						pixel += 1;
					}
				}
				break;

			case 4:
				{
					gctINT shift = (x & 1) << 2;
					gctUINT8 value = (*pixel >> shift) & 0xF;

					gcmTRACE_ZONE(
						gcvLEVEL_INFO, gcvZONE_IMAGE,
						" 0x%X", value
						);

					if (shift == 4)
					{
						pixel += 1;
					}
				}
				break;

			case 8:
				gcmTRACE_ZONE(
					gcvLEVEL_INFO, gcvZONE_IMAGE,
					" 0x%02X", *pixel
					);

				pixel += 1;
				break;

			case 16:
				gcmTRACE_ZONE(
					gcvLEVEL_INFO, gcvZONE_IMAGE,
					" 0x%04X", * (gctUINT16_PTR) pixel
					);

				pixel += 2;
				break;

			case 32:
				gcmTRACE_ZONE(
					gcvLEVEL_INFO, gcvZONE_IMAGE,
					" 0x%08X", * (gctUINT32_PTR) pixel
					);

				pixel += 4;
				break;
			}
		}

		/* Print EOL. */
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_IMAGE,
			"\n"
			);

		/* Advance to the next line. */
		line += stride;
	}
}

#endif	/* gcvDEBUG */


/*******************************************************************************
**
** vgfDumpXXXArray
**
** Dump parameter array.
**
** INPUT:
**
**    ArrayName
**       Name of the array.
**
**    Array
**       Pointer to the parameter array.
**
**    Count
**       Number of the items in the array.
**
** OUTPUT:
**
**    Nothing.
*/

/*----------------------------------------------------------------------------*/
/*---------------------- VG Function Parameter Dumping. ----------------------*/

#if gcvDEBUG

typedef enum _vgeDUMP_TYPES
{
	vgeDUMP_BYTE,
	vgeDUMP_SHORT,
	vgeDUMP_WORD,
	vgeDUMP_INT,
	vgeDUMP_DWORD,
	vgeDUMP_FLOAT
}
vgeDUMP_TYPES;

static void _DumpArray(
	vgeDUMP_TYPES DataType,
	gctSTRING ArrayName,
	const gctPOINTER * Array,
	gctUINT Count
	)
{
	static gctSTRING _arrayType[] =
	{
		"unsigned char",	/* vgeDUMP_BYTE  */
		"short",			/* vgeDUMP_SHORT */
		"unsigned short",	/* vgeDUMP_WORD  */
		"int",				/* vgeDUMP_INT   */
		"unsigned int",		/* vgeDUMP_DWORD */
		"float"				/* vgeDUMP_FLOAT */
	};

	static gctUINT _width[] =
	{
		16,		/* vgeDUMP_BYTE  */
		16,		/* vgeDUMP_SHORT */
		16,		/* vgeDUMP_WORD  */
		8,		/* vgeDUMP_INT   */
		8,		/* vgeDUMP_DWORD */
		8		/* vgeDUMP_FLOAT */
	};

	gctUINT i;
	gctUINT width;
	gctUINT8_PTR data;

	gcmTRACE_ZONE(
		gcvLEVEL_INFO, gcvZONE_PARAMETERS,
		"const %s %s[] =\n{",
		_arrayType[DataType], ArrayName
		);

    gcmASSERT(Array != gcvNULL);

	data  = (gctUINT8_PTR) Array;
	width = _width[DataType];

	for (i = 0; i < Count; i += 1)
	{
		if ((i % width) == 0)
		{
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_PARAMETERS,
				"\n\t"
				);
		}
		else
		{
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_PARAMETERS,
				" "
				);
		}

		switch (DataType)
		{
		case vgeDUMP_BYTE:
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_PARAMETERS,
				"0x%02X", * (gctUINT8_PTR) data
				);
			data += gcmSIZEOF(gctUINT8);
			break;

		case vgeDUMP_SHORT:
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_PARAMETERS,
				"%d", (gctINT) * (gctINT16_PTR) data
				);
			data += gcmSIZEOF(gctINT16);
			break;

		case vgeDUMP_WORD:
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_PARAMETERS,
				"0x%04X", * (gctUINT16_PTR) data
				);
			data += gcmSIZEOF(gctUINT16);
			break;

		case vgeDUMP_INT:
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_PARAMETERS,
				"%d", * (gctINT_PTR) data
				);
			data += gcmSIZEOF(gctINT);
			break;

		case vgeDUMP_DWORD:
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_PARAMETERS,
				"0x%08X", * (gctUINT_PTR) data
				);
			data += gcmSIZEOF(gctUINT);
			break;

		case vgeDUMP_FLOAT:
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_PARAMETERS,
				"%.10ff", * (gctFLOAT_PTR) data
				);
			data += gcmSIZEOF(gctFLOAT);
			break;
		}

		if (i < Count - 1)
		{
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_PARAMETERS,
				","
				);
		}
	}

	gcmTRACE_ZONE(
		gcvLEVEL_INFO, gcvZONE_PARAMETERS,
		"\n};\n"
		);
}

void vgfDumpByteArray(
	gctSTRING ArrayName,
	const gctPOINTER Array,
	gctUINT Count
	)
{
	_DumpArray(vgeDUMP_BYTE, ArrayName, Array, Count);
}

void vgfDumpWordArray(
	gctSTRING ArrayName,
	const gctPOINTER Array,
	gctUINT Count
	)
{
	_DumpArray(vgeDUMP_WORD, ArrayName, (gctPOINTER) Array, Count);
}

void vgfDumpDWordArray(
	gctSTRING ArrayName,
	const gctPOINTER Array,
	gctUINT Count
	)
{
	_DumpArray(vgeDUMP_DWORD, ArrayName, (gctPOINTER) Array, Count);
}

void vgfDumpIntegerArray(
	gctSTRING ArrayName,
	const gctPOINTER Array,
	gctUINT Count
	)
{
	_DumpArray(vgeDUMP_INT, ArrayName, (gctPOINTER) Array, Count);
}

void vgfDumpFloatArray(
	gctSTRING ArrayName,
	const gctPOINTER Array,
	gctUINT Count
	)
{
	_DumpArray(vgeDUMP_FLOAT, ArrayName, (gctPOINTER) Array, Count);
}

void vgfDumpPathRawData(
	VGPathDatatype Datatype,
	gctINT NumSegments,
	const unsigned char * PathSegments,
	const void * PathData
	)
{
	/* Number of coordinates for different segment commands. */
	static gctUINT _dataCount[] =
	{
		0,		/*   0: VG_CLOSE_PATH     */
		2,		/*   2: VG_MOVE_TO_ABS    */
		2,		/*   4: VG_LINE_TO_ABS    */
		1,		/*   6: VG_HLINE_TO_ABS   */
		1,		/*   8: VG_VLINE_TO_ABS   */
		4,		/*  10: VG_QUAD_TO_ABS    */
		6,		/*  12: VG_CUBIC_TO_ABS   */
		2,		/*  14: VG_SQUAD_TO_ABS   */
		4,		/*  16: VG_SCUBIC_TO_ABS  */
		5,		/*  18: VG_SCCWARC_TO_ABS */
		5,		/*  20: VG_SCWARC_TO_ABS  */
		5,		/*  22: VG_LCCWARC_TO_ABS */
		5		/*  24: VG_LCWARC_TO_ABS  */
	};

	gctINT i;
	gctUINT dataCount;
	/* Reset the coordinate count. */
	dataCount = 0;

	/* Determine the number of coordinates. */
	for (i = 0; i < NumSegments; i += 1)
	{
		/* Get the segment command. */
		gctUINT8 command = PathSegments[i] >> 1;

		/* Validate the code. */
		if (command >= gcmCOUNTOF(_dataCount))
		{
			return;
		}

		/* Update the number of coordinates. */
		dataCount += _dataCount[command];
	}

	vgmDUMP_BYTE_ARRAY(
		PathSegments, NumSegments
		);

	switch (Datatype)
	{
	case VG_PATH_DATATYPE_S_8:
		vgmDUMP_BYTE_ARRAY(
			PathData, dataCount
			);
		break;

	case VG_PATH_DATATYPE_S_16:
		vgmDUMP_WORD_ARRAY(
			PathData, dataCount
			);
		break;

	case VG_PATH_DATATYPE_S_32:
	case VG_PATH_DATATYPE_F:
		vgmDUMP_DWORD_ARRAY(
			PathData, dataCount
			);
		break;
	default:
		break;
	}
}

#endif	/* gcvDEBUG */
