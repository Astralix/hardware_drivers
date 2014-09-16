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




#ifndef __gc_vg_debug_h__
#define __gc_vg_debug_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*------------------------ Miscellaneous definitions. ------------------------*/

#if gcvDEBUG

	/* Internal validation. */
#	define vgvVALIDATE_PATH_WALKER		1
#	define vgvVALIDATE_PATH_STORAGE		0
#	define vgvVALIDATE_MEMORY_MANAGER	1

	/* API entry/exit control. */
#	define vgvENABLE_ENTRY_SCAN			0
#	define vgvDUMP_ENTRY_NAME			0
#	define vgvFINISH_ON_EXIT			0

	/* Debug counters. */
#	define vgvCOUNT_PATH_OBJECTS		0
#	define vgvCOUNT_PATH_CONTAINERS		0
#	define vgvCOUNT_FRAMES				0
#	define vgvCOUNT_SET_CONTEXT_CALLS	0
#	define vgvCOUNT_CLEAR_CALLS			0
#	define vgvCOUNT_PATH_DRAW_CALLS		0
#	define vgvCOUNT_IMAGE_DRAW_CALLS	0
#	define vgvCOUNT_IMAGE_COPY_CALLS	0
#	define vgvCOUNT_GAUSSIAN_CALLS		0

	/* Matrix debugging. */
#	define vgvENABLE_MATRIX_DUMP		0

#else

	/* Internal validation. */
#	define vgvVALIDATE_PATH_WALKER		0
#	define vgvVALIDATE_PATH_STORAGE		0
#	define vgvVALIDATE_MEMORY_MANAGER	0

	/* API entry/exit control. */
#	define vgvENABLE_ENTRY_SCAN			0
#	define vgvDUMP_ENTRY_NAME			0
#	define vgvFINISH_ON_EXIT			0

	/* Debug counters. */
#	define vgvCOUNT_PATH_OBJECTS		0
#	define vgvCOUNT_PATH_CONTAINERS		0
#	define vgvCOUNT_FRAMES				0
#	define vgvCOUNT_SET_CONTEXT_CALLS	0
#	define vgvCOUNT_CLEAR_CALLS			0
#	define vgvCOUNT_PATH_DRAW_CALLS		0
#	define vgvCOUNT_IMAGE_DRAW_CALLS	0
#	define vgvCOUNT_IMAGE_COPY_CALLS	0
#	define vgvCOUNT_GAUSSIAN_CALLS		0

	/* Matrix debugging. */
#	define vgvENABLE_MATRIX_DUMP		0

#endif

#define vgvCOUNT_CONTEXTS \
( \
	vgvCOUNT_FRAMES           | \
	vgvCOUNT_CLEAR_CALLS      | \
	vgvCOUNT_PATH_DRAW_CALLS  | \
	vgvCOUNT_IMAGE_DRAW_CALLS | \
	vgvCOUNT_IMAGE_COPY_CALLS | \
	vgvCOUNT_GAUSSIAN_CALLS     \
)

/*----------------------------------------------------------------------------*/
/*-------------------------------- Profiler. ---------------------------------*/

#if vgvENABLE_PROFILER

typedef struct _vgsPROFILE * vgsPROFILE_PTR;
typedef struct _vgsPROFILE
{
	gctSTRING				name;
	gctBOOL					processed;
	gctUINT					callCount;
	gctUINT					reference;
	struct timespec			totalTime;
	struct timespec			currentEntry;
	struct timespec			currentExit;
	struct timespec			currentTime;
}
vgsPROFILE;

void vgfProfileEntry(
	vgePROFILE Function
	);

void vgfProfileExit(
	vgePROFILE Function
	);

#endif

/*----------------------------------------------------------------------------*/
/*-------------------------- Path storage validation. ------------------------*/

#if vgvVALIDATE_PATH_STORAGE

#	define vgmVALIDATE_STORAGE(Storage) \
		gcmVERIFY_OK(vgfValidateStorage( \
			Storage, __FUNCTION__, __LINE__ \
			))

	gceSTATUS vgfValidateStorage(
		IN vgsPATHSTORAGE_PTR Storage,
		IN gctSTRING CallerName,
		IN gctUINT LineNumber
		);

#else

#	define vgmVALIDATE_STORAGE(Storage)

#endif

