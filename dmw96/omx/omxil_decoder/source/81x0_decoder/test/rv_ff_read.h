/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2007 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : RV decoder testbench
--
------------------------------------------------------------------------------*/


#ifndef __HANTRO_RV_TEST_H__
#define __HANTRO_RV_TEST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <OMX_Types.h>
#include "basetype.h"

    /*
#ifndef WIN32
	#define RV_TIME_TRACE
	#define RV_DISPLAY
	#define	RV_ERROR_SIM
#endif
*/

typedef struct
{
    u32 offset;
    u32 isValid;
} RvSliceInfo;

typedef struct
{
    FILE * pFile;
    OMX_U32** pBuffer;
    OMX_U32* pAllocLen;
    OMX_BOOL* eof;
}thread_data;

#ifdef RV_TIME_TRACE
	#include <stdio.h>
	#define TIME_PRINT(args) printf args
#else
	#define TIME_PRINT(args)
#endif

#ifdef RV_DISPLAY
	#define VIM_DISPLAY(args) V8_DisplayForVdecTwoBuf_Rotation args
#else
	#define VIM_DISPLAY(args)
#endif

void rv_display(void * pTdata);

#ifdef __cplusplus
}
#endif

#endif
