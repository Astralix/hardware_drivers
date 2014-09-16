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




#ifndef __gc_vg_state_h__
#define __gc_vg_state_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*--------------------- Value converter function types. ----------------------*/

typedef void * (* vgtVALUECONVERTER) (
	void* DestinationState,
	const void* SourceState,
	VGint Count,
	VGboolean ValidateInput,
	VGboolean ReturnNewDestination
	);

/*----------------------------------------------------------------------------*/
/*------- Individual state set functions declarations and definitions. -------*/

#define vgmSETSTATE(ParamType) \
	_Set_ ## ParamType

#define vgmSETSTATE_FUNCTION_PARAMETERS() \
	vgsCONTEXT_PTR Context, \
	VGHandle Object, \
	VGint Count, \
	const void* Values, \
	vgtVALUECONVERTER ValueConverter \

#define vgmSETSTATE_FUNCTION(ParamType) \
	static void vgmSETSTATE(ParamType) ( \
		vgmSETSTATE_FUNCTION_PARAMETERS() \
		)

#define vgmSET_ENUM(ParamType, Value) \
{ \
	static gctINT _value = Value; \
	\
	vgmSETSTATE(ParamType) ( \
		Context, \
		VG_INVALID_HANDLE, \
		1, &_value, \
		gcvNULL \
		); \
}

#define vgmSET_STATE(ParamType, DataType, Value) \
{ \
	static gct ## DataType _value = Value; \
	\
	vgmSETSTATE(ParamType) ( \
		Context, \
		VG_INVALID_HANDLE, \
		1, &_value, \
		vgfSet ## DataType ## From ## DataType \
		); \
}

#define vgmSET_OBJECT_ENUM(Object, ParamType, Value) \
{ \
	static gctINT _value = Value; \
	\
	vgmSETSTATE(ParamType) ( \
		Context, \
		(VGHandle) Object, \
		1, &_value, \
		gcvNULL \
		); \
}

#define vgmSET_NULL_ARRAY(ParamType) \
	vgmSETSTATE(ParamType) ( \
		Context, \
		VG_INVALID_HANDLE, \
		0, gcvNULL, \
		gcvNULL \
		)

#define vgmSET_OBJECT_NULL_ARRAY(Object, ParamType, DataType) \
	vgmSETSTATE(ParamType) ( \
		Context, \
		(VGHandle) Object, \
		0, gcvNULL, \
		vgfSet ## DataType ## FromFLOAT \
		)

#define vgmSET_ARRAY(ParamType, DataType, Array) \
	vgmSETSTATE(ParamType) ( \
		Context, \
		VG_INVALID_HANDLE, \
		gcmCOUNTOF(Array), Array, \
		vgfSet ## DataType ## FromFLOAT \
		)

#define vgmSET_OBJECT_ARRAY(Object, ParamType, DataType, Array) \
	vgmSETSTATE(ParamType) ( \
		Context, \
		(VGHandle) Object, \
		gcmCOUNTOF(Array), Array, \
		vgfSet ## DataType ## FromFLOAT \
		)

typedef void (* vgtSETSTATE) (
	vgmSETSTATE_FUNCTION_PARAMETERS()
	);

/*----------------------------------------------------------------------------*/
/*------- Individual state get functions declarations and definitions. -------*/

#define vgmGETSTATE_FUNCTION_PARAMETERS() \
	vgsCONTEXT_PTR Context, \
	VGHandle Object, \
	VGint Count, \
	void* Values, \
	vgtVALUECONVERTER ValueConverter \

#define vgmGETARRAYSIZE_FUNCTION_PARAMETERS() \
	vgsCONTEXT_PTR Context, \
	VGHandle Object \

#define vgmGETSTATE_FUNCTION(ParamType) \
	static void _Get_ ## ParamType ( \
		vgmGETSTATE_FUNCTION_PARAMETERS() \
		)

#define vgmGETARRAYSIZE_FUNCTION(ParamType) \
	static gctINT _GetSize_ ## ParamType ( \
		vgmGETARRAYSIZE_FUNCTION_PARAMETERS() \
		)

typedef void (* vgtGETSTATE) (
	vgmGETSTATE_FUNCTION_PARAMETERS()
	);

typedef gctINT (* vgtGETARRAYSIZE) (
	vgmGETARRAYSIZE_FUNCTION_PARAMETERS()
	);

/*----------------------------------------------------------------------------*/
/*-------------- Helper for defining the array of valid values. --------------*/

#define vgmVALID_VALUE_ARRAY(ParamType) \
	static VGint _validValues_ ## ParamType[] =

/*----------------------------------------------------------------------------*/
/*------------------- Helpers for defining the state array. -----------------*/

