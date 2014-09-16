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
--  Abstract : H264 Encoder testbench for linux
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: H264TestBench.c,v $
--  $Revision: 1.16 $
--  $Date: 2010/04/19 09:19:02 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

/* For command line structure */
#include "H264TestBench.h"

/* For parameter parsing */
#include "EncGetOption.h"

/* For SW/HW shared memory allocation */
#include "ewl.h"

/* For accessing the EWL instance inside the encoder */
#include "H264Instance.h"

/* For compiler flags, test data, debug and tracing */
#include "enccommon.h"

/* For Hantro H.264 encoder */
#include "h264encapi.h"

#ifdef INTERNAL_TEST
#include "h264encapi_ext.h"
#endif

/* For printing and file IO */
#include <stdio.h>

/* For dynamic memory allocation */
#include <stdlib.h>

/* For memset, strcpy and strlen */
#include <string.h>

#ifdef USE_EFENCE
#include "efence.h"
#endif


/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------
              
NO_OUTPUT_WRITE: Output stream is not written to file. This should be used
                 when running performance simulations.
NO_INPUT_READ:   Input frames are not read from file. This should be used
                 when running performance simulations.

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define H264ERR_OUTPUT stdout

/* The amount of entries in the NAL unit size table, max one NALU / MB row */
#define NALU_TABLE_SIZE     ((1080/16) + 3)

/* Global variables */

static char input[] = "movie_176x144.yuv";//"input.yuv";
static char output[] = "stream.h264";
static char nal_sizes_file[] = "nal_sizes.txt";

static char *streamType[2] = { "BYTE_STREAM", "NAL_UNITS" };

static option_s option[] = {
    {"help", 'H'},
    {"firstVop", 'a', 1},
    {"lastVop", 'b', 1},
    {"width", 'x', 1},
    {"height", 'y', 1},
    {"lumWidthSrc", 'w', 1},
    {"lumHeightSrc", 'h', 1},
    {"horOffsetSrc", 'X', 1},
    {"verOffsetSrc", 'Y', 1},
    {"outputRateNumer", 'f', 1},
    {"outputRateDenom", 'F', 1},
    {"inputRateNumer", 'j', 1},
    {"inputRateDenom", 'J', 1},
    {"inputFormat", 'l', 1},    /* Input image format */
    {"colorConversion", 'O', 1},    /* RGB to YCbCr conversion type */
    {"input", 'i', 1},
    {"output", 'o', 1},
    {"videoRange", 'k', 1},
    {"rotation", 'r', 1},   /* Input image rotation */
    {"intraVopRate", 'I', 1},
    {"constIntraPred", 'T', 1},
    {"disableDeblocking", 'D', 1},
    {"filterOffsetA", 'W', 1},
    {"filterOffsetB", 'E', 1},

    {"mbPerSlice", 'V', 1},

    {"bitPerSecond", 'B', 1},
    {"vopRc", 'U', 1},
    {"mbRc", 'u', 1},
    {"vopSkip", 's', 1},    /* Frame skiping */
    {"qpMin", 'n', 1},  /* Minimum frame header qp */
    {"qpMax", 'm', 1},  /* Maximum frame header qp */
    {"qpHdr", 'q', 1},  /* Defaul qp */
    {"chromaQpOffset", 'Q', 1}, /* Chroma qp index offset */
    {"hrdConformance", 'C', 1}, /* HDR Conformance (ANNEX C) */
    {"cpbSize", 'c', 1},    /* Coded Picture Buffer Size */

    {"userData", 'z', 1},  /* SEI User data file */
    {"level", 'L', 1},  /* Level * 10  (ANNEX A) */
    {"byteStream", 'R', 1},     /* Byte stream format (ANNEX B) */
    {"sei", 'S', 1},    /* SEI messages */
    {"videostab", 'Z', 1},  /* video stabilization */

    {"testId", 'e', 1},
    {"burstSize", 'N', 1},
    {"burstType", 't', 1},
    {"trigger", 'P', 1},

    {"notCoded", '0', 1},

    {0, 0, 0}
};

/* SW/HW shared memories for input/output buffers */
static EWLLinearMem_t pictureMem;
static EWLLinearMem_t pictureStabMem;
static EWLLinearMem_t outbufMem;

static FILE *yuvFile = NULL;
static off_t file_size;

i32 trigger_point = -1;      /* Logic Analyzer trigger point */

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static int AllocRes(commandLine_s * cmdl, H264EncInst enc);
static void FreeRes(H264EncInst enc);
static int OpenEncoder(commandLine_s * cml, H264EncInst * encoder);
static i32 Encode(i32 argc, char **argv, H264EncInst inst, commandLine_s * cml);
static void CloseEncoder(H264EncInst encoder);
static i32 NextVop(i32 inputRateNumer, i32 inputRateDenom, i32 outputRateNumer,
                   i32 outputRateDenom, i32 frameCnt, i32 firstVop);
static int ReadVop(u8 * image, i32 size, i32 nro, char *name, i32 width,
                   i32 height, i32 format);
static u8* ReadUserData(H264EncInst encoder, char *name);
static int Parameter(i32 argc, char **argv, commandLine_s * ep);
static void Help(void);
static void WriteStrm(FILE * fout, u32 * outbuf, u32 size, u32 endian);
static int ChangeInput(i32 argc, char **argv, char **name, option_s * option);
static void PrintNalSizes(const u32 *pNaluSizeBuf, const u8 *pOutBuf, 
        u32 strmSize, i32 byteStream);

static void WriteNalSizesToFile(const char *file, const u32 * pNaluSizeBuf,
                                u32 buffSize);

/*------------------------------------------------------------------------------

    main

------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    H264EncInst encoder;
    commandLine_s cmdl;
    H264EncApiVersion apiVer;
    H264EncBuild encBuild;
    i32 ret;

    apiVer = H264EncGetApiVersion();
    encBuild = H264EncGetBuild();

    fprintf(stdout, "H.264 Encoder API version %d.%d\n", apiVer.major,
            apiVer.minor);
    fprintf(stdout, "HW ID: 0x%08x\t SW Build: %u.%u.%u\n\n",
            encBuild.hwBuild, encBuild.swBuild / 1000000,
            (encBuild.swBuild / 1000) % 1000, encBuild.swBuild % 1000);

    if(argc < 2)
    {
        Help();
        exit(0);
    }

    remove(nal_sizes_file);
    remove("stream.trc");

    /* Parse command line parameters */
    if(Parameter(argc, argv, &cmdl) != 0)
    {
        fprintf(H264ERR_OUTPUT, "Input parameter error\n");
        return -1;
    }

    /* Encoder initialization */
    if(OpenEncoder(&cmdl, &encoder) != 0)
    {
        return -1;
    }

    /* Set the test ID for internal testing,
     * the SW must be compiled with testing flags */
    H264EncSetTestId(encoder, cmdl.testId);

    /* Allocate input and output buffers */
    if(AllocRes(&cmdl, encoder) != 0)
    {
        fprintf(H264ERR_OUTPUT, "Failed to allocate the external resources!\n");
        FreeRes(encoder);
        CloseEncoder(encoder);
        return -1;
    }

    ret = Encode(argc, argv, encoder, &cmdl);

    FreeRes(encoder);

    CloseEncoder(encoder);

    return ret;
}

