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
--  Abstract : Jpeg Encoder testbench
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: JpegTestBench.c,v $
--  $Revision: 1.12 $
--
------------------------------------------------------------------------------*/

/* For parameter parsing */
#include "EncGetOption.h"
#include "JpegTestBench.h"

/* For SW/HW shared memory allocation */
#include "ewl.h"

/* For accessing the EWL instance inside the encoder */
#include "EncJpegInstance.h"

/* For compiler flags, test data, debug and tracing */
#include "enccommon.h"

/* For Hantro Jpeg encoder */
#include "jpegencapi.h"

/* For printing and file IO */
#include <stdio.h>

/* For dynamic memory allocation */
#include <stdlib.h>

/* For memset, strcpy and strlen */
#include <string.h>

/*--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/* User selectable testbench configuration */

/* Define this if you want to save each frame of motion jpeg 
 * into frame%d.jpg */
/* #define SEPARATE_FRAME_OUTPUT */

/* Define this if yuv don't want to use debug printf */
/*#define ASIC_WAVE_TRACE_TRIGGER*/

/* Output stream is not written to file. This should be used
   when running performance simulations. */
/*#define NO_OUTPUT_WRITE */

/* Define these if you want to use testbench defined 
 * comment header */
/*#define TB_DEFINED_COMMENT */


/* Global variables */

/* Command line options */

static option_s options[] = {
    {"help", 'H', 0},
    {"inputThumb", 'I', 1},
    {"thumbnail", 'T', 1},
    {"widthThumb", 'K', 1},
    {"heightThumb", 'L', 1},
    {"lumWidthSrcThumb", 'B', 1},
    {"lumHeightSrcThumb", 'e', 1},
    {"horOffsetSrcThumb", 'A', 1},
    {"verOffsetSrcThumb", 'O', 1},
    {"input", 'i', 1},
    {"output", 'o', 1},
    {"firstVop", 'a', 1},
    {"lastVop", 'b', 1},
    {"lumWidthSrc", 'w', 1},
    {"lumHeightSrc", 'h', 1},
    {"width", 'x', 1},
    {"height", 'y', 1},
    {"horOffsetSrc", 'X', 1},
    {"verOffsetSrc", 'Y', 1},
    {"restartInterval", 'R', 1},
    {"qLevel", 'q', 1},
    {"frameType", 'g', 1},
    {"colorConversion", 'v', 1},
    {"rotation", 'G', 1},
    {"codingType", 'p', 1},
    {"codingMode", 'm', 1},
    {"markerType", 't', 1},
    {"units", 'u', 1},
    {"xdensity", 'k', 1},
    {"ydensity", 'l', 1},
    {"write", 'W', 1},
    {"comLength", 'c', 1},
    {"comFile", 'C', 1},
    {"trigger", 'P', 1},
    {NULL, 0, 0}
};

/* SW/HW shared memories for input/output buffers */
EWLLinearMem_t pictureMem;
EWLLinearMem_t outbufMem;

/* Test bench definition of comment header */
#ifdef TB_DEFINED_COMMENT
    /* COM data */
static u32 comLen = 38;
static u8 comment[39] = "This is Hantro's test COM data header.";
#endif

static JpegEncCfg cfg;

static u32 writeOutput = 1;

/* Logic Analyzer trigger point */
i32 trigger_point = -1;

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void FreeRes(JpegEncInst enc);
static int AllocRes(commandLine_s * cmdl, JpegEncInst encoder);
static int OpenEncoder(commandLine_s * cml, JpegEncInst * encoder);
static void CloseEncoder(JpegEncInst encoder);
static int ReadVop(u8 * image, i32 width, i32 height, i32 sliceNum,
                   i32 sliceRows, i32 frameNum, char *name, u32 inputMode);
static int Parameter(i32 argc, char **argv, commandLine_s * ep);
static void Help(void);
static void WriteStrm(FILE * fout, u32 * outbuf, u32 size, u32 endian);
static void WriteFrame(char *filename, u32 * strmbuf, u32 size, u32 mode);

/*------------------------------------------------------------------------------

    main

------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    JpegEncInst encoder;
    JpegEncRet ret;
    JpegEncIn encIn;
    JpegEncOut encOut;
    JpegEncApiVersion encVer;
    JpegEncBuild encBuild;

    FILE *fout = NULL;
    i32 vopBytes = 0;
    i32 mode = 0;

    commandLine_s cmdl;

    fprintf(stdout, "\n* * * * * * * * * * * * * * * * * * * * *\n\n"
            "      "
            "HANTRO JPEG ENCODER TESTBENCH\n"
            "\n* * * * * * * * * * * * * * * * * * * * *\n\n");

    if(argc < 2)
    {
        Help();
        exit(0);
    }

    /* Print API and build version numbers */
    encVer = JpegEncGetApiVersion();
    encBuild = JpegEncGetBuild();

    /* Version */
    fprintf(stdout,
            "\n7280 JPEG Encoder API v%d.%d - SW build: %d - HW build: %x\n\n",
            encVer.major, encVer.minor, encBuild.swBuild, encBuild.hwBuild);

    /* Parse command line parameters */
    if(Parameter(argc, argv, &cmdl) != 0)
    {
        fprintf(stderr, "Input parameter error\n");
        return -1;
    }

    /* Encoder initialization */
    if(OpenEncoder(&cmdl, &encoder) != 0)
    {
        return -1;
    }

    /* Allocate input and output buffers */
    if(AllocRes(&cmdl, encoder) != 0)
    {
        fprintf(stderr, "Failed to allocate the external resources!\n");
        FreeRes(encoder);
        CloseEncoder(encoder);
        return -1;
    }

