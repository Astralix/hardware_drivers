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




#ifndef __gc_glff_hash_h_
#define __gc_glff_hash_h_

#ifdef __cplusplus
extern "C" {
#endif

#define glvENABLE_HASHING 1


/******************************************************************************\
**************************** Hash Value Update Macros **************************
\******************************************************************************/

#define glmSETHASH_1BIT(Name, Value, Bit) \
	if (Value) \
	{ \
		Context->hashKey.Name |=  (1 << (Bit)); \
	} \
	else \
	{ \
		Context->hashKey.Name &= ~(1 << (Bit)); \
	}

#define glmSETHASH_2BITS(Name, Value, Slot) \
	gcmASSERT((gctUINT) (Value) <= 3); \
	{ \
		gctUINT shift = (Slot) * 2; \
		Context->hashKey.Name &= ~(3 << shift); \
		Context->hashKey.Name |=  ((Value) << shift); \
	}

#define glmSETHASH_3BITS(Name, Value, Slot) \
	gcmASSERT((gctUINT) (Value) <= 7); \
	{ \
		gctUINT shift = (Slot) * 3; \
		Context->hashKey.Name &= ~(7 << shift); \
		Context->hashKey.Name |=  ((Value) << shift); \
	}

#define glmSETHASH_4BITS(Name, Value, Slot) \
	gcmASSERT((gctUINT) (Value) <= 15); \
	{ \
		gctUINT shift = (Slot) * 4; \
		Context->hashKey.Name &= ~(15 << shift); \
		Context->hashKey.Name |=  ((Value) << shift); \
	}

#define glmCLEARHASH_2BITS(Name, Slot) \
	{ \
		gctUINT shift = (Slot) * 2; \
		Context->hashKey.Name |= (3 << shift); \
	}

#define glmCLEARHASH_3BITS(Name, Slot) \
	{ \
		gctUINT shift = (Slot) * 3; \
		Context->hashKey.Name |= (7 << shift); \
	}


/******************************************************************************\
***************************** Hash Table Structures ****************************
\******************************************************************************/

typedef struct _glsPROGRAMINFO    * glsPROGRAMINFO_PTR;
typedef struct _glsHASHTABLEENTRY * glsHASHTABLEENTRY_PTR;
typedef struct _glsHASHTABLE      * glsHASHTABLE_PTR;

typedef struct _glsPROGRAMINFO
{
	/* Timestamp. */
	gctUINT					timestamp;

	/* Shader wrappers. */
	glsSHADERCONTROL		vs;
	glsSHADERCONTROL		fs;

	/* Shader program state buffer. */
	gctSIZE_T				programSize;
	gctPOINTER				programBuffer;
	gcsHINT_PTR				hints;
}
glsPROGRAMINFO;

typedef struct _glsHASHTABLEENTRY
{
	gctPOINTER				key;
	glsPROGRAMINFO			program;
	glsHASHTABLEENTRY_PTR	next;
}
glsHASHTABLEENTRY;

typedef struct _glsHASHTABLE
{
	gctUINT					entryCount;
	glsHASHTABLEENTRY_PTR	chain;
}
glsHASHTABLE;


/******************************************************************************\
********************************** Hash Key ************************************
\******************************************************************************/

typedef struct _glsHASHKEY
{

/*--- 32-bit boundary --------------------------------------------------------*/

	gctUINT32	hashPointPrimitive						: 1;  /* [ 0: 0] */
	gctUINT32	hashPointSmoothEnabled					: 1;  /* [ 1: 1] */
	gctUINT32	hashPointSpriteEnabled					: 1;  /* [ 2: 2] */
	gctUINT32	hashLightingEnabled						: 1;  /* [ 3: 3] */
	gctUINT32	hashStageEnabled						: 4;  /* [ 7: 4] */
	gctUINT32	hashTextureFormat						: 12; /* [19: 8] */
	gctUINT32	hashTextureFunction						: 12; /* [31:20] */

/*--- 32-bit boundary --------------------------------------------------------*/

	gctUINT32	hashFogEnabled							: 1;  /* [ 0: 0] */
	gctUINT32	hashClipPlaneEnabled					: 6;  /* [ 6: 1] */
	gctUINT32	hashColorStreamEnabled					: 1;  /* [ 7: 7] */
	gctUINT32	hashNormalStreamEnabled					: 1;  /* [ 8: 8] */
	gctUINT32	hashTexCoordStreamEnabled				: 4;  /* [12: 9] */
	gctUINT32	hashPointSizeStreamEnabled				: 1;  /* [13:13] */
	gctUINT32	hashModelViewIdentity					: 1;  /* [14:14] */
	gctUINT32	hashProjectionIdentity					: 1;  /* [15:15] */
	gctUINT32	hashModelViewProjectionIdentity			: 1;  /* [16:16] */
	gctUINT32	hashModelViewInverse3x3TransIdentity	: 1;  /* [17:17] */
	gctUINT32	hashTextureIdentity						: 4;  /* [21:18] */
	gctUINT32	hashRescaleNormal						: 1;  /* [22:22] */
	gctUINT32	hashNormalizeNormal						: 1;  /* [23:23] */
	gctUINT32	hashLightEnabled						: 8;  /* [31:24] */

/*--- 32-bit boundary --------------------------------------------------------*/

	gctUINT32	hashMaterialEnabled						: 1;  /* [ 0: 0] */
	gctUINT32	hashColorCurrValueZero					: 1;  /* [ 1: 1] */
	gctUINT32	hashMultisampleEnabled					: 1;  /* [ 2: 2] */
	gctUINT32	hashFSRoundingEnabled					: 1;  /* [ 3: 3] */
	gctUINT32	hashTextureCombAlphaFunction			: 12; /* [15: 4] */
	gctUINT32	hashTextureCombColorFunction			: 16; /* [31:16] */

/*--- 32-bit boundary --------------------------------------------------------*/

	gctUINT32	hashMatrixPaletteEnabled				: 1;  /* [ 0: 0] */
	gctUINT32	hashFogMode								: 2;  /* [ 2: 1] */
	gctUINT32	hashDrawTextureOES						: 1;  /* [ 3: 3] */
	gctUINT32	hashTexCombColorScaleOne				: 4;  /* [ 7: 4] */
	gctUINT32	hashTexCombAlphaScaleOne				: 4;  /* [11: 8] */
	gctUINT32	hashTexCoordComponentCount				: 8;  /* [19:12] */
	gctUINT32	hashTwoSidedLighting					: 1;  /* [20:20] */
	gctUINT32	hashClockwiseFront						: 1;  /* [21:21] */
	gctUINT32	hashClearRectEnabled					: 1;  /* [22:22] */
	gctUINT32	hashTexABGRConvert						: 4;  /* [26:23] */
	gctUINT32	hashDepthBiasFixEnabled					: 1;  /* [27:27] */
	gctUINT32	zzz_FILLER1								: 4;  /* [31:28] */

/*--- 32-bit boundary --------------------------------------------------------*/

	gctUINT32	hashTextureCombColorSource0				: 8;  /* [ 7: 0] */
	gctUINT32	hashTextureCombColorSource1				: 8;  /* [15: 8] */
	gctUINT32	hashTextureCombColorSource2				: 8;  /* [23:16] */
	gctUINT32	hashTextureCombColorOperand0			: 8;  /* [31:24] */

/*--- 32-bit boundary --------------------------------------------------------*/

	gctUINT32	hashTextureCombColorOperand1			: 8;  /* [ 7: 0] */
	gctUINT32	hashTextureCombColorOperand2			: 8;  /* [15: 8] */
	gctUINT32	hashTextureCombAlphaSource0				: 8;  /* [23:16] */
	gctUINT32	hashTextureCombAlphaSource1				: 8;  /* [31:24] */

/*--- 32-bit boundary --------------------------------------------------------*/

	gctUINT32	hashTextureCombAlphaSource2				: 8;  /* [ 7: 0] */
	gctUINT32	hashTextureCombAlphaOperand0			: 4;  /* [11: 8] */
	gctUINT32	hashTextureCombAlphaOperand1			: 4;  /* [15:12] */
	gctUINT32	hashTextureCombAlphaOperand2			: 4;  /* [19:16] */
	gctUINT32	hashZeroEcm								: 1;  /* [20:20] */
	gctUINT32	hashZeroAcm								: 1;  /* [21:21] */
	gctUINT32	hashZeroDcm								: 1;  /* [22:22] */
	gctUINT32	hashZeroAcs								: 1;  /* [23:23] */
	gctUINT32	hashZeroScm								: 1;  /* [24:24] */
	gctUINT32	hashZeroSrm								: 1;  /* [25:25] */
	gctUINT32	hashMatrixIndexStreamEnabled			: 1;  /* [26:26] */
	gctUINT32	hashWeightStreamEnabled					: 1;  /* [27:27] */
	gctUINT32	hashMatrixIndexStreamComponents			: 2;  /* [29:28] */
	gctUINT32	hashWeightStreamComponents				: 2;  /* [31:30] */

/*--- 32-bit boundary --------------------------------------------------------*/

	gctUINT32	hashZeroAcl								: 8;  /* [ 7: 0] */
	gctUINT32	hashZeroDcl								: 8;  /* [15: 8] */
	gctUINT32	hashZeroScl								: 8;  /* [23:16] */
	gctUINT32	hashOneK0								: 8;  /* [31:24] */

/*--- 32-bit boundary --------------------------------------------------------*/

	gctUINT32	hashZeroK1								: 8;  /* [ 7: 0] */
	gctUINT32	hashZeroK2								: 8;  /* [15: 8] */
	gctUINT32	hashCrl_180								: 8;  /* [23:16] */
	gctUINT32	hashDirectionalLight					: 8;  /* [31:24] */

/*--- 32-bit boundary --------------------------------------------------------*/

	gctUINT32	hashTexCubeCoordGenEnable				: 4;  /* [ 3: 0] */
	gctUINT32	hashTexCubeCoordGenMode					: 4;  /* [ 7: 4] */
	gctUINT32	zzz_FILLER2								: 24; /* [31: 8] */

/*--- 32-bit boundary --------------------------------------------------------*/
	gctFLOAT	hashDepthBias       					    ;  /* [31:0] */
}
glsHASHKEY;


/******************************************************************************\
******************************* Shader Hash API ********************************
\******************************************************************************/

gceSTATUS glfInitializeHashTable(
	glsCONTEXT_PTR Context
	);

gceSTATUS glfFreeHashTable(
	glsCONTEXT_PTR Context
	);

gceSTATUS glfGetHashedProgram(
	glsCONTEXT_PTR Context,
	glsPROGRAMINFO_PTR* Program
	);

#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_hash_h_ */
