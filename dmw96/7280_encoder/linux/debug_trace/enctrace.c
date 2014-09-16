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
--  Description : Internal traces
--
--------------------------------------------------------------------------------
--  Version control information, please leave untouched.
--
--  $RCSfile: enctrace.c,v $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include <stdio.h>
#ifdef TRACE_TIMING
#include <sys/time.h>
#endif

#include "enctrace.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
#ifdef TRACE_TIMING
#define PAD "        "
#else
#define PAD ""
#endif

static FILE *fileStream = NULL;
static FILE *fileAsic = NULL;
static FILE *filePreProcess = NULL;
static FILE *fRlcDump = NULL;
static FILE *fRlcMb = NULL;
static FILE *fControl = NULL;
static FILE *fRlc = NULL;
static FILE *fRegs = NULL;
static FILE *fRecon = NULL;
static FILE *fTrace = NULL;

#ifdef TRACE_TIMING
static struct timeval tv;
#endif

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

void EncTraceCloseAll(void)
{
    if(fileStream != NULL)
        fclose(fileStream);
    if(fileAsic != NULL)
        fclose(fileAsic);
    if(filePreProcess != NULL)
        fclose(filePreProcess);
    if(fRlcDump != NULL)
        fclose(fRlcDump);
    if(fRlcMb != NULL)
        fclose(fRlcMb);
    if(fControl != NULL)
        fclose(fControl);
    if(fRlc != NULL)
        fclose(fRlc);
    if(fRegs != NULL)
        fclose(fRegs);
    if(fRecon != NULL)
        fclose(fRecon);
    if(fTrace != NULL)
        fclose(fTrace);
}

/*------------------------------------------------------------------------------

    EncTraceAsic

------------------------------------------------------------------------------*/
void EncTraceAsicParameters(asicData_s * asic)
{
    if(fileAsic == NULL)
        fileAsic = fopen("sw_asic.trc", "w");

    if(fileAsic == NULL)
    {
        fprintf(stderr, "Unable to open trace file: asic.trc\n");
        return;
    }

    fprintf(fileAsic, "ASIC parameters:\n");
    fprintf(fileAsic, "Input lum bus:       0x%08x\n",
            (u32) asic->regs.inputLumBase);
    fprintf(fileAsic, "Input cb bus:        0x%08x\n",
            (u32) asic->regs.inputCbBase);
    fprintf(fileAsic, "Input cr bus:        0x%08x\n\n",
            (u32) asic->regs.inputCrBase);

    /*fprintf(fileAsic, "Internal image base W: 0x%08x\n", (u32)asic->internalImageBase.virtualAddress); */
    fprintf(fileAsic, "Control base:        0x%p\n",
            asic->controlBase.virtualAddress);

    fprintf(fileAsic, "Encoding type:           %6d\n", asic->regs.codingType);
    /*
     * fprintf(fileAsic, "Input endian mode:       %6d\n",
     * asic->regs.inputEndianMode);
     * fprintf(fileAsic, "Input endian width:      %6d\n",
     * asic->regs.inputEndianWidth);
     * fprintf(fileAsic, "Output endian width:      %6d\n",
     * asic->regs.outputEndianWidth);
     */
    fprintf(fileAsic, "Mbs in row:              %6d\n", asic->regs.mbsInRow);
    fprintf(fileAsic, "Mbs in col:              %6d\n", asic->regs.mbsInCol);
    fprintf(fileAsic, "Input lum width:         %6d\n", asic->regs.pixelsOnRow);
    fprintf(fileAsic, "Frame type:              %6d\n",
            asic->regs.frameCodingType);
    fprintf(fileAsic, "QP:                      %6d\n", asic->regs.qp);
    fprintf(fileAsic, "Round control:           %6d\n",
            asic->regs.roundingCtrl);
/*    fprintf(fileAsic, "MV SAD penalty:          %6d\n", asic->regs.mvSadPenalty);
    fprintf(fileAsic, "Intra SAD penalty:       %6d\n", asic->regs.intraSadPenalty);
    fprintf(fileAsic, "4MV SAD penalty:         %6d\n", asic->regs.fourMvSadPenalty);
*/

    fprintf(fileAsic, "Burst size:              %6d\n", ENC7280_BURST_LENGTH);

}

/*------------------------------------------------------------------------------

    TraceTime

------------------------------------------------------------------------------*/
void TraceTime(FILE * fp)
{
#ifdef TRACE_TIMING
    gettimeofday(&tv, NULL);

    if(fp)
        fprintf(fp, "%6ld  ", tv.tv_usec);
#endif
    (void) fp;
}

/*------------------------------------------------------------------------------

    EncTraceAsic

------------------------------------------------------------------------------*/
void EncTraceAsicQp(i32 qp)
{
    if(fTrace == NULL)
        return;

    TraceTime(fTrace);

    fprintf(fTrace, "New target qp:       %6d\n", qp);

}