/*----------------------------------------------------------------------------*/
/*------------------------ VG API entry/exit management. ---------------------*/

#if gcvDEBUG

#	define vgmDEBUGENTRY(Context, FunctionName) \
		vgfDebugEntry(Context, FunctionName)

#	define vgmDEBUGEXIT(Context, FunctionName) \
		vgfDebugExit(Context, FunctionName)

#	define vgmERRORCALLBACK(Context, Code) \
		vgfErrorCallback(Context, Code)

	void vgfDebugEntry(
		vgsCONTEXT_PTR Context,
		gctSTRING FunctionName
		);

	void vgfDebugExit(
		vgsCONTEXT_PTR Context,
		gctSTRING FunctionName
		);

	void vgfErrorCallback(
		vgsCONTEXT_PTR Context,
		VGErrorCode Code
		);

#else

#	define vgmDEBUGENTRY(Context, FunctionName)
#	define vgmDEBUGEXIT(Context, FunctionName)
#	define vgmERRORCALLBACK(Context, Code)

#endif


/*----------------------------------------------------------------------------*/
/*------------------------- Matrix debug functions. --------------------------*/

#if vgvENABLE_MATRIX_DUMP

#	define vgmDUMP_MATRIX(Message, Matrix) \
		vgfDumpMatrix(Message, Matrix)

	void vgfDumpMatrix(
		IN gctSTRING Message,
		IN const vgsMATRIX_PTR Matrix
		);

#else

#	define vgmDUMP_MATRIX(Message, Matrix)

#endif

/*----------------------------------------------------------------------------*/
/*---------------------------- Context counter API. --------------------------*/

#if vgvCOUNT_CONTEXTS

#	define vgmREPORT_NEW_CONTEXT(Context) \
		vgfReportNewContext(Context)

#	define vgmVERIFY_CONTEXT(Context, Index) \
		vgfVerifyContext(Context, Index)

#	define vgmADVANCE_FRAME() \
		vgfAdvanceFrame()

	void vgfReportNewContext(
		vgsCONTEXT_PTR Context
		);

	gctBOOL vgfVerifyContext(
		vgsCONTEXT_PTR Context,
		gctUINT Index
		);

	void vgfAdvanceFrame(
		void
		);

#else

#	define vgmREPORT_NEW_CONTEXT(Context)
#	define vgmVERIFY_CONTEXT(Context, Index)
#	define vgmADVANCE_FRAME()

#endif

/*----------------------------------------------------------------------------*/
/*----------------------------- Frame counter API. ---------------------------*/

#if vgvCOUNT_FRAMES

#	define vgmDEFINE_FRAME_COUNTER() \
		gctUINT __frameCount;

#	define vgmADVANCE_FRAME_COUNT(Context) \
		vgfAdvanceFrameCount(Context)

	void vgfAdvanceFrameCount(
		vgsCONTEXT_PTR Context
		);

#else

#	define vgmDEFINE_FRAME_COUNTER()
#	define vgmADVANCE_FRAME_COUNT(Context)

#endif

/*----------------------------------------------------------------------------*/
/*-------------------------- Set Context counter API. ------------------------*/

#if vgvCOUNT_SET_CONTEXT_CALLS

#	define vgmDEFINE_SET_CONTEXT_COUNTER() \
		gctUINT __setContextCount;

#	define vgmRESET_SET_CONTEXT_COUNT(Context) \
		vgfResetSetContextCount(Context)

#	define vgmADVANCE_SET_CONTEXT_COUNT(Context) \
		vgfAdvanceSetContextCount(Context)

	void vgfResetSetContextCount(
		vgsCONTEXT_PTR Context
		);

	void vgfAdvanceSetContextCount(
		vgsCONTEXT_PTR Context
		);

#else

#	define vgmDEFINE_SET_CONTEXT_COUNTER()
#	define vgmRESET_SET_CONTEXT_COUNT(Context)
#	define vgmADVANCE_SET_CONTEXT_COUNT(Context)

#endif

/*----------------------------------------------------------------------------*/
/*----------------------------- Clear counter API. ---------------------------*/

#if vgvCOUNT_CLEAR_CALLS

#	define vgmDEFINE_CLEAR_COUNTER() \
		gctUINT __clearCount;

#	define vgmRESET_CLEAR_COUNT(Context) \
		vgfResetClearCount(Context)

