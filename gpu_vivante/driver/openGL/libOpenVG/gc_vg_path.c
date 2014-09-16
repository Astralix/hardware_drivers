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

/*******************************************************************************
**
** _FreePathResources
**
** Free all resources associated with the specified path VG object.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Path
**       Pointer to the path object.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _FreePathResources(
	vgsCONTEXT_PTR Context,
	vgsPATH_PTR Path
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		/* Validate the storage. */
		vgmVALIDATE_STORAGE(Context->pathStorage);
		vgmVALIDATE_STORAGE(Context->strokeStorage);

		/* Free the fill path. */
		if (Path->head != gcvNULL)
		{
			vgsPATHSTORAGE_Free(
				Context->pathStorage,
				Path->head,
				gcvTRUE
				);

			/* Reset the path pointers. */
			Path->head = gcvNULL;
			Path->tail = gcvNULL;

			/* Validate the storage. */
			vgmVALIDATE_STORAGE(Context->pathStorage);
		}

		/* Free the stroke. */
		if (Path->stroke != gcvNULL)
		{
			vgsPATHSTORAGE_Free(
				Context->strokeStorage,
				Path->stroke,
				gcvTRUE
				);

			/* Reset the stroke pointer. */
			Path->stroke = gcvNULL;

			/* Validate the storage. */
			vgmVALIDATE_STORAGE(Context->strokeStorage);
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _PathDestructor
**
** Path object destructor. Called when the object is being destroyed.
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

static gceSTATUS _PathDestructor(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_PTR Object
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		/* Cast the object. */
		vgsPATH_PTR path = (vgsPATH_PTR) Object;

		/* Free the path data. */
		gcmERR_BREAK(_FreePathResources(Context, path));
	}
	while (gcvFALSE);

	return status;
}


/*******************************************************************************
**
** _WaitForPathIdle
**
** Wait until the specified path becomes idle.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Path
**       Pointer to the path object.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _WaitForPathIdle(
	vgsCONTEXT_PTR Context,
	vgsPATH_PTR Path
	)
{
	gceSTATUS status;

	do
	{
		/* Get a shortcut to the fill path. */
		vgsPATH_DATA_PTR path = Path->head;

		/* Is there a path? */
		if (path == gcvNULL)
		{
			/* No, no need to wait for anything. */
			status = gcvSTATUS_OK;
			break;
		}

		/* Is there a completion associated? */
		if (vgmGETCMDBUFFER(path)->completion == gcvNULL)
		{
			/* No, this means the path has not yet been scheduled for
			   execution, no need to wait for it. */
			status = gcvSTATUS_OK;
			break;
		}

		/* Commit the current queue. */
		gcmERR_BREAK(gcoHAL_Commit(Context->hal, gcvFALSE));

		/* Wait for the path to complete. */
		gcmERR_BREAK(gcoHAL_WaitCompletion(Context->hal, &path->data));
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
************************** Individual State Functions **************************
\******************************************************************************/

/*----------------------------------------------------------------------------*/
/*------------------------------ VG_PATH_FORMAT ------------------------------*/

