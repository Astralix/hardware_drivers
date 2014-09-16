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
#include <string.h>

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

#define vgvSCALAR_CALL ((VGHandle) ~0)


/*******************************************************************************
**
** _SetState
**
** Main entry for setting a new value to the given state.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    Object
**       Handle to the object of which the given state is to be set.
**
**    ObjectList
**       Pointer to the object list header.
**
**    ParamType
**       Parameter to be set.
**
**    Count
**       Value component count.
**
**    Values
**       Points to the new value to be set.
**
**    StateType
**       Type of the specified value.
**
** OUTPUT:
**
**    Nothing.
*/

static void _SetState(
	vgsCONTEXT_PTR Context,
	VGHandle Object,
	vgsOBJECT_LIST_PTR ObjectList,
	VGint ParamType,
	VGint Count,
	const void * Values,
	vgvSTATETYPE StateType
	)
{
	do
	{
		VGint i;
		VGint scalarValue;
		vgsSTATEENTRY_PTR stateEntry = gcvNULL;
		vgtVALUECONVERTER converter;

		/* Verify generic failure conditions. */
		if (
			/* Values is gcvNULL and Count is greater than 0? */
			((Values == gcvNULL) && (Count > 0)) ||

			/* Count is less than 0? */
			(Count < 0)
		   )
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Find the state entry. */
		for (i = 0; i < ObjectList->stateArraySize; i++)
		{
			/* Create a shortcut to the entry. */
			stateEntry = &ObjectList->stateArray[i];

			/* Found the entry? */
			if (stateEntry->paramType == ParamType)
			{
				break;
			}
		}

		/* Was the entry found? */
		if (i == ObjectList->stateArraySize)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}
		if (stateEntry == gcvNULL)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Verify value alignment. */
		if ((gcmPTR2INT(Values) & stateEntry->alignment) != 0)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Scalar values must have count of 1. */
		if ((stateEntry->valueCount == 1) && (Count != 1))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Array in a scalar call? */
		if ((stateEntry->valueCount > 1) && (Object == vgvSCALAR_CALL))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Get the value converter. */
		converter = stateEntry->setConverters[StateType];

		/* Scalar enumeration? */
		if (stateEntry->validCount != 0)
		{
			VGint j;
			VGint * validValues;

			/* Count has to be 1. */
			if (Count != 1)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}

			/* Convert the value. */
			converter(&scalarValue, Values, 1, VG_TRUE, VG_FALSE);

			/* Start from the first valid value. */
			validValues = stateEntry->validValues;

			/* Find the value in the array. */
			for (j = 0; j < stateEntry->validCount; j++)
			{
				if (scalarValue == *validValues++)
				{
					break;
				}
			}

			/* Was the value found? */
			if (j == stateEntry->validCount)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}

			/* Set pointer to the converted value. */
			Values = &scalarValue;
		}

		/* Vector parameter? */
		else if (stateEntry->valueUnitSize > 1)
		{
			/* Count has to be a multiple of the unit size. */
			if ((Count % stateEntry->valueUnitSize) != 0)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}

			/* Cap to the maximum. */
			if (Count > stateEntry->valueCount)
			{
				Count = stateEntry->valueCount;
			}

			/* Compute the number of units to set. */
			Count /= stateEntry->valueUnitSize;
		}

		/* Array parameter. */
		else
		{
			/* Cap to the maximum. */
			if (Count > stateEntry->valueCount)
			{
				Count = stateEntry->valueCount;
			}
		}

		/* Set the parameter. */
		if (stateEntry->setState != gcvNULL)
		{
			stateEntry->setState(Context, Object, Count, Values, converter);
		}
	}
	while (gcvFALSE);
}


/*******************************************************************************
**
** _SetObjectState
**
** Main entry for setting a new value to the given state of an object.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    Object
**       Handle to the object of which the given state is to be set.
**
**    ParamType
**       Parameter to be set.
**
**    Count
**       Value component count.
**
**    Values
**       Points to the new value to be set.
**
**    StateType
**       Type of the specified value.
**
** OUTPUT:
**
**    Nothing.
*/

static void _SetObjectState(
	vgsCONTEXT_PTR Context,
	VGHandle Object,
	VGint ParamType,
	VGint Count,
	const void * Values,
	vgvSTATETYPE StateType
	)
{
	do
	{
		vgsOBJECT_PTR object;
		vgsOBJECT_LIST_PTR objectList;

		/* Cast the object. */
		object = (vgsOBJECT_PTR) Object;

		/* Get the proper object list. */
		objectList = &Context->objectCache->cache[object->type];

		/* Set the state. */
		_SetState(
			Context, Object, objectList, ParamType, Count, Values, StateType
			);
	}
	while (gcvFALSE);
}


/*******************************************************************************
**
** _GetState
**
** Main entry for retrieving value of the given state.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    Object
**       Handle to the object of which the given state is to be retrieved.
**
**    ObjectList
**       Pointer to the object list header.
**
**    ParamType
**       Parameter to be retrieved.
**
**    Count
**       Value component count.
**
**    Values
**       Points to the user-provided placeholder for the value.
**
**    StateType
**       Type of the value to be retrieved.
**
** OUTPUT:
**
**    Nothing.
*/

static void _GetState(
	vgsCONTEXT_PTR Context,
	VGHandle Object,
	vgsOBJECT_LIST_PTR ObjectList,
	VGint ParamType,
	VGint Count,
	void * Values,
	vgvSTATETYPE StateType
	)
{
	do
	{
		VGint i;
		vgsSTATEENTRY_PTR stateEntry = gcvNULL;
		vgtVALUECONVERTER converter;

		/* Verify generic failure conditions. */
		if ((Count <= 0) || (Values == gcvNULL))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Find the state entry. */
		for (i = 0; i < ObjectList->stateArraySize; i++)
		{
			/* Create a shortcut to the entry. */
			stateEntry = &ObjectList->stateArray[i];

			/* Found the entry? */
			if (stateEntry->paramType == ParamType)
			{
				break;
			}
		}

		/* Was the entry found? */
		if (i == ObjectList->stateArraySize)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}
		if (stateEntry == gcvNULL)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Want to many? */
		if (Count > stateEntry->valueCount)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Verify value alignment. */
		if ((gcmPTR2INT(Values) & stateEntry->alignment) != 0)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Vector parameter? */
		if (stateEntry->valueUnitSize > 1)
		{
			/* Count has to be a multiple of the unit size. */
			if ((Count % stateEntry->valueUnitSize) != 0)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}

			/* Compute the number of units to set. */
			Count /= stateEntry->valueUnitSize;
		}

		/* Get the value converter. */
		converter = stateEntry->getConverters[StateType];

		/* Get the parameter. */
		stateEntry->getState(Context, Object, Count, Values, converter);
	}
	while (gcvFALSE);
}


/*******************************************************************************
**
** _GetObjectState
**
** Main entry for retrieving value of the given state of an object.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    Object
**       Handle to the object of which the given state is to be retrieved.
**
**    ParamType
**       Parameter to be retrieved.
**
**    Count
**       Value component count.
**
**    Values
**       Points to the user-provided placeholder for the value.
**
**    StateType
**       Type of the value to be retrieved.
**
** OUTPUT:
**
**    Nothing.
*/

static void _GetObjectState(
	vgsCONTEXT_PTR Context,
	VGHandle Object,
	VGint ParamType,
	VGint Count,
	void * Values,
	vgvSTATETYPE StateType
	)
{
	do
	{
		vgsOBJECT_PTR object;
		vgsOBJECT_LIST_PTR objectList;

		/* Cast the object. */
		object = (vgsOBJECT_PTR) Object;

		/* Get the proper object list. */
		objectList = &Context->objectCache->cache[object->type];

		/* Retrieve the value. */
		_GetState(
			Context, Object, objectList, ParamType, Count, Values, StateType
			);
	}
	while (gcvFALSE);
}