#ifdef TRACE_STREAM
    /* Open stream tracing */
    EncOpenStreamTrace("stream.trc");
#endif

    /* Setup encoder input */
    encIn.pOutBuf = (u8 *)outbufMem.virtualAddress;

    printf("encIn.pOutBuf 0x%08x\n", (unsigned int) encIn.pOutBuf);
    encIn.busOutBuf = outbufMem.busAddress;
    encIn.outBufSize = outbufMem.size;
    encIn.frameHeader = 1;

    fout = fopen(cmdl.output, "wb");
    if(fout == NULL)
    {
        fprintf(stderr, "Failed to create the output file.\n");
        CloseEncoder(encoder);
        FreeRes(encoder);
        return -1;
    }

    /* For thumbnail run both modes 1 and 0, otherwise only mode 0 */
    if(cmdl.thumbnail)
        mode = 1;
    else
        mode = 0;

    /* thumbnail and/or full resolution image to stream */
    for ( ; mode >= 0; mode--)
    {
        i32 slice = 0, sliceRows = 0;
        i32 next = 0, last = 0, vopCnt = 0;
        i32 widthSrc, heightSrc;
        char *input;

#ifdef EWL_FILE
        SetMode(mode);
#endif

        if (mode == 1)
        {
            /* set thumbnail mode */
            ret = JpegEncSetThumbnailMode(encoder, &cfg);

            /* no slice mode in thumbnail */
            sliceRows = cmdl.lumHeightSrcThumb;

            widthSrc = cmdl.lumWidthSrcThumb;
            heightSrc = cmdl.lumHeightSrcThumb;

            input = cmdl.inputThumb;

            last = 0;
        }
        else
        {
            /* Set Full Resolution mode */
            ret = JpegEncSetFullResolutionMode(encoder, &cfg);

            /* If no slice mode, the slice equals whole frame */
            if(cmdl.partialCoding == 0)
                sliceRows = cmdl.lumHeightSrc;
            else
                sliceRows = cmdl.restartInterval * (cmdl.codingMode ? 8 : 16);

            widthSrc = cmdl.lumWidthSrc;
            heightSrc = cmdl.lumHeightSrc;

            input = cmdl.input;

            last = cmdl.lastVop;
        }

        /* Handle error situation */
        if(ret != JPEGENC_OK)
        {
#ifndef ASIC_WAVE_TRACE_TRIGGER
            printf("FAILED. Error code: %i\n", ret);
#endif
            goto end;
        }

        if(cmdl.frameType <= 1)     /* YUV 420 planar / semiplanar */
        {
            /* Bus addresses of input picture, used by hardware encoder */
            encIn.busLum = pictureMem.busAddress;
            encIn.busCb = encIn.busLum + (widthSrc * sliceRows);
            encIn.busCr = encIn.busCb +
                (((widthSrc + 1) / 2) * ((sliceRows + 1) / 2));

            /* Virtual addresses of input picture, used by software encoder */
            encIn.pLum = (u8 *)pictureMem.virtualAddress;
            encIn.pCb = encIn.pLum + (widthSrc * sliceRows);
            encIn.pCr = encIn.pCb +
                (((widthSrc + 1) / 2) * ((sliceRows + 1) / 2));
        }
        else if(cmdl.frameType == 10)   /* YUV 422 planar */
        {
            /* Bus addresses of input picture, used by hardware encoder */
            encIn.busLum = pictureMem.busAddress;
            encIn.busCb = encIn.busLum + (widthSrc * sliceRows);
            encIn.busCr = encIn.busCb + ((widthSrc + 1) / 2) * sliceRows;

            /* Virtual addresses of input picture, used by software encoder */
            encIn.pLum = (u8 *)pictureMem.virtualAddress;
            encIn.pCb = encIn.pLum + (widthSrc * sliceRows);
            encIn.pCr = encIn.pCb + ((widthSrc + 1) / 2) * sliceRows;
        }
        else    /* Interleaved YUYV or RGB format */
        {
            /* Bus addresses of input picture, used by hardware encoder */
            encIn.busLum = pictureMem.busAddress;
            encIn.busCb = encIn.busLum;
            encIn.busCr = encIn.busCb;

            /* Virtual addresses of input picture, used by software encoder */
            encIn.pLum = (u8 *)pictureMem.virtualAddress;
            encIn.pCb = encIn.pLum;
            encIn.pCr = encIn.pCb;
        }

        /* Main encoding loop */
        ret = JPEGENC_FRAME_READY;
        next = cmdl.firstVop;
        while(next <= last &&
              (ret == JPEGENC_FRAME_READY ||
               ret == JPEGENC_OUTPUT_BUFFER_OVERFLOW))
        {
#ifdef SEPARATE_FRAME_OUTPUT
            char framefile[50];

            sprintf(framefile, "frame%d%s.jpg", vopCnt, mode == 1 ? "tn" : "");
            remove(framefile);
#endif

#ifndef ASIC_WAVE_TRACE_TRIGGER
            printf("Frame %3d started...\n", vopCnt);
#endif
            fflush(stdout);

            /* Loop until one frame is encoded */
            do
            {
#ifndef NO_INPUT_YUV
                /* Read next slice */
                if(ReadVop
                   ((u8 *) pictureMem.virtualAddress,
                    widthSrc, heightSrc, slice,
                    sliceRows, next, input, cmdl.frameType) != 0)
                    break;
#endif
                ret = JpegEncEncode(encoder, &encIn, &encOut);

                switch (ret)
                {
                case JPEGENC_RESTART_INTERVAL:

#ifndef ASIC_WAVE_TRACE_TRIGGER
                    printf("Frame %3d restart interval! %6u bytes\n",
                           vopCnt, encOut.jfifSize);
                    fflush(stdout);
#endif

                    /* In thumbnail mode the slices are not needed to write
                     * separately, the whole picture is output when
                     * frame ready is returned */
                    if(writeOutput && (!cmdl.thumbnail))
                        WriteStrm(fout, outbufMem.virtualAddress, 
                                encOut.jfifSize, 0);
#ifdef SEPARATE_FRAME_OUTPUT
                    if(writeOutput)
                        WriteFrame(framefile, outbufMem.virtualAddress, 
                                encOut.jfifSize, mode);
#endif
                    vopBytes += encOut.jfifSize;
                    slice++;    /* Encode next slice */
                    break;

                case JPEGENC_FRAME_READY:
#ifndef ASIC_WAVE_TRACE_TRIGGER
                    printf("Frame %3d ready! %6u bytes\n",
                           vopCnt, encOut.jfifSize);
                    fflush(stdout);
#endif

                    /* Thumbnail is not needed to write separately,
                     * it is included in the full resolution picture */
                    if(writeOutput && (mode == 0))
                        WriteStrm(fout, outbufMem.virtualAddress, 
                                encOut.jfifSize, 0);
#ifdef SEPARATE_FRAME_OUTPUT
                    if(writeOutput)
                        WriteFrame(framefile, outbufMem.virtualAddress, 
                                encOut.jfifSize, mode);
#endif
                    vopBytes = 0;
                    slice = 0;
                    break;

                case JPEGENC_OUTPUT_BUFFER_OVERFLOW:

#ifndef ASIC_WAVE_TRACE_TRIGGER
                    printf("Frame %3d lost! Output buffer overflow.\n",
                           vopCnt);
#endif

                    /* For debugging */
                    if(writeOutput)
                        WriteStrm(fout, outbufMem.virtualAddress, 
                                outbufMem.size, 0);
                    /* Rewind the file back this vop's bytes 
                    fseek(fout, -vopBytes, SEEK_CUR);*/
                    vopBytes = 0;
                    slice = 0;
                    break;

                default:

#ifndef ASIC_WAVE_TRACE_TRIGGER
                    printf("FAILED. Error code: %i\n", ret);
#endif
                    /* For debugging */
                    if(writeOutput)
                        WriteStrm(fout, outbufMem.virtualAddress,
                                encOut.jfifSize, 0);
                    break;
                }
            }
            while(ret == JPEGENC_RESTART_INTERVAL);

            vopCnt++;
            next = vopCnt + cmdl.firstVop;
        }   /* End of main encoding loop */

    }   /* End of encoding modes */