vgmGETSTATE_FUNCTION(VG_PATH_FORMAT)
{
	ValueConverter(
		Values, &((vgsPATH_PTR) Object)->format, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*----------------------------- VG_PATH_DATATYPE -----------------------------*/

vgmGETSTATE_FUNCTION(VG_PATH_DATATYPE)
{
	ValueConverter(
		Values, &((vgsPATH_PTR) Object)->datatype, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------------ VG_PATH_SCALE -------------------------------*/

vgmGETSTATE_FUNCTION(VG_PATH_SCALE)
{
	ValueConverter(
		Values, &((vgsPATH_PTR) Object)->scale, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------------- VG_PATH_BIAS -------------------------------*/

vgmGETSTATE_FUNCTION(VG_PATH_BIAS)
{
	ValueConverter(
		Values, &((vgsPATH_PTR) Object)->bias, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*--------------------------- VG_PATH_NUM_SEGMENTS ---------------------------*/

vgmGETSTATE_FUNCTION(VG_PATH_NUM_SEGMENTS)
{
	ValueConverter(
		Values, &((vgsPATH_PTR) Object)->numSegments, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*---------------------------- VG_PATH_NUM_COORDS ----------------------------*/

vgmGETSTATE_FUNCTION(VG_PATH_NUM_COORDS)
{
	ValueConverter(
		Values, &((vgsPATH_PTR) Object)->numCoords, 1, VG_FALSE, VG_TRUE
		);
}


/******************************************************************************\
***************************** Context State Array ******************************
\******************************************************************************/

static vgsSTATEENTRY _stateArray[] =
{
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_PATH_FORMAT,       INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_PATH_DATATYPE,     INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_PATH_SCALE,        FLOAT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_PATH_BIAS,         FLOAT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_PATH_NUM_SEGMENTS, INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_PATH_NUM_COORDS,   INT)
};


/******************************************************************************\
************************* Internal Path Management API *************************
\******************************************************************************/

/*******************************************************************************
**
** vgfReferencePath
**
** vgfReferencePath increments the reference count of the given VGPath object.
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
**    vgsPATH_PTR * Path
**       Handle to the new VGPath object.
*/

gceSTATUS vgfReferencePath(
	vgsCONTEXT_PTR Context,
	vgsPATH_PTR * Path
	)
{
	gceSTATUS status, last;
	vgsPATH_PTR path = gcvNULL;

	do
	{
		/* Create the object if does not exist yet. */
		if (*Path == gcvNULL)
		{
			/* Allocate the object structure. */
			gcmERR_BREAK(gcoOS_Allocate(
				Context->os,
				gcmSIZEOF(vgsPATH),
				(gctPOINTER *) &path
				));

			/* Reset the object. */
			gcmVERIFY_OK(gcoOS_ZeroMemory(path, sizeof(vgsPATH)));

			/* Initialize the object structure. */
			path->object.type      = vgvOBJECTTYPE_PATH;
			path->object.userValid = VG_TRUE;

			/* Insert the object into the cache. */
			gcmERR_BREAK(vgfObjectCacheInsert(
				Context, (vgsOBJECT_PTR) path
				));

			/* Set the result pointer. */
			*Path = path;
		}

		/* Increment the reference count. */
		(*Path)->object.referenceCount++;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Roll back. */
	if (path != gcvNULL)
	{
		gcmCHECK_STATUS(gcoOS_Free(Context->os, path));
	}

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgfSetPathObjectList
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

void vgfSetPathObjectList(
	vgsOBJECT_LIST_PTR ListEntry
	)
{
	ListEntry->stateArray     = _stateArray;
	ListEntry->stateArraySize = gcmCOUNTOF(_stateArray);
	ListEntry->destructor     = _PathDestructor;
}


/*******************************************************************************
**
** vgfFreePathDataCallback
**
** The function is called by the path storage manager when it is not allowed
** to allocate any more memory and is forced to free least recently used paths.
**
** INPUT:
**
**    Path
**       Handle to the valid VGPath object whose path data is being freed.
**
** OUTPUT:
**
**    Nothing.
*/

void vgfFreePathDataCallback(
	IN vgsPATH_PTR Path,
	IN vgsPATH_DATA_PTR PathData
	)
{
	/* Only stroke path is allowed to be freed. */
	gcmASSERT(Path->strokeValid);
	gcmASSERT(Path->stroke == PathData);

	/* Reset stoke path. */
	Path->strokeValid = gcvFALSE;
	Path->stroke      = gcvNULL;
}


/*******************************************************************************
**
** vgfDrawPath
**
** Draw the specified path with specified paint modes.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Target
**       Pointer to the target image object.
**
**    Path
**       Handle to a valid VGPath object to be drawn.
**
**    PaintModes
**       Bitwise OR of VG_FILL_PATH and/or VG_STROKE_PATH (VGPaintMode).
**
**    FillPaint, StrokePaint
**       Pointers to the paint objects.
**
**    ColorTransformEnable
**       Color transformation enable.
**
**    Mask
**       Mask enable flag.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgfDrawPath(
	IN vgsCONTEXT_PTR Context,
	IN vgsIMAGE_PTR Target,
	IN vgsPATH_PTR Path,
	IN VGbitfield PaintModes,
	IN vgsPAINT_PTR FillPaint,
	IN vgsPAINT_PTR StrokePaint,
	IN gctBOOL ColorTransformEnable,
	IN gctBOOL Mask
	)
{
	gceSTATUS status;

	do
	{
		gctBOOL useSoftwareTS;

		/* Do we have a fill path? */
		if (Path->head == gcvNULL)
		{
			/* No, ignore the call. */
			status = gcvSTATUS_OK;
			break;
		}

		/* Set target. */
		gcmERR_BREAK(gcoVG_SetTarget(
			Context->vg,
			Target->surface
			));

		/* Update states. */
		gcmERR_BREAK(vgfUpdateStates(
			Context,
			gcvVG_IMAGE_NONE,
			Context->halBlendMode,
			ColorTransformEnable,
			Context->scissoringEnable,
			Mask,
			gcvFALSE
			));

		/* Determine whether we should use software or hardware tesselation. */
		useSoftwareTS
			= Context->useSoftwareTS
			|| (Path->hasArcs && !Context->vg20);

		if ((PaintModes & VG_FILL_PATH) == VG_FILL_PATH)
		{
			gcmERR_BREAK(vgfUpdatePaint(
				Context,
				Context->drawPathFillSurfaceToPaint,
				FillPaint
				));

			gcmERR_BREAK(gcoVG_SetFillRule(
				Context->vg, Context->halFillRule
				));

			gcmERR_BREAK(gcoVG_DrawPath(
				Context->vg,
				&Path->head->data,
				Path->scale,
				Path->bias,
				useSoftwareTS
				));
		}

		if ((PaintModes & VG_STROKE_PATH) == VG_STROKE_PATH)
		{
			gcmERR_BREAK(vgfUpdatePaint(
				Context,
				Context->drawPathStrokeSurfaceToPaint,
				StrokePaint
				));

			/* Check the line width. */
			if (Context->strokeLineWidth <= 0.0f)
			{
				/* The stroke path would be empty with width <= 0. */
				break;
			}

			/* Update the stroke path if necessary. */
			gcmERR_BREAK(vgfUpdateStroke(Context, Path));

			/* Stroke is empty? */
			if (Path->stroke == gcvNULL)
			{
				/* Yes, ignore. */
				break;
			}

			gcmERR_BREAK(gcoVG_SetFillRule(
				Context->vg, gcvVG_NON_ZERO
				));

#if vgvSTROKE_COPY_OPTIMIZE
			gcmERR_BREAK(gcoVG_DrawPath(
				Context->vg,
				&Path->stroke->data,
				Path->strokeScale,
				0.0f,
				useSoftwareTS
				));
#else
			gcmERR_BREAK(gcoVG_DrawPath(
				Context->vg,
				&Path->stroke->data,
				Path->scale,
				Path->bias,
				useSoftwareTS
				));
#endif

			/* Mark as the most recently used. */
			vgsPATHSTORAGE_UpdateMRU(
				Context->strokeStorage,
				Path->stroke
				);
		}

		/* Arm the flush states. */
		Context->imageDirty = *Target->imageDirtyPtr = vgvIMAGE_NOT_READY;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgfGetPathDataSize
**
** Converts the specified path data type to the size of one data item in bytes.
**
** INPUT:
**
**    Type
**       Valid data type.
**
** OUTPUT:
**
**    The size of data item in bytes.
*/

gctUINT vgfGetPathDataSize(
	gcePATHTYPE Type
	)
{
	/* Path data sizes. */
	static gctUINT _datatypeSize[] =
	{
		gcmSIZEOF(gctINT8),		/* gcePATHTYPE_INT8  */
		gcmSIZEOF(gctINT16),	/* gcePATHTYPE_INT16 */
		gcmSIZEOF(gctINT32),	/* gcePATHTYPE_INT32 */
		gcmSIZEOF(gctFLOAT)		/* gcePATHTYPE_FLOAT */
	};

	/* Make sure we are within range. */
	gcmASSERT((Type >= 0) && (Type < gcmCOUNTOF(_datatypeSize)));

	/* Return the size. */
	return _datatypeSize[Type];
}


/*******************************************************************************
**
** vgfGetSegmentDataCount
**
** Converts the specified path segment command to its data count.
**
** INPUT:
**
**    Command
**       Valid path segment command.
**
** OUTPUT:
**
**    The number of data items for the command.
*/

gctUINT vgfGetSegmentDataCount(
	gceVGCMD Command
	)
{
	/* Number of coordinates for different segment commands. */
	static gctUINT _dataCount[] =
	{
		0,                                   /*   0: gcvVGCMD_END             */
		0,                                   /*   1: gcvVGCMD_CLOSE           */
		2,                                   /*   2: gcvVGCMD_MOVE            */
		2,                                   /*   3: gcvVGCMD_MOVE_REL        */
		2,                                   /*   4: gcvVGCMD_LINE            */
		2,                                   /*   5: gcvVGCMD_LINE_REL        */
		4,                                   /*   6: gcvVGCMD_QUAD            */
		4,                                   /*   7: gcvVGCMD_QUAD_REL        */
		6,                                   /*   8: gcvVGCMD_CUBIC           */
		6,                                   /*   9: gcvVGCMD_CUBIC_REL       */
		0,                                   /*  10: gcvVGCMD_BREAK           */
		0,                                   /*  11: gcvVGCMD_HLINE           */
		0,                                   /*  12: gcvVGCMD_HLINE_REL       */
		0,                                   /*  13: gcvVGCMD_VLINE           */
		0,                                   /*  14: gcvVGCMD_VLINE_REL       */
		0,                                   /*  15: gcvVGCMD_SQUAD           */
		0,                                   /*  16: gcvVGCMD_SQUAD_REL       */
		0,                                   /*  17: gcvVGCMD_SCUBIC          */
		0,                                   /*  18: gcvVGCMD_SCUBIC_REL      */
		0,                                   /*  19: gcvVGCMD_SCCWARC         */
		0,                                   /*  20: gcvVGCMD_SCCWARC_REL     */
		0,                                   /*  21: gcvVGCMD_SCWARC          */
		0,                                   /*  22: gcvVGCMD_SCWARC_REL      */
		0,                                   /*  23: gcvVGCMD_LCCWARC         */
		0,                                   /*  24: gcvVGCMD_LCCWARC_REL     */
		0,                                   /*  25: gcvVGCMD_LCWARC          */
		0,                                   /*  26: gcvVGCMD_LCWARC_REL      */
		0,                                   /*  27: **** R E S E R V E D *****/
		0,                                   /*  28: **** R E S E R V E D *****/
		0,                                   /*  29: **** R E S E R V E D *****/
		0,                                   /*  30: **** R E S E R V E D *****/
		0,                                   /*  31: **** R E S E R V E D *****/

		0,                                   /*  32: ***** I N V A L I D ******/
		0,                                   /*  33: ***** I N V A L I D ******/
		0,                                   /*  34: ***** I N V A L I D ******/
		0,                                   /*  35: ***** I N V A L I D ******/
		1,                                   /*  36: gcvVGCMD_HLINE_EMUL      */
		1,                                   /*  37: gcvVGCMD_HLINE_EMUL_REL  */
		0,                                   /*  38: ***** I N V A L I D ******/
		0,                                   /*  39: ***** I N V A L I D ******/
		0,                                   /*  40: ***** I N V A L I D ******/
		0,                                   /*  41: ***** I N V A L I D ******/
		0,                                   /*  42: ***** I N V A L I D ******/
		0,                                   /*  43: ***** I N V A L I D ******/
		0,                                   /*  44: ***** I N V A L I D ******/
		0,                                   /*  45: ***** I N V A L I D ******/
		0,                                   /*  46: ***** I N V A L I D ******/
		0,                                   /*  47: ***** I N V A L I D ******/
		0,                                   /*  48: ***** I N V A L I D ******/
		0,                                   /*  49: ***** I N V A L I D ******/
		0,                                   /*  50: ***** I N V A L I D ******/
		0,                                   /*  51: ***** I N V A L I D ******/
		0,                                   /*  52: ***** I N V A L I D ******/
		0,                                   /*  53: ***** I N V A L I D ******/
		0,                                   /*  54: ***** I N V A L I D ******/
		0,                                   /*  55: ***** I N V A L I D ******/
		0,                                   /*  56: ***** I N V A L I D ******/
		0,                                   /*  57: ***** I N V A L I D ******/
		0,                                   /*  58: ***** I N V A L I D ******/
		0,                                   /*  59: ***** I N V A L I D ******/
		0,                                   /*  60: ***** I N V A L I D ******/
		0,                                   /*  61: ***** I N V A L I D ******/
		0,                                   /*  62: ***** I N V A L I D ******/
		0,                                   /*  63: ***** I N V A L I D ******/

		0,                                   /*  64: ***** I N V A L I D ******/
		0,                                   /*  65: ***** I N V A L I D ******/
		0,                                   /*  66: ***** I N V A L I D ******/
		0,                                   /*  67: ***** I N V A L I D ******/
		1,                                   /*  68: gcvVGCMD_VLINE_EMUL      */
		1,                                   /*  69: gcvVGCMD_VLINE_EMUL_REL  */
		0,                                   /*  70: ***** I N V A L I D ******/
		0,                                   /*  71: ***** I N V A L I D ******/
		0,                                   /*  72: ***** I N V A L I D ******/
		0,                                   /*  73: ***** I N V A L I D ******/
		0,                                   /*  74: ***** I N V A L I D ******/
		0,                                   /*  75: ***** I N V A L I D ******/
		0,                                   /*  76: ***** I N V A L I D ******/
		0,                                   /*  77: ***** I N V A L I D ******/
		0,                                   /*  78: ***** I N V A L I D ******/
		0,                                   /*  79: ***** I N V A L I D ******/
		0,                                   /*  80: ***** I N V A L I D ******/
		0,                                   /*  81: ***** I N V A L I D ******/
		0,                                   /*  82: ***** I N V A L I D ******/
		0,                                   /*  83: ***** I N V A L I D ******/
		0,                                   /*  84: ***** I N V A L I D ******/
		0,                                   /*  85: ***** I N V A L I D ******/
		0,                                   /*  86: ***** I N V A L I D ******/
		0,                                   /*  87: ***** I N V A L I D ******/
		0,                                   /*  88: ***** I N V A L I D ******/
		0,                                   /*  89: ***** I N V A L I D ******/
		0,                                   /*  90: ***** I N V A L I D ******/
		0,                                   /*  91: ***** I N V A L I D ******/
		0,                                   /*  92: ***** I N V A L I D ******/
		0,                                   /*  93: ***** I N V A L I D ******/
		0,                                   /*  94: ***** I N V A L I D ******/
		0,                                   /*  95: ***** I N V A L I D ******/

		0,                                   /*  96: ***** I N V A L I D ******/
		0,                                   /*  97: ***** I N V A L I D ******/
		0,                                   /*  98: ***** I N V A L I D ******/
		0,                                   /*  99: ***** I N V A L I D ******/
		0,                                   /* 100: ***** I N V A L I D ******/
		0,                                   /* 101: ***** I N V A L I D ******/
		2,                                   /* 102: gcvVGCMD_SQUAD_EMUL      */
		2,                                   /* 103: gcvVGCMD_SQUAD_EMUL_REL  */
		4,                                   /* 104: gcvVGCMD_SCUBIC_EMUL     */
		4,                                   /* 105: gcvVGCMD_SCUBIC_EMUL_REL */
		0,                                   /* 106: ***** I N V A L I D ******/
		0,                                   /* 107: ***** I N V A L I D ******/
		0,                                   /* 108: ***** I N V A L I D ******/
		0,                                   /* 109: ***** I N V A L I D ******/
		0,                                   /* 110: ***** I N V A L I D ******/
		0,                                   /* 111: ***** I N V A L I D ******/
		0,                                   /* 112: ***** I N V A L I D ******/
		0,                                   /* 113: ***** I N V A L I D ******/
		0,                                   /* 114: ***** I N V A L I D ******/
		0,                                   /* 115: ***** I N V A L I D ******/
		0,                                   /* 116: ***** I N V A L I D ******/
		0,                                   /* 117: ***** I N V A L I D ******/
		0,                                   /* 118: ***** I N V A L I D ******/
		0,                                   /* 119: ***** I N V A L I D ******/
		0,                                   /* 120: ***** I N V A L I D ******/
		0,                                   /* 121: ***** I N V A L I D ******/
		0,                                   /* 122: ***** I N V A L I D ******/
		0,                                   /* 123: ***** I N V A L I D ******/
		0,                                   /* 124: ***** I N V A L I D ******/
		0,                                   /* 125: ***** I N V A L I D ******/
		0,                                   /* 126: ***** I N V A L I D ******/
		0,                                   /* 127: ***** I N V A L I D ******/

		0,                                   /* 128: ***** I N V A L I D ******/
		0,                                   /* 129: ***** I N V A L I D ******/
		0,                                   /* 130: ***** I N V A L I D ******/
		0,                                   /* 131: ***** I N V A L I D ******/
		2,                                   /* 132: gcvVGCMD_ARC_LINE        */
		2,                                   /* 133: gcvVGCMD_ARC_LINE_REL    */
		4,                                   /* 134: gcvVGCMD_ARC_QUAD        */
		4,                                   /* 135: gcvVGCMD_ARC_QUAD_REL    */
	};

	/* Make sure we are within range. */
	gcmASSERT((Command >= 0) && (Command < gcmCOUNTOF(_dataCount)));

	/* Return the size. */
	return _dataCount[Command];
}


/******************************************************************************\
************************** OpenVG Path Management API **************************
\******************************************************************************/

/*******************************************************************************
**
** vgCreatePath
**
** vgCreatePath creates a new path that is ready to accept segment data and
** returns a VGPath handle to it. The path data will be formatted in the
** format given by PathFormat, typically VG_PATH_FORMAT_STANDARD. The Datatype
** parameter contains a value from the VGPathDatatype enumeration indicating
** the datatype that will be used for coordinate data. The Capabilities
** argument is a bitwise OR of the desired VGPathCapabilities values. Bits
** of capabilities that do not correspond to values from VGPathCapabilities
** have no effect. If an error occurs, VG_INVALID_HANDLE is returned.
**
** The Scale and Bias parameters are used to interpret each coordinate of the
** path data; an incoming coordinate value V will be interpreted as the value
** (Scale * V + Bias). Scale must not equal 0. The Datatype, Scale, and Bias
** together define a valid coordinate data range for the path; segment commands
** that attempt to place a coordinate in the path that is outside this range
** will overflow silently, resulting in an undefined coordinate value.
** Functions that query a path containing such values, such as vgPathLength
** and vgPointAlongPath, also return undefined results.
**
** The SegmentCapacityHint parameter provides a hint as to the total number of
** segments that will eventually be stored in the path. The CoordCapacityHint
** parameter provides a hint as to the total number of specified coordinates
** (as defined in the "Coordinates" column of Table 6) that will eventually be
** stored in the path. A value less than or equal to 0 for either hint
** indicates that the capacity is unknown. The path storage space will in any
** case grow as needed, regardless of the hint values. However, supplying hints
** may improve performance by reducing the need to allocate additional space as
** the path grows. Implementations should allow applications to append segments
** and coordinates up to the stated capacity in small batches without degrading
** performance due to excessive memory reallocation.
**
** INPUT:
**
**    PathFormat
**       Format of the data path (VG_PATH_FORMAT_STANDARD).
**
**    Datatype
**       One of:
**          VG_PATH_DATATYPE_S_8
**          VG_PATH_DATATYPE_S_16
**          VG_PATH_DATATYPE_S_32
**          VG_PATH_DATATYPE_F
**
**    Scale, Bias
**       Parameters used to interpret each coordinate of the path data.
**
**    SegmentCapacityHint
**    CoordCapacityHint
**       Memory allocation hints.
**
**    Capabilities
**       Bitwise OR of VGPathCapabilities values.
**
** OUTPUT:
**
**    VGPath
**       Handle to the new VGPath object.
*/

VG_API_CALL VGPath VG_API_ENTRY vgCreatePath(
	VGint PathFormat,
	VGPathDatatype Datatype,
	VGfloat Scale,
	VGfloat Bias,
	VGint SegmentCapacityHint,
	VGint CoordCapacityHint,
	VGbitfield Capabilities
	)
{
	vgsPATH_PTR path = gcvNULL;

	vgmENTERAPI(vgCreatePath)
	{
		/* Data type transaltion table. */
		static gcePATHTYPE _dataType[] =
		{
			gcePATHTYPE_INT8,				/* VG_PATH_DATATYPE_S_8  */
			gcePATHTYPE_INT16,				/* VG_PATH_DATATYPE_S_16 */
			gcePATHTYPE_INT32,				/* VG_PATH_DATATYPE_S_32 */
			gcePATHTYPE_FLOAT				/* VG_PATH_DATATYPE_F    */
		};

		gceSTATUS status;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%04X, 0x%04X, %.10ff, %.10ff, %d, %d, 0x%08X);\n",
			__FUNCTION__,
			PathFormat, Datatype,
			Scale, Bias,
			SegmentCapacityHint, CoordCapacityHint,
			Capabilities
			);

		/* Validate the arguments. */
		if (PathFormat != VG_PATH_FORMAT_STANDARD)
		{
			vgmERROR(VG_UNSUPPORTED_PATH_FORMAT_ERROR);
			break;
		}

		if ((Datatype != VG_PATH_DATATYPE_S_8) &&
			(Datatype != VG_PATH_DATATYPE_S_16) &&
			(Datatype != VG_PATH_DATATYPE_S_32) &&
			(Datatype != VG_PATH_DATATYPE_F))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		if (Scale == 0.0f)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Ignore undefined capability bits. */
		Capabilities &= VG_PATH_CAPABILITY_ALL;

		/* Allocate a new object. */
		if (gcmIS_ERROR(vgfReferencePath(Context, &path)))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}

		/* Query path data storage attributes. */
		gcmERR_BREAK(gcoHAL_QueryPathStorage(
			Context->hal, &path->pathInfo
			));

		/* Initialize the path object. */
		path->format       = PathFormat;
		path->scale        = Scale;
		path->bias         = Bias;
		path->capabilities = Capabilities;

		/* Set datatype. */
		path->datatype    = Datatype;
		path->halDataType = _dataType[Datatype];

		/* No ARCs yet. */
		path->hasArcs = gcvFALSE;

		/* Set coordiate access functions. */
		vgfGetCoordinateAccessArrays(
			Scale, Bias,
			&path->getArray,
			&path->setArray,
			&path->getcopyArray
			);

		/* Assign the path count. */
		vgmSET_PATH_NUMBER(Context, path);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s() = 0x%08X;\n",
			__FUNCTION__,
			path
			);
	}
	vgmLEAVEAPI(vgCreatePath);

	return (VGPath) path;
}


/*******************************************************************************
**
** vgClearPath
**
** vgClearPath removes all segment command and coordinate data associated with
** a path. The handle continues to be valid for use in the future, and the path
** format and datatype retain their existing values. The capabilities argument
** is a bitwise OR of the desired VGPathCapabilities values. Bits of
** capabilities that do not correspond to values from VGPathCapabilities have
** no effect. Using vgClearPath may be more efficient than destroying and
** re-creating a path for short-lived paths.
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object to be reset.
**
**    Capabilities
**       Bitwise OR of VGPathCapabilities values.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgClearPath(
	VGPath Path,
	VGbitfield Capabilities
	)
{
	vgmENTERAPI(vgClearPath)
	{
		gceSTATUS status;
		vgsPATH_PTR path;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X);\n",
			__FUNCTION__,
			Path, Capabilities
			);

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Ignore undefined capability bits. */
		Capabilities &= VG_PATH_CAPABILITY_ALL;

		/* Cast the object. */
		path = (vgsPATH_PTR) Path;

		/* Free the path data. */
		gcmERR_BREAK(_FreePathResources(Context, path));

		/* Reset the data counters. */
		path->numSegments = 0;
		path->numCoords   = 0;

		/* Set new capabilities. */
		path->capabilities = Capabilities;
	}
	vgmLEAVEAPI(vgClearPath);
}


/*******************************************************************************
**
** vgDestroyPath
**
** vgDestroyPath releases any resources associated with path, and makes
** the handle invalid in all contexts that shared it.
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object to be destroyed.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgDestroyPath(
	VGPath Path
	)
{
	vgmENTERAPI(vgDestroyPath)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X);\n",
			__FUNCTION__,
			Path
			);

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Invalidate the object. */
		((vgsOBJECT_PTR) Path)->userValid = VG_FALSE;

		/* Decrement the reference count. */
		gcmVERIFY_OK(vgfDereferenceObject(
			Context,
			(vgsOBJECT_PTR *) &Path
			));
	}
	vgmLEAVEAPI(vgDestroyPath);
}


/*******************************************************************************
**
** vgRemovePathCapabilities
**
** The vgRemovePathCapabilities function requests the set of capabilities
** specified in the Capabilities argument to be disabled for the given path.
** The Capabilities argument is a bitwise OR of the VGPathCapabilities values
** whose removal is requested. Attempting to remove a capability that is
** already disabled has no effect. Bits of capabilities that do not correspond
** to values from VGPathCapabilities have no effect.
**
** An implementation may choose to ignore the request to remove a particular
** capability if no significant performance improvement would result. In this
** case, vgGetPathCapabilities will continue to report the capability as
** enabled.
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
**    Capabilities
**       Capabilities to be removed.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgRemovePathCapabilities(
	VGPath Path,
	VGbitfield Capabilities
	)
{
	vgmENTERAPI(vgRemovePathCapabilities)
	{
		vgsPATH_PTR path;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X);\n",
			__FUNCTION__,
			Path, Capabilities
			);

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Ignore undefined capability bits. */
		Capabilities &= VG_PATH_CAPABILITY_ALL;

		/* Cast the object. */
		path = (vgsPATH_PTR) Path;

		/* Remove path capabilities. */
		path->capabilities &= ~Capabilities;
	}
	vgmLEAVEAPI(vgRemovePathCapabilities);
}


/*******************************************************************************
**
** vgGetPathCapabilities
**
** The vgGetPathCapabilities function returns the current capabilities of the
** path, as a bitwise OR of VGPathCapabilities constants. If an error occurs,
** 0 is returned.
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
** OUTPUT:
**
**    VGbitfield
**       Current capabilities of the path.
*/

VG_API_CALL VGbitfield VG_API_ENTRY vgGetPathCapabilities(
	VGPath Path
	)
{
	VGbitfield capabilities = 0;

	vgmENTERAPI(vgGetPathCapabilities)
	{
		vgsPATH_PTR path;

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		path = (vgsPATH_PTR) Path;

		/* Return the capabilities. */
		capabilities = path->capabilities;
	}
	vgmLEAVEAPI(vgGetPathCapabilities);

	/* Return the capabilities. */
	return capabilities;
}


/*******************************************************************************
**
** vgAppendPath
**
** vgAppendPath appends a copy of all path segments from SourcePath onto the
** end of the existing data in DestinationPath. It is legal for SourcePath and
** DestinationPath to be handles to the same path object, in which case the
** contents of the path are duplicated. If SourcePath and DestinationPath are
** handles to distinct path objects, the contents of SourcePath will not be
** affected by the call.
**
** The VG_PATH_CAPABILITY_APPEND_FROM capability must be enabled for
** SourcePath, and the VG_PATH_CAPABILITY_APPEND_TO capability must be
** enabled for DestinationPath.
**
** If the scale and bias of DestinationPath define a narrower range than that
** of SourcePath, overflow may occur silently.
**
** INPUT:
**
**    DestinationPath
**       Handle to a valid VGPath object to append the source segments to.
**
**    SourcePath
**       Handle to a valid VGPath object to append the source segments from.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgAppendPath(
	VGPath DestinationPath,
	VGPath SourcePath
	)
{
	vgmENTERAPI(vgAppendPath)
	{
		gceSTATUS status;
		vgsPATH_PTR dstPath;
		vgsPATH_PTR srcPath;
		vgsPATHWALKER source;
		vgsPATHWALKER destination;
		vgtSEGMENTHANDLER * appendSegment;
		gctUINT appendCount;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X);\n",
			__FUNCTION__,
			DestinationPath, SourcePath
			);


		/***********************************************************************
		** Validate inputs.
		*/

		/* Validate the destination path object. */
		if (!vgfVerifyUserObject(Context, DestinationPath, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Validate the source path object. */
		if (!vgfVerifyUserObject(Context, SourcePath, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the objects. */
		dstPath = (vgsPATH_PTR) DestinationPath;
		srcPath = (vgsPATH_PTR) SourcePath;

		/* Verify the capabilities. */
		if ((dstPath->capabilities & VG_PATH_CAPABILITY_APPEND_TO) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			break;
		}

		if ((srcPath->capabilities & VG_PATH_CAPABILITY_APPEND_FROM) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			break;
		}


		/***********************************************************************
		** Prepare to walk the paths.
		*/

		/* Is there data in the source? */
		if (srcPath->numSegments == 0)
		{
			break;
		}

		/* Initialize walkers. */
		vgsPATHWALKER_InitializeReader(
			Context, Context->pathStorage, &source, gcvNULL, srcPath
			);

		vgsPATHWALKER_InitializeWriter(
			Context, Context->pathStorage, &destination, dstPath
			);

		/* Get append function array. */
		vgfGetAppendArray(&appendSegment, &appendCount);


		/***********************************************************************
		** Make sure the destination path is not in use.
		*/

		status = _WaitForPathIdle(Context, dstPath);

		if (gcmIS_ERROR(status))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}


		/***********************************************************************
		** Append the source path.
		*/

		while (gcvTRUE)
		{
			/* Get the current command. */
			gceVGCMD command = source.command;

			/* Call the handler. */
			gcmASSERT((gctUINT) command < appendCount);
			status = appendSegment[command] (&destination, &source);

			if (gcmIS_ERROR(status))
			{
				vgmERROR(VG_OUT_OF_MEMORY_ERROR);
				break;
			}

			/* Advance to the next segment. */
			status = vgsPATHWALKER_NextSegment(&source);

			/* Finished? */
			if (status == gcvSTATUS_NO_MORE_DATA)
			{
				/* Close the buffer. */
				status = vgsPATHWALKER_DoneWriting(&destination);

				if (gcmIS_ERROR(status))
				{
					vgmERROR(VG_OUT_OF_MEMORY_ERROR);
					break;
				}

				/* Invalidate stroke path. */
				dstPath->strokeValid = gcvFALSE;

				/* Success. */
				break;
			}
		}

		/* Roll back. */
		if (gcmIS_ERROR(status))
		{
			/* Free path data. */
			vgsPATHWALKER_Rollback(&destination);
		}
	}
	vgmLEAVEAPI(vgAppendPath);
}


/*******************************************************************************
**
** vgAppendPathData
**
** vgAppendPathData appends data taken from PathData to the given path
** DestinationPath. The data are formatted using the path format of
** DestinationPath (as returned by querying the path's VG_PATH_FORMAT
** parameter using vgGetParameteri). The NumSegments parameter gives
** the total number of entries in the PathSegments array, and must be
** greater than 0. Legal values for the PathSegments array are the values
** from the VGPathCommand enumeration as well as VG_CLOSE_PATH and
** (VG_CLOSE_PATH | VG_RELATIVE) (which are synonymous).
**
** The PathData pointer must be aligned on a 1-, 2-, or 4-byte boundary
** depending on the size of the coordinate datatype (as returned by querying
** the path's VG_PATH_DATATYPE parameter using vgGetParameteri).
** The VG_PATH_CAPABILITY_APPEND_TO capability must be enabled for path.
**
** Each incoming coordinate value, regardless of datatype, is transformed
** by the scale factor and bias of the path.
**
** INPUT:
**
**    DestinationPath
**       Handle to a valid VGPath object to append the data to.
**
**    NumSegments
**       The total number of entries in the PathSegments array.
**
**    PathSegments
**       Pointer to an array of path segment commands (VGPathCommand).
**
**    PathData
**       Pointer to the coordinate data to append.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgAppendPathData(
	VGPath DestinationPath,
	VGint NumSegments,
	const VGubyte * PathSegments,
	const void * PathData
	)
{
	vgmENTERAPI(vgAppendPathData)
	{
		gceSTATUS status;
		vgsPATH_PTR dstPath;
		vgsPATHWALKER source;
		vgsPATHWALKER destination;
		gctUINT datatypeSize;
		gctUINT currentSegment;
		vgtSEGMENTHANDLER * importSegment;
		gctUINT importCount;


		/***********************************************************************
		** Validate inputs.
		*/

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, DestinationPath, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		dstPath = (vgsPATH_PTR) DestinationPath;

		/* Verify the capability. */
		if ((dstPath->capabilities & VG_PATH_CAPABILITY_APPEND_TO) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			break;
		}

		/* Get the datatype size. */
		datatypeSize = vgfGetPathDataSize(dstPath->halDataType);

		/* Verify alignment. */
		if (vgmIS_MISSALIGNED(PathData, datatypeSize))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate the number of segments. */
		if (NumSegments <= 0)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate the data pointers. */
		if ((PathSegments == gcvNULL) || (PathData == gcvNULL))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		vgmDUMP_PATH_RAW_DATA(
			dstPath->datatype, NumSegments, PathSegments, PathData
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, %d, PathSegments, PathData);\n",
			__FUNCTION__,
			DestinationPath, NumSegments
			);


		/***********************************************************************
		** Prepare to walk the paths.
		*/

		/* Initialize walkers. */
		vgsPATHWALKER_InitializeImport(
			Context, Context->pathStorage, &source, dstPath, PathData
			);

		vgsPATHWALKER_InitializeWriter(
			Context, Context->pathStorage, &destination, dstPath
			);

		/* Get import function array. */
		vgfGetImportArray(&importSegment, &importCount);

		/* Initialize the first segment. */
		currentSegment = 0;


		/***********************************************************************
		** Make sure the destination path is not in use.
		*/

		status = _WaitForPathIdle(Context, dstPath);

		if (gcmIS_ERROR(status))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}


		/***********************************************************************
		** Import the path data.
		*/

		while (gcvTRUE)
		{
			/* Get the segment command. */
			VGPathCommand command = PathSegments[currentSegment];

			/* Validate the code. */
			if ((gctUINT) command > VG_LCWARC_TO_REL)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				status = gcvSTATUS_INVALID_ARGUMENT;
				break;
			}

			/* Import the segment. */
			gcmASSERT((gctUINT) command < importCount);
			status = importSegment[command] (&destination, &source);

			if (gcmIS_ERROR(status))
			{
				vgmERROR(VG_OUT_OF_MEMORY_ERROR);
				break;
			}

			/* Advance to the next segment. */
			currentSegment += 1;

			/* Finished? */
			if (currentSegment == NumSegments)
			{
				/* Close the buffer. */
				status = vgsPATHWALKER_DoneWriting(&destination);

				if (gcmIS_ERROR(status))
				{
					vgmERROR(VG_OUT_OF_MEMORY_ERROR);
					break;
				}

				/* Invalidate stroke path. */
				dstPath->strokeValid = gcvFALSE;

				/* Success. */
				break;
			}
		}

		/* Roll back. */
		if (gcmIS_ERROR(status))
		{
			/* Free path data. */
			vgsPATHWALKER_Rollback(&destination);
		}
	}
	vgmLEAVEAPI(vgAppendPathData);
}