#	define vgmADVANCE_CLEAR_COUNT(Context) \
		vgfAdvanceClearCount(Context)

	void vgfResetClearCount(
		vgsCONTEXT_PTR Context
		);

	void vgfAdvanceClearCount(
		vgsCONTEXT_PTR Context
		);

#else

#	define vgmDEFINE_CLEAR_COUNTER()
#	define vgmRESET_CLEAR_COUNT(Context)
#	define vgmADVANCE_CLEAR_COUNT(Context)

#endif

/*----------------------------------------------------------------------------*/
/*--------------------------- Draw path counter API. -------------------------*/

#if vgvCOUNT_PATH_DRAW_CALLS

#	define vgmDEFINE_DRAWPATH_COUNTER() \
		gctUINT __drawPathCount;

#	define vgmRESET_DRAWPATH_COUNT(Context) \
		vgfResetPathDrawCount(Context)

#	define vgmADVANCE_DRAWPATH_COUNT(Context) \
		vgfAdvancePathDrawCount(Context)

	void vgfResetPathDrawCount(
		vgsCONTEXT_PTR Context
		);

	void vgfAdvancePathDrawCount(
		vgsCONTEXT_PTR Context
		);

#else

#	define vgmDEFINE_DRAWPATH_COUNTER()
#	define vgmRESET_DRAWPATH_COUNT(Context)
#	define vgmADVANCE_DRAWPATH_COUNT(Context)

#endif

/*----------------------------------------------------------------------------*/
/*--------------------------- Draw image counter API. ------------------------*/

#if vgvCOUNT_IMAGE_DRAW_CALLS

#	define vgmDEFINE_DRAWIMAGE_COUNTER() \
		gctUINT __drawImageCount;

#	define vgmRESET_DRAWIMAGE_COUNT(Context) \
		vgfResetDrawImageCount(Context)

#	define vgmADVANCE_DRAWIMAGE_COUNT(Context) \
		vgfAdvanceImageDrawCount(Context)

	void vgfResetDrawImageCount(
		vgsCONTEXT_PTR Context
		);

	void vgfAdvanceImageDrawCount(
		vgsCONTEXT_PTR Context
		);

#else

#	define vgmDEFINE_DRAWIMAGE_COUNTER()
#	define vgmRESET_DRAWIMAGE_COUNT(Context)
#	define vgmADVANCE_DRAWIMAGE_COUNT(Context)

#endif

/*----------------------------------------------------------------------------*/
/*--------------------------- Copy image counter API. ------------------------*/

#if vgvCOUNT_IMAGE_COPY_CALLS

#	define vgmDEFINE_COPYIMAGE_COUNTER() \
		gctUINT __copyImageCount;

#	define vgmRESET_COPYIMAGE_COUNT(Context) \
		vgfResetCopyImageCount(Context)

#	define vgmADVANCE_COPYIMAGE_COUNT(Context) \
		vgfAdvanceImageCopyCount(Context)

	void vgfResetCopyImageCount(
		vgsCONTEXT_PTR Context
		);

	void vgfAdvanceImageCopyCount(
		vgsCONTEXT_PTR Context
		);

#else

#	define vgmDEFINE_COPYIMAGE_COUNTER()
#	define vgmRESET_COPYIMAGE_COUNT(Context)
#	define vgmADVANCE_COPYIMAGE_COUNT(Context)

#endif

/*----------------------------------------------------------------------------*/
/*-------------------------- Gaussian Blur counter API. ----------------------*/

#if vgvCOUNT_GAUSSIAN_CALLS

#	define vgmDEFINE_GAUSSIAN_COUNTER() \
		gctUINT __drawGaussianCount;

#	define vgmRESET_GAUSSIAN_COUNT(Context) \
		vgfResetGaussianCount(Context)

#	define vgmADVANCE_GAUSSIAN_COUNT(Context) \
		vgfAdvanceGaussianCount(Context)

	void vgfResetGaussianCount(
		vgsCONTEXT_PTR Context
		);

	void vgfAdvanceGaussianCount(
		vgsCONTEXT_PTR Context
		);

#else

#	define vgmDEFINE_GAUSSIAN_COUNTER()
#	define vgmRESET_GAUSSIAN_COUNT(Context)
#	define vgmADVANCE_GAUSSIAN_COUNT(Context)

#endif