/*******************************************************************************
**
** _GetVectorSize
**
** Get the current number of items in the specified state.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    Object
**       Handle to the object of which the given state is to be retrieved.
**
**    ObjectList
**       Pointer to the object list header.
**
**    ParamType
**       Parameter to query.
**
** OUTPUT:
**
**    Nothing.
*/

static VGint _GetVectorSize(
	vgsCONTEXT_PTR Context,
	VGHandle Object,
	vgsOBJECT_LIST_PTR ObjectList,
	VGint ParamType
	)
{
	VGint result = 0;

	do
	{
		VGint i;
		vgsSTATEENTRY_PTR stateEntry = gcvNULL;

		/* Find the state entry. */
		for (i = 0; i < ObjectList->stateArraySize; i++)
		{
			/* Create a shortcut to the entry. */
			stateEntry = &ObjectList->stateArray[i];

			/* Found the entry? */
			if (stateEntry->paramType == ParamType)
			{
				break;
			}
		}

		/* Was the entry found? */
		if (i == ObjectList->stateArraySize)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}
		if (stateEntry == gcvNULL)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}
		/* A single value? */
		if (stateEntry->getArraySize == gcvNULL)
		{
			/* Yes. */
			result = 1;
		}
		else
		{
			/* Query and set the result. */
			result = stateEntry->getArraySize(Context, Object);
		}
	}
	while (gcvFALSE);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** _GetObjectVectorSize
**
** Get the current number of items in the specified state of an object.
**
** INPUT:
**
**    Context
**       Pointer to the current context.
**
**    Object
**       Handle to the object of which the given state is to be retrieved.
**
**    ParamType
**       Parameter to query.
**
** OUTPUT:
**
**    Nothing.
*/

static VGint _GetObjectVectorSize(
	vgsCONTEXT_PTR Context,
	VGHandle Object,
	VGint ParamType
	)
{
	VGint result = 0;

	do
	{
		vgsOBJECT_PTR object;
		vgsOBJECT_LIST_PTR objectList;

		/* Cast the object. */
		object = (vgsOBJECT_PTR) Object;

		/* Get the proper object list. */
		objectList = &Context->objectCache->cache[object->type];

		/* Retrieve the size. */
		result = _GetVectorSize(Context, Object, objectList, ParamType);
	}
	while (gcvFALSE);

	/* Return result. */
	return result;
}


/******************************************************************************\
************************ Internal State Management API *************************
\******************************************************************************/

/*******************************************************************************
**
** vgfSetXXXFromXXX
**
** The set of functions performs type conversion between input and output.
**
** INPUT:
**
**    DestinationState
**       Points to the destination state pointer. The pointer is auto-incremen-
**       ted after the function returns.
**
**    SourceState
**       Points to the source state pointer. The pointer is auto-incremented
**       after the function returns.
**
**    Count
**       The number of values to convert.
**
**    ValidateInput
**       Input validation enable.
**
**    ReturnNewDestination
**       If not zero, the function will return the pointer to the advanced
**       destination, otherwise the pointer to the new source is returned.
**
** OUTPUT:
**
**    Nothing.
*/

void * vgfSetINTFromINT(
	void* DestinationState,
	const void* SourceState,
	VGint Count,
	VGboolean ValidateInput,
	VGboolean ReturnNewDestination
	)
{
	/* Cast the pointers. */
	VGint * destinationState = (VGint *) DestinationState;
	VGint * sourceState      = (VGint *) SourceState;

	/* Copy the value set. */
	while (Count--)
	{
		/* Copy the value and advance. */
		*destinationState++ = *sourceState++;
	}

	/* Return advanced pointer. */
	return ReturnNewDestination
		? (void *) destinationState
		: (void *) sourceState;
}

void * vgfSetINTFromFLOAT(
	void* DestinationState,
	const void* SourceState,
	VGint Count,
	VGboolean ValidateInput,
	VGboolean ReturnNewDestination
	)
{
	/* Cast the pointers. */
	VGint   * destinationState = (VGint   *) DestinationState;
	VGfloat * sourceState      = (VGfloat *) SourceState;

	/* Convert the value set. */
	while (Count--)
	{
		/* Floor the value; use double, because it may overflow
		   in extreme cases. */
		double value = (double) floor(*sourceState++);

		/* Clamp to the valid range. */
		value = (VGint) gcmCLAMP(value, gcvMAX_NEG_INT, gcvMAX_POS_INT);

		/* Convert the value and advance. */
		*destinationState++ = (VGint) value;
	}

	/* Return advanced pointer. */
	return ReturnNewDestination
		? (void *) destinationState
		: (void *) sourceState;
}

void * vgfSetFLOATFromINT(
	void* DestinationState,
	const void* SourceState,
	VGint Count,
	VGboolean ValidateInput,
	VGboolean ReturnNewDestination
	)
{
	/* Cast the pointers. */
	VGfloat * destinationState = (VGfloat *) DestinationState;
	VGint   * sourceState      = (VGint   *) SourceState;

	/* Convert the value set. */
	while (Count--)
	{
		/* Convert the value and advance. */
		*destinationState++ = (VGfloat) *sourceState++;
	}

	/* Return advanced pointer. */
	return ReturnNewDestination
		? (void *) destinationState
		: (void *) sourceState;
}

void * vgfSetFLOATFromFLOAT(
	void* DestinationState,
	const void* SourceState,
	VGint Count,
	VGboolean ValidateInput,
	VGboolean ReturnNewDestination
	)
{
	/* Cast the pointers. */
	VGfloat * destinationState = (VGfloat *) DestinationState;
	VGfloat * sourceState      = (VGfloat *) SourceState;

	/* Validation required? */
	if (ValidateInput)
	{
		/* Convert the value set. */
		while (Count--)
		{
			/* Get the value. */
			VGfloat value = *sourceState++;

			/* Not a number? */
			if (gcmIS_NAN(value))
			{
				value = 0.0f;
			}

			/* Clamp to the valid range. */
			value = gcmCLAMP(value, gcvMAX_NEG_FLOAT, gcvMAX_POS_FLOAT);

			/* Copy the value and advance. */
			*destinationState++ = value;
		}
	}
	else
	{
		/* Copy the value set. */
		while (Count--)
		{
			/* Copy the value and advance. */
			*destinationState++ = *sourceState++;
		}
	}

	/* Return advanced pointer. */
	return ReturnNewDestination
		? (void *) destinationState
		: (void *) sourceState;
}


/******************************************************************************\
************************** Individual State Functions **************************
\******************************************************************************/

/*----------------------------------------------------------------------------*/
/*------------------------------ VG_MATRIX_MODE ------------------------------*/

vgmVALID_VALUE_ARRAY(VG_MATRIX_MODE)
{
	VG_MATRIX_PATH_USER_TO_SURFACE,
	VG_MATRIX_IMAGE_USER_TO_SURFACE,
	VG_MATRIX_FILL_PAINT_TO_USER,
	VG_MATRIX_STROKE_PAINT_TO_USER,
	VG_MATRIX_GLYPH_USER_TO_SURFACE
};

vgmSETSTATE_FUNCTION(VG_MATRIX_MODE)
{
	/* Set new mode. */
	Context->matrixMode
		= (VGMatrixMode) * (VGint *) Values;

	/* Set the current matrix pointer. */
	switch (Context->matrixMode)
	{
	case VG_MATRIX_PATH_USER_TO_SURFACE:
		Context->matrix = &Context->pathUserToSurface;
		break;

	case VG_MATRIX_IMAGE_USER_TO_SURFACE:
		Context->matrix = &Context->imageUserToSurface;
		break;

	case VG_MATRIX_FILL_PAINT_TO_USER:
		Context->matrix = &Context->fillPaintToUser;
		break;

	case VG_MATRIX_STROKE_PAINT_TO_USER:
		Context->matrix = &Context->strokePaintToUser;
		break;

	case VG_MATRIX_GLYPH_USER_TO_SURFACE:
		Context->matrix = &Context->glyphUserToSurface;
		break;

	default:
		gcmASSERT(gcvFALSE);
	}
}