/*******************************************************************************
**
** vgModifyPathCoords
**
** vgModifyPathCoords modifies the coordinate data for a contiguous range of
** segments of DestinationPath, starting at StartSegment (where 0 is the index
** of the first path segment) and having length NumSegments. The data in
** PathData must be formatted in exactly the same manner as the original
** coordinate data for the given segment range, unless the path has been
** transformed using vgTransformPath or interpolated using vgInterpolatePath.
** In these cases, the path will have been subject to the segment promotion
** rules specified in those functions.
**
** The PathData pointer must be aligned on a 1-, 2-, or 4-byte boundary
** depending on the size of the coordinate datatype (as returned by querying
** the path's VG_PATH_DATATYPE parameter using vgGetParameteri). The
** VG_PATH_CAPABILITY_MODIFY capability must be enabled for path.
**
** Each incoming coordinate value, regardless of datatype, is transformed
** by the scale factor and bias of the path.
**
** INPUT:
**
**    DestinationPath
**       Handle to a valid VGPath object to append the data to.
**
**    StartSegment
**    NumSegments
**       The subpath to be modified.
**
**    PathData
**       Pointer to the coordinate data.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgModifyPathCoords(
	VGPath DestinationPath,
	VGint StartSegment,
	VGint NumSegments,
	const void * PathData
	)
{
	vgmENTERAPI(vgModifyPathCoords)
	{
		gceSTATUS status;
		vgsPATH_PTR dstPath;
		vgsPATHWALKER source;
		vgsPATHWALKER destination;
		vgsCONTROL_COORD_PTR controlCoordinates;
		vgtSEGMENTHANDLER * handler;
		gctUINT entryCount;
		gctUINT currentSegment;
		gctUINT lastSegment;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, %d, %d, PathData);\n",
			__FUNCTION__,
			DestinationPath, StartSegment, NumSegments
			);


		/***********************************************************************
		** Validate inputs.
		*/

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, DestinationPath, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		dstPath = (vgsPATH_PTR) DestinationPath;

		/* Verify the capabilities. */
		if ((dstPath->capabilities & VG_PATH_CAPABILITY_MODIFY) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			break;
		}

		/* Validate the arguments. */
		if ((PathData == gcvNULL)  ||
			(StartSegment <  0)    ||
			(NumSegments  <= 0)    ||
			(StartSegment + NumSegments > dstPath->numSegments))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}


		/***********************************************************************
		** Prepare to walk the paths.
		*/

		/* Set the pointer to the control coordinates. */
		controlCoordinates = &dstPath->coords;

		/* Reset the control coordinates. */
		controlCoordinates->startX   = 0.0f;
		controlCoordinates->startY   = 0.0f;
		controlCoordinates->lastX    = 0.0f;
		controlCoordinates->lastY    = 0.0f;
		controlCoordinates->controlX = 0.0f;
		controlCoordinates->controlY = 0.0f;

		/* Initialize walkers. */
		vgsPATHWALKER_InitializeImport(
			Context, Context->pathStorage, &source, dstPath, PathData
			);

		vgsPATHWALKER_InitializeReader(
			Context, Context->pathStorage, &destination, controlCoordinates, dstPath
			);

		/* Get the inital function array. */
		if (StartSegment == 0)
		{
			vgfGetModifyArray(&handler, &entryCount);
		}
		else
		{
			vgfGetSkipArray(&handler, &entryCount);
		}

		/* Initialize the first and last segments. */
		currentSegment = 0;

		/* Determine the last segment. */
		lastSegment = StartSegment + NumSegments;


		/***********************************************************************
		** Make sure the destination path is not in use.
		*/

		status = _WaitForPathIdle(Context, dstPath);

		if (gcmIS_ERROR(status))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}


		/***********************************************************************
		** Modify the coordinates.
		*/

		while (gcvTRUE)
		{
			/* Get the current command. */
			gceVGCMD command = destination.command;

			/* Modify coordinates. */
			gcmASSERT((gctUINT) command < entryCount);
			status = handler[command] (&destination, &source);

			/* Error? */
			if (gcmIS_ERROR(status))
			{
				vgmERROR(VG_OUT_OF_MEMORY_ERROR);
				break;
			}

			/* Finished? */
			if (status == gcvSTATUS_NO_MORE_DATA)
			{
				break;
			}

			/* Update the segment counter. */
			currentSegment += 1;

			/* Skipping finished? */
			if (currentSegment == StartSegment)
			{
				/* Get the modification function array. */
				vgfGetModifyArray(&handler, &entryCount);
			}

			/* Still more to modify? */
			if (currentSegment < lastSegment)
			{
				/* Advance to the next segment. */
				gcmVERIFY_OK(vgsPATHWALKER_NextSegment(&destination));

				/* Close the loop. */
				continue;
			}

			/* Done with modifications? */
			if (currentSegment == lastSegment)
			{
				/* Get the modification function array. */
				vgfGetUpdateArray(&handler, &entryCount);
			}

			/* Advance to the next segment. */
			status = vgsPATHWALKER_NextSegment(&destination);

			/* Finished? */
			if (status == gcvSTATUS_NO_MORE_DATA)
			{
				break;
			}
		}

		/* Invalidate stroke path. */
		dstPath->strokeValid = gcvFALSE;

		/* Finalize the path structure. */
		gcmVERIFY_OK(gcoVG_FinalizePath(
			Context->vg, &dstPath->head->data
			));
	}
	vgmLEAVEAPI(vgModifyPathCoords);
}


