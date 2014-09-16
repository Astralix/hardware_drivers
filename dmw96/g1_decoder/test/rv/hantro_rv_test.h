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
--  Abstract : RV decoder testbench
--
------------------------------------------------------------------------------*/


#ifndef __HANTRO_RV_TEST_H__
#define __HANTRO_RV_TEST_H__
 
#ifdef __cplusplus
extern "C" {
#endif

    /*
#ifndef WIN32
	#define RV_TIME_TRACE
	#define RV_DISPLAY
	#define	RV_ERROR_SIM
#endif
*/

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

void rv_display(const char *filename,
		char *filenamePpCfg,
		int startframeid,
		int maxnumber,
		int iswritefile, 
		int isdisplay,
		int displaymode,
		int rotationmode,
		int islog2std,
		int islog2file,
		int blayerenable,
		int errconceal,
		int randomerror);
 
#ifdef __cplusplus
}
#endif
 
#endif