end:

#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("Release encoder\n");
#endif

#ifdef TRACE_STREAM
    EncCloseStreamTrace();
#endif

    /* Free all resources */
    CloseEncoder(encoder);
    if(fout != NULL)
        fclose(fout);
    FreeRes(encoder);

    return 0;
}

/*------------------------------------------------------------------------------

    AllocRes

    Allocation of the physical memories used by both SW and HW: 
    the input picture and the output stream buffer.

    NOTE! The implementation uses the EWL instance from the encoder
          for OS independence. This is not recommended in final environment 
          because the encoder will release the EWL instance in case of error.
          Instead, the memories should be allocated from the OS the same way
          as inside EWLMallocLinear().

------------------------------------------------------------------------------*/
int AllocRes(commandLine_s * cmdl, JpegEncInst enc)
{
    i32 sliceRows = 0;
    u32 pictureSize;
    u32 outbufSize;
    i32 ret;

    /* Set slice size and output buffer size
     * For output buffer size, 1 byte/pixel is enough for most images.
     * Some extra is needed for testing purposes (noise input) */

    if(cmdl->partialCoding == 0)
    {
        sliceRows = cmdl->lumHeightSrc;
    }
    else
    {
        sliceRows = cmdl->restartInterval * (cmdl->codingMode ? 8 : 16);
    }
            
    outbufSize = cmdl->width * sliceRows * 2 + 8192;

    if(cmdl->thumbnail)
        outbufSize += cmdl->widthThumb * cmdl->heightThumb;

    /* calculate picture size */
    if(cmdl->frameType <= 1)
    {
        /* Input picture in YUV 4:2:0 format */
        pictureSize = cmdl->lumWidthSrc * sliceRows * 3 / 2;
    }
    else
    {
        /* Input picture in YUYV 4:2:2 format */
        pictureSize = cmdl->lumWidthSrc * sliceRows * 2;
    }

     /* Here we use the EWL instance directly from the encoder 
     * because it is the easiest way to allocate the linear memories */
    ret = EWLMallocLinear(((jpegInstance_s *)enc)->asic.ewl, pictureSize, 
                &pictureMem);
    if (ret != EWL_OK)
        return 1;

    /* this is limitation for FPGA testing */
    outbufSize = outbufSize < (1024*1024*7) ? outbufSize : (1024*1024*7);

    ret = EWLMallocLinear(((jpegInstance_s *)enc)->asic.ewl, outbufSize, 
                &outbufMem);
    if (ret != EWL_OK)
        return 1;

#ifndef ASIC_WAVE_TRACE_TRIGGER
    if(!cmdl->thumbnail)
        printf("Input %dx%d encoding at %dx%d ", cmdl->lumWidthSrc,
               cmdl->lumHeightSrc, cmdl->width, cmdl->height);
    else
        printf("Input %dx%d + %dx%d encoding at %dx%d + %dx%d ",
               cmdl->lumWidthSrcThumb, cmdl->lumHeightSrcThumb,
               cmdl->lumWidthSrc, cmdl->lumHeightSrc,
               cmdl->widthThumb, cmdl->heightThumb, cmdl->width, cmdl->height);

    if(cmdl->partialCoding != 0)
        printf("in slices of %dx%d", cmdl->width, sliceRows);
    printf("\n");
#endif


#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("Input buffer size:          %d bytes\n", pictureMem.size);
    printf("Input buffer bus address:   0x%08x\n", pictureMem.busAddress);
    printf("Input buffer user address:  %10p\n", pictureMem.virtualAddress);
    printf("Output buffer size:         %d bytes\n", outbufMem.size);
    printf("Output buffer bus address:  0x%08x\n", outbufMem.busAddress);
    printf("Output buffer user address: %10p\n",  outbufMem.virtualAddress);
#endif

    return 0;
}

