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
--  $RCSfile: EncProfile.c,v $
--  $Date: 2008/04/15 10:01:02 $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "EncProfile.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef enum {
	SIMPLE_L0  = 8,
	SIMPLE_L0B = 9,
	SIMPLE_L1 = 1,
	SIMPLE_L2 = 2,
	SIMPLE_L3 = 3,
	SIMPLE_L4A = 4,
	SIMPLE_L5 = 5,
	SIMPLE_L6 = 6,
	MAIN_L2 = 50,
	MAIN_L3 = 51,
	MAIN_L4 = 52,
	ADVANCED_SIMPLE_L0 = 240,
	ADVANCED_SIMPLE_L1 = 241,
	ADVANCED_SIMPLE_L2 = 242,
	ADVANCED_SIMPLE_L3 = 243,
	ADVANCED_SIMPLE_L4 = 244,
	ADVANCED_SIMPLE_L5 = 245,
	H263_P0_L10 = 1001,
	H263_P0_L20 = 1002,
	H263_P0_L30 = 1003,
	H263_P0_L40 = 1004,
	H263_P0_L50 = 1005,
	H263_P0_L60 = 1006,
	H263_P0_L70 = 1007
} profile_e;

typedef struct {
	i32	mbPerVopMax;
	i32	videoBufferSize;/* units of 16384 bits */
	i32	vpSizeMax;	/* Max video packet length (bits) */
	i32	bitPerSecondMax;/* Max bitrate (kbit/s) 1kbit = 1000bit */
} parameter_s;

static const parameter_s parameter[] = {
	{99,   10,  2048,  64},		/* Simple L0  */
	{99,   20,  2048,  128},	/* Simple L0B */
	{99,   10,  2048,  64},		/* Simple L1 */
	{396,  40,  4096,  128},	/* Simple L2 */
	{396,  40,  8192,  384},	/* Simple L3 */
	{1200, 80,  16384, 4000},	/* Simple L4A */
	{1620, 112, 16384, 8000},	/* Simple L5 */
	{3600, 248, 16384, 12000},	/* Simple L6 */
	{594,  80,  8192,  2000},	/* Main L2 */
	{1620, 320, 16384, 15000},	/* Main L3 */
	{8160, 760, 16384, 38400},	/* Main L4 */
	{99,   10,  2048,  128},	/* Advanced Simple L0 */
	{99,   10,  2048,  128},	/* Advanced Simple L1 */
	{396,  40,  4096,  384},	/* Advanced Simple L2 */
	{396,  40,  4096,  768},	/* Advanced Simple L3 */
	{792,  80,  8192,  3000},	/* Advanced Simple L4 */
	{1620, 112, 16384, 8000},	/* Advanced Simple L5 */
	{99,   5,   0,     64},		/* H.263 Profile 0 Level 10  */
	{396,  18,  0,     128},	/* H.263 Profile 0 Level 20  */
	{396,  20,  0,     384},	/* H.263 Profile 0 Level 30  */
	{396,  33,  0,     2048},	/* H.263 Profile 0 Level 40  */
	{396,  50,  0,     4096},	/* H.263 Profile 0 Level 50  */
	{810,  99,  0,     8192},	/* H.263 Profile 0 Level 60  */
	{1620, 198, 0,     16384}	/* H.263 Profile 0 Level 70  */
        /* videoBufferSize for H.263 is
         * (BppMax + 4*bitPerSecondMax/(30000/1001)) / 16384
         * where BppMax is 64*1024 bits for picture sizes upto QCIF
         *                256*1024 bits for picture sizes upto CIF and
         *                512*1024 bits for picture sizes upto 4CIF
         * vpSize for H.263 is not applicable */
};

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

	EncProfile

