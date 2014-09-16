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
--  $RCSfile: EncVideoObjectPlane.h,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_VIDEO_OBJECT_PLANE_H__
#define __ENC_VIDEO_OBJECT_PLANE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "encputbits.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
/* vopType (Table 6-20) */
typedef enum
{
    IVOP = 0,
    PVOP = 1,
    BVOP = 2,
    SVOP = 3
} vopType_e;

/* Video Object Plane */
typedef struct
{
    true_e header;  /* ENCHW_YES/ENCHW_NO */
    true_e vopCoded;    /* ENCHW_YES/ENCHW_NO */
    true_e hec; /* ENCHW_YES/ENCHW_NO */
    true_e hecVp;   /* ENCHW_YES/ENCHW_NO for next video packet */
    true_e acPred;  /* ENCHW_YES/ENCHW_NO */
    vopType_e vopType;  /* Table 6-20 */
    i32 roundControl;   /* 1/0 */
    i32 intraDcVlcThr;  /* Table 6-21 */
    i32 vopFcodeForward;    /* Table 7-5 */
    i32 vopTimeInc; /* 1-16 bit */
    i32 moduloTimeBase; /* Seconds elapsed since last ref */
    i32 vopTimeIncRes;  /* Input frame rate numerator */
} vop_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void EncVopInit(vop_s * vop);
void EncVopHdr(stream_s *, const vop_s *, i32);
void EncVpHdr(stream_s * stream, vop_s * vop, u32 mbNum, u32 mbNumBits, u32 qp);

#endif