/*------------------------------------------------------------------------------

    FreeRes

------------------------------------------------------------------------------*/
void FreeRes(JpegEncInst enc)
{
    EWLFreeLinear(((jpegInstance_s *)enc)->asic.ewl, &pictureMem); 
    EWLFreeLinear(((jpegInstance_s *)enc)->asic.ewl, &outbufMem); 
}

/*------------------------------------------------------------------------------

    OpenEncoder

------------------------------------------------------------------------------*/
int OpenEncoder(commandLine_s * cml, JpegEncInst * pEnc)
{
    JpegEncRet ret;
    JpegEncInst encoder;

#ifndef TB_DEFINED_COMMENT
    FILE *fileCom = NULL;
#endif

    /* Encoder initialization */
    if(cml->width == DEFAULT)
        cml->width = cml->lumWidthSrc;

    if(cml->height == DEFAULT)
        cml->height = cml->lumHeightSrc;

    cfg.rotation = (JpegEncPictureRotation)cml->rotation;
    cfg.inputWidth = (cml->lumWidthSrc + 15) & (~15);   /* API limitation */
    cfg.inputHeight = cml->lumHeightSrc;
    cfg.cfgThumb.inputWidth = (cml->lumWidthSrcThumb + 15) & (~15); /* API limitation */
    cfg.cfgThumb.inputHeight = cml->lumHeightSrcThumb;

    if(cfg.rotation)
    {
        /* full */
        cfg.xOffset = cml->verOffsetSrc;
        cfg.yOffset = cml->horOffsetSrc;

        cfg.codingWidth = cml->height;
        cfg.codingHeight = cml->width;
        cfg.xDensity = cml->ydensity;
        cfg.yDensity = cml->xdensity;

        /* thumbnail */
        cfg.cfgThumb.codingWidth = cml->heightThumb;
        cfg.cfgThumb.codingHeight = cml->widthThumb;

        cfg.cfgThumb.xOffset = cml->verOffsetSrcThumb;
        cfg.cfgThumb.yOffset = cml->horOffsetSrcThumb;

    }
    else
    {
        /* full */
        cfg.xOffset = cml->horOffsetSrc;
        cfg.yOffset = cml->verOffsetSrc;

        cfg.codingWidth = cml->width;
        cfg.codingHeight = cml->height;
        cfg.xDensity = cml->xdensity;
        cfg.yDensity = cml->ydensity;

        /* thumbnail */
        cfg.cfgThumb.codingWidth = cml->widthThumb;
        cfg.cfgThumb.codingHeight = cml->heightThumb;

        cfg.cfgThumb.xOffset = cml->horOffsetSrcThumb;
        cfg.cfgThumb.yOffset = cml->verOffsetSrcThumb;
    }

    cfg.qLevel = cml->qLevel;
    cfg.restartInterval = cml->restartInterval;
    cfg.codingType = (JpegEncCodingType)cml->partialCoding;
    cfg.frameType = (JpegEncFrameType)cml->frameType;
    cfg.unitsType = (JpegEncAppUnitsType)cml->unitsType;
    cfg.markerType = (JpegEncTableMarkerType)cml->markerType;
    cfg.colorConversion.type = (JpegEncColorConversionType)cml->colorConversion;
    if (cfg.colorConversion.type == JPEGENC_RGBTOYUV_USER_DEFINED)
    {
        /* User defined RGB to YCbCr conversion coefficients, scaled by 16-bits */
        cfg.colorConversion.coeffA = 20000;
        cfg.colorConversion.coeffB = 44000;
        cfg.colorConversion.coeffC = 5000;
        cfg.colorConversion.coeffE = 35000;
        cfg.colorConversion.coeffF = 38000;
    }
    writeOutput = cml->write;
    cfg.codingMode = (JpegEncCodingMode)cml->codingMode;

#ifdef NO_OUTPUT_WRITE
    writeOutput = 0;
#endif

/* use either "hard-coded"/testbench COM data or user specific */
#ifdef TB_DEFINED_COMMENT
    cfg.comLength = comLen;
    cfg.pCom = comment;
#else
    cfg.comLength = cml->comLength;

    if(cfg.comLength)
    {
        /* allocate mem for & read comment data */
        cfg.pCom = (u8 *) malloc(cfg.comLength);

        fileCom = fopen(cml->com, "rb");
        if(fileCom == NULL)
        {
            fprintf(stderr, "\nUnable to open COMMENT file: %s\n", cml->com);
            return -1;
        }

        fread(cfg.pCom, 1, cfg.comLength, fileCom);
        fclose(fileCom);
    }

#endif

    cfg.thumbnail = cml->thumbnail;

#ifndef ASIC_WAVE_TRACE_TRIGGER
    fprintf(stdout, "Init config: %dx%d + %dx%d => %dx%d   \n",
            cfg.inputWidth, cfg.inputHeight, cfg.xOffset, cfg.yOffset,
            cfg.codingWidth, cfg.codingHeight);

    fprintf(stdout,
            "\n\t**********************************************************\n");
    fprintf(stdout, "\n\t-JPEG: ENCODER CONFIGURATION\n");
    fprintf(stdout, "\t-JPEG: qp \t\t:%d\n", cfg.qLevel);
    fprintf(stdout, "\t-JPEG: inX \t\t:%d\n", cfg.inputWidth);
    fprintf(stdout, "\t-JPEG: inY \t\t:%d\n", cfg.inputHeight);
    fprintf(stdout, "\t-JPEG: outX \t\t:%d\n", cfg.codingWidth);
    fprintf(stdout, "\t-JPEG: outY \t\t:%d\n", cfg.codingHeight);
    fprintf(stdout, "\t-JPEG: rst \t\t:%d\n", cfg.restartInterval);
    fprintf(stdout, "\t-JPEG: xOff \t\t:%d\n", cfg.xOffset);
    fprintf(stdout, "\t-JPEG: yOff \t\t:%d\n", cfg.yOffset);
    fprintf(stdout, "\t-JPEG: frameType \t:%d\n", cfg.frameType);
    fprintf(stdout, "\t-JPEG: colorConversionType :%d\n", cfg.colorConversion.type);
    fprintf(stdout, "\t-JPEG: colorConversionA    :%d\n", cfg.colorConversion.coeffA);
    fprintf(stdout, "\t-JPEG: colorConversionB    :%d\n", cfg.colorConversion.coeffB);
    fprintf(stdout, "\t-JPEG: colorConversionC    :%d\n", cfg.colorConversion.coeffC);
    fprintf(stdout, "\t-JPEG: colorConversionE    :%d\n", cfg.colorConversion.coeffE);
    fprintf(stdout, "\t-JPEG: colorConversionF    :%d\n", cfg.colorConversion.coeffF);
    fprintf(stdout, "\t-JPEG: rotation \t:%d\n", cfg.rotation);
    fprintf(stdout, "\t-JPEG: codingType \t:%d\n", cfg.codingType);
    fprintf(stdout, "\t-JPEG: codingMode \t:%d\n", cfg.codingMode);
    fprintf(stdout, "\t-JPEG: markerType \t:%d\n", cfg.markerType);
    fprintf(stdout, "\t-JPEG: units \t\t:%d\n", cfg.unitsType);
    fprintf(stdout, "\t-JPEG: xDen \t\t:%d\n", cfg.xDensity);
    fprintf(stdout, "\t-JPEG: yDen \t\t:%d\n", cfg.yDensity);
    fprintf(stdout, "\t-JPEG: thumb \t\t:%d\n", cfg.thumbnail);
    fprintf(stdout, "\t-JPEG: inX thumbnail\t:%d\n", cfg.cfgThumb.inputWidth);
    fprintf(stdout, "\t-JPEG: inY thumbnail\t:%d\n", cfg.cfgThumb.inputHeight);
    fprintf(stdout, "\t-JPEG: outX thumbnail\t:%d\n", cfg.cfgThumb.codingWidth);
    fprintf(stdout, "\t-JPEG: outY thumbnail\t:%d\n",
            cfg.cfgThumb.codingHeight);
    fprintf(stdout, "\t-JPEG: xOff thumbnail\t:%d\n", cfg.cfgThumb.xOffset);
    fprintf(stdout, "\t-JPEG: yOff thumbnail\t:%d\n", cfg.cfgThumb.yOffset);

    fprintf(stdout, "\t-JPEG: First vop\t:%d\n", cml->firstVop);
    fprintf(stdout, "\t-JPEG: Last vop\t\t:%d\n", cml->lastVop);
#ifdef TB_DEFINED_COMMENT
    fprintf(stdout, "\n\tNOTE! Using comment values defined in testbench!\n");
#else
    fprintf(stdout, "\t-JPEG: comlen \t\t:%d\n", cfg.comLength);
    fprintf(stdout, "\t-JPEG: COM \t\t:%s\n", cfg.pCom);
#endif
    fprintf(stdout,
            "\n\t**********************************************************\n\n");
#endif

    if((ret = JpegEncInit(&cfg, pEnc)) != JPEGENC_OK)
    {
        fprintf(stderr,
                "Failed to initialize the encoder. Error code: %8i\n", ret);
        return -1;
    }

    encoder = *pEnc;

    return 0;
}