------------------------------------------------------------------------------*/
bool_e EncInitProfile(profile_s *profile)
{
	i32 index = 6;
	i32 tmp;

	switch ((profile_e)profile->profile) {
		case SIMPLE_L0:
			index = 0;
			break;
		case SIMPLE_L0B:
			index = 1;
			break;
		case SIMPLE_L1:
			index = 2;
			break;
		case SIMPLE_L2:
			index = 3;
			break;
		case SIMPLE_L3:
			index = 4;
			break;
		case SIMPLE_L4A:
			index = 5;
			break;
		case SIMPLE_L5:
			index = 6;
			break;
		case SIMPLE_L6:
			index = 7;
			break;
		case MAIN_L2:
			index = 8;
			break;
		case MAIN_L3:
			index = 9;
			break;
		case MAIN_L4:
			index = 10;
			break;
		case ADVANCED_SIMPLE_L0:
			index = 11;
			break;
		case ADVANCED_SIMPLE_L1:
			index = 12;
			break;
		case ADVANCED_SIMPLE_L2:
			index = 13;
			break;
		case ADVANCED_SIMPLE_L3:
			index = 14;
			break;
		case ADVANCED_SIMPLE_L4:
			index = 15;
			break;
		case ADVANCED_SIMPLE_L5:
			index = 16;
			break;
		case H263_P0_L10:
			index = 17;
			break;
		case H263_P0_L20:
			index = 18;
			break;
		case H263_P0_L30:
			index = 19;
			break;
		case H263_P0_L40:
			index = 20;
			break;
		case H263_P0_L50:
			index = 21;
			break;
		case H263_P0_L60:
			index = 22;
			break;
		case H263_P0_L70:
			index = 23;
			break;
		default:
			return ENCHW_NOK;
	}

	/* Profile depend parameters. The default value of vbv_occupancy is 170
	 * × vbv_buffer_size, where vbv_occupancy is in 64-bit units and
	 * vbv_buffer_size is in 16384-bit units. This corresponds to an
	 * initial occupancy of approximately two-thirds of the full buffer
	 * size. User can set buffer size if needed or it comes from profile */
	if (profile->videoBufferSize == 1) {
		tmp = parameter[index].videoBufferSize;
	} else {
		tmp = profile->videoBufferSize;
	}
	profile->videoBufferSize = tmp * 16384;
	profile->videoBufferBitCnt = tmp * 170 * 64;

	profile->mbPerVopMax = parameter[index].mbPerVopMax;
	profile->vpSizeMax = parameter[index].vpSizeMax;
	profile->bitPerSecondMax = parameter[index].bitPerSecondMax;

	return ENCHW_OK;
}