/*------------------------------------------------------------------------------

    EncTraceAsic

------------------------------------------------------------------------------*/
void EncTraceAsicStatus(i32 status, u32 mbNum)
{
    if(fTrace == NULL)
        return;

    TraceTime(fTrace);

    fprintf(fTrace, "MB %4d    ASIC status: %4d MB    ",
            mbNum, (status & 0xFFFF0000) >> 16);

    if(status & ASIC_STATUS_ERROR)
        fprintf(fTrace, "ERROR    ");

    if(status & ASIC_STATUS_BUFF_FULL)
        fprintf(fTrace, "BUFF_FULL    ");

    if(status & ASIC_STATUS_FRAME_READY)
        fprintf(fTrace, "FRAME_READY    ");

    if(status & ASIC_STATUS_ENABLE)
        fprintf(fTrace, "ENABLE    ");

    fprintf(fTrace, "\n");
}

/*------------------------------------------------------------------------------

    EncTraceAsic

------------------------------------------------------------------------------*/
void EncTraceAsicEvent(i32 mbNum)
{
    if(fTrace == NULL)
        return;

    TraceTime(fTrace);

    fprintf(fTrace, "Waiting for ASIC event. Current mbNum %d\n", mbNum);

}

/*------------------------------------------------------------------------------

    EncTracePreProcess

------------------------------------------------------------------------------*/
void EncTracePreProcess(preProcess_s * preProcess)
{
    if(filePreProcess == NULL)
        filePreProcess = fopen("sw_preprocess.trc", "w");

    if(filePreProcess == NULL)
    {
        fprintf(stderr, "Unable to open trace file: preprocess.trc\n");
        return;
    }

    fprintf(filePreProcess, "Input width: %d\n", preProcess->lumWidthSrc);
    fprintf(filePreProcess, "Input height: %d\n", preProcess->lumHeightSrc);
    fprintf(filePreProcess, "Hor offset src: %d\n", preProcess->horOffsetSrc);
    fprintf(filePreProcess, "Ver offset src: %d\n", preProcess->verOffsetSrc);
}

/*------------------------------------------------------------------------------

    EncTraceRlc

------------------------------------------------------------------------------*/
void EncTraceRlc(const u32 * ptr, u32 block, u32 run, i32 level)
{
    static const u32 *lastPtr = NULL;
    static i32 addrCnt = (-4);

    if(fRlc == NULL)
        fRlc = fopen("sw_rlc.trc", "w");

    if(fRlc == NULL)
        return;

    if(lastPtr != ptr)
        addrCnt += 4;

    lastPtr = ptr;

    fprintf(fRlc, "0x%08x\t%p\t%8d\tb=%d\t%2d  %3d\n",
            addrCnt, ptr, *ptr, block, run, level);
}

/*------------------------------------------------------------------------------

    EncTraceRlcMb

------------------------------------------------------------------------------*/
void EncTraceRlcMb(u32 mbNum, u32 * lastRlc)
{
    static i32 vop = -1;

    if(fRlc == NULL)
        fRlc = fopen("sw_rlc.trc", "w");

    if(fRlc == NULL)
        return;

    if(mbNum == 0)
        vop++;

    fprintf(fRlc, "vop=%d mb=%d  last rlc=%p\n", vop, mbNum, lastRlc);

}

/*------------------------------------------------------------------------------

    EncTraceRegs

------------------------------------------------------------------------------*/
void EncTraceRegs(const void *ewl, u32 mbNum)
{
    u32 i;

    if(fRegs == NULL)
        fRegs = fopen("/tmp/sw_reg.trc", "w");

    if(fRegs == NULL)
        fRegs = stderr;

    if(mbNum == 0)
        fprintf(fRegs, "\n=== NEW VOP ====================================\n");

    fprintf(fRegs, "\nASIC register dump, mb=%4d\n\n", mbNum);

    /* Dump registers */
    for(i = 0; i <= 0x70; i += 4)
        fprintf(fRegs, "Reg 0x%02x    0x%08x\n", i, EWLReadReg(ewl, i));

    fprintf(fRegs, "\n");

    /*fclose(fRegs);
     * fRegs = NULL; */

}

/*------------------------------------------------------------------------------

    EncDumpRlc

------------------------------------------------------------------------------*/
void EncDumpRlc(const i16 * data, u32 size)
{
    if(fRlcDump == NULL)
        fRlcDump = fopen("/tmp/sw_dumprlc.bin", "wb");

    if(fRlcDump)
        fwrite(data, 1, size, fRlcDump);
}

/*------------------------------------------------------------------------------

    EncDumpRlcMb

------------------------------------------------------------------------------*/
void EncDumpRlcMb(const i16 * data)
{
    static u8 zeroTbl[512] = { 0 };

    if(fRlcMb == NULL)
        fRlcMb = fopen("sw_dumprlcmb.bin", "wb");

    /* Dump 1536 bytes (maximum RLC length for one macroblock)
     * from the beginning of each macroblock and pad with zero
     * so that each macroblock begins at 2k boundary */
    if(fRlcMb)
    {
        fwrite(data, 1, 1536, fRlcMb);
        fwrite(zeroTbl, 1, 512, fRlcMb);
    }
}