/*******************************************************************************
**
** vgTransformPath
**
** vgTransformPath appends a transformed copy of SourcePath to the current
** contents of DestinationPath. The appended path is equivalent to the results
** of applying the current path user-to-surface transformation
** (VG_MATRIX_PATH_USER_TO_SURFACE) to SourcePath.
**
** It is legal for SourcePath and DestinationPath to be handles to the same
** path object, in which case the transformed path will be appended to the
** existing path. If SourcePath and DestinationPath are handles to distinct
** path objects, the contents of SourcePath will not be affected by the call.
**
** All HLINE_TO_* and VLINE_TO_* segments present in SourcePath are implicitly
** converted to LINE_TO_* segments prior to applying the transformation. The
** original copies of these segments in SourcePath remain unchanged.
**
** Any *ARC_TO segments are transformed, but the endpoint parametrization of
** the resulting arc segments are implementation-dependent. The results of
** calling vgInterpolatePath on a transformed path that contains such segments
** are undefined.
**
** The VG_PATH_CAPABILITY_TRANSFORM_FROM capability must be enabled for
** SourcePath, and the VG_PATH_CAPABILITY_TRANSFORM_TO capability must be
** enabled for DestinationPath.
**
** Overflow may occur silently if coordinates are transformed outside the
** datatype range of DestinationPath.
**
** INPUT:
**
**    DestinationPath
**       Handle to a valid VGPath object to append the transformed path to.
**
**    SourcePath
**       Handle to a valid VGPath object for the source path.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgTransformPath(
	VGPath DestinationPath,
	VGPath SourcePath
	)
{
	vgmENTERAPI(vgTransformPath)
	{
		gceSTATUS status;
		vgsPATH_PTR dstPath;
		vgsPATH_PTR srcPath;
		vgsPATHWALKER source;
		vgsPATHWALKER destination;
		vgtTRANSFORMFUNCTION * transformSegment;
		gctUINT transformCount;
		vgsMATRIX_PTR matrix;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, %d, %d, PathData);\n",
			__FUNCTION__,
			DestinationPath, SourcePath
			);


		/***********************************************************************
		** Validate inputs.
		*/

		/* Validate the destination path object. */
		if (!vgfVerifyUserObject(Context, DestinationPath, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Validate the source path object. */
		if (!vgfVerifyUserObject(Context, SourcePath, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the objects. */
		dstPath = (vgsPATH_PTR) DestinationPath;
		srcPath = (vgsPATH_PTR) SourcePath;

		/* Verify the capabilities. */
		if ((dstPath->capabilities & VG_PATH_CAPABILITY_TRANSFORM_TO) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			break;
		}

		if ((srcPath->capabilities & VG_PATH_CAPABILITY_TRANSFORM_FROM) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			break;
		}


		/***********************************************************************
		** Prepare to walk the paths.
		*/

		/* Is there data in the source? */
		if (srcPath->numSegments == 0)
		{
			break;
		}

		/* Initialize walkers. */
		vgsPATHWALKER_InitializeReader(
			Context, Context->pathStorage, &source, gcvNULL, srcPath
			);

		vgsPATHWALKER_InitializeWriter(
			Context, Context->pathStorage, &destination, dstPath
			);

		/* Get the transform function array. */
		vgfGetTransformArray(&transformSegment, &transformCount);

		/* Get the transformation matrix. */
		matrix = vgmGETMATRIX(&Context->pathUserToSurface);


		/***********************************************************************
		** Make sure the destination path is not in use.
		*/

		status = _WaitForPathIdle(Context, dstPath);

		if (gcmIS_ERROR(status))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}


		/***********************************************************************
		** Transform the path.
		*/

		while (gcvTRUE)
		{
			/* Get the current command. */
			gceVGCMD command = source.command;

			/* Transform the current segment. */
			gcmASSERT((gctUINT) command < transformCount);
			status = transformSegment[command] (
				&destination, &source, matrix
				);

			if (gcmIS_ERROR(status))
			{
				vgmERROR(VG_OUT_OF_MEMORY_ERROR);
				break;
			}

			/* Advance to the next segment. */
			status = vgsPATHWALKER_NextSegment(&source);

			/* Finished? */
			if (status == gcvSTATUS_NO_MORE_DATA)
			{
				/* Close the buffer. */
				status = vgsPATHWALKER_DoneWriting(&destination);

				if (gcmIS_ERROR(status))
				{
					vgmERROR(VG_OUT_OF_MEMORY_ERROR);
					break;
				}

				/* Invalidate stroke path. */
				dstPath->strokeValid = gcvFALSE;

				/* Success. */
				break;
			}
		}

		/* Roll back. */
		if (gcmIS_ERROR(status))
		{
			/* Free path data. */
			vgsPATHWALKER_Rollback(&destination);
		}
	}
	vgmLEAVEAPI(vgTransformPath);
}


/*******************************************************************************
**
** vgInterpolatePath
**
** The vgInterpolatePath function appends a path, defined by interpolation (or
** extrapolation) between the paths StartPath and EndPath by the given amount,
** to the path DestinationPath. It returns VG_TRUE if interpolation was
** successful (i.e., the paths had compatible segment types after normaliza-
** tion), and VG_FALSE otherwise. If interpolation is unsuccessful,
** DestinationPath is left unchanged.
**
** It is legal for DestinationPath to be a handle to the same path object as
** either StartPath or EndPath or both, in which case the contents of the
** source path or paths referenced by DestinationPath will have the interpo-
** lated path appended. If DestinationPath is not the handle to the same path
** object as either StartPath or EndPath, the contents of StartPath and EndPath
** will not be affected by the call.
**
** Overflow may occur silently if the datatype of DestinationPath has insuffi-
** cient range to store an interpolated coordinate value.
**
** The VG_PATH_CAPABILITY_INTERPOLATE_FROM capability must be enabled for both
** of StartPath and EndPath, and the INTERPOLATE_TO capability must be enabled
** for DestinationPath.
**
** INPUT:
**
**    DestinationPath
**       Handle to a valid VGPath object to append the transformed path to.
**
**    StartPath
**    EndPath
**       Handles to valid VGPath objects for the source paths.
**
**    Amount
**       Interpolation amount.
**
** OUTPUT:
**
**    VGboolean
**       VG_TRUE if the function succeedes.
*/

VG_API_CALL VGboolean VG_API_ENTRY vgInterpolatePath(
	VGPath DestinationPath,
	VGPath StartPath,
	VGPath EndPath,
	VGfloat Amount
	)
{
	VGboolean result = VG_TRUE;

	vgmENTERAPI(vgInterpolatePath)
	{
		gceSTATUS status;

		/* Path objects. */
		vgsPATH_PTR startPath;
		vgsPATH_PTR endPath;
		vgsPATH_PTR dstPath;

		/* Path and command walkers. */
		vgsPATHWALKER start, end;
		vgsPATHWALKER destination;
		gceVGCMD startGroup;
		gceVGCMD startCommand;
		gceVGCMD endGroup;
		gceVGCMD endCommand;
		gceVGCMD dstCommand;

		/* Coordinates for interpolation. */
		gctFLOAT startCoordinates[6];
		gctFLOAT endCoordinates[6];

		vgsCONTROL_COORD startCC;
		vgsCONTROL_COORD endCC;

		/* Function arrays. */
		vgtNORMALIZEFUNCTION * normalizeSegment;
		vgtINTERPOLATEFUNCTION * interpolateSegment;
		gctUINT normalizeCount, interpolateCount;

		/* Source preference. */
		gctBOOL pickStart;

		/* Loop variables. */
		gctUINT segmentsLeft;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X, 0x%08X, %.10ff);\n",
			__FUNCTION__,
			DestinationPath, StartPath, EndPath, Amount
			);


		/***********************************************************************
		** Validate inputs.
		*/

		/* Validate the start path object. */
		if (!vgfVerifyUserObject(Context, StartPath, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			result = gcvFALSE;
			break;
		}

		/* Validate the end path object. */
		if (!vgfVerifyUserObject(Context, EndPath, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			result = gcvFALSE;
			break;
		}

		/* Validate the destination path object. */
		if (!vgfVerifyUserObject(Context, DestinationPath, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			result = gcvFALSE;
			break;
		}

		startPath = (vgsPATH_PTR) StartPath;
		endPath   = (vgsPATH_PTR) EndPath;
		dstPath   = (vgsPATH_PTR) DestinationPath;

		if ((startPath->capabilities & VG_PATH_CAPABILITY_INTERPOLATE_FROM) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			result = VG_FALSE;
			break;
		}

		if ((endPath->capabilities & VG_PATH_CAPABILITY_INTERPOLATE_FROM) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			result = VG_FALSE;
			break;
		}

		if ((dstPath->capabilities & VG_PATH_CAPABILITY_INTERPOLATE_TO) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			result = VG_FALSE;
			break;
		}

		/* Source paths must have the same, greater then zero, sizes. */
		if ((startPath->numSegments == 0) ||
			(startPath->numSegments != endPath->numSegments))
		{
			result = VG_FALSE;
			break;
		}


		/***********************************************************************
		** Prepare to walk the paths.
		*/

		/* Initialize walkers. */
		vgsPATHWALKER_InitializeReader(
			Context, Context->pathStorage, &start, &startCC, startPath
			);

		vgsPATHWALKER_InitializeReader(
			Context, Context->pathStorage, &end, &endCC, endPath
			);

		vgsPATHWALKER_InitializeWriter(
			Context, Context->pathStorage, &destination, dstPath
			);

		vgfGetNormalizationArray(&normalizeSegment, &normalizeCount);
		vgfGetInterpolationArray(&interpolateSegment, &interpolateCount);

		/* Reset the control coordinates. */
		startCC.startX   = 0.0f;
		startCC.startY   = 0.0f;
		startCC.lastX    = 0.0f;
		startCC.lastY    = 0.0f;
		startCC.controlX = 0.0f;
		startCC.controlY = 0.0f;

		endCC.startX   = 0.0f;
		endCC.startY   = 0.0f;
		endCC.lastX    = 0.0f;
		endCC.lastY    = 0.0f;
		endCC.controlX = 0.0f;
		endCC.controlY = 0.0f;

		/* Determine source preference. */
		pickStart = (Amount < 0.5f);

		/* Set the number of segments to interpolate. */
		segmentsLeft = startPath->numSegments;


		/***********************************************************************
		** Make sure the destination path is not in use.
		*/

		status = _WaitForPathIdle(Context, dstPath);

		if (gcmIS_ERROR(status))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}


		/***********************************************************************
		** Interpolate paths.
		*/

		while (gcvTRUE)
		{
			/* Normalize start path segment for iterpolation. */
			normalizeSegment[start.command] (
				&start, startCoordinates, &startGroup, &startCommand
				);

			/* Normalize end path segment for iterpolation. */
			normalizeSegment[end.command] (
				&end, endCoordinates, &endGroup, &endCommand
				);

			/* Make sure the commands are compatible. */
			if (startGroup != endGroup)
			{
				result = VG_FALSE;
				status = gcvSTATUS_INVALID_ARGUMENT;
				break;
			}

			/* Determine the destination VG command. */
			dstCommand = pickStart ? startCommand : endCommand;

			/* Interpolate between the sources. */
			status = interpolateSegment[dstCommand] (
				&destination,
				startCoordinates,
				endCoordinates,
				Amount
				);

			if (gcmIS_ERROR(status))
			{
				vgmERROR(VG_OUT_OF_MEMORY_ERROR);
				result = VG_FALSE;
				break;
			}

			/* Update the number of segments left to process. */
			segmentsLeft -= 1;

			/* Are we done? */
			if (segmentsLeft == 0)
			{
				/* Close the buffer. */
				status = vgsPATHWALKER_DoneWriting(&destination);

				if (gcmIS_ERROR(status))
				{
					vgmERROR(VG_OUT_OF_MEMORY_ERROR);
					result = VG_FALSE;
					break;
				}

				/* Invalidate stroke path. */
				dstPath->strokeValid = gcvFALSE;

				/* Success. */
				break;
			}

			/* Advance the source paths. */
			gcmVERIFY_OK(vgsPATHWALKER_NextSegment(&start));
			gcmVERIFY_OK(vgsPATHWALKER_NextSegment(&end));
		}

		/* Roll back. */
		if (gcmIS_ERROR(status))
		{
			/* Free path data. */
			vgsPATHWALKER_Rollback(&destination);
		}
	}
	vgmLEAVEAPI(vgInterpolatePath);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** vgPathLength
**
** The vgPathLength function returns the length of a given portion of a path
** in the user coordinate system (that is, in the path's own coordinate system,
** disregarding any matrix settings). Only the subpath consisting of the
** NumSegments path segments beginning with StartSegment (where the initial
** path segment has index 0) is used. If an error occurs, -1.0f is returned.
**
** The VG_PATH_CAPABILITY_PATH_LENGTH capability must be enabled for path.
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
**    StartSegment
**    NumSegments
**       The subpath the length of which is to be determined.
**
** OUTPUT:
**
**    VGfloat
**       Length of the given portion of the path.
*/

VG_API_CALL VGfloat VG_API_ENTRY vgPathLength(
	VGPath Path,
	VGint StartSegment,
	VGint NumSegments
	)
{
	VGfloat pathLength = -1.0f;

	vgmENTERAPI(vgPathLength)
	{
		gctBOOL success;
		vgsPATH_PTR path;

		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		path = (vgsPATH_PTR) Path;

		/* Verify the capabilities. */
		if ((path->capabilities & VG_PATH_CAPABILITY_PATH_LENGTH) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			break;
		}

		/* Validate the arguments. */
		if ((StartSegment <  0)    ||
			(NumSegments  <= 0)    ||
			(StartSegment + NumSegments > path->numSegments))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Determine the length of the path. */
		success = vgfComputePointAlongPath(
			Context,
			path,
			StartSegment,
			NumSegments,
			gcvMAX_POS_FLOAT,		/* Point far away. */
			gcvNULL, gcvNULL,		/* Don't need the coordinates. */
			gcvNULL, gcvNULL,		/* Don't need the tangent. */
			&pathLength,
			gcvNULL, gcvNULL,		/* Don't need the box. */
			gcvNULL, gcvNULL
			);

		/* Empty path? */
		if (!success)
		{
			pathLength = 0.0f;
		}
	}
	vgmLEAVEAPI(vgPathLength);

	/* Return result. */
	return pathLength;
}


/*******************************************************************************
**
** vgPointAlongPath
**
** The vgPointAlongPath function returns the point lying a given distance
** along a given portion of a path and the unit-length tangent vector at that
** point. Only the subpath consisting of the NumSegments path segments
** beginning with StartSegment (where the initial path segment has index 0) is
** used. For the remainder of this section we refer only to this subpath when
** discussing paths.
**
** If Distance is less than or equal to 0, the starting point of the path is
** used. If Distance is greater than or equal to the path length (i.e., the
** value returned by vgPathLength when called with the same StartSegment and
** NumSegments parameters), the visual ending point of the path is used.
**
** Intermediate values return the (X, Y) coordinates and tangent vector of the
** point at the given distance along the path. Because it is not possible in
** general to compute exact distances along a path, an implementation is not
** required to use exact computation even for segments where such computation
** would be possible. For example, the path:
**
**    MOVE_TO 0, 0; LINE_TO 10, 0   // Draw a line of length 10.
**    MOVE_TO 10, 10                // Create a discontinuity.
**    LINE_TO 10, 20                // Draw a line of length 10.
**
** may return either (10, 0) or (10, 10) (or points nearby) as the point at
** distance 10.0. Implementations are not required to compute distances
** exactly, as long as they satisfy the constraint that as distance increases
** monotonically the returned point and tangent move forward monotonically
** along the path.
**
** Where the implementation is able to determine that the point being queried
** lies exactly at a discontinuity or cusp, the incoming point and tangent
** should be returned. In the example above, returning the pre-discontinuity
** point (10, 0) and incoming tangent (1, 0) is preferred to returning the
** post-discontinuity point (10, 10) and outgoing tangent (0, 1).
**
** The VG_PATH_CAPABILITY_POINT_ALONG_PATH capability must be enabled for path.
**
** If the reference arguments X and Y are both non-gcvNULL, and the
** VG_PATH_CAPABILITY_POINT_ALONG_PATH capability is enabled for path, the
** point (X, Y) is returned in X and Y. Otherwise the variables referenced by
** x and y are not written.
**
** If the reference arguments tangentX and tangentY are both non-gcvNULL, and the
** VG_PATH_CAPABILITY_TANGENT_ALONG_PATH capability is enabled for path, the
** geometric tangent vector at the point (x, y) is returned in tangentX and
** tangentY. Otherwise the variables referenced by tangentX and tangentY are
** not written.
**
** Where the incoming tangent is defined, vgPointAlongPath returns it. Where
** only the outgoing tangent is defined, the outgoing tangent is returned.
**
** The points returned by vgPointAlongPath are not guaranteed to match the path
** as rendered; some deviation is to be expected.
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
**    StartSegment
**    NumSegments
**       The subpath to use in point determination.
**
**    Distance
**       Distance along the subpath.
**
** OUTPUT:
**
**    X, Y
**    TangentX, TangentY
**       Computed coordinates and tangent of the point.
*/

VG_API_CALL void VG_API_ENTRY vgPointAlongPath(
	VGPath Path,
	VGint StartSegment,
	VGint NumSegments,
	VGfloat Distance,
	VGfloat * X,
	VGfloat * Y,
	VGfloat * TangentX,
	VGfloat * TangentY
	)
{
	vgmENTERAPI(vgPointAlongPath)
	{
		gctBOOL success;
		vgsPATH_PTR path;

		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		path = (vgsPATH_PTR) Path;

		/* Verify the capabilities. */
		if ((X != gcvNULL) && (Y != gcvNULL))
		{
			if ((path->capabilities & VG_PATH_CAPABILITY_POINT_ALONG_PATH) == 0)
			{
				vgmERROR(VG_PATH_CAPABILITY_ERROR);
				break;
			}

			if (vgmIS_MISSALIGNED(X, 4) || vgmIS_MISSALIGNED(Y, 4))
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}
		}

		if ((TangentX != gcvNULL) && (TangentY != gcvNULL))
		{
			if ((path->capabilities & VG_PATH_CAPABILITY_TANGENT_ALONG_PATH) == 0)
			{
				vgmERROR(VG_PATH_CAPABILITY_ERROR);
				break;
			}

			if (vgmIS_MISSALIGNED(TangentX, 4) || vgmIS_MISSALIGNED(TangentY, 4))
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}
		}

		/* Validate the arguments. */
		if ((StartSegment <  0)    ||
			(NumSegments  <= 0)    ||
			(StartSegment + NumSegments > path->numSegments))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Determine the length of the path. */
		success = vgfComputePointAlongPath(
			Context,
			path,
			StartSegment,
			NumSegments,
			Distance,
			X, Y,
			TangentX, TangentY,
			gcvNULL,				/* Don't need the length. */
			gcvNULL, gcvNULL,		/* Don't need the box. */
			gcvNULL, gcvNULL
			);

		/* Empty path? */
		if (!success)
		{
			if ((X != gcvNULL) && (Y != gcvNULL))
			{
				* X = 0.0f;
				* Y = 0.0f;
			}

			if ((TangentX != gcvNULL) && (TangentY != gcvNULL))
			{
				* TangentX = 1.0f;
				* TangentY = 0.0f;
			}
		}
	}
	vgmLEAVEAPI(vgPointAlongPath);
}