/*------------------------------------------------------------------------------

    Encode

    Do the encoding.

    Params:
        argc    - number of arguments to the application
        argv    - argument list as provided to the application
        encoder - encoder instance
        cml     - processed comand line options
    
    Return:
        0   - for success
        -1  - error

------------------------------------------------------------------------------*/
i32 Encode(i32 argc, char **argv, H264EncInst encoder, commandLine_s * cml)
{
    H264EncIn encIn;
    H264EncOut encOut;
    H264EncRet ret;
    H264EncRateCtrl rc;
    int intraPeriodCnt = 0, codedFrameCnt = 0, next = 0, src_img_size;
    int nextNotCoded = 0;
    u32 frameCnt = 0;
    u32 streamSize = 0;
    u32 bitrate = 0;
    FILE *fout = NULL;
    u8 *pUserData;

    encIn.pNaluSizeBuf = NULL;
    encIn.naluSizeBufSize = 0;

    encIn.pOutBuf = outbufMem.virtualAddress;
    encIn.busOutBuf = outbufMem.busAddress;
    encIn.outBufSize = outbufMem.size;

    /* Allocate memory for NAL unit size buffer, optional */

    encIn.naluSizeBufSize = NALU_TABLE_SIZE * sizeof(u32);
    encIn.pNaluSizeBuf = (u32 *) malloc(encIn.naluSizeBufSize);

    if(!encIn.pNaluSizeBuf)
    {
        fprintf(H264ERR_OUTPUT,
                "WARNING! Failed to allocate NAL unit size buffer.\n");
    }

    /* Source Image Size */
    if(cml->inputFormat <= 1)
    {
        src_img_size = cml->lumWidthSrc * cml->lumHeightSrc +
            2 * (((cml->lumWidthSrc + 1) >> 1) *
                 ((cml->lumHeightSrc + 1) >> 1));
    }
    else
    {
        src_img_size = cml->lumWidthSrc * cml->lumHeightSrc * 2;
    }

    printf("Reading input from file <%s>, frame size %d bytes.\n",
            cml->input, src_img_size);


    /* Start stream */
    ret = H264EncStrmStart(encoder, &encIn, &encOut);
    if(ret != H264ENC_OK)
    {
        fprintf(H264ERR_OUTPUT, "Failed to start the stream. Error code: %8i\n",
                ret);
        return -1;
    }

    fout = fopen(cml->output, "wb");
    if(fout == NULL)
    {
        fprintf(H264ERR_OUTPUT, "Failed to create the output file.\n");
        return -1;
    }

    WriteStrm(fout, outbufMem.virtualAddress, encOut.streamSize, 0);
    if(cml->byteStream == 0)
    {
        WriteNalSizesToFile(nal_sizes_file, encIn.pNaluSizeBuf,
                            encIn.naluSizeBufSize);
    }

    streamSize += encOut.streamSize;

    H264EncGetRateCtrl(encoder, &rc);

    /* Allocate a buffer for user data and read data from file */
    pUserData = ReadUserData(encoder, cml->userData);

    printf("\n");
    printf("Input | Pic | QP | Type |  BR avg  | ByteCnt (inst) | NALU sizes\n");
    printf("-----------------------------------------------------------------\n");
    printf("      |     | %2d | HDR  |          | %7i %6i | ",
            rc.qpHdr, streamSize, encOut.streamSize);
    PrintNalSizes(encIn.pNaluSizeBuf, (u8 *) outbufMem.virtualAddress,
            encOut.streamSize, cml->byteStream);
    printf("\n");

    /* Setup encoder input */
    {
        u32 w = (cml->lumWidthSrc + 15) & (~0x0f);

        encIn.busLuma = pictureMem.busAddress;

        encIn.busChromaU = encIn.busLuma + (w * cml->lumHeightSrc);
        encIn.busChromaV = encIn.busChromaU +
            (((w + 1) >> 1) * ((cml->lumHeightSrc + 1) >> 1));
    }

    /* First frame is always intra with time increment = 0 */
    encIn.codingType = H264ENC_INTRA_FRAME;
    encIn.timeIncrement = 0;

    encIn.busLumaStab = pictureStabMem.busAddress;

    intraPeriodCnt = cml->intraVopRate;
    nextNotCoded = cml->notCoded;
    /* Main encoding loop */
  nextinput:
    while((next = NextVop(cml->inputRateNumer, cml->inputRateDenom,
                          cml->outputRateNumer, cml->outputRateDenom, frameCnt,
                          cml->firstVop)) <= cml->lastVop)
    {
#ifndef NO_INPUT_READ
        /* Read next frame */
        if(ReadVop((u8 *) pictureMem.virtualAddress, 
                    src_img_size, next, cml->input,
                   cml->lumWidthSrc, cml->lumHeightSrc, cml->inputFormat) != 0)
            break;

        if(cml->videoStab > 0)
        {
            /* Stabilize the frame after current frame */
            i32 nextStab = NextVop(cml->inputRateNumer, cml->inputRateDenom,
                          cml->outputRateNumer, cml->outputRateDenom, frameCnt+1,
                          cml->firstVop);

            if(ReadVop((u8 *) pictureStabMem.virtualAddress, 
                        src_img_size, nextStab, cml->input,
                       cml->lumWidthSrc, cml->lumHeightSrc,
                       cml->inputFormat) != 0)
                break;
        }
#endif

        /* Select frame type */
        if((cml->intraVopRate != 0) && (intraPeriodCnt >= cml->intraVopRate))
            encIn.codingType = H264ENC_INTRA_FRAME;
        else
            encIn.codingType = H264ENC_PREDICTED_FRAME;

        if(encIn.codingType == H264ENC_INTRA_FRAME)
            intraPeriodCnt = 0;

        if(nextNotCoded && nextNotCoded == frameCnt)
        {
            encIn.codingType = H264ENC_NOTCODED_FRAME;
            nextNotCoded += cml->notCodedInterval;
        }

        ret = H264EncStrmEncode(encoder, &encIn, &encOut);

        H264EncGetRateCtrl(encoder, &rc);

        streamSize += encOut.streamSize;

        /* Note: This will overflow if large output rate numerator is used */
        if((frameCnt+1) && cml->outputRateDenom)
            bitrate = 8 * ((streamSize/(frameCnt+1)) * 
                    (u32) cml->outputRateNumer / (u32) cml->outputRateDenom);

        switch (ret)
        {
        case H264ENC_FRAME_READY:

            printf("%5i | %3i | %2i | %s | %8u | %7i %6i | ", 
                next, frameCnt, rc.qpHdr, 
                encOut.codingType == H264ENC_INTRA_FRAME ? " I  " :
                encOut.codingType == H264ENC_PREDICTED_FRAME ? " P  " : "skip",
                bitrate, streamSize, encOut.streamSize);
            PrintNalSizes(encIn.pNaluSizeBuf, (u8 *) outbufMem.virtualAddress,
                encOut.streamSize, cml->byteStream);
            printf("\n");

            WriteStrm(fout, outbufMem.virtualAddress, encOut.streamSize, 0);

            if(cml->byteStream == 0)
            {
                WriteNalSizesToFile(nal_sizes_file, encIn.pNaluSizeBuf,
                                    encIn.naluSizeBufSize);
            }

            if (pUserData)
            {
                /* We want the user data to be written only once so
                 * we disable the user data and free the memory after
                 * first frame has been encoded. */
                H264EncSetSeiUserData(encoder, NULL, 0);
                free(pUserData);
                pUserData = NULL;
            }

            break;

        case H264ENC_OUTPUT_BUFFER_OVERFLOW:
            printf("%5i | %3i | %2i | %s | %8u | %7i %6i | \n", 
                next, frameCnt, rc.qpHdr, "lost",
                bitrate, streamSize, encOut.streamSize);
            break;

        default:
            printf("FAILED. Error code: %i\n", ret);
            /* For debugging, can be removed */
            WriteStrm(fout, outbufMem.virtualAddress, encOut.streamSize, 0);
            /* We try to continue encoding the next frame */
            break;
        }

        encIn.timeIncrement = cml->outputRateDenom;

        frameCnt++;

        if (encOut.codingType != H264ENC_NOTCODED_FRAME) {
            intraPeriodCnt++; codedFrameCnt++;
        }

    }   /* End of main encoding loop */

    /* Change next input sequence */
    if(ChangeInput(argc, argv, &cml->input, option))
    {
        if(yuvFile != NULL)
        {
            fclose(yuvFile);
            yuvFile = NULL;
        }

        frameCnt = 0;
        goto nextinput;
    }

    /* End stream */
    ret = H264EncStrmEnd(encoder, &encIn, &encOut);
    if(ret != H264ENC_OK)
    {
        fprintf(H264ERR_OUTPUT, "Failed to end the stream. Error code: %8i\n",
                ret);
    }
    else
    {
        streamSize += encOut.streamSize;
        printf("      |     |    | END  |          | %7i %6i | ", 
                streamSize, encOut.streamSize);
        PrintNalSizes(encIn.pNaluSizeBuf, (u8 *) outbufMem.virtualAddress,
                encOut.streamSize, cml->byteStream);
        printf("\n");

        WriteStrm(fout, outbufMem.virtualAddress, encOut.streamSize, 0);

        if(cml->byteStream == 0)
        {
            WriteNalSizesToFile(nal_sizes_file, encIn.pNaluSizeBuf,
                                encIn.naluSizeBufSize);
        }
    }

    printf("\nBitrate target %d bps, actual %d bps (%d%%).\n",
            rc.bitPerSecond, bitrate,
            (rc.bitPerSecond) ? bitrate*100/rc.bitPerSecond : 0);
    printf("Total of %d frames processed, %d frames encoded, %d bytes.\n",
            frameCnt, codedFrameCnt, streamSize);


    /* Free all resources */
    if(fout != NULL)
        fclose(fout);

    if(yuvFile != NULL)
        fclose(yuvFile);

    if(encIn.pNaluSizeBuf != NULL)
        free(encIn.pNaluSizeBuf);

    return 0;
}

