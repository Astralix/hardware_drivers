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
--  Abstract : API's internal static data storage definition
--
------------------------------------------------------------------------------*/

#ifndef RV_APISTORAGE_H
#define RV_APISTORAGE_H

#include "dwl.h"

typedef struct
{
    enum
    {
        UNINIT,
        INITIALIZED,
        HEADERSDECODED,
        STREAMDECODING,
        HW_PIC_STARTED,
        HW_STRM_ERROR
    } DecStat;

    enum
    {
        NO_BUFFER = 0,
        BUFFER_0,
        BUFFER_1,
        BUFFER_2,
        BUFFER_3
    } bufferForPp;

    DWLLinearMem_t InternalFrameIn;
    DWLLinearMem_t InternalFrameOut;

    u32 firstHeaders;
    u32 disableFilter;
    u32 externalBuffers;    /* application gives frame buffers */

} DecApiStorage;

#endif /* #ifndef RV_APISTORAGE_H */