/*******************************************************************************
**
** vgPathBounds
**
** The vgPathBounds function returns an axis-aligned bounding box that tightly
** bounds the interior of the given path. Stroking parameters are ignored. If
** path is empty, MinX and MinY are set to 0 and Width and Height are set to -1.
** If path contains a single point, MinX and MinY are set to the coordinates of
** the point and Width and Height are set to 0.
**
** The VG_PATH_CAPABILITY_PATH_BOUNDS capability must be enabled for path.
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
** OUTPUT:
**
**    MinX, MinY
**    Width, Height
**       Computed bounding box.
*/

VG_API_CALL void VG_API_ENTRY vgPathBounds(
	VGPath Path,
	VGfloat * MinX,
	VGfloat * MinY,
	VGfloat * Width,
	VGfloat * Height
	)
{
	vgmENTERAPI(vgPathBounds)
	{
		gctBOOL success;
		vgsPATH_PTR path;
		gctFLOAT pathLeft;
		gctFLOAT pathTop;
		gctFLOAT pathRight;
		gctFLOAT pathBottom;

		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		path = (vgsPATH_PTR) Path;

		/* Verify the capabilities. */
		if ((path->capabilities & VG_PATH_CAPABILITY_PATH_BOUNDS) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			break;
		}

		/* Validate the arguments. */
		if ((MinX  == gcvNULL) || (MinY   == gcvNULL) ||
			(Width == gcvNULL) || (Height == gcvNULL))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Verify alignment. */
		if (vgmIS_MISSALIGNED(MinX,  4) || vgmIS_MISSALIGNED(MinY,   4) ||
			vgmIS_MISSALIGNED(Width, 4) || vgmIS_MISSALIGNED(Height, 4))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Determine the length of the path. */
		success = vgfComputePointAlongPath(
			Context,
			path,
			0, path->numSegments,	/* Walk the entire path. */
			gcvMAX_POS_FLOAT,		/* Point far away. */
			gcvNULL, gcvNULL,		/* Don't need the coordinates. */
			gcvNULL, gcvNULL,		/* Don't need the tangent. */
			gcvNULL,				/* Don't need the length. */
			&pathLeft, &pathTop,
			&pathRight, &pathBottom
			);

		/* Path not empty? */
		if (success)
		{
			* MinX   = pathLeft;
			* MinY   = pathTop;
			* Width  = pathRight  - pathLeft;
			* Height = pathBottom - pathTop;
		}
		else
		{
			* MinX   = 0;
			* MinY   = 0;
			* Width  = -1;
			* Height = -1;
		}
	}
	vgmLEAVEAPI(vgPathBounds);
}