vgmGETSTATE_FUNCTION(VG_MATRIX_MODE)
{
	ValueConverter(
		Values, &Context->matrixMode, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------------- VG_FILL_RULE -------------------------------*/

vgmVALID_VALUE_ARRAY(VG_FILL_RULE)
{
	VG_EVEN_ODD,
	VG_NON_ZERO
};

vgmSETSTATE_FUNCTION(VG_FILL_RULE)
{
	static gceFILL_RULE _fillRule[] =
	{
		gcvVG_EVEN_ODD,			/* VG_EVEN_ODD */
		gcvVG_NON_ZERO			/* VG_NON_ZERO */
	};

	gctUINT index;

	Context->fillRule
		= (VGFillRule) * (VGint *) Values;

	index = Context->fillRule - VG_EVEN_ODD;
	Context->halFillRule = _fillRule[index];
}

vgmGETSTATE_FUNCTION(VG_FILL_RULE)
{
	ValueConverter(
		Values, &Context->fillRule, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*----------------------------- VG_IMAGE_QUALITY -----------------------------*/

vgmVALID_VALUE_ARRAY(VG_IMAGE_QUALITY)
{
	VG_IMAGE_QUALITY_NONANTIALIASED,
	VG_IMAGE_QUALITY_FASTER,
	VG_IMAGE_QUALITY_BETTER
};

vgmSETSTATE_FUNCTION(VG_IMAGE_QUALITY)
{
	Context->imageQuality
		= (VGImageQuality) * (VGint *) Values;
}

vgmGETSTATE_FUNCTION(VG_IMAGE_QUALITY)
{
	ValueConverter(
		Values, &Context->imageQuality, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*--------------------------- VG_RENDERING_QUALITY ---------------------------*/

vgmVALID_VALUE_ARRAY(VG_RENDERING_QUALITY)
{
	VG_RENDERING_QUALITY_NONANTIALIASED,
	VG_RENDERING_QUALITY_FASTER,
	VG_RENDERING_QUALITY_BETTER
};

vgmSETSTATE_FUNCTION(VG_RENDERING_QUALITY)
{
	static gceRENDER_QUALITY _renderingQuality[] =
	{
		gcvVG_NONANTIALIASED,	/* VG_RENDERING_QUALITY_NONANTIALIASED */
		gcvVG_2X2_MSAA,			/* VG_RENDERING_QUALITY_FASTER         */
		gcvVG_4X4_MSAA			/* VG_RENDERING_QUALITY_BETTER         */
	};

	gctUINT index;

	Context->renderingQuality
		= (VGRenderingQuality) * (VGint *) Values;

	index = Context->renderingQuality - VG_RENDERING_QUALITY_NONANTIALIASED;
	Context->halRenderingQuality = _renderingQuality[index];
}

vgmGETSTATE_FUNCTION(VG_RENDERING_QUALITY)
{
	ValueConverter(
		Values, &Context->renderingQuality, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------------- VG_BLEND_MODE ------------------------------*/

vgmVALID_VALUE_ARRAY(VG_BLEND_MODE)
{
	VG_BLEND_SRC,
	VG_BLEND_SRC_OVER,
	VG_BLEND_DST_OVER,
	VG_BLEND_SRC_IN,
	VG_BLEND_DST_IN,
	VG_BLEND_MULTIPLY,
	VG_BLEND_SCREEN,
	VG_BLEND_DARKEN,
	VG_BLEND_LIGHTEN,
	VG_BLEND_ADDITIVE
};

vgmSETSTATE_FUNCTION(VG_BLEND_MODE)
{
	static gceVG_BLEND _blendMode[] =
	{
		gcvVG_BLEND_SRC,		/* VG_BLEND_SRC      */
		gcvVG_BLEND_SRC_OVER,	/* VG_BLEND_SRC_OVER */
		gcvVG_BLEND_DST_OVER,	/* VG_BLEND_DST_OVER */
		gcvVG_BLEND_SRC_IN,		/* VG_BLEND_SRC_IN   */
		gcvVG_BLEND_DST_IN,		/* VG_BLEND_DST_IN   */
		gcvVG_BLEND_MULTIPLY,	/* VG_BLEND_MULTIPLY */
		gcvVG_BLEND_SCREEN,		/* VG_BLEND_SCREEN   */
		gcvVG_BLEND_DARKEN,		/* VG_BLEND_DARKEN   */
		gcvVG_BLEND_LIGHTEN,	/* VG_BLEND_LIGHTEN  */
		gcvVG_BLEND_ADDITIVE	/* VG_BLEND_ADDITIVE */
	};

	gctUINT index;

	Context->blendMode = (VGBlendMode) * (VGint *) Values;

	index = Context->blendMode - VG_BLEND_SRC;
	Context->halBlendMode = _blendMode[index];
}

vgmGETSTATE_FUNCTION(VG_BLEND_MODE)
{
	ValueConverter(
		Values, &Context->blendMode, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------------- VG_IMAGE_MODE ------------------------------*/

vgmVALID_VALUE_ARRAY(VG_IMAGE_MODE)
{
	VG_DRAW_IMAGE_NORMAL,
	VG_DRAW_IMAGE_MULTIPLY,
	VG_DRAW_IMAGE_STENCIL
};

vgmSETSTATE_FUNCTION(VG_IMAGE_MODE)
{
	static gceVG_BLEND _imageMode[] =
	{
		gcvVG_IMAGE_NORMAL,		/* VG_DRAW_IMAGE_NORMAL   */
		gcvVG_IMAGE_MULTIPLY,	/* VG_DRAW_IMAGE_MULTIPLY */
		gcvVG_IMAGE_STENCIL		/* VG_DRAW_IMAGE_STENCIL  */
	};

	gctUINT index;

	Context->imageMode
		= (VGImageMode) * (VGint *) Values;

	index = Context->imageMode - VG_DRAW_IMAGE_NORMAL;
	Context->halImageMode = _imageMode[index];
}

vgmGETSTATE_FUNCTION(VG_IMAGE_MODE)
{
	ValueConverter(
		Values, &Context->imageMode, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------------- VG_SCISSORING ------------------------------*/

vgmVALID_VALUE_ARRAY(VG_SCISSORING)
{
	VG_FALSE,
	VG_TRUE
};

vgmSETSTATE_FUNCTION(VG_SCISSORING)
{
	Context->scissoringEnable
		= (VGboolean) * (VGint *) Values;
}

vgmGETSTATE_FUNCTION(VG_SCISSORING)
{
	ValueConverter(
		Values, &Context->scissoringEnable, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*----------------------------- VG_SCISSOR_RECTS -----------------------------*/

vgmSETSTATE_FUNCTION(VG_SCISSOR_RECTS)
{
	VGint i;

	for (i = 0; i < Count; i++)
	{
		Values = ValueConverter(
			&Context->scissoringRects[i], Values, 4, VG_TRUE, VG_FALSE
			);
	}

	Context->scissoringRectsCount = Count;
	Context->scissoringRectsDirty = VG_TRUE;
}

vgmGETSTATE_FUNCTION(VG_SCISSOR_RECTS)
{
	VGint i;

	for (i = 0; i < Count; i++)
	{
		Values = ValueConverter(
			Values, &Context->scissoringRects[i], 4, VG_FALSE, VG_TRUE
			);
	}
}

vgmGETARRAYSIZE_FUNCTION(VG_SCISSOR_RECTS)
{
	return Context->scissoringRectsCount * 4;
}

/*----------------------------------------------------------------------------*/
/*----------------------------- VG_COLOR_TRANSFORM ---------------------------*/

vgmVALID_VALUE_ARRAY(VG_COLOR_TRANSFORM)
{
	VG_FALSE,
	VG_TRUE
};

vgmSETSTATE_FUNCTION(VG_COLOR_TRANSFORM)
{
	Context->colTransform
		= (VGboolean) * (VGint *) Values;
}

vgmGETSTATE_FUNCTION(VG_COLOR_TRANSFORM)
{
	ValueConverter(
		Values, &Context->colTransform, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------- VG_COLOR_TRANSFORM_VALUES ------------------------*/

vgmSETSTATE_FUNCTION(VG_COLOR_TRANSFORM_VALUES)
{
	if (Count == 0)
	{
		vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
	}
	else
	{
		ValueConverter(
			Context->colTransformValues, Values, 8, VG_FALSE, VG_FALSE
			);

		/* Invalidate color transform values. */
		Context->colTransformValuesDirty = gcvTRUE;
	}
}

vgmGETSTATE_FUNCTION(VG_COLOR_TRANSFORM_VALUES)
{
	ValueConverter(
		Values, &Context->colTransformValues, 8, VG_FALSE, VG_TRUE
		);
}

vgmGETARRAYSIZE_FUNCTION(VG_COLOR_TRANSFORM_VALUES)
{
	return 8;
}

/*----------------------------------------------------------------------------*/
/*--------------------------- VG_STROKE_LINE_WIDTH ---------------------------*/

vgmSETSTATE_FUNCTION(VG_STROKE_LINE_WIDTH)
{
	ValueConverter(
		&Context->strokeLineWidth, Values, 1, VG_FALSE, VG_FALSE
		);
}

vgmGETSTATE_FUNCTION(VG_STROKE_LINE_WIDTH)
{
	ValueConverter(
		Values, &Context->strokeLineWidth, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*----------------------------- VG_STROKE_CAP_STYLE --------------------------*/

vgmVALID_VALUE_ARRAY(VG_STROKE_CAP_STYLE)
{
	VG_CAP_BUTT,
	VG_CAP_ROUND,
	VG_CAP_SQUARE
};

vgmSETSTATE_FUNCTION(VG_STROKE_CAP_STYLE)
{
	static gceCAP_STYLE _capStyle[] =
	{
		gcvCAP_BUTT,		/* VG_CAP_BUTT   */
		gcvCAP_ROUND,		/* VG_CAP_ROUND  */
		gcvCAP_SQUARE		/* VG_CAP_SQUARE */
	};

	gctUINT index;

	Context->strokeCapStyle
		= (VGCapStyle) * (VGint *) Values;

	index = Context->strokeCapStyle - VG_CAP_BUTT;
	Context->halStrokeCapStyle = _capStyle[index];
}

vgmGETSTATE_FUNCTION(VG_STROKE_CAP_STYLE)
{
	ValueConverter(
		Values, &Context->strokeCapStyle, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*---------------------------- VG_STROKE_JOIN_STYLE --------------------------*/

vgmVALID_VALUE_ARRAY(VG_STROKE_JOIN_STYLE)
{
	VG_JOIN_MITER,
	VG_JOIN_ROUND,
	VG_JOIN_BEVEL
};

vgmSETSTATE_FUNCTION(VG_STROKE_JOIN_STYLE)
{
	static gceJOIN_STYLE _joinStyle[] =
	{
		gcvJOIN_MITER,		/* VG_JOIN_MITER   */
		gcvJOIN_ROUND,		/* VG_JOIN_ROUND  */
		gcvJOIN_BEVEL		/* VG_JOIN_BEVEL */
	};

	gctUINT index;

	Context->strokeJoinStyle
		= (VGJoinStyle) * (VGint *) Values;

	index = Context->strokeJoinStyle - VG_JOIN_MITER;
	Context->halStrokeJoinStyle = _joinStyle[index];
}

vgmGETSTATE_FUNCTION(VG_STROKE_JOIN_STYLE)
{
	ValueConverter(
		Values, &Context->strokeJoinStyle, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*--------------------------- VG_STROKE_MITER_LIMIT --------------------------*/

vgmSETSTATE_FUNCTION(VG_STROKE_MITER_LIMIT)
{
	ValueConverter(
		&Context->strokeMiterLimit, Values, 1, VG_FALSE, VG_FALSE
		);
}

vgmGETSTATE_FUNCTION(VG_STROKE_MITER_LIMIT)
{
	ValueConverter(
		Values, &Context->strokeMiterLimit, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*-------------------------- VG_STROKE_DASH_PATTERN --------------------------*/

vgmSETSTATE_FUNCTION(VG_STROKE_DASH_PATTERN)
{
	Context->strokeDashPtrnCount = Count;

	if (Count == 0)
	{
		Context->strokeDashPtrnEnable = VG_FALSE;
	}
	else
	{
		Context->strokeDashPtrnEnable = VG_TRUE;
		ValueConverter(
			Context->strokeDashPtrn, Values, Count, VG_FALSE, VG_FALSE
			);
	}
}

vgmGETSTATE_FUNCTION(VG_STROKE_DASH_PATTERN)
{
	if (Count <= Context->strokeDashPtrnCount)
	{
		ValueConverter(
			Values, &Context->strokeDashPtrn, Count, VG_FALSE, VG_TRUE
			);
	}
	else
	{
		vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
	}
}

vgmGETARRAYSIZE_FUNCTION(VG_STROKE_DASH_PATTERN)
{
	return Context->strokeDashPtrnCount;
}

/*----------------------------------------------------------------------------*/
/*--------------------------- VG_STROKE_DASH_PHASE ---------------------------*/

vgmSETSTATE_FUNCTION(VG_STROKE_DASH_PHASE)
{
	ValueConverter(
		&Context->strokeDashPhase, Values, 1, VG_FALSE, VG_FALSE
		);
}

vgmGETSTATE_FUNCTION(VG_STROKE_DASH_PHASE)
{
	ValueConverter(
		Values, &Context->strokeDashPhase, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------- VG_STROKE_DASH_PHASE_RESET -----------------------*/

vgmVALID_VALUE_ARRAY(VG_STROKE_DASH_PHASE_RESET)
{
	VG_FALSE,
	VG_TRUE
};

vgmSETSTATE_FUNCTION(VG_STROKE_DASH_PHASE_RESET)
{
	Context->strokeDashPhaseReset
		= (VGboolean) * (VGint *) Values;
}

vgmGETSTATE_FUNCTION(VG_STROKE_DASH_PHASE_RESET)
{
	ValueConverter(
		Values, &Context->strokeDashPhaseReset, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*---------------------------- VG_TILE_FILL_COLOR ----------------------------*/

vgmSETSTATE_FUNCTION(VG_TILE_FILL_COLOR)
{
	if (Count == 0)
	{
		vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
	}
	else
	{
		ValueConverter(
			&Context->tileFillColor, Values, 4, VG_FALSE, VG_FALSE
			);

		gcmVERIFY_OK(gcoVG_SetTileFillColor(
			Context->vg,
			Context->tileFillColor[0],
			Context->tileFillColor[1],
			Context->tileFillColor[2],
			Context->tileFillColor[3]
			));
	}
}

vgmGETSTATE_FUNCTION(VG_TILE_FILL_COLOR)
{
	ValueConverter(
		Values, &Context->tileFillColor, 4, VG_FALSE, VG_TRUE
		);
}

vgmGETARRAYSIZE_FUNCTION(VG_TILE_FILL_COLOR)
{
	return 4;
}

/*----------------------------------------------------------------------------*/
/*------------------------------ VG_CLEAR_COLOR ------------------------------*/

vgmSETSTATE_FUNCTION(VG_CLEAR_COLOR)
{
	if (Count == 0)
	{
		vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
	}
	else
	{
		gctFLOAT_PTR floarColor;
		gctUINT8_PTR byteColor;

		ValueConverter(
			&Context->clearColor, Values, 4, VG_FALSE, VG_FALSE
			);

		/* Make shortcuts to the color arrays. */
		floarColor = Context->clearColor;
		byteColor  = Context->halClearColor;

		/* Convert to 8-bit per component. */
		byteColor[0] = gcoVG_PackColorComponent(floarColor[0]);
		byteColor[1] = gcoVG_PackColorComponent(floarColor[1]);
		byteColor[2] = gcoVG_PackColorComponent(floarColor[2]);
		byteColor[3] = gcoVG_PackColorComponent(floarColor[3]);
	}
}

vgmGETSTATE_FUNCTION(VG_CLEAR_COLOR)
{
	ValueConverter(
		Values, &Context->clearColor, 4, VG_FALSE, VG_TRUE
		);
}

vgmGETARRAYSIZE_FUNCTION(VG_CLEAR_COLOR)
{
	return 4;
}

/*----------------------------------------------------------------------------*/
/*----------------------------- VG_GLYPH_ORIGIN ------------------------------*/

vgmSETSTATE_FUNCTION(VG_GLYPH_ORIGIN)
{
	if (Count == 0)
	{
		vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
	}
	else
	{
		ValueConverter(
			&Context->glyphOrigin, Values, 2, VG_FALSE, VG_FALSE
			);
	}
}

vgmGETSTATE_FUNCTION(VG_GLYPH_ORIGIN)
{
	ValueConverter(
		Values, &Context->glyphOrigin, 2, VG_FALSE, VG_TRUE
		);
}

vgmGETARRAYSIZE_FUNCTION(VG_GLYPH_ORIGIN)
{
	return 2;
}

/*----------------------------------------------------------------------------*/
/*-------------------------------- VG_MASKING --------------------------------*/

vgmVALID_VALUE_ARRAY(VG_MASKING)
{
	VG_FALSE,
	VG_TRUE
};

vgmSETSTATE_FUNCTION(VG_MASKING)
{
	Context->maskingEnable
		= (VGboolean) * (VGint *) Values;
}

vgmGETSTATE_FUNCTION(VG_MASKING)
{
	ValueConverter(
		Values, &Context->maskingEnable, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------------ VG_PIXEL_LAYOUT -----------------------------*/

vgmVALID_VALUE_ARRAY(VG_PIXEL_LAYOUT)
{
	VG_PIXEL_LAYOUT_UNKNOWN,
	VG_PIXEL_LAYOUT_RGB_VERTICAL,
	VG_PIXEL_LAYOUT_BGR_VERTICAL,
	VG_PIXEL_LAYOUT_RGB_HORIZONTAL,
	VG_PIXEL_LAYOUT_BGR_HORIZONTAL
};

vgmSETSTATE_FUNCTION(VG_PIXEL_LAYOUT)
{
	Context->pixelLayout
		= (VGPixelLayout) * (VGint *) Values;
}

vgmGETSTATE_FUNCTION(VG_PIXEL_LAYOUT)
{
	ValueConverter(Values,
		&Context->pixelLayout, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*----------------------------- VG_SCREEN_LAYOUT -----------------------------*/

vgmGETSTATE_FUNCTION(VG_SCREEN_LAYOUT)
{
	ValueConverter(
		Values, &Context->screenLayout, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*-------------------------- VG_FILTER_FORMAT_LINEAR -------------------------*/

vgmVALID_VALUE_ARRAY(VG_FILTER_FORMAT_LINEAR)
{
	VG_FALSE,
	VG_TRUE
};

vgmSETSTATE_FUNCTION(VG_FILTER_FORMAT_LINEAR)
{
	Context->fltFormatLinear
		= (VGboolean) * (VGint *) Values;
}

vgmGETSTATE_FUNCTION(VG_FILTER_FORMAT_LINEAR)
{
	ValueConverter(
		Values, &Context->fltFormatLinear, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*----------------------- VG_FILTER_FORMAT_PREMULTIPLIED ---------------------*/

vgmVALID_VALUE_ARRAY(VG_FILTER_FORMAT_PREMULTIPLIED)
{
	VG_FALSE,
	VG_TRUE
};

vgmSETSTATE_FUNCTION(VG_FILTER_FORMAT_PREMULTIPLIED)
{
	Context->fltFormatPremultiplied
		= (VGboolean) * (VGint *) Values;
}

vgmGETSTATE_FUNCTION(VG_FILTER_FORMAT_PREMULTIPLIED)
{
	ValueConverter(
		Values, &Context->fltFormatPremultiplied, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------- VG_FILTER_CHANNEL_MASK ---------------------------*/

vgmSETSTATE_FUNCTION(VG_FILTER_CHANNEL_MASK)
{
	static const gceCHANNEL _channelEnable[] =
	{
		gcvCHANNEL_XXXX,
		gcvCHANNEL_XXXA,
		gcvCHANNEL_XXBX,
		gcvCHANNEL_XXBA,
		gcvCHANNEL_XGXX,
		gcvCHANNEL_XGXA,
		gcvCHANNEL_XGBX,
		gcvCHANNEL_XGBA,
		gcvCHANNEL_RXXX,
		gcvCHANNEL_RXXA,
		gcvCHANNEL_RXBX,
		gcvCHANNEL_RXBA,
		gcvCHANNEL_RGXX,
		gcvCHANNEL_RGXA,
		gcvCHANNEL_RGBX,
		gcvCHANNEL_RGBA
	};

	/* Store VG value for query. */
	ValueConverter(
		&Context->fltChannelMask, Values, 1, VG_TRUE, VG_FALSE
		);

	/* Determine capped channel mask. */
	Context->fltCapChannelMask = Context->fltChannelMask & vgvRGBA;

	/* Translate VG mask to HAL. */
	Context->fltHalChannelMask = _channelEnable[Context->fltCapChannelMask];
}

vgmGETSTATE_FUNCTION(VG_FILTER_CHANNEL_MASK)
{
	ValueConverter(
		Values, &Context->fltChannelMask, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*-------------------------- VG_MAX_SCISSOR_RECTS ----------------------------*/

vgmGETSTATE_FUNCTION(VG_MAX_SCISSOR_RECTS)
{
	ValueConverter(
		Values, &Context->maxScissorRects, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*--------------------------- VG_MAX_DASH_COUNT ------------------------------*/

vgmGETSTATE_FUNCTION(VG_MAX_DASH_COUNT)
{
	ValueConverter(
		Values, &Context->maxDashCount, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*--------------------------- VG_MAX_KERNEL_SIZE -----------------------------*/

vgmGETSTATE_FUNCTION(VG_MAX_KERNEL_SIZE)
{
	ValueConverter(
		Values, &Context->maxKernelSize, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*---------------------- VG_MAX_SEPARABLE_KERNEL_SIZE ------------------------*/

vgmGETSTATE_FUNCTION(VG_MAX_SEPARABLE_KERNEL_SIZE)
{
	ValueConverter(
		Values, &Context->maxSeparableKernelSize, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------ VG_MAX_COLOR_RAMP_STOPS ---------------------------*/

vgmGETSTATE_FUNCTION(VG_MAX_COLOR_RAMP_STOPS)
{
	ValueConverter(
		Values, &Context->maxColorRampStops, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*--------------------------- VG_MAX_IMAGE_WIDTH -----------------------------*/

vgmGETSTATE_FUNCTION(VG_MAX_IMAGE_WIDTH)
{
	ValueConverter(
		Values, &Context->maxImageWidth, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*-------------------------- VG_MAX_IMAGE_HEIGHT -----------------------------*/

vgmGETSTATE_FUNCTION(VG_MAX_IMAGE_HEIGHT)
{
	ValueConverter(
		Values, &Context->maxImageHeight, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*-------------------------- VG_MAX_IMAGE_PIXELS -----------------------------*/

vgmGETSTATE_FUNCTION(VG_MAX_IMAGE_PIXELS)
{
	ValueConverter(
		Values, &Context->maxImagePixels, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*--------------------------- VG_MAX_IMAGE_BYTES -----------------------------*/

vgmGETSTATE_FUNCTION(VG_MAX_IMAGE_BYTES)
{
	ValueConverter(
		Values, &Context->maxImageBytes, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------------ VG_MAX_FLOAT --------------------------------*/

vgmGETSTATE_FUNCTION(VG_MAX_FLOAT)
{
	static gctFLOAT _maxFloat = gcvMAX_POS_FLOAT;

	ValueConverter(
		Values, &_maxFloat, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*--------------------- VG_MAX_GAUSSIAN_STD_DEVIATION ------------------------*/

vgmGETSTATE_FUNCTION(VG_MAX_GAUSSIAN_STD_DEVIATION)
{
	ValueConverter(
		Values, &Context->maxGaussianDeviation, 1, VG_FALSE, VG_TRUE
		);
}


/******************************************************************************\
***************************** Context State Array ******************************
\******************************************************************************/

static vgsSTATEENTRY _stateArray[] =
{
	/* Mode settings. */
	vgmDEFINE_ENUMERATED_ENTRY(VG_MATRIX_MODE),
	vgmDEFINE_ENUMERATED_ENTRY(VG_FILL_RULE),
	vgmDEFINE_ENUMERATED_ENTRY(VG_IMAGE_QUALITY),
	vgmDEFINE_ENUMERATED_ENTRY(VG_RENDERING_QUALITY),
	vgmDEFINE_ENUMERATED_ENTRY(VG_BLEND_MODE),
	vgmDEFINE_ENUMERATED_ENTRY(VG_IMAGE_MODE),

	/* Scissoring. */
	vgmDEFINE_ENUMERATED_ENTRY(VG_SCISSORING),
	vgmDEFINE_ARRAY_ENTRY(VG_SCISSOR_RECTS, INT, 4, vgvMAX_SCISSOR_RECTS),

	/* Color transformation. */
	vgmDEFINE_ENUMERATED_ENTRY(VG_COLOR_TRANSFORM),
	vgmDEFINE_ARRAY_ENTRY(VG_COLOR_TRANSFORM_VALUES, FLOAT, 8, 1),

	/* Stroke parameters. */
	vgmDEFINE_SCALAR_ENTRY(VG_STROKE_LINE_WIDTH, FLOAT),
	vgmDEFINE_ENUMERATED_ENTRY(VG_STROKE_CAP_STYLE),
	vgmDEFINE_ENUMERATED_ENTRY(VG_STROKE_JOIN_STYLE),
	vgmDEFINE_SCALAR_ENTRY(VG_STROKE_MITER_LIMIT, FLOAT),
	vgmDEFINE_ARRAY_ENTRY(VG_STROKE_DASH_PATTERN, FLOAT, 1, vgvMAX_DASH_COUNT),
	vgmDEFINE_SCALAR_ENTRY(VG_STROKE_DASH_PHASE, FLOAT),
	vgmDEFINE_ENUMERATED_ENTRY(VG_STROKE_DASH_PHASE_RESET),

	/* Edge fill color for VG_TILE_FILL tiling mode. */
	vgmDEFINE_ARRAY_ENTRY(VG_TILE_FILL_COLOR, FLOAT, 4, 1),

	/* Color for vgClear and vgClearImage. */
	vgmDEFINE_ARRAY_ENTRY(VG_CLEAR_COLOR, FLOAT, 4, 1),

	/* Glyph origin. */
	vgmDEFINE_ARRAY_ENTRY(VG_GLYPH_ORIGIN, FLOAT, 2, 1),

	/* Enable/disable alpha masking. */
	vgmDEFINE_ENUMERATED_ENTRY(VG_MASKING),

	/* Pixel layout information. */
	vgmDEFINE_ENUMERATED_ENTRY(VG_PIXEL_LAYOUT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_SCREEN_LAYOUT, INT),

	/* Source format selection for image filters. */
	vgmDEFINE_ENUMERATED_ENTRY(VG_FILTER_FORMAT_LINEAR),
	vgmDEFINE_ENUMERATED_ENTRY(VG_FILTER_FORMAT_PREMULTIPLIED),

	/* Destination write enable mask for image filters. */
	vgmDEFINE_SCALAR_ENTRY(VG_FILTER_CHANNEL_MASK, INT),

	/* Implementation limits (read-only). */
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_MAX_SCISSOR_RECTS, INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_MAX_DASH_COUNT, INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_MAX_KERNEL_SIZE, INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_MAX_SEPARABLE_KERNEL_SIZE, INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_MAX_COLOR_RAMP_STOPS, INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_MAX_IMAGE_WIDTH, INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_MAX_IMAGE_HEIGHT, INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_MAX_IMAGE_PIXELS, INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_MAX_IMAGE_BYTES, INT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_MAX_FLOAT, FLOAT),
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_MAX_GAUSSIAN_STD_DEVIATION, INT),
};


/******************************************************************************\
*************** A dummy object container for the context states. ***************
\******************************************************************************/

static vgsOBJECT_LIST _contextStates =
{
	/* State array. */
	_stateArray,

	/* State array size. */
	gcmCOUNTOF(_stateArray),

	/* Null destructor. */
	gcvNULL,
};


/*******************************************************************************
**
** vgfSetDefaultStates
**
** Set all context states to their default values.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
** OUTPUT:
**
**    Nothing.
*/

void vgfSetDefaultStates(
	vgsCONTEXT_PTR Context
	)
{
	static VGfloat colTransformValues[8] =
		{ 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };

	static vgtFLOATVECTOR4 tileFillColor =
		{ 0.0f, 0.0f, 0.0f, 0.0f };

	static vgtFLOATVECTOR4 clearColor =
		{ 0.0f, 0.0f, 0.0f, 0.0f };

	static VGfloat glyphOrigin[2] =
		{ 0.0f, 0.0f };

	/* Mode settings. */
	vgmSET_ENUM(VG_FILL_RULE,         VG_EVEN_ODD);
	vgmSET_ENUM(VG_IMAGE_QUALITY,     VG_IMAGE_QUALITY_FASTER);
	vgmSET_ENUM(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);
	vgmSET_ENUM(VG_BLEND_MODE,        VG_BLEND_SRC_OVER);
	vgmSET_ENUM(VG_IMAGE_MODE,        VG_DRAW_IMAGE_NORMAL);
	vgmSET_ENUM(VG_COLOR_TRANSFORM,   VG_FALSE);
	vgmSET_ENUM(VG_SCISSORING,        VG_FALSE);

	/* Scissoring. */
	Context->scissoringRectsCount = 0;
	Context->scissoringRectsDirty = VG_FALSE;

	/* Color transformation. */
	vgmSET_ARRAY(VG_COLOR_TRANSFORM_VALUES, FLOAT, colTransformValues);

	/* Stroke parameters. */
	Context->strokeLineWidth      = 1.0f;
	Context->strokeCapStyle       = VG_CAP_BUTT;
	Context->strokeJoinStyle      = VG_JOIN_MITER;
	Context->strokeMiterLimit     = 4.0f;
	Context->strokeDashPtrnEnable = VG_FALSE;
	Context->strokeDashPtrnCount  = 0;
	Context->strokeDashPhase      = 0.0f;
	Context->strokeDashPhaseReset = VG_FALSE;

	/* Edge fill color for VG_TILE_FILL tiling mode. */
	vgmSET_ARRAY(VG_TILE_FILL_COLOR, FLOAT, tileFillColor);

	/* Color for vgClear and vgClearImage. */
	vgmSET_ARRAY(VG_CLEAR_COLOR, FLOAT, clearColor);

	/* Glyph origin. */
	vgmSET_ARRAY(VG_GLYPH_ORIGIN, FLOAT, glyphOrigin);

	/* Enable/disable alpha masking. */
	Context->maskingEnable = VG_FALSE;

	/* Pixel layout information. */
	Context->pixelLayout  = VG_PIXEL_LAYOUT_UNKNOWN;
	Context->screenLayout = VG_PIXEL_LAYOUT_UNKNOWN;

	/* Source format selection for image filters. */
	Context->fltFormatLinear        = VG_FALSE;
	Context->fltFormatPremultiplied = VG_FALSE;

	/* Destination write enable mask for image filters. */
	vgmSET_STATE(VG_FILTER_CHANNEL_MASK, INT, vgvRGBA);

	/* Implementation limits (read-only). */
	Context->maxScissorRects        = vgvMAX_SCISSOR_RECTS;
	Context->maxDashCount           = vgvMAX_DASH_COUNT;
	Context->maxKernelSize          = vgvMAX_KERNEL_SIZE;
	Context->maxSeparableKernelSize = vgvMAX_SEPARABLE_KERNEL_SIZE;
	Context->maxColorRampStops      = vgvMAX_COLOR_RAMP_STOPS;
	Context->maxImageWidth          = vgvMAX_IMAGE_WIDTH;
	Context->maxImageHeight         = vgvMAX_IMAGE_HEIGHT;
	Context->maxImagePixels         = vgvMAX_IMAGE_PIXELS;
	Context->maxImageBytes          = vgvMAX_IMAGE_BYTES;
	Context->maxGaussianDeviation   = vgvMAX_GAUSSIAN_STD_DEVIATION;

	/* Initialize the matrices. */
	vgfInitializeMatrixSet(Context);
}


/*******************************************************************************
**
** vgfUpdateStates
**
** Program the hardware with all required states.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    FillRule
**       Fill mode value.
**
**    ImageMode
**       Image mode value.
**
**    BlendMode
**       Specifies the blending mode.
**
**    ScissorEnable
**       Scissor enable flag.
**
**    MaskingEnable
**       Mask enable flag.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgfUpdateStates(
	IN vgsCONTEXT_PTR Context,
	IN gceVG_IMAGE ImageMode,
	IN gceVG_BLEND BlendMode,
	IN gctBOOL ColorTransformEnable,
	IN gctBOOL ScissorEnable,
	IN gctBOOL MaskingEnable,
	IN gctBOOL DitherEnable
	)
{
	gceSTATUS status;

	do
	{
		/* VG_RENDERING_QUALITY */
		gcmERR_BREAK(gcoVG_SetRenderingQuality(
			Context->vg, Context->halRenderingQuality
			));

		/* Set masking. */
		gcmERR_BREAK(gcoVG_EnableMask(
			Context->vg, MaskingEnable
			));

		/* Flush mask image. */
		if (MaskingEnable)
		{
			/* Was the mask modified? */
			if (Context->maskDirty)
			{
				/* Yes, flush it. */
				gcmERR_BREAK(gcoVG_FlushMask(Context->vg));

				/* Reset the flush request. */
				Context->maskDirty = gcvFALSE;
			}

			gcmERR_BREAK(vgfFlushImage(
				Context, &Context->maskImage, gcvFALSE
				));

			/* Mark mask as being used. */
			*Context->maskImage.imageDirtyPtr = vgvIMAGE_NOT_FINISHED;
		}

		/* VG_BLEND_MODE */
		gcmERR_BREAK(gcoVG_SetBlendMode(
			Context->vg, BlendMode
			));

		/* enable the dither */
		gcmERR_BREAK(gcoVG_EnableDither(
			Context->vg, DitherEnable
			));

		/* VG_SCISSORING */
		gcmERR_BREAK(gcoVG_EnableScissor(
			Context->vg, ScissorEnable
			));

		/* VG_SCISSOR_RECTS */
		if (ScissorEnable && Context->scissoringRectsDirty)
		{
			gcmERR_BREAK(gcoVG_SetScissor(
				Context->vg,
				Context->scissoringRectsCount,
				Context->scissoringRects
				));

			Context->scissoringRectsDirty = VG_FALSE;
		}

		/* VG_COLOR_TRANSFORM */
		gcmERR_BREAK(gcoVG_EnableColorTransform(
			Context->vg, ColorTransformEnable
			));

		/* Update color transformation. */
		if (Context->colTransform && Context->colTransformValuesDirty)
		{
			gcmERR_BREAK(gcoVG_SetColorTransform(
				Context->vg, Context->colTransformValues
				));

			/* Reset dirty. */
			Context->colTransformValuesDirty = gcvFALSE;
		}

		/* Set image mode. */
		gcmERR_BREAK(gcoVG_SetImageMode(
			Context->vg, ImageMode
			));
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
************************** OpenVG State Management API *************************
\******************************************************************************/

/*******************************************************************************
**
** vgSet
**
** The vgSet functions set the value of a parameter on the current context.
**
** INPUT:
**
**    ParamType
**       Parameter to be set.
**
**    Count
**       Value component count.
**
**    Value(s)
**       New value to be set.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgSetf(
	VGParamType ParamType,
	VGfloat Value
	)
{
	vgmENTERAPI(vgSetf)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%04X, %.10ff);\n",
			__FUNCTION__,
			ParamType, Value
			);

		_SetState(
			Context, vgvSCALAR_CALL, &_contextStates,
			ParamType, 1, &Value, vgvSTATETYPE_FLOAT
			);
	}
	vgmLEAVEAPI(vgSetf);
}

VG_API_CALL void VG_API_ENTRY vgSeti(
	VGParamType ParamType,
	VGint Value
	)
{
	vgmENTERAPI(vgSeti)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%04X, %d);\n",
			__FUNCTION__,
			ParamType, Value
			);

		_SetState(
			Context, vgvSCALAR_CALL, &_contextStates,
			ParamType, 1, &Value, vgvSTATETYPE_INT
			);
	}
	vgmLEAVEAPI(vgSeti);
}

VG_API_CALL void VG_API_ENTRY vgSetfv(
	VGParamType ParamType,
	VGint Count,
	const VGfloat * Values
	)
{
	vgmENTERAPI(vgSetfv)
	{
		vgmDUMP_FLOAT_ARRAY(
			Values, Count
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%04X, %d, Values);\n",
			__FUNCTION__,
			ParamType, Count
			);

		_SetState(
			Context, VG_INVALID_HANDLE, &_contextStates,
			ParamType, Count, Values, vgvSTATETYPE_FLOAT
			);
	}
	vgmLEAVEAPI(vgSetfv);
}

VG_API_CALL void VG_API_ENTRY vgSetiv(
	VGParamType ParamType,
	VGint Count,
	const VGint * Values
	)
{
	vgmENTERAPI(vgSetiv)
	{
		vgmDUMP_INTEGER_ARRAY(
			Values, Count
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%04X, %d, Values);\n",
			__FUNCTION__,
			ParamType, Count
			);

		_SetState(
			Context, VG_INVALID_HANDLE, &_contextStates,
			ParamType, Count, Values, vgvSTATETYPE_INT
			);
	}
	vgmLEAVEAPI(vgSetiv);
}


/*******************************************************************************
**
** vgGetVectorSize
**
** The vgGetVectorSize function returns the maximum number of elements in the
** vector that will be retrieved by the vgGetiv or vgGetfv functions if called
** with the given ParamType argument. For scalar values, 1 is returned. If
** vgGetiv or vgGetfv is called with a smaller value for count than that
** returned by vgGetVectorSize, only the first count elements of the vector
** are retrieved. Use of a greater value for count will result in an error.
**
** INPUT:
**
**    ParamType
**       Parameter in question.
**
** OUTPUT:
**
**    VGint
**       Number of components in the queried parameter.
*/

VG_API_CALL VGint VG_API_ENTRY vgGetVectorSize(
	VGParamType ParamType
	)
{
	VGint result = 0;

	vgmENTERAPI(vgGetVectorSize)
	{
		result = _GetVectorSize(
			Context, VG_INVALID_HANDLE, &_contextStates, ParamType
			);
	}
	vgmLEAVEAPI(vgGetVectorSize);

	return result;
}


/*******************************************************************************
**
** vgGet
**
** The vgGet functions return the value of a parameter on the current context.
**
** INPUT:
**
**    ParamType
**       Parameter to be queried.
**
**    Count
**       Value component count to be returned.
**
**    Values
**       Points to the array for the returned value(s).
**
** OUTPUT:
**
**    VGfloat/VGint
**       Value of the queried parameter.
*/

VG_API_CALL VGfloat VG_API_ENTRY vgGetf(
	VGParamType ParamType
	)
{
	VGfloat result = 0.0f;

	vgmENTERAPI(vgGetf)
	{
		_GetState(
			Context, VG_INVALID_HANDLE, &_contextStates,
			ParamType, 1, &result, vgvSTATETYPE_FLOAT
			);
	}
	vgmLEAVEAPI(vgGetf);

	return result;
}

VG_API_CALL VGint VG_API_ENTRY vgGeti(
	VGParamType ParamType
	)
{
	VGint result = 0;

	vgmENTERAPI(vgGeti)
	{
		_GetState(
			Context, VG_INVALID_HANDLE, &_contextStates,
			ParamType, 1, &result, vgvSTATETYPE_INT
			);
	}
	vgmLEAVEAPI(vgGeti);

	return result;
}

VG_API_CALL void VG_API_ENTRY vgGetfv(
	VGParamType ParamType,
	VGint Count,
	VGfloat * Values
	)
{
	vgmENTERAPI(vgGetfv)
	{
		_GetState(
			Context, VG_INVALID_HANDLE, &_contextStates,
			ParamType, Count, Values, vgvSTATETYPE_FLOAT
			);
	}
	vgmLEAVEAPI(vgGetfv);
}

VG_API_CALL void VG_API_ENTRY vgGetiv(
	VGParamType ParamType,
	VGint Count,
	VGint * Values
	)
{
	vgmENTERAPI(vgGetiv)
	{
		_GetState(
			Context, VG_INVALID_HANDLE, &_contextStates,
			ParamType, Count, Values, vgvSTATETYPE_INT
			);
	}
	vgmLEAVEAPI(vgGetiv);
}


/*******************************************************************************
**
** vgSetParameter
**
** The vgSetParameter functions set the value of a parameter on a given
** VGHandle-based object.
**
** INPUT:
**
**    Object
**       Handle to the object.
**
**    ParamType
**       Parameter to be set.
**
**    Count
**       Value component count.
**
**    Value(s)
**       New value(s) to be set.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgSetParameterf(
	VGHandle Object,
	VGint ParamType,
	VGfloat Value
	)
{
	vgmENTERAPI(vgSetParameterf)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%04X, %.10ff);\n",
			__FUNCTION__,
			Object, ParamType, Value
			);

		if (!vgfVerifyUserObject(Context, Object, vgvOBJECTTYPE_COUNT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		_SetObjectState(
			Context, Object, ParamType, 1, &Value, vgvSTATETYPE_FLOAT
			);
	}
	vgmLEAVEAPI(vgSetParameterf);
}

VG_API_CALL void VG_API_ENTRY vgSetParameteri(
	VGHandle Object,
	VGint ParamType,
	VGint Value
	)
{
	vgmENTERAPI(vgSetParameteri)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%04X, %d);\n",
			__FUNCTION__,
			Object, ParamType, Value
			);

		if (!vgfVerifyUserObject(Context, Object, vgvOBJECTTYPE_COUNT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		_SetObjectState(
			Context, Object, ParamType, 1, &Value, vgvSTATETYPE_INT
			);
	}
	vgmLEAVEAPI(vgSetParameteri);
}

VG_API_CALL void VG_API_ENTRY vgSetParameterfv(
	VGHandle Object,
	VGint ParamType,
	VGint Count,
	const VGfloat * Values
	)
{
	vgmENTERAPI(vgSetParameterfv)
	{
		vgmDUMP_FLOAT_ARRAY(
			Values, Count
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%04X, %d, Values);\n",
			__FUNCTION__,
			Object, ParamType, Count
			);

		if (!vgfVerifyUserObject(Context, Object, vgvOBJECTTYPE_COUNT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		_SetObjectState(
			Context, Object, ParamType, Count, Values, vgvSTATETYPE_FLOAT
			);
	}
	vgmLEAVEAPI(vgSetParameterfv);
}

VG_API_CALL void VG_API_ENTRY vgSetParameteriv(
	VGHandle Object,
	VGint ParamType,
	VGint Count,
	const VGint * Values
	)
{
	vgmENTERAPI(vgSetParameteriv)
	{
		vgmDUMP_INTEGER_ARRAY(
			Values, Count
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%04X, %d, Values);\n",
			__FUNCTION__,
			Object, ParamType, Count
			);

		if (!vgfVerifyUserObject(Context, Object, vgvOBJECTTYPE_COUNT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		_SetObjectState(
			Context, Object, ParamType, Count, Values, vgvSTATETYPE_INT
			);
	}
	vgmLEAVEAPI(vgSetParameteriv);
}


/*******************************************************************************
**
** vgGetParameterVectorSize
**
** The vgGetParameterVectorSize function returns the number of elements in the
** vector that will be returned by the vgGetParameteriv or vgGetParameterfv
** functions if called with the given ParamType argument. For scalar values,
** 1 is returned. If vgGetParameteriv or vgGetParameterfv is called with a
** smaller value for count than that returned by vgGetParameterVectorSize,
** only the first count elements of the vector are retrieved. Use of a greater
** value for count will result in an error.
**
** INPUT:
**
**    Object
**       Handle to the object.
**
**    ParamType
**       Parameter in question.
**
** OUTPUT:
**
**    VGint
**       Number of components in the queried parameter.
*/

VG_API_CALL VGint VG_API_ENTRY vgGetParameterVectorSize(
	VGHandle Object,
	VGint ParamType
	)
{
	VGint result = 0;

	vgmENTERAPI(vgGetParameterVectorSize)
	{
		if (!vgfVerifyUserObject(Context, Object, vgvOBJECTTYPE_COUNT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		result = _GetObjectVectorSize(Context, Object, ParamType);
	}
	vgmLEAVEAPI(vgGetParameterVectorSize);

	return result;
}


/*******************************************************************************
**
** vgGetParameter
**
** The vgGetParameter functions return the value of a parameter on a given
** VGHandle-based object.
**
** INPUT:
**
**    Object
**       Handle to the object.
**
**    ParamType
**       Parameter to be queried.
**
**    Count
**       Value component count to be returned.
**
**    Values
**       Points to the array for the returned value(s).
**
** OUTPUT:
**
**    VGfloat/VGint
**       Value of the queried parameter.
*/

VG_API_CALL VGfloat VG_API_ENTRY vgGetParameterf(
	VGHandle Object,
	VGint ParamType
	)
{
	VGfloat result = 0.0f;

	vgmENTERAPI(vgGetParameterf)
	{
		if (!vgfVerifyUserObject(Context, Object, vgvOBJECTTYPE_COUNT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		_GetObjectState(
			Context, Object, ParamType, 1, &result, vgvSTATETYPE_FLOAT
			);
	}
	vgmLEAVEAPI(vgGetParameterf);

	return result;
}

VG_API_CALL VGint VG_API_ENTRY vgGetParameteri(
	VGHandle Object,
	VGint ParamType
	)
{
	VGint result = 0;

	vgmENTERAPI(vgGetParameteri)
	{
		if (!vgfVerifyUserObject(Context, Object, vgvOBJECTTYPE_COUNT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		_GetObjectState(
			Context, Object, ParamType, 1, &result, vgvSTATETYPE_INT
			);
	}
	vgmLEAVEAPI(vgGetParameteri);

	return result;
}

VG_API_CALL void VG_API_ENTRY vgGetParameterfv(
	VGHandle Object,
	VGint ParamType,
	VGint Count,
	VGfloat * Values
	)
{
	vgmENTERAPI(vgGetParameterfv)
	{
		if (!vgfVerifyUserObject(Context, Object, vgvOBJECTTYPE_COUNT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		_GetObjectState(
			Context, Object, ParamType, Count, Values, vgvSTATETYPE_FLOAT
			);
	}
	vgmLEAVEAPI(vgGetParameterfv);
}

VG_API_CALL void VG_API_ENTRY vgGetParameteriv(
	VGHandle Object,
	VGint ParamType,
	VGint Count,
	VGint * Values
	)
{
	vgmENTERAPI(vgGetParameteriv)
	{
		if (!vgfVerifyUserObject(Context, Object, vgvOBJECTTYPE_COUNT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		_GetObjectState(
			Context, Object, ParamType, Count, Values, vgvSTATETYPE_INT
			);
	}
	vgmLEAVEAPI(vgGetParameteriv);
}
