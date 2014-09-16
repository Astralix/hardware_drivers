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
--  $RCSfile: EncProfile.h,v $
--  $Author: sapi $
--  $Date: 2008/05/23 11:50:53 $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_PROFILE__
#define __ENC_PROFILE__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef struct {
	u32	width;		/* Width of image rounded next mb boundary */
	u32	height;		/* Height of image rounded next mb boundary */
	u32	bitPerSecond;	/* Input bit rate per second */
	u32	vopTimeIncRes;	/* Input frame rate numerator */
	u32	timeInc;	/* Input frame rate denominator */
	u32	vpSize;		/* Video packet size */
	true_e	dataPart;	/* Data partition */
	u32	mvRange;	/* 0 -> mv range [-16 15], 1 -> mv [-32 31] */
	u32	intraDcVlcThr;	/* Table 6-21 */
	true_e	mbRc;		/* Mb header qp can vary, check point rc */
	true_e	acPred;		/* ENCHW_YES/ENCHW_NO */
	u32	profile;	/* Profile and level (Table G-1) */
	u32	videoBufferSize;/* Rate control need this */
	u32	videoBufferBitCnt;
	u32	mbPerVopMax;	/* Profile limitation of macroblocks per vop */
	u32	vpSizeMax;	/* Profile limitation of vp size */
	u32	bitPerSecondMax;/* Profile limitation of bitrate */
} profile_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
bool_e EncInitProfile(profile_s *profile);
bool_e EncProfileCheck(profile_s *profile);

#endif

