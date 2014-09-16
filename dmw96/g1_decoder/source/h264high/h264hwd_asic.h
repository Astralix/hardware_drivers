/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2011 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Hardware interface read/write
--
------------------------------------------------------------------------------*/
#ifndef __H264ASIC_H__
#define __H264ASIC_H__

#include "basetype.h"
#include "dwl.h"
#include "h264hwd_container.h"
#include "h264hwd_storage.h"

#define ASIC_MB_RLC_BUFFER_SIZE     880 /* bytes */
#define ASIC_MB_CTRL_BUFFER_SIZE    8   /* bytes */
#define ASIC_MB_MV_BUFFER_SIZE      64  /* bytes */
#define ASIC_MB_I4X4_BUFFER_SIZE    8   /* bytes */
#define ASIC_CABAC_INIT_BUFFER_SIZE 3680/* bytes */
#define ASIC_SCALING_LIST_SIZE      6*16+2*64
#define ASIC_POC_BUFFER_SIZE        34*4

#define X170_DEC_TIMEOUT            0x00FFU
#define X170_DEC_SYSTEM_ERROR       0x0FFFU
#define X170_DEC_HW_RESERVED        0xFFFFU

/* asic macroblock types */
typedef enum H264AsicMbTypes
{
    HW_P_16x16 = 0,
    HW_P_16x8 = 1,
    HW_P_8x16 = 2,
    HW_P_8x8 = 3,
    HW_I_4x4 = 4,
    HW_I_16x16 = 5,
    HW_I_PCM = 6,
    HW_P_SKIP = 7
} H264AsicMbTypes_t;

u32 AllocateAsicBuffers(decContainer_t * pDecCont,
                        DecAsicBuffers_t * asicBuff, u32 mbs);
void ReleaseAsicBuffers(const void *dwl, DecAsicBuffers_t * asicBuff);

void PrepareIntra4x4ModeData(storage_t * pStorage,
                             DecAsicBuffers_t * pAsicBuff);
void PrepareMvData(storage_t * pStorage, DecAsicBuffers_t * pAsicBuff);

void PrepareRlcCount(storage_t * pStorage, DecAsicBuffers_t * pAsicBuff);

void H264SetupVlcRegs(decContainer_t * pDecCont);

void H264InitRefPicList(decContainer_t *pDecCont);

u32 H264RunAsic(decContainer_t * pDecCont, DecAsicBuffers_t * pAsicBuff);

#endif /* __H264ASIC_H__ */