/*------------------------------------------------------------------------------

    EncDumpControl

------------------------------------------------------------------------------*/
void EncDumpControl(const u32 * data, u32 length)
{
    if(fControl == NULL)
        fControl = fopen("sw_dumpcontrol.bin", "wb");

    if(fControl)
        fwrite(data, 1, length, fControl);
}

u32 SwapEndian(u32 tmp)
{

    u32 swapped;

    swapped = tmp >> 24;
    swapped |= (tmp >> 8) & (0xFF00);
    swapped |= (tmp << 8) & (0xFF0000);
    swapped |= tmp << 24;

    return swapped;
}

/*------------------------------------------------------------------------------

    EncDumpRecon

------------------------------------------------------------------------------*/
void EncDumpRecon(asicData_s * asic)
{
    if(fRecon == NULL)
        fRecon = fopen("sw_recon_internal.yuv", "wb");

    if(fRecon)
    {
        u32 index;
        u32 size = asic->regs.mbsInCol * asic->regs.mbsInRow * 16 * 16;

        if(asic->regs.internalImageLumBaseW ==
           asic->internalImageLuma[0].busAddress)
            index = 0;
        else
            index = 1;

        {
            u32 *pTmp = asic->internalImageLuma[index].virtualAddress;

            fwrite(pTmp, 1, size, fRecon);
        }

        {
            u32 *pTmp = asic->internalImageChroma[index].virtualAddress;

            fwrite(pTmp, 1, size / 2, fRecon);
        }
    }
}

/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/
void EncTraceBufferingInit(hwSwBuffering_s * buff)
{
    if(!fTrace)
        fTrace = fopen("sw_buffering.trc", "w");

    if(!fTrace)
        return;

    fprintf(fTrace, "Buffer amount: %d\n", buff->bufferAmount);
    fprintf(fTrace, "MB per frame: %d\n", buff->mbPerFrame);
}

/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/
void EncTraceBuffering(hwSwBuffering_s * buff)
{
    u32 i;

    if(!fTrace)
        fTrace = fopen("sw_buffering.trc", "w");

    if(!fTrace)
        return;

    fprintf(fTrace, "\n");

    TraceTime(fTrace);

    fprintf(fTrace, "| State: ");
    switch (buff->state)
    {
    case IDLE:
        fprintf(fTrace, "IDLE");
        break;
    case HWON_SWOFF:
        fprintf(fTrace, "HWON_SWOFF");
        break;
    case HWON_SWON:
        fprintf(fTrace, "HWON_SWON");
        break;
    case HWOFF_SWON:
        fprintf(fTrace, "HWOFF_SWON");
        break;
    case DONE:
        fprintf(fTrace, "DONE");
        break;
    }

    fprintf(fTrace, "\n");
    fprintf(fTrace, PAD "| HW buffer: %d     last HW MB: %d\n",
            buff->hwBuffer, buff->hwMb);
    fprintf(fTrace, PAD "| SW buffer: %d     next SW MB: %d\n",
            buff->swBuffer, buff->swMb);
    fprintf(fTrace, PAD "| SW buffer position: %10p\n", buff->swBufferPos);

    for(i = 0; i < buff->bufferAmount; i++)
    {
        fprintf(fTrace,
                PAD
                "| Buffer %d virtual: %p    bus: 0x%08x    limit: %6d\n",
                i,
                buff->buffer[i].virtualAddress,
                buff->buffer[i].busAddress, buff->bufferLimit[i]);
        fprintf(fTrace,
                PAD "|         last: %p       full: %d\n",
                buff->bufferLast[i], buff->bufferFull[i]);
    }

    fprintf(fTrace, "\n");
}

/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/
void EncTraceBufferingSwEnd(hwSwBuffering_s * buff)
{
    if(!fTrace)
        fTrace = fopen("sw_buffering.trc", "w");

    if(!fTrace)
        return;

    TraceTime(fTrace);

    fprintf(fTrace, "SW reached end of buffer %d at %p\n",
            buff->swBuffer, buff->bufferLast[buff->swBuffer]);
}

/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/
void EncTraceBufferingHwEnd(hwSwBuffering_s * buff)
{
    if(!fTrace)
        fTrace = fopen("sw_buffering.trc", "w");

    if(!fTrace)
        return;

    TraceTime(fTrace);

    fprintf(fTrace, "HW reached end of buffer %d at %p\n",
            buff->hwBuffer, buff->bufferLast[buff->hwBuffer]);

}

/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/
void EncTraceBufferingAsicEnable(hwSwBuffering_s * buff)
{
    if(!fTrace)
        fTrace = fopen("sw_buffering.trc", "w");

    if(!fTrace)
        return;

    TraceTime(fTrace);

    fprintf(fTrace, "HW enable with buffer %d at %p\n",
            buff->hwBuffer, buff->buffer[buff->hwBuffer].virtualAddress);
}
