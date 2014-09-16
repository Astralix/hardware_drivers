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
--  Description : Debug Trace funtions
--
------------------------------------------------------------------------------*/

#ifndef __JPEGDEC_TRACE__
#define __JPEGDEC_TRACE__
#include <stdio.h>

#include "basetype.h"
#include "jpegdeccontainer.h"

void PrintJPEGReg(u32 * regBase);
void ppRegDump(const u32 * regBase);
void DumpJPEGCtrlReg(u32 * regBase, FILE *fd);
void HexDumpJPEGCtrlReg(u32 * regBase, FILE *fd);
void HexDumpJPEGTables(u32 * regBase, JpegDecContainer *pJpegDecCont, FILE *fd);
void HexDumpJPEGOutput(JpegDecContainer *pJpegDecCont, FILE *fd);
void HexDumpRegs(u32 * regBase, FILE *fd);
void ResetAsicBuffers(JpegDecContainer *pJpegDecCont, FILE *fd);


#endif /* __JPEGDEC_TRACE__ */
