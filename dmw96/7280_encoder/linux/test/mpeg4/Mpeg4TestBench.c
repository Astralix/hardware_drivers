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
--  Abstract : MPEG4 Encoder testbench
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: Mpeg4TestBench.c,v $
--  $Revision: 1.11 $
--  $Name: Rel7280_1_1_DSPGroup $
--
------------------------------------------------------------------------------*/

/* For parameter parsing */
#include "EncGetOption.h"
#include "Mpeg4TestBench.h"

/* For SW/HW shared memory allocation */
#include "ewl.h"

/* For accessing the EWL instance inside the encoder */
#include "EncInstance.h"

/* For compiler flags, test data, debug and tracing */
#include "enccommon.h"

/* For Hantro MPEG4 encoder */
#include "mp4encapi.h"

#ifdef INTERNAL_TEST
#include "mp4encapi_ext.h"
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

#define MP4ERR_OUTPUT stdout

/* The amount of entries in the VP size table, maximum one VP / MB */
#define VP_TABLE_SIZE (5120 + 1)

/* Global variables */

static char input[] = "input.yuv";
static char output[] = "stream.mpeg4";

static option_s option[] = {
    {"help", 'H', 0},

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
    {"rotation", 'r', 1},   /* Input image rotation */
    {"inputFormat", 'l', 1},    /* Input image format */
    {"colorConversion", 'O', 1},    /* RGB to YCbCr conversion type */
    {"input", 'i', 1},
    {"output", 'o', 1},

    {"scheme", 'S', 1},
    {"profile", 'p', 1},
    {"videoRange", 'k', 1},
    {"intraVopRate", 'I', 1},
    {"goVopRate", 'W', 1},
    {"cir", 'C', 1},

    {"vpSize", 'V', 1}, /* Error resilience tools */
    {"dataPart", 'D', 1},
    {"rvlc", 'R', 1},
    {"hec", 'E', 1},
    {"gobPlace", 'G', 1},

    {"bitPerSecond", 'B', 1},   /* Bitrate */
    {"vopRc", 'U', 1},  /* Vop rate control */
    {"mbRc", 'u', 1},   /* Mb rate control */
    {"videoBufferSize", 'v', 1},    /* Video buffer verifyer */
    {"vopSkip", 's', 1},    /* Vop skiping */
    {"qpMin", 'n', 1},  /* Minimum VOP header qp */
    {"qpMax", 'm', 1},  /* Maximum VOP header qp */
    {"qpHdr", 'q', 1},  /* Defaul first VOP header qp */

    {"userDataVos", 'z', 1},    /* User Data */
    {"userDataVisObj", 'c', 1}, /* User Data */
    {"userDataVol", 'd', 1},    /* User Data */
    {"userDataGov", 'g', 1},    /* User Data */

    {"stabilization", 'Z', 1},  /* video stabilization */

    {"testId", 'e', 1}, /* test pattern ID */
    {"burstSize", 'N', 1},  /* HW burst size */
    {"trigger", 'P', 1},
    {NULL, 0, 0}    /* Last line must be like this */
};

/* SW/HW shared memories for input/output buffers */
static EWLLinearMem_t pictureMem;
static EWLLinearMem_t pictureStabMem;
static EWLLinearMem_t outbufMem;

static FILE *yuvFile = NULL;
static long int file_size;

i32 trigger_point = -1;      /* Logic Analyzer trigger point */

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void FreeRes(MP4EncInst encoder);
static int AllocRes(commandLine_s * cmdl, MP4EncInst encoder);
static int OpenEncoder(commandLine_s * cml, MP4EncInst * encoder);
static void CloseEncoder(MP4EncInst encoder);
static int ReadVop(u8 * image, i32 size, i32 nro, char *name, i32 width,
                   i32 height, i32 format);
static int Parameter(i32 argc, char **argv, commandLine_s * ep);
static void Help(const char *exe);
static void WriteStrm(FILE * fout, u32 * outbuf, u32 size, u32 endian);
static MP4EncStrmType SelectStreamType(commandLine_s * cml);
static int UserData(MP4EncInst inst, char *name, MP4EncUsrDataType);
static i32 NextVop(i32 inputRateNumer, i32 inputRateDenom, i32 outputRateNumer,
                   i32 outputRateDenom, i32 frameCnt, i32 firstVop);
static int ChangeInput(i32 argc, char **argv, char **name, option_s * option);
static void PrintVpSizes(const u32 *pVpBuf, const u8 *pOutBuf, u32 strmSize);