/*------------------------------------------------------------------------------

    AllocRes

    Allocation of the physical memories used by both SW and HW: 
    the input pictures and the output stream buffer.

    NOTE! The implementation uses the EWL instance from the encoder
          for OS independence. This is not recommended in final environment 
          because the encoder will release the EWL instance in case of error.
          Instead, the memories should be allocated from the OS the same way
          as inside EWLMallocLinear().

------------------------------------------------------------------------------*/
int AllocRes(commandLine_s * cmdl, H264EncInst enc)
{
    i32 ret;
    u32 pictureSize;
    u32 outbufSize;


    if(cmdl->inputFormat <= 1)
    {
        /* Input picture in planar YUV 4:2:0 format */
        pictureSize =
            ((cmdl->lumWidthSrc + 15) & (~15)) * cmdl->lumHeightSrc * 3 / 2;
    }
    else
    {
        /* Input picture in YUYV 4:2:2 format */
        pictureSize =
            ((cmdl->lumWidthSrc + 15) & (~15)) * cmdl->lumHeightSrc * 2;
    }

    printf("Input %dx%d encoding at %dx%d\n", cmdl->lumWidthSrc,
           cmdl->lumHeightSrc, cmdl->width, cmdl->height);

    /* Here we use the EWL instance directly from the encoder 
     * because it is the easiest way to allocate the linear memories */
    ret = EWLMallocLinear(((h264Instance_s *)enc)->asic.ewl, pictureSize, 
                &pictureMem);
    if (ret != EWL_OK)
        return 1;
    
   
    if(cmdl->videoStab > 0) {
        ret = EWLMallocLinear(((h264Instance_s *)enc)->asic.ewl, pictureSize, 
                &pictureStabMem);
        if (ret != EWL_OK)
            return 1;
    }

    outbufSize = 4 * pictureMem.size < (1024 * 1024 * 3 / 2) ?
        4 * pictureMem.size : (1024 * 1024 * 3 / 2);

    ret = EWLMallocLinear(((h264Instance_s *)enc)->asic.ewl, outbufSize, 
                &outbufMem);
    if (ret != EWL_OK)
        return 1;

    printf("Input buffer size:          %d bytes\n", pictureMem.size);
    printf("Input buffer bus address:   0x%08x\n", pictureMem.busAddress);
    printf("Input buffer user address:  0x%08x\n", (u32) pictureMem.virtualAddress);
    printf("Output buffer size:         %d bytes\n", outbufMem.size);
    printf("Output buffer bus address:  0x%08x\n", outbufMem.busAddress);
    printf("Output buffer user address: 0x%08x\n", (u32) outbufMem.virtualAddress);

    return 0;
}

/*------------------------------------------------------------------------------

    FreeRes

    Release all resources allcoated byt AllocRes()

------------------------------------------------------------------------------*/
void FreeRes(H264EncInst enc)
{
    EWLFreeLinear(((h264Instance_s *)enc)->asic.ewl, &pictureMem); 
    EWLFreeLinear(((h264Instance_s *)enc)->asic.ewl, &pictureStabMem); 
    EWLFreeLinear(((h264Instance_s *)enc)->asic.ewl, &outbufMem); 
}