/*------------------------------------------------------------------------------

    CloseEncoder

------------------------------------------------------------------------------*/
void CloseEncoder(JpegEncInst encoder)
{
    JpegEncRet ret;

    if((ret = JpegEncRelease(encoder)) != JPEGENC_OK)
    {
        fprintf(stderr,
                "Failed to release the encoder. Error code: %8i\n", ret);
    }
}

/*------------------------------------------------------------------------------

    Parameter

------------------------------------------------------------------------------*/
int Parameter(i32 argc, char **argv, commandLine_s * cml)
{
    i32 ret;
    char *optarg;
    argument_s argument;
    int status = 0;

    memset(cml, DEFAULT, sizeof(commandLine_s));
    strcpy(cml->input, "input.yuv");
    strcpy(cml->inputThumb, "inputThumbnail.yuv");
    strcpy(cml->com, "com.txt");
    strcpy(cml->output, "stream.jpg");
    cml->firstVop = 0;
    cml->lastVop = 0;
    cml->lumWidthSrc = 176;
    cml->lumHeightSrc = 144;
    cml->width = DEFAULT;
    cml->height = DEFAULT;
    cml->horOffsetSrc = 0;
    cml->verOffsetSrc = 0;
    cml->qLevel = 1;
    cml->restartInterval = 0;
    cml->thumbnail = 0;
    cml->lumWidthSrcThumb = 0;
    cml->lumHeightSrcThumb = 0;
    cml->widthThumb = 0;
    cml->heightThumb = 0;
    cml->horOffsetSrcThumb = 0;
    cml->verOffsetSrcThumb = 0;
    cml->frameType = 0;
    cml->colorConversion = 0;
    cml->rotation = 0;
    cml->partialCoding = 0;
    cml->codingMode = 0;
    cml->markerType = 0;
    cml->unitsType = 0;
    cml->xdensity = 1;
    cml->ydensity = 1;
    cml->write = 1;
    cml->comLength = 0;

    argument.optCnt = 1;
    while((ret = EncGetOption(argc, argv, options, &argument)) != -1)
    {
        if(ret == -2)
        {
            status = -1;
        }
        optarg = argument.optArg;
        switch (argument.shortOpt)
        {
        case 'H':
            Help();
            exit(0);
        case 'i':
            if(strlen(optarg) < MAX_PATH)
            {
                strcpy(cml->input, optarg);
            }
            else
            {
                status = -1;
            }
            break;
        case 'I':
            if(strlen(optarg) < MAX_PATH)
            {
                strcpy(cml->inputThumb, optarg);
            }
            else
            {
                status = -1;
            }
            break;
        case 'o':
            if(strlen(optarg) < MAX_PATH)
            {
                strcpy(cml->output, optarg);
            }
            else
            {
                status = -1;
            }
            break;
        case 'C':
            if(strlen(optarg) < MAX_PATH)
            {
                strcpy(cml->com, optarg);
            }
            else
            {
                status = -1;
            }
            break;
        case 'a':
            cml->firstVop = atoi(optarg);
            break;
        case 'b':
            cml->lastVop = atoi(optarg);
            break;
        case 'x':
            cml->width = atoi(optarg);
            break;
        case 'y':
            cml->height = atoi(optarg);
            break;
        case 'w':
            cml->lumWidthSrc = atoi(optarg);
            break;
        case 'h':
            cml->lumHeightSrc = atoi(optarg);
            break;
        case 'X':
            cml->horOffsetSrc = atoi(optarg);
            break;
        case 'Y':
            cml->verOffsetSrc = atoi(optarg);
            break;
        case 'R':
            cml->restartInterval = atoi(optarg);
            break;
        case 'q':
            cml->qLevel = atoi(optarg);
            break;
        case 'g':
            cml->frameType = atoi(optarg);
            break;
        case 'v':
            cml->colorConversion = atoi(optarg);
            break;
        case 'G':
            cml->rotation = atoi(optarg);
            break;
        case 'p':
            cml->partialCoding = atoi(optarg);
            break;
        case 'm':
            cml->codingMode = atoi(optarg);
            break;
        case 't':
            cml->markerType = atoi(optarg);
            break;
        case 'u':
            cml->unitsType = atoi(optarg);
            break;
        case 'k':
            cml->xdensity = atoi(optarg);
            break;
        case 'l':
            cml->ydensity = atoi(optarg);
            break;
        case 'T':
            cml->thumbnail = atoi(optarg);
            break;
        case 'K':
            cml->widthThumb = atoi(optarg);
            break;
        case 'L':
            cml->heightThumb = atoi(optarg);
            break;
        case 'B':
            cml->lumWidthSrcThumb = atoi(optarg);
            break;
        case 'A':
            cml->horOffsetSrcThumb = atoi(optarg);
            break;
        case 'O':
            cml->verOffsetSrcThumb = atoi(optarg);
            break;
        case 'e':
            cml->lumHeightSrcThumb = atoi(optarg);
            break;
        case 'W':
            cml->write = atoi(optarg);
            break;
        case 'c':
            cml->comLength = atoi(optarg);
            break;
        case 'P':
            trigger_point = atoi(optarg);
            break;
        case -1:
            break;
        default:
            break;
        }
    }

    return status;
}