/*------------------------------------------------------------------------------

	EncProfileCheck

------------------------------------------------------------------------------*/
bool_e EncProfileCheck(profile_s *profile)
{
	u32 tmp;

	/* Image size */
	tmp = (profile->width/16) * (profile->height/16);
	if (tmp > profile->mbPerVopMax) {
		return ENCHW_NOK;
	}

	/* Videpacket size */
	if (profile->dataPart == ENCHW_YES) {
		if (profile->vpSize > profile->vpSizeMax) {
			return ENCHW_NOK;
		}
	}

	/* Bitrate */
	tmp = profile->bitPerSecondMax * 1000;
	if (profile->bitPerSecond > tmp) {
		return ENCHW_NOK;
	}

	/* Simple profile levels 0 and 0b */
	if (((profile_e)profile->profile == SIMPLE_L0) ||
	    ((profile_e)profile->profile == SIMPLE_L0B)) {
		if (profile->vopTimeIncRes > profile->timeInc*15) {
			return ENCHW_NOK;
		}
		if (profile->mvRange != 0) {
			return ENCHW_NOK;
		}
		if (profile->intraDcVlcThr != 0) {
			return ENCHW_NOK;
		}
		if ((profile->width > 176) || (profile->height > 144)) {
			return ENCHW_NOK;
		}
		if ((profile->acPred == ENCHW_YES) && (profile->mbRc == ENCHW_YES)) {
			return ENCHW_NOK;
		}
	}

	/* Advanced Simple profile level 0 */
	if ((profile_e)profile->profile == ADVANCED_SIMPLE_L0) {
		if ((profile->acPred == ENCHW_YES) && (profile->mbRc == ENCHW_YES)) {
			return ENCHW_NOK;
		}
	}

	/* Maximum amount of AC-predicted macroblocks is limited:
	 * Advanced Simple Profile Level 4, 396 macroblocks
	 * Advanced Simple Profile Level 5, 405 macroblocks */
	if ((profile_e)profile->profile == ADVANCED_SIMPLE_L4) {
		tmp = (profile->width/16) * (profile->height/16);
		if ((profile->acPred == ENCHW_YES) && (tmp > 396)) {
			return ENCHW_NOK;
		}
	}
	if ((profile_e)profile->profile == ADVANCED_SIMPLE_L5) {
		tmp = (profile->width/16) * (profile->height/16);
		if ((profile->acPred == ENCHW_YES) && (tmp > 405)) {
			return ENCHW_NOK;
		}
	}

	/* H.263 profile 0 level 10 */
	if ((profile_e)profile->profile == H263_P0_L10) {
		if (((profile->width != 128) || (profile->height != 96)) &&
		    ((profile->width != 176) || (profile->height != 144))) {
			return ENCHW_NOK;
		}
		if (profile->vopTimeIncRes > profile->timeInc*15) {
			return ENCHW_NOK;
		}
	}

	/* H.263 profile 0 level 20 */
	if ((profile_e)profile->profile == H263_P0_L20) {
		/* Upto 15fps for CIF and
		   Upto 30fps for QCIF */
		if (((profile->width != 128) || (profile->height != 96)) &&
		    ((profile->width != 176) || (profile->height != 144)) &&
		    ((profile->width != 352) || (profile->height != 288))) {
			return ENCHW_NOK;
		}
		if ((profile->vopTimeIncRes > profile->timeInc*15) && 
		    ((profile->width > 176) || (profile->height > 144))) {
			return ENCHW_NOK;
		}
		if (profile->vopTimeIncRes > profile->timeInc*30) {
			return ENCHW_NOK;
		}
	}
	/* H.263 profile 0 levels 30 and 40 */
	if (((profile_e)profile->profile == H263_P0_L30) ||
	    ((profile_e)profile->profile == H263_P0_L40)) {
		if (((profile->width != 128) || (profile->height != 96)) &&
		    ((profile->width != 176) || (profile->height != 144)) &&
		    ((profile->width != 352) || (profile->height != 288))) {
			return ENCHW_NOK;
		}
		if (profile->vopTimeIncRes > profile->timeInc*30) {
			return ENCHW_NOK;
		}
	}

	/* H.263 profile 0 level 50 */
	if ((profile_e)profile->profile == H263_P0_L50) {
		/* Upto 50fps for CIF and upto 60fps for 352x240 */
		if ((profile->width > 352) || (profile->height > 288)) {
			return ENCHW_NOK;
		}
		if ((profile->vopTimeIncRes > profile->timeInc*50) && 
				(profile->height > 240)) {
			return ENCHW_NOK;
		}
		if (profile->vopTimeIncRes > profile->timeInc*60) {
			return ENCHW_NOK;
		}
	}

	/* H.263 profile 0 level 60 */
	if ((profile_e)profile->profile == H263_P0_L60) {
		/* Upto 50fps for 720x288 and upto 60fps for 720x240 */
		if ((profile->width > 720) || (profile->height > 288)) {
			return ENCHW_NOK;
		}
		if ((profile->vopTimeIncRes > profile->timeInc*50) && 
				(profile->height > 240)) {
			return ENCHW_NOK;
		}
		if (profile->vopTimeIncRes > profile->timeInc*60) {
			return ENCHW_NOK;
		}
	}

	/* H.263 profile 0 level 70 */
	if ((profile_e)profile->profile == H263_P0_L70) {
		/* Upto 50fps for 720x576 and upto 60fps for 720x480 */
		if ((profile->width > 720) || (profile->height > 576)) {
			return ENCHW_NOK;
		}
		if ((profile->vopTimeIncRes > profile->timeInc*50) && 
				(profile->height > 480)) {
			return ENCHW_NOK;
		}
		if (profile->vopTimeIncRes > profile->timeInc*60) {
			return ENCHW_NOK;
		}
	}

	return ENCHW_OK;
}