/*----------------------------------------------------------------------------*/
/*-------------------------- Path object counter API. ------------------------*/

#if vgvCOUNT_PATH_OBJECTS

#	define vgmDEFINE_PATH_COUNTER() \
		gctUINT __pathCount;

#	define vgmSET_PATH_NUMBER(Context, Path) \
		vgfSetPathNumber(Context, Path)

	void vgfSetPathNumber(
		vgsCONTEXT_PTR Context,
		vgsPATH_PTR Path
		);

#else

#	define vgmDEFINE_PATH_COUNTER()
#	define vgmSET_PATH_NUMBER(Context, Path)

#endif

/*----------------------------------------------------------------------------*/
/*------------------------ Path container counter API. -----------------------*/

#if vgvCOUNT_PATH_CONTAINERS

#	define vgmDEFINE_CONTAINER_COUNTER() \
		gctUINT __containerCount;

#	define vgmSET_CONTAINER_NUMBER(Context, Container) \
		vgfSetContainerNumber(Context, Container)

	void vgfSetContainerNumber(
		vgsCONTEXT_PTR Context,
		vgsCONTAINER_PTR Container
		);

#else

#	define vgmDEFINE_CONTAINER_COUNTER()
#	define vgmSET_CONTAINER_NUMBER(Context, Container)

#endif

/*----------------------------------------------------------------------------*/
/*-------------------------- VG Image debugging API. -------------------------*/

#if gcvDEBUG

void vgfDumpImage(
	IN vgsCONTEXT_PTR Context,
	IN gctSTRING Message,
	IN vgsIMAGE_PTR Image,
	IN gctINT32 X,
	IN gctINT32 Y,
	IN gctINT32 Width,
	IN gctINT32 Height
	);

#endif

/*----------------------------------------------------------------------------*/
/*---------------------- VG Function Parameter Dumping. ----------------------*/

#if gcvDEBUG

#	define vgmDUMP_BYTE_ARRAY(Array, Count) \
		vgfDumpByteArray(#Array, (gctPOINTER) Array, Count)

#	define vgmDUMP_WORD_ARRAY(Array, Count) \
		vgfDumpWordArray(#Array, (gctPOINTER) Array, Count)

#	define vgmDUMP_DWORD_ARRAY(Array, Count) \
		vgfDumpDWordArray(#Array, (gctPOINTER) Array, Count)

#	define vgmDUMP_INTEGER_ARRAY(Array, Count) \
		vgfDumpIntegerArray(#Array, (gctPOINTER) Array, Count)

#	define vgmDUMP_FLOAT_ARRAY(Array, Count) \
		vgfDumpFloatArray(#Array, (gctPOINTER) Array, Count)

#	define vgmDUMP_PATH_RAW_DATA(Datatype, Count, Segments, Data) \
		vgfDumpPathRawData(Datatype, Count, Segments, Data)

	void vgfDumpByteArray(
		gctSTRING ArrayName,
		const gctPOINTER Array,
		gctUINT Count
		);

	void vgfDumpWordArray(
		gctSTRING ArrayName,
		const gctPOINTER Array,
		gctUINT Count
		);

	void vgfDumpDWordArray(
		gctSTRING ArrayName,
		const gctPOINTER Array,
		gctUINT Count
		);

	void vgfDumpIntegerArray(
		gctSTRING ArrayName,
		const gctPOINTER Array,
		gctUINT Count
		);

	void vgfDumpFloatArray(
		gctSTRING ArrayName,
		const gctPOINTER Array,
		gctUINT Count
		);

	void vgfDumpPathRawData(
		VGPathDatatype Datatype,
		gctINT NumSegments,
		const unsigned char * PathSegments,
		const void * PathData
		);

#else

#	define vgmDUMP_BYTE_ARRAY(Array, Count)
#	define vgmDUMP_WORD_ARRAY(Array, Count)
#	define vgmDUMP_DWORD_ARRAY(Array, Count)
#	define vgmDUMP_INTEGER_ARRAY(Array, Count)
#	define vgmDUMP_FLOAT_ARRAY(Array, Count)
#	define vgmDUMP_PATH_RAW_DATA(Datatype, Count, Segments, Data)

#endif

#define _GC_OBJ_ZONE				(gcvZONE_API_VG11 | 0xffff)

#ifdef __cplusplus
}
#endif

#endif