/*------------------------------------------------------------------------------

    ReadVop

    Read raw YUV image data from file
    Image is divided into slices, each slice consists of equal amount of 
    image rows except for the bottom slice which may be smaller than the 
    others. sliceNum is the number of the slice to be read
    and sliceRows is the amount of rows in each slice (or 0 for all rows).

------------------------------------------------------------------------------*/
int ReadVop(u8 * image, i32 width, i32 height, i32 sliceNum, i32 sliceRows,
            i32 frameNum, char *name, u32 inputMode)
{
    FILE *file = NULL;
    i32 frameSize;
    i32 frameOffset;
    i32 sliceLumOffset = 0;
    i32 sliceCbOffset = 0;
    i32 sliceCrOffset = 0;
    i32 sliceLumSize;        /* The size of one slice in bytes */
    i32 sliceCbSize;
    i32 sliceCrSize;
    i32 sliceLumSizeRead;    /* The size of the slice to be read */
    i32 sliceCbSizeRead;
    i32 sliceCrSizeRead;

    if(sliceRows == 0)
    {
        sliceRows = height;
    }

    if(inputMode == 0)
    {
        /* YUV 4:2:0 planar */
        frameSize = width * height + (width / 2 * height / 2) * 2;
        sliceLumSizeRead = sliceLumSize = width * sliceRows;
        sliceCbSizeRead = sliceCbSize = width / 2 * sliceRows / 2;
        sliceCrSizeRead = sliceCrSize = width / 2 * sliceRows / 2;
    }
    else if(inputMode == 1)
    {
        /* YUV 4:2:0 semiplanar */
        frameSize = width * height + (width / 2 * height / 2) * 2;
        sliceLumSizeRead = sliceLumSize = width * sliceRows;
        sliceCbSizeRead = sliceCbSize = width * sliceRows / 2;
        sliceCrSizeRead = sliceCrSize = 0;
    }
    else if(inputMode == 10)
    {
        /* YUV 4:2:2 planar */
        frameSize = width * height + (width / 2 * height) * 2;
        sliceLumSizeRead = sliceLumSize = width * sliceRows;
        sliceCbSizeRead = sliceCbSize = width / 2 * sliceRows;
        sliceCrSizeRead = sliceCrSize = width / 2 * sliceRows;
    }
    else
    {
        /* YUYV 4:2:2, this includes both luminance and chrominance */
        frameSize = width * height * 2;
        sliceLumSizeRead = sliceLumSize = width * sliceRows * 2;
        sliceCbSizeRead = sliceCbSize = 0;
        sliceCrSizeRead = sliceCrSize = 0;
    }

    /* The bottom slice may be smaller than the others */
    if(sliceRows * (sliceNum + 1) > height)
    {
        sliceRows = height - sliceRows * sliceNum;

        if(inputMode == 0) {
            sliceLumSizeRead = width * sliceRows;
            sliceCbSizeRead = width / 2 * sliceRows / 2;
            sliceCrSizeRead = width / 2 * sliceRows / 2;
        } else if(inputMode == 1) {
            sliceLumSizeRead = width * sliceRows;
            sliceCbSizeRead = width * sliceRows / 2;
        } else if(inputMode == 10) {
            sliceLumSizeRead = width * sliceRows;
            sliceCbSizeRead = width / 2 * sliceRows;
            sliceCrSizeRead = width / 2 * sliceRows;
        } else {
            sliceLumSizeRead = width * sliceRows * 2;
        }
    }

    /* Offset for frame start from start of file */
    frameOffset = frameSize * frameNum;
    /* Offset for slice luma start from start of frame */
    sliceLumOffset = sliceLumSize * sliceNum;
    /* Offset for slice cb start from start of frame */
    if(inputMode <= 1 || inputMode == 10)
        sliceCbOffset = width * height + sliceCbSize * sliceNum;
    /* Offset for slice cr start from start of frame */
    if(inputMode == 0)
        sliceCrOffset = width * height +
                        width/2 * height/2 + sliceCrSize * sliceNum;
    else if(inputMode == 10)
        sliceCrOffset = width * height +
                        width/2 * height + sliceCrSize * sliceNum;

    /* Read input from file frame by frame */
#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("Reading frame %d slice %d (%d bytes) from %s... ",
           frameNum, sliceNum, 
           sliceLumSizeRead + sliceCbSizeRead + sliceCrSizeRead, name);
    fflush(stdout);