/*------------------------------------------------------------------------------

    main

------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    MP4EncInst encoder;
    MP4EncRet ret;
    MP4EncIn encIn;
    MP4EncOut encOut;
    MP4EncRateCtrl rc;

    FILE *fout = NULL;
    int intraVopCnt = 0, codedVopCnt = 0, next = 0, src_img_size;
    u32 vopCnt = 0;
    u32 streamSize = 0;
    u32 bitrate = 0;

    commandLine_s cmdl;

    MP4EncApiVersion apiVer;
    MP4EncBuild encBuild;

    apiVer = MP4EncGetApiVersion();
    encBuild = MP4EncGetBuild();
    fprintf(stdout, "API version %d.%d\n", apiVer.major, apiVer.minor);
    fprintf(stdout, "HW ID: 0x%08x\t SW Build: %u.%u.%u\n\n",
            encBuild.hwBuild, encBuild.swBuild / 1000000,
            (encBuild.swBuild / 1000) % 1000, encBuild.swBuild % 1000);

    encIn.pVpBuf = NULL;
    encIn.vpBufSize = 0;

    if(argc < 2)
    {
        Help(argv[0]);
        exit(0);
    }

    /* Parse command line parameters */
    if(Parameter(argc, argv, &cmdl) != 0)
    {
        fprintf(MP4ERR_OUTPUT, "Input parameter error\n");
        return -1;
    }

    /* Encoder initialization */
    if(OpenEncoder(&cmdl, &encoder) != 0)
    {
        return -1;
    }

    printf("Encoder opened\n");

    /* Set the test ID for internal testing,
     * the SW must be compiled with testing flags */
    MP4EncSetTestId(encoder, cmdl.testId);

    /* Allocate input and output buffers */
    if(AllocRes(&cmdl, encoder) != 0)
    {
        fprintf(MP4ERR_OUTPUT, "Failed to allocate the external resources!\n");
        FreeRes(encoder);
        CloseEncoder(encoder);
        return -1;
    }

    printf("Allocated resources\n");

    encIn.outBufBusAddress = outbufMem.busAddress;
    encIn.pOutBuf = outbufMem.virtualAddress;
    encIn.outBufSize = outbufMem.size;

    /* Allocate memory for video packet size buffer, optional */
    encIn.vpBufSize = VP_TABLE_SIZE * sizeof(u32);
    encIn.pVpBuf = (u32 *) malloc(encIn.vpBufSize);
    if(!encIn.pVpBuf)
    {
        fprintf(MP4ERR_OUTPUT, "WARNING! Failed to allocate VP buffer.\n");
    }

#ifdef INTERNAL_TEST
    MP4EncSetHwBurstSize(encoder, cmdl.burst);
    printf("Set HW Burst Size: %d\n", cmdl.burst);