/*------------------------------------------------------------------------------

    OpenEncoder
        Create and configure an encoder instance.

    Params:
        cml     - processed comand line options
        pEnc    - place where to save the new encoder instance    
    Return:
        0   - for success
        -1  - error

------------------------------------------------------------------------------*/
int OpenEncoder(commandLine_s * cml, H264EncInst * pEnc)
{
    H264EncRet ret;
    H264EncConfig cfg;
    H264EncCodingCtrl codingCfg;
    H264EncRateCtrl rcCfg;
    H264EncPreProcessingCfg preProcCfg;

    H264EncInst encoder;

    /* Encoder initialization */
    if(cml->width == DEFAULT)
        cml->width = cml->lumWidthSrc;

    if(cml->height == DEFAULT)
        cml->height = cml->lumHeightSrc;

    /* outputRateNumer */
    if(cml->outputRateNumer == DEFAULT)
    {
        cml->outputRateNumer = cml->inputRateNumer;
    }

    /* outputRateDenom */
    if(cml->outputRateDenom == DEFAULT)
    {
        cml->outputRateDenom = cml->inputRateDenom;
    }

    if(cml->rotation)
    {
        cfg.width = cml->height;
        cfg.height = cml->width;
    }
    else
    {
        cfg.width = cml->width;
        cfg.height = cml->height;
    }

    cfg.frameRateDenom = cml->outputRateDenom;
    cfg.frameRateNum = cml->outputRateNumer;
    if(cml->byteStream)
        cfg.streamType = H264ENC_BYTE_STREAM;
    else
        cfg.streamType = H264ENC_NAL_UNIT_STREAM;

    cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_3;

    if(cml->level != DEFAULT && cml->level != 0)
        cfg.profileAndLevel = (H264EncProfileAndLevel)cml->level;
        

    printf
        ("Init config: size %dx%d   %d/%d fps  %s P&L %d\n",
         cfg.width, cfg.height, cfg.frameRateNum,
         cfg.frameRateDenom, streamType[cfg.streamType], cfg.profileAndLevel);

    if((ret = H264EncInit(&cfg, pEnc)) != H264ENC_OK)
    {
        fprintf(H264ERR_OUTPUT,
                "Failed to initialize the encoder. Error code: %8i\n", ret);
        return -1;
    }

    encoder = *pEnc;

    /* Encoder setup: rate control */
    if((ret = H264EncGetRateCtrl(encoder, &rcCfg)) != H264ENC_OK)
    {
        fprintf(H264ERR_OUTPUT, "Failed to get RC info. Error code: %8i\n",
                ret);
        CloseEncoder(encoder);
        return -1;
    }
    else
    {
        printf("Get rate control: qp %2d [%2d, %2d]  %8d bps  "
               "pic %d mb %d skip %d  hrd %d\n  cpbSize %d gopLen %d\n",
               rcCfg.qpHdr, rcCfg.qpMin, rcCfg.qpMax, rcCfg.bitPerSecond,
               rcCfg.pictureRc, rcCfg.mbRc, rcCfg.pictureSkip, rcCfg.hrd,
               rcCfg.hrdCpbSize, rcCfg.gopLen);

        if(cml->qpHdr != DEFAULT)
            rcCfg.qpHdr = cml->qpHdr;
        if(cml->qpMin != DEFAULT)
            rcCfg.qpMin = cml->qpMin;
        if(cml->qpMax != DEFAULT)
            rcCfg.qpMax = cml->qpMax;
        if(cml->vopSkip != DEFAULT)
            rcCfg.pictureSkip = cml->vopSkip;
        if(cml->vopRc != DEFAULT)
            rcCfg.pictureRc = cml->vopRc;
        if(cml->mbRc != DEFAULT)
            rcCfg.mbRc = cml->mbRc != 0 ? 1 : 0;
        if(cml->bitPerSecond != DEFAULT)
            rcCfg.bitPerSecond = cml->bitPerSecond;

        if(cml->hrdConformance != DEFAULT)
            rcCfg.hrd = cml->hrdConformance;

        if(cml->cpbSize != DEFAULT)
            rcCfg.hrdCpbSize = cml->cpbSize;

        if(cml->intraVopRate != 0)
            rcCfg.gopLen = cml->intraVopRate;
        
        printf("Set rate control: qp %2d [%2d, %2d]  %8d bps  "
               "pic %d mb %d skip %d  hrd %d\n  cpbSize %d gopLen %d\n",
               rcCfg.qpHdr, rcCfg.qpMin, rcCfg.qpMax, rcCfg.bitPerSecond,
               rcCfg.pictureRc, rcCfg.mbRc, rcCfg.pictureSkip, rcCfg.hrd,
               rcCfg.hrdCpbSize, rcCfg.gopLen);

        if((ret = H264EncSetRateCtrl(encoder, &rcCfg)) != H264ENC_OK)
        {
            fprintf(H264ERR_OUTPUT, "Failed to set RC info. Error code: %8i\n",
                    ret);
            CloseEncoder(encoder);
            return -1;
        }
    }

    /* Encoder setup: coding control */
    if((ret = H264EncGetCodingCtrl(encoder, &codingCfg)) != H264ENC_OK)
    {
        fprintf(H264ERR_OUTPUT, "Failed to get CODING info. Error code: %8i\n",
                ret);
        CloseEncoder(encoder);
        return -1;
    }
    else
    {
        if(cml->mbPerSlice != DEFAULT)
        {
            codingCfg.sliceSize = cml->mbPerSlice / (cfg.width / 16);
        }

        if(cml->constIntraPred != DEFAULT)
        {
            if(cml->constIntraPred != 0)
                codingCfg.constrainedIntraPrediction = 1;
            else
                codingCfg.constrainedIntraPrediction = 0;
        }

        if(cml->disableDeblocking != 0)
            codingCfg.disableDeblockingFilter = 1;
        else
            codingCfg.disableDeblockingFilter = 0;

        if((cml->disableDeblocking != 1) &&
           ((cml->filterOffsetA != 0) || (cml->filterOffsetB != 0)))
            codingCfg.disableDeblockingFilter = 1;

        if(cml->videoRange != DEFAULT)
        {
            if(cml->videoRange != 0)
                codingCfg.videoFullRange = 1;
            else
                codingCfg.videoFullRange = 0;
        }

        if(cml->sei)
            codingCfg.seiMessages = 1;
        else
            codingCfg.seiMessages = 0;

        printf
            ("Set coding control: SEI %d  Slice %5d  deblocking %d "
             "constIntra %d  videoRange %d\n",
             codingCfg.seiMessages, codingCfg.sliceSize,
             codingCfg.disableDeblockingFilter,
             codingCfg.constrainedIntraPrediction, codingCfg.videoFullRange);

        if((ret = H264EncSetCodingCtrl(encoder, &codingCfg)) != H264ENC_OK)
        {
            fprintf(H264ERR_OUTPUT,
                    "Failed to set CODING info. Error code: %8i\n", ret);
            CloseEncoder(encoder);
            return -1;
        }

#ifdef INTERNAL_TEST
        /* Set some values outside the product API for internal
         * testing purposes */

        H264EncSetChromaQpIndexOffset(encoder, cml->chromaQpOffset);
        printf("Set ChromaQpIndexOffset: %d\n", cml->chromaQpOffset);

        H264EncSetHwBurstSize(encoder, cml->burst);
        printf("Set HW Burst Size: %d\n", cml->burst);
        H264EncSetHwBurstType(encoder, cml->bursttype);
        printf("Set HW Burst Type: %d\n", cml->bursttype);
        if(codingCfg.disableDeblockingFilter == 1)
        {
            H264EncFilter advCoding;

            H264EncGetFilter(encoder, &advCoding);

            advCoding.disableDeblocking = cml->disableDeblocking;

            advCoding.filterOffsetA = cml->filterOffsetA * 2;
            advCoding.filterOffsetB = cml->filterOffsetB * 2;

            printf
                ("Set filter params: disableDeblocking %d filterOffsetA = %i filterOffsetB = %i\n",
                 advCoding.disableDeblocking, advCoding.filterOffsetA,
                 advCoding.filterOffsetB);

            ret = H264EncSetFilter(encoder, &advCoding);
            if(ret != H264ENC_OK)
            {
                fprintf(H264ERR_OUTPUT,
                        "Failed to set FILTER info. Error code: %8i\n", ret);
                CloseEncoder(encoder);
                return -1;
            }
        }
#endif

    }

    /* PreP setup */
    if((ret = H264EncGetPreProcessing(encoder, &preProcCfg)) != H264ENC_OK)
    {
        fprintf(H264ERR_OUTPUT, "Failed to get PreP info. Error code: %8i\n",
                ret);
        CloseEncoder(encoder);
        return -1;
    }
    printf
        ("Get PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d "
           ": stab %d : cc %d\n",
         preProcCfg.origWidth, preProcCfg.origHeight, preProcCfg.xOffset,
         preProcCfg.yOffset, preProcCfg.inputType, preProcCfg.rotation,
         preProcCfg.videoStabilization, preProcCfg.colorConversion.type);

    preProcCfg.inputType = (H264EncPictureType)cml->inputFormat;
    preProcCfg.rotation = (H264EncPictureRotation)cml->rotation;

    preProcCfg.origWidth =
        cml->lumWidthSrc /*(cml->lumWidthSrc + 15) & (~0x0F) */ ;
    preProcCfg.origHeight = cml->lumHeightSrc;

    if(cml->horOffsetSrc != DEFAULT)
        preProcCfg.xOffset = cml->horOffsetSrc;
    if(cml->verOffsetSrc != DEFAULT)
        preProcCfg.yOffset = cml->verOffsetSrc;
    if(cml->videoStab != DEFAULT)
        preProcCfg.videoStabilization = cml->videoStab;
    if(cml->colorConversion != DEFAULT)
        preProcCfg.colorConversion.type = 
                        (H264EncColorConversionType)cml->colorConversion;
    if(preProcCfg.colorConversion.type == H264ENC_RGBTOYUV_USER_DEFINED)
    {
        preProcCfg.colorConversion.coeffA = 20000;
        preProcCfg.colorConversion.coeffB = 44000;
        preProcCfg.colorConversion.coeffC = 5000;
        preProcCfg.colorConversion.coeffE = 35000;
        preProcCfg.colorConversion.coeffF = 38000;
    }

    printf
        ("Set PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d "
           ": stab %d : cc %d\n",
         preProcCfg.origWidth, preProcCfg.origHeight, preProcCfg.xOffset,
         preProcCfg.yOffset, preProcCfg.inputType, preProcCfg.rotation,
         preProcCfg.videoStabilization, preProcCfg.colorConversion.type);

    if((ret = H264EncSetPreProcessing(encoder, &preProcCfg)) != H264ENC_OK)
    {
        fprintf(H264ERR_OUTPUT, "Failed to set PreP info. Error code: %8i\n",
                ret);
        CloseEncoder(encoder);
        return -1;
    }
    return 0;
}