/*******************************************************************************
**
** vgPathTransformedBounds
**
** The vgPathTransformedBounds function returns an axis-aligned bounding box
** that is guaranteed to enclose the geometry of the given path following
** transformation by the current path-user-to-surface transform. The returned
** bounding box is not guaranteed to fit tightly around the path geometry. If
** path is empty, MinX and MinY are set to 0 and Width and Height are set to -1.
** If path contains a single point, MinX and MinY are set to the transformed
** coordinates of the point and Width and Height are set to 0.
**
** The VG_PATH_CAPABILITY_PATH_TRANSFORMED_BOUNDS capability must be enabled
** for path.
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
** OUTPUT:
**
**    MinX, MinY
**    Width, Height
**       Computed transformed bounding box.
*/

VG_API_CALL void VG_API_ENTRY vgPathTransformedBounds(
	VGPath Path,
	VGfloat * MinX,
	VGfloat * MinY,
	VGfloat * Width,
	VGfloat * Height
	)
{
	vgmENTERAPI(vgPathTransformedBounds)
	{
		gctBOOL success;
		vgsPATH_PTR path;
		gctFLOAT pathLeft, pathTop, pathRight, pathBottom;
		gctFLOAT xformLeft, xformTop, xformRight, xformBottom;

		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		path = (vgsPATH_PTR) Path;

		/* Verify the capabilities. */
		if ((path->capabilities & VG_PATH_CAPABILITY_PATH_TRANSFORMED_BOUNDS) == 0)
		{
			vgmERROR(VG_PATH_CAPABILITY_ERROR);
			break;
		}

		/* Validate the arguments. */
		if ((MinX  == gcvNULL) || (MinY   == gcvNULL) ||
			(Width == gcvNULL) || (Height == gcvNULL))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Verify alignment. */
		if (vgmIS_MISSALIGNED(MinX,  4) || vgmIS_MISSALIGNED(MinY,   4) ||
			vgmIS_MISSALIGNED(Width, 4) || vgmIS_MISSALIGNED(Height, 4))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Determine the length of the path. */
		success = vgfComputePointAlongPath(
			Context,
			path,
			0, path->numSegments,	/* Walk the entire path. */
			gcvMAX_POS_FLOAT,		/* Point far away. */
			gcvNULL, gcvNULL,		/* Don't need the coordinates. */
			gcvNULL, gcvNULL,		/* Don't need the tangent. */
			gcvNULL,				/* Don't need the length. */
			&pathLeft, &pathTop,
			&pathRight, &pathBottom
			);

		/* Path not empty? */
		if (success)
		{
			vgtFLOATVECTOR3 lt, rt, lb, rb;

			/* Initialize left-top corner point. */
			lt[0] = pathLeft;
			lt[1] = pathTop;
			lt[2] = 1.0f;

			/* Initialize left-top corner point. */
			rt[0] = pathRight;
			rt[1] = pathTop;
			rt[2] = 1.0f;

			/* Initialize left-top corner point. */
			lb[0] = pathLeft;
			lb[1] = pathBottom;
			lb[2] = 1.0f;

			/* Initialize left-top corner point. */
			rb[0] = pathRight;
			rb[1] = pathBottom;
			rb[2] = 1.0f;

			/* Transform the corners. */
			vgfMultiplyVector3ByMatrix3x3(
				lt, vgmGETMATRIX(&Context->pathUserToSurface), lt
				);

			vgfMultiplyVector3ByMatrix3x3(
				rt, vgmGETMATRIX(&Context->pathUserToSurface), rt
				);

			vgfMultiplyVector3ByMatrix3x3(
				lb, vgmGETMATRIX(&Context->pathUserToSurface), lb
				);

			vgfMultiplyVector3ByMatrix3x3(
				rb, vgmGETMATRIX(&Context->pathUserToSurface), rb
				);

			/* Determine the normalized box. */
			xformLeft   = gcmMIN(gcmMIN(gcmMIN(lt[0], rt[0]), lb[0]), rb[0]);
			xformTop    = gcmMIN(gcmMIN(gcmMIN(lt[1], rt[1]), lb[1]), rb[1]);
			xformRight  = gcmMAX(gcmMAX(gcmMAX(lt[0], rt[0]), lb[0]), rb[0]);
			xformBottom = gcmMAX(gcmMAX(gcmMAX(lt[1], rt[1]), lb[1]), rb[1]);

			/* Set the result. */
			* MinX   = xformLeft;
			* MinY   = xformTop;
			* Width  = xformRight  - xformLeft;
			* Height = xformBottom - xformTop;
		}
		else
		{
			* MinX   = 0;
			* MinY   = 0;
			* Width  = -1;
			* Height = -1;
		}
	}
	vgmLEAVEAPI(vgPathTransformedBounds);
}


