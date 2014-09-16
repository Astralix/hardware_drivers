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
--  Abstract : algorithm header file
--
------------------------------------------------------------------------------*/

/*****************************************************************************/

#ifndef DECMBDESC_H_DEFINED
#define DECMBDESC_H_DEFINED

#include "basetype.h"

typedef struct DecMBDesc_t
{
    u8 typeOfMB; /* inter,interq,inter4v,intra,intraq,stuffing */
    u8 errorStatus;  /* bit 7 indicates whether mb is concealed or not
                         * bit 1 indicates if whole macro block was lost
                         * bit 0 indicates if macro block texture was lost
                         * (in data partitioned packets) */

} DecMBDesc;

#endif