/*------------------------------------------------------------------------------

    CloseEncoder
       Release an encoder insatnce.

   Params:
        encoder - the instance to be released
------------------------------------------------------------------------------*/
void CloseEncoder(H264EncInst encoder)
{
    H264EncRet ret;

    if((ret = H264EncRelease(encoder)) != H264ENC_OK)
    {
        fprintf(H264ERR_OUTPUT,
                "Failed to release the encoder. Error code: %8i\n", ret);
    }
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int ParseDelim(char *optArg, char delim)
{
    i32 i;

    for (i = 0; i < (i32)strlen(optArg); i++)
        if (optArg[i] == delim)
        {
            optArg[i] = 0;
            return i;
        }

    return -1;
}

/*------------------------------------------------------------------------------

    Parameter
        Process the testbench calling arguments.

    Params:
        argc    - number of arguments to the application
        argv    - argument list as provided to the application
        cml     - processed comand line options   
    Return:
        0   - for success
        -1  - error    

------------------------------------------------------------------------------*/
int Parameter(i32 argc, char **argv, commandLine_s * cml)
{
    i32 ret;
    char *optArg;
    argument_s argument;
    i32 status = 0;

    memset(cml, 0, sizeof(commandLine_s));

    cml->width = DEFAULT;
    cml->height = DEFAULT;
    cml->outputRateNumer = DEFAULT;
    cml->outputRateDenom = DEFAULT;
    cml->qpHdr = DEFAULT;
    cml->qpMin = DEFAULT;
    cml->qpMax = DEFAULT;
    cml->vopRc = DEFAULT;
    cml->mbRc = DEFAULT;
    cml->cpbSize = DEFAULT;
    cml->constIntraPred = DEFAULT;
    cml->horOffsetSrc = DEFAULT;
    cml->verOffsetSrc = DEFAULT;
    cml->videoStab = DEFAULT;

    cml->input = input;
    cml->output = output;
    cml->firstVop = 0;
    cml->lastVop = 100;
    cml->inputRateNumer = 30;
    cml->inputRateDenom = 1;

    cml->lumWidthSrc = 176;
    cml->lumHeightSrc = 144;
    cml->rotation = 0;
    cml->inputFormat = 0;

    cml->bitPerSecond = 64000;

    cml->chromaQpOffset = 0;
    cml->disableDeblocking = 0;
    cml->filterOffsetA = 0;
    cml->filterOffsetB = 0;
    cml->burst = 16;
    cml->bursttype = 0;
    cml->testId = 0;

    cml->sei = 0;
    cml->byteStream = 1;
    cml->hrdConformance = 0;
    cml->videoRange = 0;
    cml->vopSkip = 0;

    argument.optCnt = 1;
    while((ret = EncGetOption(argc, argv, option, &argument)) != -1)
    {
        if(ret == -2)
        {
            status = 1;
        }
        optArg = argument.optArg;
        switch (argument.shortOpt)
        {
        case 'i':
            cml->input = optArg;
            break;
        case 'o':
            cml->output = optArg;
            break;
        case 'z':
            cml->userData = optArg;
            break;
        case 'a':
            cml->firstVop = atoi(optArg);
            break;
        case 'b':
            cml->lastVop = atoi(optArg);
            break;
        case 'x':
            cml->width = atoi(optArg);
            break;
        case 'y':
            cml->height = atoi(optArg);
            break;
        case 'w':
            cml->lumWidthSrc = atoi(optArg);
            break;
        case 'h':
            cml->lumHeightSrc = atoi(optArg);
            break;
        case 'X':
            cml->horOffsetSrc = atoi(optArg);
            break;
        case 'Y':
            cml->verOffsetSrc = atoi(optArg);
            break;
        case 'f':
            cml->outputRateNumer = atoi(optArg);
            break;
        case 'F':
            cml->outputRateDenom = atoi(optArg);
            break;
        case 'j':
            cml->inputRateNumer = atoi(optArg);
            break;
        case 'J':
            cml->inputRateDenom = atoi(optArg);
            break;
        case 'T':
            cml->constIntraPred = atoi(optArg);
            break;
        case 'D':
            cml->disableDeblocking = atoi(optArg);
            break;
        case 'W':
            cml->filterOffsetA = atoi(optArg);
            break;
        case 'E':
            cml->filterOffsetB = atoi(optArg);
            break;
        case 'I':
            cml->intraVopRate = atoi(optArg);
            break;
        case 'N':
            cml->burst = atoi(optArg);
            break;
        case 't':
            cml->bursttype = atoi(optArg);
            break;
        case 'q':
            cml->qpHdr = atoi(optArg);
            break;
        case 'n':
            cml->qpMin = atoi(optArg);
            break;
        case 'm':
            cml->qpMax = atoi(optArg);
            break;
        case 'B':
            cml->bitPerSecond = atoi(optArg);
            break;
        case 'U':
            cml->vopRc = atoi(optArg);
            break;
        case 'u':
            cml->mbRc = atoi(optArg);
            break;
        case 's':
            cml->vopSkip = atoi(optArg);
            break;
        case 'r':
            cml->rotation = atoi(optArg);
            break;
        case 'l':
            cml->inputFormat = atoi(optArg);
            break;
        case 'O':
            cml->colorConversion = atoi(optArg);
            break;
        case 'Q':
            cml->chromaQpOffset = atoi(optArg);
            break;
        case 'V':
            cml->mbPerSlice = atoi(optArg);
            break;
        case 'k':
            cml->videoRange = atoi(optArg);
            break;
        case 'L':
            cml->level = atoi(optArg);
            break;
        case 'C':
            cml->hrdConformance = atoi(optArg);
            break;
        case 'c':
            cml->cpbSize = atoi(optArg);
            break;
        case 'e':
            cml->testId = atoi(optArg);
            break;
        case 'S':
            cml->sei = atoi(optArg);
            break;
        case 'P':
            trigger_point = atoi(optArg);
            break;
        case 'R':
            cml->byteStream = atoi(optArg);
            break;
        case 'Z':
            cml->videoStab = atoi(optArg);
            break;
        case '0':
            {
                int i;
                /* Argument must be "xx:yy", replace ':' with 0 */
                if ((i = ParseDelim(optArg, ':')) == -1) break;
                /* xx is first not coded picture */
                cml->notCoded = atoi(optArg);
                /* yy is not coded picture interval */
                cml->notCodedInterval = atoi(optArg+i+1);
            }
            break;
        default:
            break;
        }
    }

    return status;
}

/*------------------------------------------------------------------------------

    ReadVop

    Read raw YUV 4:2:0 or 4:2:2 image data from file

    Params:
        image   - buffer where the image will be saved
        size    - amount of image data to be read
        nro     - picture number to be read
        name    - name of the file to read
        width   - image width in pixels
        height  - image height in pixels
        format  - image format (YUV 420 planar or semiplanar, or YUV 422)

    Returns:
        0 - for success
        non-zero error code 
------------------------------------------------------------------------------*/
int ReadVop(u8 * image, i32 size, i32 nro, char *name, i32 width, i32 height,
            i32 format)
{
    int ret = 0;

    if(yuvFile == NULL)
    {
        yuvFile = fopen(name, "rb");

        if(yuvFile == NULL)
        {
            fprintf(H264ERR_OUTPUT, "\nUnable to open YUV file: %s\n", name);
            ret = -1;
            goto end;
        }

        fseeko(yuvFile, 0, SEEK_END);
        file_size = ftello(yuvFile);
    }

    /* Stop if over last frame of the file */
    if((off_t)size * (nro + 1) > file_size)
    {
        printf("\nCan't read frame, EOF\n");
        ret = -1;
        goto end;
    }

    if(fseeko(yuvFile, (off_t)size * nro, SEEK_SET) != 0)
    {
        fprintf(H264ERR_OUTPUT, "\nI can't seek frame no: %i from file: %s\n",
                nro, name);
        ret = -1;
        goto end;
    }

    if((width & 0x0f) == 0)
        fread(image, 1, size, yuvFile);
    else
    {
        i32 i;
        u8 *buf = image;
        i32 scan = (width + 15) & (~0x0f);

        /* Read the frame so that scan (=stride) is multiple of 16 pixels */

        if(format == 0) /* YUV 4:2:0 planar */
        {
            /* Y */
            for(i = 0; i < height; i++)
            {
                fread(buf, 1, width, yuvFile);
                buf += scan;
            }
            /* Cb */
            for(i = 0; i < (height / 2); i++)
            {
                fread(buf, 1, width / 2, yuvFile);
                buf += scan / 2;
            }
            /* Cr */
            for(i = 0; i < (height / 2); i++)
            {
                fread(buf, 1, width / 2, yuvFile);
                buf += scan / 2;
            }
        }
        else if(format == 1)    /* YUV 4:2:0 semiplanar */
        {
            /* Y */
            for(i = 0; i < height; i++)
            {
                fread(buf, 1, width, yuvFile);
                buf += scan;
            }
            /* CbCr */
            for(i = 0; i < (height / 2); i++)
            {
                fread(buf, 1, width, yuvFile);
                buf += scan;
            }
        }
        else    /* YUV 4:2:2 interleaved */
        {
            for(i = 0; i < height; i++)
            {
                fread(buf, 1, width * 2, yuvFile);
                buf += scan * 2;
            }
        }

    }

  end:

    return ret;
}

/*------------------------------------------------------------------------------

    ReadUserData
        Read user data from file and pass to encoder

    Params:
        name - name of file in which user data is located

    Returns:
        NULL - when user data reading failed
        pointer - allocated buffer containing user data

------------------------------------------------------------------------------*/
u8* ReadUserData(H264EncInst encoder, char *name)
{
    FILE *file = NULL;
    i32 byteCnt;
    u8 *data;

    if(name == NULL)
    {
        return NULL;
    }

    /* Get user data length from file */
    file = fopen(name, "rb");
    if(file == NULL)
    {
        fprintf(H264ERR_OUTPUT, "Unable to open User Data file: %s\n", name);
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    byteCnt = ftell(file);
    rewind(file);

    /* Minimum size of user data */
    if (byteCnt < 16)
        byteCnt = 16;

    /* Maximum size of user data */
    if (byteCnt > 2048)
        byteCnt = 2048;

    /* Allocate memory for user data */
    if((data = (u8 *) malloc(sizeof(u8) * byteCnt)) == NULL)
    {
        fclose(file);
        fprintf(H264ERR_OUTPUT, "Unable to alloc User Data memory\n");
        return NULL;
    }

    /* Read user data from file */
    fread(data, sizeof(u8), byteCnt, file);
    fclose(file);

    printf("User data: %d bytes [%d %d %d %d ...]\n",
        byteCnt, data[0], data[1], data[2], data[3]);

    /* Pass the data buffer to encoder
     * The encoder reads the buffer during following H264EncStrmEncode() calls.
     * User data writing must be disabled (with SetSeiUserData(enc, 0, 0)) */
    H264EncSetSeiUserData(encoder, data, byteCnt);

    return data;
}

/*------------------------------------------------------------------------------

    Help
        Print out some instructions about usage.
------------------------------------------------------------------------------*/
void Help(void)
{
    fprintf(stdout, "Usage:  %s [options] -i inputfile\n\n", "h264_testenc");

    fprintf(stdout,
            "  -i[s] --input             Read input from file. [input.yuv]\n"
            "  -o[s] --output            Write output to file. [stream.h264]\n"
            "  -a[n] --firstVop          First vop of input file. [0]\n"
            "  -b[n] --lastVop           Last vop of input file. [100]\n"
            "  -w[n] --lumWidthSrc       Width of source image. [176]\n"
            "  -h[n] --lumHeightSrc      Height of source image. [144]\n");
    fprintf(stdout,
            "  -x[n] --width             Width of output image. [--lumWidthSrc]\n"
            "  -y[n] --height            Height of output image. [--lumHeightSrc]\n"
            "  -X[n] --horOffsetSrc      Output image horizontal offset. [0]\n"
            "  -Y[n] --verOffsetSrc      Output image vertical offset. [0]\n");
    fprintf(stdout,
            "  -f[n] --outputRateNumer   1..65535 Output vop rate numerator. [30]\n"
            "  -F[n] --outputRateDenom   1..65535 Output vop rate denominator. [1]\n"
            "  -j[n] --inputRateNumer    1..65535 Input vop rate numerator. [30]\n"
            "  -J[n] --inputRateDenom    1..65535 Input vop rate denominator. [1]\n");

    fprintf(stdout, "\n  -L[n] --level             10..32, H264 Level. [30]\n");
    fprintf(stdout, "\n  -R[n] --byteStream        Stream type. [1]\n"
            "                            1 - byte stream according to Annex B.\n"
            "                            0 - NAL units. Nal sizes returned in <nal_sizes.txt>\n");
    fprintf(stdout,
            "\n  -S[n] --sei               Enable/Disable SEI messages.[0]\n");

    fprintf(stdout,
            "\n  -r[n] --rotation          Rotate input image. [0]\n"
            "                                0 - disabled\n"
            "                                1 - 90 degrees right\n"
            "                                2 - 90 degrees right\n"
            "  -l[n] --inputFormat       Input YUV format. [0]\n"
            "                                0 - YUV420\n"
            "                                1 - YUV420 semiplanar\n"
            "                                2 - YUYV422\n"
            "                                3 - UYVY422\n"
            "                                4 - RGB565\n"
            "                                5 - BGR565\n"
            "                                6 - RGB555\n"
            "                                7 - BGR555\n"
            "                                8 - RGB444\n"
            "                                9 - BGR444\n"
            "  -O[n] --colorConversion   RGB to YCbCr color conversion type. [0]\n"
            "                                0 - BT.601\n"
            "                                1 - BT.709\n"
            "                                2 - User defined\n"
            "  -k[n] --videoRange        0..1 Video range. [0]\n\n");

    fprintf(stdout,
            "  -Z[n] --videoStab         Video stabilization. n > 0 enabled [0]\n\n");

    fprintf(stdout,
            "  -T[n] --constIntraPred    0=OFF, 1=ON Constrained intra pred flag [0]\n"
            "  -D[n] --disableDeblocking 0..2 Value of disable_deblocking_filter_idc [0]\n"
            "  -I[n] --intraVopRate      Intra vop rate. [0]\n"
            "  -V[n] --mbPerSlice        Slice size in macroblocks. Should be a\n"
            "                            multiple of MBs per row. [0]\n");

    fprintf(stdout,
            "\n  -B[n] --bitPerSecond      Bitrate. [64000]\n"
            "  -U[n] --vopRc             0=OFF, 1=ON Vop rc (Source model rc). [1]\n"
            "  -u[n] --mbRc              0=OFF, 1=ON Mb rc (Check point rc). [1]\n"
            "  -C[n] --hrdConformance    0=OFF, 1=ON HRD conformance. [0]\n"
            "  -c[n] --cpbSize           HRD Coded Picture Buffer size in bits. [0]\n");

    fprintf(stdout,
            "\n  -s[n] --vopSkip           0=OFF, 1=ON Vop skip rate control. [0]\n"
            "  -q[n] --qpHdr             -1..51, Default frame header qp. [26]\n"
            "                             -1=Encoder calculates initial QP\n"
            "  -n[n] --qpMin             0..51, Minimum frame header qp. [10]\n"
            "  -m[n] --qpMax             0..51, Maximum frame header qp. [51]\n");

    fprintf(stdout,
            "\n  -z[n] --userData          SEI User data file name.\n"
            "  -0[m]:[n]  --notCoded     firstPic:picInterval for forcing not coded pictures.\n"
            "                              0:0=disabled [0:0]\n");

    fprintf(stdout,
            "\nTesting parameters that are not supported for end-user:\n"
            "  -Q[n] --chromaQpOffset    -12..12 Chroma QP offset. [0]\n"
            "  -W[n] --filterOffsetA     -6..6 Deblockiong filter offset A. [0]\n"
            "  -E[n] --filterOffsetB     -6..6 Deblockiong filter offset B. [0]\n"
            "  -N[n] --burstSize          0..63 HW bus burst size. [16]\n"
            "  -t[n] --burstType          0=SIGLE, 1=INCR HW bus burst type. [0]\n"
            "  -e[n] --testId            Internal test ID. [0]\n"
            "  -P[n] --trigger           Logic Analyzer trigger at picture <n>. [-1]\n"
            "\n");
    ;
}

/*------------------------------------------------------------------------------

    WriteStrm
        Write encoded stream to file

    Params:
        fout    - file to write
        strbuf  - data to be written
        size    - amount of data to write
        endian  - data endianess, big or little

------------------------------------------------------------------------------*/
void WriteStrm(FILE * fout, u32 * strmbuf, u32 size, u32 endian)
{

#ifdef NO_OUTPUT_WRITE
    return;
#endif

    /* Swap the stream endianess before writing to file if needed */
    if(endian == 1)
    {
        u32 i = 0, words = (size + 3) / 4;

        while(words)
        {
            u32 val = strmbuf[i];
            u32 tmp = 0;

            tmp |= (val & 0xFF) << 24;
            tmp |= (val & 0xFF00) << 8;
            tmp |= (val & 0xFF0000) >> 8;
            tmp |= (val & 0xFF000000) >> 24;
            strmbuf[i] = tmp;
            words--;
            i++;
        }

    }

    /* Write the stream to file */
    fwrite(strmbuf, 1, size, fout);
}

/*------------------------------------------------------------------------------

    NextVop

    Function calculates next input vop depending input and output frame
    rates.

    Input   inputRateNumer  (input.yuv) frame rate numerator.
            inputRateDenom  (input.yuv) frame rate denominator
            outputRateNumer (stream.mpeg4) frame rate numerator.
            outputRateDenom (stream.mpeg4) frame rate denominator.
            frameCnt        Frame counter.
            firstVop        The first vop of input.yuv sequence.

    Return  next    The next vop of input.yuv sequence.

------------------------------------------------------------------------------*/
i32 NextVop(i32 inputRateNumer, i32 inputRateDenom, i32 outputRateNumer,
            i32 outputRateDenom, i32 frameCnt, i32 firstVop)
{
    u32 sift;
    u32 skip;
    u32 numer;
    u32 denom;
    u32 next;

    numer = (u32) inputRateNumer *(u32) outputRateDenom;
    denom = (u32) inputRateDenom *(u32) outputRateNumer;

    if(numer >= denom)
    {
        sift = 9;
        do
        {
            sift--;
        }
        while(((numer << sift) >> sift) != numer);
    }
    else
    {
        sift = 17;
        do
        {
            sift--;
        }
        while(((numer << sift) >> sift) != numer);
    }
    skip = (numer << sift) / denom;
    next = (((u32) frameCnt * skip) >> sift) + (u32) firstVop;

    return (i32) next;
}

/*------------------------------------------------------------------------------

    ChangeInput
        Change input file.
    Params:
        argc    - number of arguments to the application
        argv    - argument list as provided to the application
        option  - list of accepted cmdline options

    Returns:
        0 - for success
------------------------------------------------------------------------------*/
int ChangeInput(i32 argc, char **argv, char **name, option_s * option)
{
    i32 ret;
    argument_s argument;
    i32 enable = 0;

    argument.optCnt = 1;
    while((ret = EncGetOption(argc, argv, option, &argument)) != -1)
    {
        if((ret == 1) && (enable))
        {
            *name = argument.optArg;
            printf("\nNext file: %s\n", *name);
            return 1;
        }
        if(argument.optArg == *name)
        {
            enable = 1;
        }
    }

    return 0;
}

/*------------------------------------------------------------------------------

    API tracing
        TRacing as defined by the API.
    Params:
        msg - null terminated tracing message
------------------------------------------------------------------------------*/
void H264EncTrace(const char *msg)
{
    static FILE *fp = NULL;

    if(fp == NULL)
        fp = fopen("api.trc", "wt");

    if(fp)
        fprintf(fp, "%s\n", msg);
}

/*------------------------------------------------------------------------------
    Get out pure NAL units from byte stream format (one picture data)
    This is an example!

    Params:
        pNaluSizeBuf - buffer where the individual NAL size are (return by API)
        pStream- buffre containign  the whole picture data
------------------------------------------------------------------------------*/
void NalUnitsFromByteStream(const u32 * pNaluSizeBuf, const u8 * pStream)
{
    u32 nalSize, nal;
    const u8 *pNalBase;

    nal = 0;
    pNalBase = pStream + 4; /* skip the 4-byte startcode */

    while(pNaluSizeBuf[nal] != 0)   /* after the last NAL unit size we have a zero */
    {
        nalSize = pNaluSizeBuf[nal] - 4;

        /* now we have the pure NAL unit, do something with it */
        /* DoSomethingWithThisNAL(pNalBase, nalSize); */

        pNalBase += pNaluSizeBuf[nal];  /* next NAL data base address */
        nal++;  /* next NAL unit */
    }
}

/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/
void PrintNalSizes(const u32 *pNaluSizeBuf, const u8 *pOutBuf, u32 strmSize, 
        i32 byteStream)
{
    u32 nalu = 0, naluSum = 0;

    /* Step through the NALU size buffer */
    while(pNaluSizeBuf && pNaluSizeBuf[nalu] != 0) 
    {
#ifdef INTERNAL_TEST
        /* In byte stream format each NAL must start with 
         * start code: any number of leading zeros + 0x000001 */
        if (byteStream) {
            int zero_count = 0;
            const u8 *pTmp = pOutBuf + naluSum;

            /* count zeros, shall be at least 2 */
            while(*pTmp++ == 0x00)
                zero_count++;

            if(zero_count < 2 || pTmp[-1] != 0x01)
                printf
                    ("Error: NAL unit %d at %d doesn't start with '00 00 01'  ",
                     nalu, naluSum);
        }
#endif

        naluSum += pNaluSizeBuf[nalu];
        printf("%d  ", pNaluSizeBuf[nalu++]);
    }

#ifdef INTERNAL_TEST
    /* NAL sum must be the same as the whole frame stream size */
    if (naluSum && naluSum != strmSize)
        printf("Error: Sum of NAL size (%d) does not match stream size\n",
                naluSum);
#endif
}

/*------------------------------------------------------------------------------
    WriteNalSizesToFile    
        Dump NAL size to a file
    
    Params:
        file         - file name where toi dump
        pNaluSizeBuf - buffer where the individual NAL size are (return by API)
        buffSize     - size in bytes of the above buffer
------------------------------------------------------------------------------*/
void WriteNalSizesToFile(const char *file, const u32 * pNaluSizeBuf,
                         u32 buffSize)
{
    FILE *fo;
    u32 offset = 0;

    fo = fopen(file, "at");

    if(fo == NULL)
    {
        printf("FAILED to open NAL size tracing file <%s>\n", file);
        return;
    }

    while(offset < buffSize && *pNaluSizeBuf != 0)
    {
        fprintf(fo, "%d\n", *pNaluSizeBuf++);
        offset += sizeof(u32);
    }

    fclose(fo);
}