#endif

    /* Source Image Size */
    if(cmdl.inputFormat <= 1)
    {
        src_img_size = cmdl.lumWidthSrc * cmdl.lumHeightSrc +
            2 * (((cmdl.lumWidthSrc + 1) >> 1) *
                 ((cmdl.lumHeightSrc + 1) >> 1));
    }
    else
    {
        src_img_size = cmdl.lumWidthSrc * cmdl.lumHeightSrc * 2;
    }

    printf("Reading input from file <%s>, frame size %d bytes.\n\n",
            cmdl.input, src_img_size);

    printf("Input | Pic | QP | Type |  BR avg  | ByteCnt (inst) | Time code  | VP sizes\n");
    printf("---------------------------------------------------------------------------\n");

    /* Start stream */
    ret = MP4EncStrmStart(encoder, &encIn, &encOut);
    if(ret != ENC_OK)
    {
        fprintf(MP4ERR_OUTPUT, "Failed to start the stream. Error code: %8i\n",
                ret);
        FreeRes(encoder);
        CloseEncoder(encoder);
        return -1;
    }
    else
    {
        fout = fopen(cmdl.output, "wb");
        if(fout == NULL)
        {
            fprintf(MP4ERR_OUTPUT, "Failed to create the output file.\n");
            FreeRes(encoder);
            CloseEncoder(encoder);
            return -1;
        }

        MP4EncGetRateCtrl(encoder, &rc);

        streamSize = encOut.strmSize;

        printf("      |     | %2d | HDR  |          | %7i %6i |            |\n", 
                rc.qpHdr, streamSize, encOut.strmSize);

        WriteStrm(fout, outbufMem.virtualAddress, encOut.strmSize, 0);
    }

    /* Setup encoder input */
    {
        u32 w = (cmdl.lumWidthSrc + 15) & (~0x0f);

        encIn.busLuma = pictureMem.busAddress;

        encIn.busChromaU = encIn.busLuma + (w * cmdl.lumHeightSrc);
        encIn.busChromaV = encIn.busChromaU +
            (((w + 1) >> 1) * ((cmdl.lumHeightSrc + 1) >> 1));
    }

    encIn.vopType = INTRA_VOP;
    encIn.timeIncr = 0;

    encIn.busLumaStab = pictureStabMem.busAddress;

    intraVopCnt = cmdl.intraVopRate;

    /* Main encoding loop */
  nextinput:
    while((next = NextVop(cmdl.inputRateNumer, cmdl.inputRateDenom,
                          cmdl.outputRateNumer, cmdl.outputRateDenom, vopCnt,
                          cmdl.firstVop)) <= cmdl.lastVop)
    {
        if(vopCnt != 0)
            encIn.timeIncr = cmdl.outputRateDenom;

        /* Select VOP type */
        if(intraVopCnt == cmdl.intraVopRate)
        {
            encIn.vopType = INTRA_VOP;
            intraVopCnt = 0;
        }
        else
        {
            encIn.vopType = PREDICTED_VOP;
        }

#ifndef NO_INPUT_READ
        /* Read next vop */
        if(ReadVop
           ((u8 *) pictureMem.virtualAddress, 
            src_img_size, next, cmdl.input, cmdl.lumWidthSrc,
            cmdl.lumHeightSrc, cmdl.inputFormat) != 0)
            break;

        if(cmdl.videoStab > 0)
        {
            i32 nextStab = NextVop(cmdl.inputRateNumer, cmdl.inputRateDenom,
                          cmdl.outputRateNumer, cmdl.outputRateDenom, vopCnt+1,
                          cmdl.firstVop);

            if(ReadVop((u8 *) pictureStabMem.virtualAddress, 
                       src_img_size, nextStab, cmdl.input,
                       cmdl.lumWidthSrc, cmdl.lumHeightSrc,
                       cmdl.inputFormat) != 0)
                break;
        }
#endif

        /* Set Group of Vop header */
        if(vopCnt != 0 && cmdl.goVopRate != 0 && (vopCnt % cmdl.goVopRate) == 0)
        {
            MP4EncCodingCtrl codingCfg;

            if(MP4EncGetCodingCtrl(encoder, &codingCfg) != ENC_OK)
                printf("Could not get coding control.\n");

            codingCfg.insGOV = 1;

            if(MP4EncSetCodingCtrl(encoder, &codingCfg) != ENC_OK)
                printf("Could not set GOV header.\n");

        }

        /* Loop until one VOP is encoded */
        do
        {
            ret = MP4EncStrmEncode(encoder, &encIn, &encOut);

            MP4EncGetRateCtrl(encoder, &rc);

            streamSize += encOut.strmSize;

            /* Note: This will overflow if large output rate numerator is used */
            if((vopCnt+1) && cmdl.outputRateDenom)
                bitrate = 8 * ((streamSize/(vopCnt+1)) * 
                        (u32)cmdl.outputRateNumer / (u32)cmdl.outputRateDenom);

            switch (ret)
            {
            case ENC_VOP_READY:
                printf("%5i | %3i | %2i | %s | %8u | %7i %6i | %u.%u.%02u:%03u | ", 
                    next, vopCnt, rc.qpHdr, 
                    encOut.vopType == INTRA_VOP ? " I  " :
                    encOut.vopType == PREDICTED_VOP ? " P  " : "skip",
                    bitrate, streamSize, encOut.strmSize,
                    encOut.timeCode.hours,
                    encOut.timeCode.minutes,
                    encOut.timeCode.seconds,
                    encOut.timeCode.timeRes == 0 ? 0 :
                    (encOut.timeCode.timeIncr * 1000) /
                    encOut.timeCode.timeRes);

                PrintVpSizes(encIn.pVpBuf, (u8 *) outbufMem.virtualAddress, 
                            encOut.strmSize);
                printf("\n");

                WriteStrm(fout, outbufMem.virtualAddress, encOut.strmSize, 0);

                break;

            case ENC_GOV_READY:
                printf("      |     |    | GOV  |          | %7i %6i |            |\n", 
                    streamSize, encOut.strmSize);
                WriteStrm(fout, outbufMem.virtualAddress, encOut.strmSize, 0);
                break;

            case ENC_OUTPUT_BUFFER_OVERFLOW:
                printf("%5i | %3i | %2i | %s | %8u | %7i %6i | \n", 
                    next, vopCnt, rc.qpHdr, "lost",
                    bitrate, streamSize, encOut.strmSize);
                break;
            case ENC_HW_RESET:
                printf("\tFAILED. HW Reset detected: %i\n", ret);
                goto end;   /* end encoding */
            case ENC_HW_BUS_ERROR:
                printf("\tFAILED. HW Bus error detected: %i\n", ret);
                goto end;   /* end encoding */
            default:
                printf("\tFAILED. Error code: %i\n", ret);
                goto end;   /* end encoding */
            }
        }
        while(ret == ENC_GOV_READY);

        vopCnt++;

        if (encOut.vopType != NOTCODED_VOP) {
            intraVopCnt++; codedVopCnt++;
        }

    }   /* End of main encoding loop */

    /* Change next input sequence */
    if(ChangeInput(argc, argv, &cmdl.input, option))
    {
        if(yuvFile != NULL)
        {
            fclose(yuvFile);
            yuvFile = NULL;
        }

        vopCnt = 0;
        goto nextinput;
    }

  end:

    /* End stream */
    ret = MP4EncStrmEnd(encoder, &encIn, &encOut);
    if(ret != ENC_OK)
    {
        fprintf(MP4ERR_OUTPUT, "Failed to end the stream. Error code: %8i\n",
                ret);
    }
    else
    {
        streamSize += encOut.strmSize;
        printf("      |     |    | END  |          | %7i %6i |            |\n", 
                streamSize, encOut.strmSize);

        WriteStrm(fout, outbufMem.virtualAddress, encOut.strmSize, 0);
    }

    printf("\nBitrate target %d bps, actual %d bps (%d%%).\n",
            rc.bitPerSecond, bitrate,
            (rc.bitPerSecond) ? bitrate*100/rc.bitPerSecond : 0);
    printf("Total of %d frames processed, %d frames encoded, %d bytes.\n",
            vopCnt, codedVopCnt, streamSize);

    /* Free all resources */
    FreeRes(encoder);

    CloseEncoder(encoder);

    if(fout != NULL)
        fclose(fout);

    if(yuvFile != NULL)
        fclose(yuvFile);

    if(encIn.pVpBuf != NULL)
        free(encIn.pVpBuf);

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
int AllocRes(commandLine_s * cmdl, MP4EncInst enc)
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
    ret = EWLMallocLinear(((instance_s *)enc)->asic.ewl, pictureSize, 
                &pictureMem);
    if (ret != EWL_OK)
        return 1;
    
    if(cmdl->videoStab > 0) {
        ret = EWLMallocLinear(((instance_s *)enc)->asic.ewl, pictureSize, 
                &pictureStabMem);
        if (ret != EWL_OK)
            return 1;
    }

    outbufSize = 4 * pictureMem.size < (1024 * 1024 * 3 / 2) ?
        4 * pictureMem.size : (1024 * 1024 * 3 / 2);

    ret = EWLMallocLinear(((instance_s *)enc)->asic.ewl, outbufSize, &outbufMem);
    if (ret != EWL_OK)
        return 1;

    printf("Input buffer size:          %d bytes\n", pictureMem.size);
    printf("Input buffer bus address:   0x%08x\n", pictureMem.busAddress);
    printf("Input buffer user address:  %10p\n", pictureMem.virtualAddress);
    printf("Output buffer size:         %d bytes\n", outbufMem.size);
    printf("Output buffer bus address:  0x%08x\n", outbufMem.busAddress);
    printf("Output buffer user address: %10p\n",  outbufMem.virtualAddress);

    return 0;
}