#endif

    file = fopen(name, "rb");
    if(file == NULL)
    {
        fprintf(stderr, "\nUnable to open VOP file: %s\n", name);
        return -1;
    }

    fseek(file, frameOffset + sliceLumOffset, SEEK_SET);
    fread(image, 1, sliceLumSizeRead, file);
    if (sliceCbSizeRead) 
    {
        fseek(file, frameOffset + sliceCbOffset, SEEK_SET);
        fread(image + sliceLumSize, 1, sliceCbSizeRead, file);
    }
    if (sliceCrSizeRead) 
    {
        fseek(file, frameOffset + sliceCrOffset, SEEK_SET);
        fread(image + sliceLumSize + sliceCbSize, 1, sliceCrSizeRead, file);
    }

    /* Stop if last VOP of the file */
    if(feof(file))
    {
        fprintf(stderr, "\nI can't read VOP no: %d ", frameNum);
        fprintf(stderr, "from file: %s\n", name);
        fclose(file);
        return -1;
    }

#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("OK\n");
    fflush(stdout);
#endif

    fclose(file);

    return 0;
}

/*------------------------------------------------------------------------------

    Help

------------------------------------------------------------------------------*/
void Help(void)
{
    fprintf(stdout, "Usage:  %s [options] -i inputfile\n", "jpeg_testenc");
    fprintf(stdout,
            "  -H    --help              Display this help.\n"
            "  -W[n] --write             0=NO, 1=YES write output. [1]\n"
            "  -i[s] --input             Read input from file. [input.yuv]\n"
            "  -I[s] --inputThumb        Read thumbnail input from file. [inputThumbnail.yuv]\n"
            "  -o[s] --output            Write output to file. [stream.jpg]\n"
            "  -a[n] --firstVop          First vop of input file. [0]\n"
            "  -b[n] --lastVop           Last vop of input file. [0]\n"
            "  -w[n] --lumWidthSrc       Width of source image. [176]\n"
            "  -h[n] --lumHeightSrc      Height of source image. [144]\n");
    fprintf(stdout,
            "  -x[n] --width             Width of output image. [--lumWidthSrc]\n"
            "  -y[n] --height            Height of output image. [--lumHeightSrc]\n"
            "  -X[n] --horOffsetSrc      Output image horizontal offset. [0]\n"
            "  -Y[n] --verOffsetSrc      Output image vertical offset. [0]\n");
    fprintf(stdout,
            "  -R[n] --restartInterval   Restart interval in MCU rows. [0]\n"
            "  -q[n] --qLevel            0..10, quantization scale. [1]\n"
            "  -g[n] --frameType         Input format. [0]\n"
            "                               0 - YUV420\n"
            "                               1 - YUV420 semiplanar\n"
            "                               2 - YUYV422\n"
            "                               3 - UYVY422\n"
            "                               4 - RGB565\n"
            "                               5 - BRG565\n"
            "                               6 - RGB555\n"
            "                               7 - BRG555\n"
            "                               8 - RGB444\n"
            "                               9 - BGR444\n"
            "                               10 - YUV422 planar\n"
            "  -v[n] --colorConversion   RGB to YCbCr color conversion type. [0]\n"
            "                               0 - BT.601\n"
            "                               1 - BT.709\n"
            "                               2 - User defined\n"
            "  -G[n] --rotation          Rotate input image. [0]\n"
            "                               0 - disabled\n"
            "                               1 - 90 degrees right\n"
            "                               2 - 90 degrees right\n"
            "  -p[n] --codingType        0=whole frame, 1=partial frame encoding. [0]\n"
            "  -m[n] --codingMode        0=YUV 4:2:0, 1=YUV 4:2:2. [0]\n"
            "  -t[n] --markerType        Quantization/Huffman table markers. [0]\n"
            "                               0 = single marker\n"
            "                               1 = multi marker\n"
            "  -u[n] --units             Units type of x- and y-density. [0]\n"
            "                               0 = pixel aspect ratio\n"
            "                               1 = dots/inch\n"
            "                               2 = dots/cm\n"
            "  -k[n] --xdensity          Xdensity to APP0 header. [1]\n"
            "  -l[n] --ydensity          Ydensity to APP0 header. [1]\n");
    fprintf(stdout,
            "  -T[n] --thumbnail         0=NO, 1=YES Thumbnail to stream. [0]\n"
            "  -B[n] --lumWidthSrcThumb  Width of thumbnail source image. [176]\n"
            "  -e[n] --lumHeightSrcThumb Height of thumbnail source image. [144]\n"
            "  -K[n] --widthThumb        Width of thumbnail output image. [--lumWidthSrcThumb]\n"
            "  -L[n] --heightThumb       Height of thumbnail output image. [--lumHeightSrcThumb]\n"
            "  -A[n] --horOffsetSrcThumb Thumbnail output image horizontal offset. [0]\n"
            "  -O[n] --verOffsetSrcThumb Thumbnail output image vertical offset. [0]\n");
#ifdef TB_DEFINED_COMMENT
    fprintf(stdout, 
            "\n   Using comment values defined in testbench!\n");
#else
    fprintf(stdout,
            "  -c[n] --comLength         Comment header data length. [0]\n"
            "  -C[s] --comFile           Comment header data file. [com.txt]\n");
#endif
    fprintf(stdout,
            "\nTesting parameters that are not supported for end-user:\n"
            "  -P[n] --trigger           Logic Analyzer trigger at picture <n>. [-1]\n"
            "\n");
}

