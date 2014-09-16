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
--  $RCSfile: EncStartCode.h,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_START_CODE_H__
#define __ENC_START_CODE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
/* Table 6-3 */
/* Start code prefix */
#define START_CODE_PREFIX_VAL       1
#define START_CODE_PREFIX_NUM       24

/* User Data */
#define START_CODE_USER_DATA_VAL    0xB2
#define START_CODE_USER_DATA_NUM    8

/* Visual Object Sequence */
#define START_CODE_VOS_VAL          0xB0
#define START_CODE_VOS_NUM          8

/* Visual Object Sequence End*/
#define START_CODE_VOS_END_VAL      0xB1
#define START_CODE_VOS_END_NUM      8

/* Visual Object */
#define START_CODE_VO_VAL           0xB5
#define START_CODE_VO_NUM           8

/* Video Object Layer */
#define START_CODE_VOL_VAL          0x20
#define START_CODE_VOL_NUM          8

/* Group Of Vop */
#define START_CODE_GOB_VAL          0xB3
#define START_CODE_GOB_NUM          8

/* Video Object */
#define START_CODE_VOB_VAL          0x00
#define START_CODE_VOB_NUM          8

/* Video Object Plane */
#define START_CODE_VOP_VAL          0xB6
#define START_CODE_VOP_NUM          8

/* Short Video Start Marker */
#define START_CODE_SVH_S_VAL        0x20
#define START_CODE_SVH_S_NUM        22

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

#endif