/*------------------------------------------------------------------------------

    FreeRes

    Release all resources allocated byt AllocRes()

------------------------------------------------------------------------------*/
void FreeRes(MP4EncInst enc)
{
    EWLFreeLinear(((instance_s *)enc)->asic.ewl, &pictureMem); 
    EWLFreeLinear(((instance_s *)enc)->asic.ewl, &pictureStabMem); 
    EWLFreeLinear(((instance_s *)enc)->asic.ewl, &outbufMem); 
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
int OpenEncoder(commandLine_s * cml, MP4EncInst * pEnc)
{
    MP4EncRet ret;
    MP4EncCfg cfg;
    MP4EncCodingCtrl codingCfg;
    MP4EncRateCtrl rcCfg;
    MP4EncStreamInfo strmInfo;
    MP4EncPreProcessingCfg preProcCfg;

    MP4EncInst encoder;

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

    cfg.frmRateDenom = cml->outputRateDenom;
    cfg.frmRateNum = cml->outputRateNumer;

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

    cfg.strmType = SelectStreamType(cml);
    cfg.profileAndLevel = (MP4EncProfileAndLevel)cml->profile;

    printf
        ("Init config: Size %dx%d fps %d/%d Strm Type %d P&L %d\n",
         cfg.width, cfg.height, cfg.frmRateNum,
         cfg.frmRateDenom, cfg.strmType, cfg.profileAndLevel);

    if((ret = MP4EncInit(&cfg, pEnc)) != ENC_OK)
    {
        fprintf(MP4ERR_OUTPUT,
                "Failed to initialize the encoder. Error code: %8i\n", ret);
        return -1;
    }

    encoder = *pEnc;

    /* Encoder setup: stream info */
    if((ret = MP4EncGetStreamInfo(encoder, &strmInfo)) != ENC_OK)
    {
        fprintf(MP4ERR_OUTPUT, "Failed to get stream info. Error code: %8i\n",
                ret);
        CloseEncoder(encoder);
        return -1;
    }
    else
    {
        printf("Get Stream Info: videoRange %d parWidth %d parHeight %d\n",
               strmInfo.videoRange, strmInfo.pixelAspectRatioWidth,
               strmInfo.pixelAspectRatioHeight);

        strmInfo.videoRange = cml->videoRange;

        printf("Set Stream Info: videoRange %d parWidth %d parHeight %d\n",
               strmInfo.videoRange, strmInfo.pixelAspectRatioWidth,
               strmInfo.pixelAspectRatioHeight);

        if((ret = MP4EncSetStreamInfo(encoder, &strmInfo)) != ENC_OK)
        {
            fprintf(MP4ERR_OUTPUT,
                    "Failed to set stream info. Error code: %8i\n", ret);
            CloseEncoder(encoder);
            return -1;
        }
    }

    /* Encoder setup: rate control */
    if((ret = MP4EncGetRateCtrl(encoder, &rcCfg)) != ENC_OK)
    {
        fprintf(MP4ERR_OUTPUT, "Failed to get RC info. Error code: %8i\n", ret);
        CloseEncoder(encoder);
        return -1;
    }
    else
    {
        printf("Get rate control: qp %2d [%2d, %2d]   %8d bps   "
               "%d %d %d vbv: %3d gop: %d\n",
               rcCfg.qpHdr, rcCfg.qpMin, rcCfg.qpMax, rcCfg.bitPerSecond,
               rcCfg.vopRc, rcCfg.mbRc, rcCfg.vopSkip, rcCfg.vbv, rcCfg.gopLen);

        if(cml->qpHdr != DEFAULT)
            rcCfg.qpHdr = cml->qpHdr;
        if(cml->qpMin != DEFAULT)
            rcCfg.qpMin = cml->qpMin;
        if(cml->qpMax != DEFAULT)
            rcCfg.qpMax = cml->qpMax;
        if(cml->vopSkip != DEFAULT)
            rcCfg.vopSkip = cml->vopSkip;
        if(cml->vopRc != DEFAULT)
            rcCfg.vopRc = cml->vopRc;
        if(cml->mbRc != DEFAULT)
            rcCfg.mbRc = cml->mbRc;
        if(cml->bitPerSecond != DEFAULT)
            rcCfg.bitPerSecond = cml->bitPerSecond;
        if(cml->videoBufferSize != DEFAULT)
            rcCfg.vbv = cml->videoBufferSize;
        if ( cml->intraVopRate != DEFAULT )
            rcCfg.gopLen = cml->intraVopRate; 

        if (rcCfg.gopLen < 1 || rcCfg.gopLen > 128)
            rcCfg.gopLen = 128;

        printf("Set rate control: qp %2d [%2d, %2d]   %8d bps   "
               "%d %d %d vbv: %3d gop: %d\n",
               rcCfg.qpHdr, rcCfg.qpMin, rcCfg.qpMax, rcCfg.bitPerSecond,
               rcCfg.vopRc, rcCfg.mbRc, rcCfg.vopSkip, rcCfg.vbv, rcCfg.gopLen);

        if((ret = MP4EncSetRateCtrl(encoder, &rcCfg)) != ENC_OK)
        {
            fprintf(MP4ERR_OUTPUT, "Failed to set RC info. Error code: %8i\n",
                    ret);
            CloseEncoder(encoder);
            return -1;
        }
    }

    /* Encoder setup: coding control */
    if((ret = MP4EncGetCodingCtrl(encoder, &codingCfg)) != ENC_OK)
    {
        fprintf(MP4ERR_OUTPUT, "Failed to get CODING info. Error code: %8i\n",
                ret);
        CloseEncoder(encoder);
        return -1;
    }
    else
    {
        printf
            ("Get coding control: VP %5d   HEC %d   GOB 0x%08x   GOV %d\n",
             codingCfg.vpSize, codingCfg.insHEC, codingCfg.insGOB,
             codingCfg.insGOV);

        if(cml->vpSize != DEFAULT && cml->vpSize != 0)
            codingCfg.vpSize = cml->vpSize;

        if(cml->hec != DEFAULT)
        {
            if(cml->hec != 0)
                codingCfg.insHEC = 1;
            else
                codingCfg.insHEC = 0;
        }

        if(cml->gobPlace != DEFAULT)
            codingCfg.insGOB = cml->gobPlace;

        printf
            ("Set coding control: VP %5d   HEC %d   GOB 0x%08x   GOV %d\n",
             codingCfg.vpSize, codingCfg.insHEC, codingCfg.insGOB,
             codingCfg.insGOV);

        if((ret = MP4EncSetCodingCtrl(encoder, &codingCfg)) != ENC_OK)
        {
            fprintf(MP4ERR_OUTPUT,
                    "Failed to set CODING info. Error code: %8i\n", ret);
            CloseEncoder(encoder);
            return -1;
        }
    }

    /* User data */
    if(UserData(encoder, cml->userDataVos, MPEG4_VOS_USER_DATA) != 0)
    {
        CloseEncoder(encoder);
        return -1;
    }
    if(UserData(encoder, cml->userDataVisObj, MPEG4_VO_USER_DATA) != 0)
    {
        CloseEncoder(encoder);
        return -1;
    }
    if(UserData(encoder, cml->userDataVol, MPEG4_VOL_USER_DATA) != 0)
    {
        CloseEncoder(encoder);
        return -1;
    }
    if(UserData(encoder, cml->userDataGov, MPEG4_GOV_USER_DATA) != 0)
    {
        CloseEncoder(encoder);
        return -1;
    }

    /* PreP setup */
    if((ret = MP4EncGetPreProcessing(encoder, &preProcCfg)) != ENC_OK)
    {
        fprintf(MP4ERR_OUTPUT, "Failed to get PreP info. Error code: %8i\n",
                ret);
        CloseEncoder(encoder);
        return -1;
    }
    printf
        ("Get PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d"
         ": stab %d : cc %d\n",
         preProcCfg.origWidth, preProcCfg.origHeight, preProcCfg.xOffset,
         preProcCfg.yOffset, preProcCfg.inputType, preProcCfg.rotation,
         preProcCfg.videoStabilization, preProcCfg.colorConversion.type);

    preProcCfg.inputType = (MP4EncPictureType)cml->inputFormat;
    preProcCfg.rotation = (MP4EncPictureRotation)cml->rotation;

    preProcCfg.origWidth =
        cml->lumWidthSrc /* (cml->lumWidthSrc + 15) & (~0x0F) */ ;
    preProcCfg.origHeight = cml->lumHeightSrc;

    if(cml->horOffsetSrc != DEFAULT)
        preProcCfg.xOffset = cml->horOffsetSrc;
    if(cml->verOffsetSrc != DEFAULT)
        preProcCfg.yOffset = cml->verOffsetSrc;
    if(cml->colorConversion != DEFAULT)
        preProcCfg.colorConversion.type = 
                        (MP4EncColorConversionType)cml->colorConversion;
    if(preProcCfg.colorConversion.type == ENC_RGBTOYUV_USER_DEFINED)
    {
        preProcCfg.colorConversion.coeffA = 20000;
        preProcCfg.colorConversion.coeffB = 44000;
        preProcCfg.colorConversion.coeffC = 5000;
        preProcCfg.colorConversion.coeffE = 35000;
        preProcCfg.colorConversion.coeffF = 38000;
    }

    if(cml->videoStab >= 0)
    {
        preProcCfg.videoStabilization = cml->videoStab;
    }

    printf
        ("Set PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d"
         ": stab %d : cc %d\n",
         preProcCfg.origWidth, preProcCfg.origHeight, preProcCfg.xOffset,
         preProcCfg.yOffset, preProcCfg.inputType, preProcCfg.rotation,
         preProcCfg.videoStabilization, preProcCfg.colorConversion.type);

    if((ret = MP4EncSetPreProcessing(encoder, &preProcCfg)) != ENC_OK)
    {
        fprintf(MP4ERR_OUTPUT, "Failed to set PreP info. Error code: %8i\n",
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
void CloseEncoder(MP4EncInst encoder)
{
    MP4EncRet ret;

    if((ret = MP4EncRelease(encoder)) != ENC_OK)
    {
        fprintf(MP4ERR_OUTPUT,
                "Failed to release the encoder. Error code: %8i\n", ret);
    }
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
    int status = 0;

    /* use UNCHANGED for values that use encoder API defaults */

    cml->input = input;
    cml->output = output;
    cml->userDataVos = NULL;
    cml->userDataVisObj = NULL;
    cml->userDataVol = NULL;
    cml->userDataGov = NULL;
    cml->firstVop = 0;
    cml->lastVop = 100;
    cml->width = UNCHANGED;
    cml->height = UNCHANGED;
    cml->lumWidthSrc = 176;
    cml->lumHeightSrc = 144;
    cml->horOffsetSrc = UNCHANGED;
    cml->verOffsetSrc = UNCHANGED;
    cml->outputRateNumer = UNCHANGED;
    cml->outputRateDenom = UNCHANGED;
    cml->inputRateNumer = 30;
    cml->inputRateDenom = 1;
    cml->scheme = 0;
    cml->profile = 5;
    cml->videoRange = 0;
    cml->intraVopRate = 128;
    cml->goVopRate = 0;
    cml->vpSize = UNCHANGED;
    cml->dataPart = UNCHANGED;
    cml->rvlc = UNCHANGED;
    cml->hec = UNCHANGED;
    cml->gobPlace = 0;
    cml->bitPerSecond = UNCHANGED;
    cml->vopRc = UNCHANGED;
    cml->mbRc = UNCHANGED;
    cml->videoBufferSize = UNCHANGED;
    cml->vopSkip = UNCHANGED;
    cml->qpHdr = UNCHANGED;
    cml->qpMin = UNCHANGED;
    cml->qpMax = UNCHANGED;
    cml->rotation = 0;
    cml->videoStab = UNCHANGED;
    cml->colorConversion = UNCHANGED;
    cml->inputFormat = 0;
    cml->burst = 16;
    cml->testId = 0;

    argument.optCnt = 1;
    while((ret = EncGetOption(argc, argv, option, &argument)) != -1)
    {
        if(ret == -2)
        {
            status = -1;
        }
        optArg = argument.optArg;
        switch (argument.shortOpt)
        {
        case 'H':
            Help(argv[0]);
            exit(0);
        case 'i':
            cml->input = optArg;
            break;
        case 'z':
            cml->userDataVos = optArg;
            break;
        case 'c':
            cml->userDataVisObj = optArg;
            break;
        case 'd':
            cml->userDataVol = optArg;
            break;
        case 'g':
            cml->userDataGov = optArg;
            break;
        case 'o':
            cml->output = optArg;
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
        case 'S':
            cml->scheme = atoi(optArg);
            break;
        case 'p':
            cml->profile = atoi(optArg);
            break;
        case 'k':
            cml->videoRange = atoi(optArg);
            break;
        case 'I':
            cml->intraVopRate = atoi(optArg);
            break;
        case 'W':
            cml->goVopRate = atoi(optArg);
            break;
        case 'V':
            cml->vpSize = atoi(optArg);
            break;
        case 'D':
            cml->dataPart = atoi(optArg);
            break;
        case 'R':
            cml->rvlc = atoi(optArg);
            break;
        case 'E':
            cml->hec = atoi(optArg);
            break;
        case 'G':
            cml->gobPlace = atoi(optArg);
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
        case 'v':
            cml->videoBufferSize = atoi(optArg);
            break;
        case 's':
            cml->vopSkip = atoi(optArg);
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
        case 'r':
            cml->rotation = atoi(optArg);
            break;
        case 'l':
            cml->inputFormat = atoi(optArg);
            break;
        case 'O':
            cml->colorConversion = atoi(optArg);
            break;
        case 'e':
            cml->testId = atoi(optArg);
            break;
        case 'N':
            cml->burst = atoi(optArg);
            break;
        case 'P':
            trigger_point = atoi(optArg);
            break;
        case 'Z':
            cml->videoStab = atoi(optArg);
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
            fprintf(MP4ERR_OUTPUT, "Unable to open YUV file: %s\n", name);
            ret = -1;
            goto end;
        }

        fseek(yuvFile, 0, SEEK_END);
        file_size = ftell(yuvFile);
    }

    /* Stop if over last frame of the file */
    if(size * (nro + 1) > file_size)
    {
        printf("Can't read frame, EOF\n");
        ret = -1;
        goto end;
    }

    if(fseek(yuvFile, size * nro, SEEK_SET) != 0)
    {
        fprintf(MP4ERR_OUTPUT, "I can't seek frame no: %i\n", nro);
        fprintf(MP4ERR_OUTPUT, "From file: %s\n", name);
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

        if(format == 0)
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
        else if(format == 1)
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
        else
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

    UserData
        Read user data from file

    Params:
        inst - encoder instance
        name - name of file in which user data si located
        type - user data type

    Returns:
        0 - for success
       -1 - error code

------------------------------------------------------------------------------*/
int UserData(MP4EncInst inst, char *name, MP4EncUsrDataType type)
{
    FILE *file = NULL;
    i32 byteCnt;
    u8 *data;

    if(name == NULL)
    {
        return 0;
    }

    /* Get user data length from file */
    file = fopen(name, "rb");
    if(file == NULL)
    {
        fprintf(MP4ERR_OUTPUT, "Unable to open User Data file: %s\n", name);
        return -1;
    }
    fseek(file, 0L, SEEK_END);
    byteCnt = ftell(file);
    rewind(file);

    /* Allocate memory for user data */
    if((data = (u8 *) malloc(sizeof(u8) * byteCnt)) == NULL)
    {
        fprintf(MP4ERR_OUTPUT, "Unable to alloc User Data memory\n");
        return -1;
    }

    /* Read user data from file */
    fread(data, sizeof(u8), byteCnt, file);
    fclose(file);

    /* Set stream user data */
    MP4EncSetUsrData(inst, data, byteCnt, type);

    free(data);

    return 0;
}

/*------------------------------------------------------------------------------

    Help
        Print out some instructions about usage.
    Params: 
        exe - testbench name
------------------------------------------------------------------------------*/
void Help(const char *exe)
{
    fprintf(stdout, "Usage:  %s [options] [-i <inputfile>] [-o <outputfile>]\n",
            exe);
    fprintf(stdout,
            "  -H    --help             Display this help.\n\n"
            "  -S[n] --scheme           0=MPEG4, 1=SVH, 3=H263.[0]\n\n"
            "  -i[s] --input            Read input from file. [input.yuv]\n"
            "  -o[s] --output           Write output to file. [stream.mpeg4]\n"
            "  -a[n] --firstVop         First vop of input file. [0]\n"
            "  -b[n] --lastVop          Last vop of input file. [100]\n"
            "  -w[n] --lumWidthSrc      Width of source image. [176]\n"
            "  -h[n] --lumHeightSrc     Height of source image. [144]\n");
    fprintf(stdout,
            "  -x[n] --width            Width of output image. [--lumWidthSrc]\n"
            "  -y[n] --height           Height of output image. [--lumHeightSrc]\n"
            "  -X[n] --horOffsetSrc     Output image horizontal offset. [0]\n"
            "  -Y[n] --verOffsetSrc     Output image vertical offset. [0]\n");
    fprintf(stdout,
            "  -j[n] --inputRateNumer   Input vop rate numerator. [30]\n"
            "  -J[n] --inputRateDenom   Input vop rate denominator. [1]\n"
            "  -f[n] --outputRateNumer  Output vop rate numerator. [--inputRateNumer]\n"
            "  -F[n] --outputRateDenom  Output vop rate denominator. [--inputRateDenom]\n\n");

    fprintf(stdout,
            "  -p[n] --profile          Profile and Level code. [5]\n"
            "                               1=Simple Profile/Level 1,\n"
            "                               2=Simple Profile/Level 2,\n"
            "                               3=Simple Profile/Level 3,\n"
            "                               4=Simple Profile/Level 4A,\n"
            "                               5=Simple Profile/Level 5,\n"
            "                               6=Simple Profile/Level 6,\n"
            "                               8=Simple Profile/Level 0,\n"
            "                               9=Simple Profile/Level 0B,\n"
            "                              52=Main Profile/Level 4,\n"
            "                             243=Advanced Simple Profile/Level 3,\n"
            "                             244=Advanced Simple Profile/Level 4,\n"
            "                             245=Advanced Simple Profile/Level 5,\n"
            "                            1001=H.263 Level 10,\n"
            "                            1002=H.263 Level 20,\n"
            "                            1003=H.263 Level 30,\n"
            "                            1004=H.263 Level 40,\n"
            "                            1005=H.263 Level 50,\n"
            "                            1006=H.263 Level 60,\n"
            "                            1007=H.263 Level 70\n\n");

    fprintf(stdout,
            "  -k[n] --videoRange       Source video range.[0]\n"
            "                                 n=0 [16,235]\n"
            "                                 n=1 [0,255]\n");

    fprintf(stdout,
            "  -I[n] --intraVopRate     Intra vop rate. [128]\n"
            "  -W[n] --goVopRate        Group of vop (GOVOP) header rate. [0]\n");
    fprintf(stdout,
            "  -V[n] --vpSize           Video packet size, bits. [0]\n"
            "  -D[n] --dataPart         0=OFF, 1=ON Data partition. [0]\n"
            "  -R[n] --rvlc             0=OFF, 1=ON Reversible vlc. [0]\n"
            "  -E[n] --hec              0=OFF, 1=ON Header extension code. [0]\n"
            "  -G[n] --gobPlace         Groups of blocks. Bit pattern define GOB place. [0]\n\n");

    fprintf(stdout,
            "  -B[n] --bitPerSecond     Bitrate. [Profile/Level maximum]\n"
            "  -U[n] --vopRc            0=OFF, 1=ON Vop qp (Source model rc). [1]\n"
            "  -u[n] --mbRc             0=OFF, 1=ON Mb qp (Check point rc). [1]\n"
            "  -v[n] --videoBufferSize  0=OFF, 1=ON Video buffer verifier. [1]\n"
            "  -s[n] --vopSkip          0=OFF, 1=ON Vop skip rate control. [0]\n"
            "  -q[n] --qpHdr            1..31, Default VOP header qp. [10]\n"
            "                             -1=Encoder calculates initial QP\n"
            "  -n[n] --qpMin            1..31, Minimum VOP header qp. [1]\n"
            "  -m[n] --qpMax            1..31, Maximum VOP header qp. [31]\n\n");
    fprintf(stdout,
            "  -z[n] --userDataVos      User data file name of Vos.\n"
            "  -c[n] --userDataVisObj   User data file name of VisObj.\n"
            "  -d[n] --userDataVol      User data file name of Vol.\n"
            "  -g[n] --userDataGov      User data file name of Gov.\n\n");

    fprintf(stdout,
            "  -r[n] --rotation         Source image rotation.[0]\n"
            "                               n=0, no rotation.\n"
            "                               n=1, 90 (clockwise rotation).\n"
            "                               n=2, -90 (counter-clockwise rotation). [0]\n\n");

    fprintf(stdout,
            "  -l[n] --inputFormat      Source image format.[0]\n"
            "                               n=0, planar YCbCr 4:2:0.\n"
            "                               n=1, semiplanar YCbCr 4:2:0.\n"
            "                               n=2, YCbYCr 4:2:2.\n"
            "                               n=3, CbYCrY 4:2:2.\n"
            "                               n=4, RGB565.\n"
            "                               n=5, BGR565.\n"
            "                               n=6, RGB555.\n"
            "                               n=7, BGR555.\n"
            "                               n=8, RGB444.\n"
            "                               n=9, BGR444.\n"
            "  -O[n] --colorConversion  RGB to YCbCr color conversion type. [0]\n"
            "                               0 - BT.601\n"
            "                               1 - BT.709\n"
            "                               2 - User defined\n\n");

    fprintf(stdout,
            "  -Z[n] --stabilization    Video stabilization. n > 0 enabled [0]\n\n");

    fprintf(stdout,
            " The following parameters are not supported by the product API.\n"
            " They are controlled with software algorithms and are provided\n"
            " here only for internal testing purposes.\n"
            "  -N[n] --burstSize         0..63 HW bus burst size. [16]\n"
            "  -e[n] --testId            Internal test ID. [0]\n"
            "  -P[n] --trigger           Logic Analyzer trigger at picture <n>. [-1]\n"
            "\n");
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

    Select stream type based on scheme and encoding parameters

------------------------------------------------------------------------------*/
MP4EncStrmType SelectStreamType(commandLine_s * cml)
{
    /* Short Video Header stream */
    if(cml->scheme == 1)
        return MPEG4_SVH_STRM;

    /* H.263 stream */
    if(cml->scheme == 3)
    {
        return H263_STRM;
    }

    if(cml->rvlc != DEFAULT && cml->rvlc != 0)
        return MPEG4_VP_DP_RVLC_STRM;

    if(cml->dataPart != DEFAULT && cml->dataPart != 0)
        return MPEG4_VP_DP_STRM;

    if(cml->vpSize != DEFAULT && cml->vpSize != 0)
        return MPEG4_VP_STRM;

    return MPEG4_PLAIN_STRM;
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

------------------------------------------------------------------------------*/
void PrintVpSizes(const u32 *pVpBuf, const u8 *pOutBuf, u32 strmSize)
{
    u32 vp = 0, vpSum = 0;

    /* Step through the VP size buffer */
    while(pVpBuf && pVpBuf[vp] != 0) 
    {
#ifdef INTERNAL_TEST
        /* Each VP must start with zero bytes */
        if (pOutBuf[vpSum  ] != 0 ||
            pOutBuf[vpSum+1] != 0)
            printf("Error: VP %d at %d does not start with '0x0000'  ", vp, vpSum);
#endif

        vpSum += pVpBuf[vp];
        printf("%d  ", pVpBuf[vp++]);
    }

#ifdef INTERNAL_TEST
    /* VP sum must be the same as the whole frame stream size */
    if (vpSum && vpSum != strmSize)
        printf("Error: Sum of VP size (%d) does not match stream size\n",
                vpSum);
#endif
}

/*------------------------------------------------------------------------------

    API tracing
        TRacing as defined by the API.
    Params:
        msg - null terminated tracing message
------------------------------------------------------------------------------*/
void MP4Enc_Trace(const char *msg)
{
    static FILE *fp = NULL;

    if(fp == NULL)
        fp = fopen("api.trc", "wt");

    if(fp)
        fprintf(fp, "%s\n", msg);
}


