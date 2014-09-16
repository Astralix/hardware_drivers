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
--  $RCSfile: EncShortVideoHeader.h,v $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_SHORT_VIDEO_HEADER_H__
#define __ENC_SHORT_VIDEO_HEADER_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "EncVideoObjectPlane.h"
#include "encputbits.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef struct
{
    true_e header;  /* ENCHW_YES/ENCHW_NO */
    vopType_e vopType;  /* Table 6-20 */
    i32 tempRef;
    u32 splitScreenIndicator;
    u32 documentCameraIndicator;
    u32 fullPictureFreezeRelease;
    u32 sourceFormat;   /* Table 6-25 */
    i32 gobPlace;   /* Define place of gob */
    u32 mbInGob;    /* Total number of macroblocks in GOB */
    i32 gobNum; /* GOB number */
    u32 gobFrameId; /* Some fields has been changed */
    u32 gobFrameIdBit;  /* GobFrameId bit field */
    i32 timeCode;   /* TimeCode remainder */

    true_e plusHeader;  /* ENCHW_YES/ENCHW_NO */
    i32 roundControl;   /* 1/0 */
    i32 videoObjectLayerWidth;  /* vol->videoObjectLayerWidth */
    i32 videoObjectLayerHeight; /* vol->videoObjectLayerHeight */
    i32 pwi;    /* Pixel per line (pwi+1)*4 */
    i32 phi;    /* Numer of lines phi*4 */
    i32 ufepTempRef;    /* Last UFEP="001" time cnt */
    i32 ufepTempRefPrev;    /* Previous value of svh->tempRef */
    i32 ufepCnt;    /* Last UFEP="001" vop cnt */
    u32 gobs;   /* number of GOBs in picture */
} svh_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void EncSvhInit(svh_s *);
void EncSvhHdr(stream_s *, svh_s *, i32 qp);
void EncGobFrameIdUpdate(svh_s *);
bool_e EncSvhCheck(svh_s *);

#endif