/*------------------------------------------------------------------------------

    Write encoded stream to file

------------------------------------------------------------------------------*/
void WriteStrm(FILE * fout, u32 * strmbuf, u32 size, u32 endian)
{

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

#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("Writing stream (%i bytes)... ", size);
    fflush(stdout);
#endif

    fwrite(strmbuf, 1, size, fout);

#ifndef ASIC_WAVE_TRACE_TRIGGER
    printf("OK\n");
    fflush(stdout);
#endif

}

/*------------------------------------------------------------------------------

    Write encoded frame to file

------------------------------------------------------------------------------*/
void WriteFrame(char *filename, u32 * strmbuf, u32 size, u32 mode)
{
    u8 *startOfThumb = NULL;
    u32 sizeThumb = 0;

    FILE *fp;

    fp = fopen(filename, "ab");

    if(!mode)   /* Full resolution mode */
    {
        if(fp)
        {
            fwrite(strmbuf, 1, size, fp);
            fclose(fp);
        }
    }
    else    /* Thumbnail mode */
    {
        /* flush APP0 stuff and set pointer for thumbnail */
        sizeThumb = size - 0x1e;
        startOfThumb = (u8 *) strmbuf + 30;

        if(fp)
        {
            fwrite(startOfThumb, 1, sizeThumb, fp);
            fclose(fp);
        }
    }
}

/*------------------------------------------------------------------------------

    API tracing

------------------------------------------------------------------------------*/
void JpegEnc_Trace(const char *msg)
{
    static FILE *fp = NULL;

    if(fp == NULL)
        fp = fopen("api.trc", "wt");

    if(fp)
        fprintf(fp, "%s\n", msg);
}