/*******************************************************************************
**
** vgDrawPath
**
** Filling and stroking are performed by the vgDrawPath function.
** The PaintModes argument is a bitwise OR of values from the VGPaintMode
** enumeration, determining whether the path is to be filled (VG_FILL_PATH),
** stroked (VG_STROKE_PATH), or both (VG_FILL_PATH | VG_STROKE_PATH). If both
** filling and stroking are to be performed, the path is first filled,
** then stroked.
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
**    PaintModes
**       Bitwise OR of VG_FILL_PATH and/or VG_STROKE_PATH (VGPaintMode).
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgDrawPath(
	VGPath Path,
	VGbitfield PaintModes
	)
{
	vgmENTERAPI(vgDrawPath)
	{
		vgsPATH_PTR path;

		vgmADVANCE_DRAWPATH_COUNT(Context);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X);\n",
			__FUNCTION__,
			Path, PaintModes
			);

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Validate paint modes. */
		if ((PaintModes == 0) ||
			((PaintModes & ~(VG_STROKE_PATH | VG_FILL_PATH)) != 0))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Cast the object. */
		path = (vgsPATH_PTR) Path;

		/* Set the proper matrices. */
		Context->drawPathFillSurfaceToPaint
			= &Context->pathFillSurfaceToPaint;

		Context->drawPathStrokeSurfaceToPaint
			= &Context->pathStrokeSurfaceToPaint;

		/* Draw the path. */
		gcmVERIFY_OK(vgfDrawPath(
			Context,
			&Context->targetImage,
			path,
			PaintModes,
			Context->fillPaint,
			Context->strokePaint,
			Context->colTransform,
			Context->maskingEnable
			));
	}
	vgmLEAVEAPI(vgDrawPath);
}
