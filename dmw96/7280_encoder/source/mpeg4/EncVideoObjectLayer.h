/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract :
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: EncVideoObjectLayer.h,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_VIDEO_OBJECT_LAYER_H__
#define __ENC_VIDEO_OBJECT_LAYER_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "EncUserData.h"
#include "encputbits.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef struct {
	true_e	vbvParameter;			/* ENCHW_YES/ENCHW_NO */
	i32	chromaFormat;			/* Table 6-13 */
	i32	lowDelay;			/* value = [0,1] */
	i32	firstHalfBitRate;		/* 15 bit */
	i32	latterHalfBitRate;		/* 15 bit */
	i32	firstHalfVbvBufferSize;		/* 15 bit */
	i32	latterHalfVbvBufferSize;	/*  3 bit */
	i32	firstHalfVbvOccupancy;		/* 11 bit */
	i32	latterHalfVbvOccupancy;		/* 15 bit */
} volControlParameter_s;

/* videoObjectLayerShape (This is index to Table 6-14) */
typedef enum {
	RECTANGULAR = 0,
	BINARY      = 1,
	BINARYONLY  = 2,
	GRAYSCALE   = 3
} layerShape_e;

/* Video Object Layer */
typedef struct {
	true_e	header;				/* ENCHW_YES/ENCHW_NO */
	true_e	isObjectLayerIdentifier;	/* ENCHW_YES/ENCHW_NO */
	true_e	isVolControlParameter;		/* ENCHW_YES/ENCHW_NO */
	true_e	fixedVopRate;			/* ENCHW_YES/ENCHW_NO */
	true_e	complexityEstimationDisable;	/* ENCHW_YES/ENCHW_NO */
	true_e	resyncMarkerDisable;		/* ENCHW_YES/ENCHW_NO */
	true_e	dataPart;		/* ENCHW_YES/ENCHW_NO */
	true_e	rvlc;			/* ENCHW_YES/ENCHW_NO */
	i32	randomAccessibleVol;		/* value = [0,1] */
	i32	videoObjectTypeIndication;	/* Table 6-10 */
	i32	videoObjectLayerVerid;		/* Table 6-11 */
	i32	videoObjectLayerPriority;	/* value = [1,2...7] */
	i32	aspectRatioInfo;		/* Table 6-12 */
	i32	parWidth;			/* value = [1...255] */
	i32	parHeight;			/* value = [1...255] */
	i32	vopTimeIncRes;	/* Input frame rate numerator */
	i32	fixedVopTimeInc;		/* Input frame rate numerator */
	i32	videoObjectLayerWidth;		/* 13 bit */
	i32	videoObjectLayerHeight;		/* 13 bit */
	volControlParameter_s volControlParameter;
	userData_s userData;			/* User data */
} vol_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void EncVolInit(vol_s *vol);
void EncVolHdr(stream_s *, vol_s *);

#endif