#define vgmDEFINE_ENUMERATED_ENTRY(ParamType) \
	{ \
		ParamType, \
		gcmSIZEOF(gctINT) - 1, \
		1, 1, \
		gcmCOUNTOF(_validValues_ ## ParamType), \
		_validValues_ ## ParamType, \
		{ \
			vgfSetINTFromINT, \
			vgfSetINTFromFLOAT \
		}, \
		{ \
			vgfSetINTFromINT, \
			vgfSetFLOATFromINT \
		}, \
		_Set_ ## ParamType, \
		_Get_ ## ParamType, \
		gcvNULL \
	}

#define vgmDEFINE_SCALAR_ENTRY(ParamType, DataType) \
	{ \
		ParamType, \
		gcmSIZEOF(gct ## DataType) - 1, \
		1, 1, \
		0, gcvNULL, \
		{ \
			vgfSet ## DataType ## FromINT, \
			vgfSet ## DataType ## FromFLOAT \
		}, \
		{ \
			vgfSetINTFrom   ## DataType, \
			vgfSetFLOATFrom ## DataType \
		}, \
		_Set_ ## ParamType, \
		_Get_ ## ParamType, \
		gcvNULL \
	}

#define vgmDEFINE_SCALAR_READONLY_ENTRY(ParamType, DataType) \
	{ \
		ParamType, \
		gcmSIZEOF(gct ## DataType) - 1, \
		1, 1, \
		0, gcvNULL, \
		{ \
			gcvNULL, \
			gcvNULL \
		}, \
		{ \
			vgfSetINTFrom   ## DataType, \
			vgfSetFLOATFrom ## DataType \
		}, \
		gcvNULL, \
		_Get_ ## ParamType, \
		gcvNULL \
	}

#define vgmDEFINE_ARRAY_ENTRY(ParamType, DataType, UnitSize, UnitCount) \
	{ \
		ParamType, \
		gcmSIZEOF(gct ## DataType) - 1, \
		UnitSize * UnitCount, \
		UnitSize, \
		0, gcvNULL, \
		{ \
			vgfSet ## DataType ## FromINT, \
			vgfSet ## DataType ## FromFLOAT \
		}, \
		{ \
			vgfSetINTFrom   ## DataType, \
			vgfSetFLOATFrom ## DataType \
		}, \
		_Set_ ## ParamType, \
		_Get_ ## ParamType, \
		_GetSize_ ## ParamType \
	}

/*----------------------------------------------------------------------------*/
/*------------------- Incoming/outgoing state value types. -------------------*/

typedef enum
{
	vgvSTATETYPE_INT   = 0,
	vgvSTATETYPE_FLOAT = 1,

	vgvSTATETYPE_COUNT
}
vgvSTATETYPE;

/*----------------------------------------------------------------------------*/
/*----------------------- State table entry definition. ----------------------*/

typedef struct _vgsSTATEENTRY * vgsSTATEENTRY_PTR;
typedef struct _vgsSTATEENTRY
{
	/* Parameter type. */
	VGint				paramType;

	/* Value alignemnt. */
	VGint				alignment;

	/* Total number of values this parameter can store. */
	VGint				valueCount;

	/* The size of a value unit (for ex. vectors have 4). */
	VGint				valueUnitSize;

	/* Number of values in the array of valid values pointed to by
	   validValues member. If validCount is not zero, the parameter
	   is a scalar enumerated parameter. */
	VGint				validCount;
	VGint*				validValues;

	/* Value converters. */
	vgtVALUECONVERTER	setConverters[vgvSTATETYPE_COUNT];
	vgtVALUECONVERTER	getConverters[vgvSTATETYPE_COUNT];

	/* State set/get functions. */
	vgtSETSTATE			setState;
	vgtGETSTATE			getState;
	vgtGETARRAYSIZE		getArraySize;
}
vgsSTATEENTRY;

/*----------------------------------------------------------------------------*/
/*---------------- State vector size retrieval function type. ----------------*/

typedef VGint (* vgtGETVECTORSIZE) (
	VGParamType ParamType,
	vgsSTATEENTRY_PTR StateArray,
	VGint StateArraySize
	);

/*----------------------------------------------------------------------------*/
/*-------------------------- State converter functions. ----------------------*/

void * vgfSetINTFromINT(
	void* DestinationState,
	const void* SourceState,
	VGint Count,
	VGboolean ValidateInput,
	VGboolean ReturnNewDestination
	);

void * vgfSetINTFromFLOAT(
	void* DestinationState,
	const void* SourceState,
	VGint Count,
	VGboolean ValidateInput,
	VGboolean ReturnNewDestination
	);

void * vgfSetFLOATFromINT(
	void* DestinationState,
	const void* SourceState,
	VGint Count,
	VGboolean ValidateInput,
	VGboolean ReturnNewDestination
	);

void * vgfSetFLOATFromFLOAT(
	void* DestinationState,
	const void* SourceState,
	VGint Count,
	VGboolean ValidateInput,
	VGboolean ReturnNewDestination
	);

/*----------------------------------------------------------------------------*/
/*------------------ State management function declarations. -----------------*/

void vgfSetDefaultStates(
	vgsCONTEXT_PTR Context
	);

gceSTATUS vgfUpdateStates(
	IN vgsCONTEXT_PTR Context,
	IN gceVG_IMAGE ImageMode,
	IN gceVG_BLEND BlendMode,
	IN gctBOOL ColorTransformEnable,
	IN gctBOOL ScissorEnable,
	IN gctBOOL MaskingEnable,
	IN gctBOOL DitherEnable
	);

#ifdef __cplusplus
}
#endif

#endif
