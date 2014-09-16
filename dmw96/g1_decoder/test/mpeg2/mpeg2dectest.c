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
--  Abstract : MPEG-2 Decoder Testbench
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifndef MPEG2DEC_EXTERNAL_ALLOC_DISABLE
#include <fcntl.h>
#include <sys/mman.h>
#endif

#include "dwl.h"
#include "mpeg2decapi.h"
#ifdef USE_EFENCE
#include "efence.h"
#endif

#include "mpeg2hwd_container.h"
#include "regdrv.h"

#ifdef PP_PIPELINE_ENABLED
#include "ppapi.h"
#include "pptestbench.h"
#endif

#ifdef ASIC_TRACE_SUPPORT
#include "trace.h"
#endif

#include "tb_cfg.h"
#include "tb_tiled.h"

#ifdef MD5SUM
#include "tb_md5.h"
#endif

#include "tb_sw_performance.h"

/*#define DISABLE_WHOLE_STREAM_BUFFERING 0*/
#define DEFAULT -1
#define VOP_START_CODE 0xb6010000

/* Size of stream buffer */
#define STREAMBUFFER_BLOCKSIZE 0xfffff
#define MPEG2_WHOLE_STREAM_SAFETY_LIMIT (10*10*1024)

#define MPEG2_FRAME_BUFFER_SIZE ((1280 * 720 * 3) / 2)  /* 720p frame */
#define MPEG2_NUM_BUFFERS 3 /* number of output buffers for ext alloc */

/* Function prototypes */

void printTimeCode(Mpeg2DecTime * timecode);
static u32 readDecodeUnit(FILE * fp, u8 * frameBuffer);
static void Error(char *format, ...);
void decRet(Mpeg2DecRet ret);
void decNextPictureRet(Mpeg2DecRet ret);
void printMpeg2Version(void);
i32 AllocatePicBuffers(Mpeg2DecLinearMem * buffer, DecContainer * container);

void decsw_performance(void)
{
}

void WriteOutput(char *filename, char *filenameTiled, u8 * data,
                 u32 frameNumber, u32 width, u32 height, u32 interlaced,
                 u32 top, u32 firstField, u32 output_picture_endian,
                 Mpeg2DecPicture DecPicture, u32 codedHeight, u32 tiledMode);
		 
#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 frameNumber, Mpeg2DecPicture DecPicture,
                    Mpeg2DecInst decoder);
#endif


/* stream start address */
u8 *byteStrmStart;
u32 bFrames;

/* stream used in SW decode */
u32 traceUsedStream = 0;
u32 previousUsed = 0;

/* SW/SW testing, read stream trace file */
FILE *fStreamTrace = NULL;

/* output file */
FILE *fout = NULL;
FILE *fTiledOutput = NULL;
static u32 StartCode;

i32 strmRew = 0;
u32 length;
u32 writeOutput = 1;
u32 cropOutput = 0;
u8 disableResync = 0;
u8 strmEnd = 0;
u8 *streamStop = NULL;

u32 streamSize = 0;
u32 stopDecoding = 0;
u32 streamTruncate = 0;
u32 streamPacketLoss = 0;
u32 hdrsRdy = 0;
u32 picRdy = 0;
u32 streamHeaderCorrupt = 0;    
TBCfg tbCfg;

/* Give stream to decode as one chunk */
u32 wholeStreamMode = 0;
u32 cumulativeErrorMbs = 0;

u32 planarOutput = 0;
u32 bFrames;
u32 interlacedField = 0;

u32 usePeekOutput = 0;
u32 skipNonReference = 0;

u32 picDisplayNumber = 0;
u32 frameNumber = 0;

u32 tiledOutput = DEC_REF_FRM_RASTER_SCAN;
u32 dpbMode = DEC_DPB_DEFAULT;
u32 convertToFrameDpb = 0;
u32 convertTiledOutput = 0;

FILE *findex = NULL;
u32 saveIndex = 0;
u32 useIndex = 0;
off64_t curIndex = 0;
off64_t nextIndex = 0;

#ifdef ASIC_TRACE_SUPPORT
extern u32 hwDecPicCount;
extern u32 useMpeg2Idct;
extern u32 gHwVer;
#endif

#ifdef MPEG2_EVALUATION
extern u32 gHwVer;
#endif

int main(int argc, char **argv)
{
    FILE *fTBCfg;
    u8 *pStrmData = 0;
    u32 maxNumFrames;   /* todo! */
    u32 numFrameBuffers = 0;
    u8 *imageData;
    u32 picSize = 0;

    u32 i, streamLen = 0;
    u32 outp_byte_size;
    u32 vpNum = 0;
    u32 allocateBuffers = 0;    /* Allocate buffers in test bench */
    u32 bufferSelector = 0; /* Which frame buffer is in use */
    u32 disablePictureFreeze = 0;
    u32 rlcMode = 0;

#ifdef ASIC_TRACE_SUPPORT
    u32 tmp = 0;
#endif
    /*
     * Decoder API structures
     */
    Mpeg2DecInst decoder;
    Mpeg2DecRet ret;
    Mpeg2DecRet infoRet;
    Mpeg2DecInput DecIn;
    Mpeg2DecOutput DecOut;
    Mpeg2DecInfo Decinfo;
    Mpeg2DecPicture DecPic;
    DWLLinearMem_t streamMem;
    u32 picId = 0;

    char outFileName[256] = "";
    char outFileNameTiled[256] = "out_tiled.yuv";

    char outNames[256] = "out_1.yuv";

    FILE *fIn = NULL;
    FILE *fout = NULL;
    FILE *fConfig = NULL;
    FILE *fPipMask = NULL;
    Mpeg2DecLinearMem picBuffer[MPEG2_NUM_BUFFERS] = { 0 };

    u32 clock_gating = DEC_X170_INTERNAL_CLOCK_GATING;
    u32 data_discard = DEC_X170_DATA_DISCARD_ENABLE;
    u32 latency_comp = DEC_X170_LATENCY_COMPENSATION;
    u32 output_picture_endian = DEC_X170_OUTPUT_PICTURE_ENDIAN;
    u32 bus_burst_length = DEC_X170_BUS_BURST_LENGTH;
    u32 asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
    u32 output_format = DEC_X170_OUTPUT_FORMAT;

#ifdef PP_PIPELINE_ENABLED
    PPApiVersion ppVer;
    PPBuild ppBuild;
#endif

    u32 seedRnd = 0;
    u32 streamBitSwap = 0;
    i32 corruptedBytes = 0;


#ifndef EXPIRY_DATE
#define EXPIRY_DATE (u32)0xFFFFFFFF
#endif /* EXPIRY_DATE */

    /* expiry stuff */
    {
        u8 tmBuf[7];
        time_t sysTime;
        struct tm * tm;
        u32 tmp1;

        /* Check expiry date */
        time(&sysTime);
        tm = localtime(&sysTime);
        strftime(tmBuf, sizeof(tmBuf), "%y%m%d", tm);
        tmp1 = 1000000+atoi(tmBuf);
        if (tmp1 > (EXPIRY_DATE) && (EXPIRY_DATE) > 1 )
        {
            fprintf(stderr,
                "EVALUATION PERIOD EXPIRED.\n"
                "Please contact On2 Sales.\n");
            return -1;
        }
    }

    outp_byte_size = 0;

    INIT_SW_PERFORMANCE;
    
#ifdef ASIC_TRACE_SUPPORT
    gHwVer = 8190; /* default to 8190 mode */
#endif    

#ifdef MPEG2_EVALUATION_8170
    gHwVer = 8170;
#elif MPEG2_EVALUATION_8190
    gHwVer = 8190;
#elif MPEG2_EVALUATION_9170
    gHwVer = 9170;
#elif MPEG2_EVALUATION_9190
    gHwVer = 9190;
#elif MPEG2_EVALUATION_G1
    gHwVer = 10000;
#endif   

#ifndef PP_PIPELINE_ENABLED
    if(argc < 2)
    {

        printf("\n8170 MPEG-2 Decoder Testbench\n\n");
        printf("USAGE:\n%s [options] stream.mpeg2\n", argv[0]);
        printf("-Ooutfile write output to \"outfile\" (default out.yuv)\n");
        printf("-Nn to decode only first n frames of the stream\n");
        printf("-X to not to write output picture\n");
        printf("-Bn to use n frame buffers in decoder\n");
        printf("-E use tiled reference frame format.\n");
        printf("-G convert tiled output pictures to raster scan\n");
        printf("-Sfile.hex stream control trace file\n");
#ifdef ASIC_TRACE_SUPPORT
        printf("-R use reference decoder IDCT (sw/sw integration only)\n");
#endif
        printf("-W whole stream mode - give stream to decoder in one chunk\n");
        printf("-I save index file\n");
        printf
            ("-T write tiled output (out_tiled.yuv) by converting raster scan output\n");
        printf("-Y Write output as Interlaced Fields (instead of Frames).\n");
        printf
            ("-C crop output picture to real picture dimensions (only planar)\n");
        printf("-Q Skip decoding non-reference pictures.\n"); 
        printf("-Z output pictures using Mpeg2DecPeek() function\n");
        printf("--separate-fields-in-dpb DPB stores interlaced content"\
                    " as fields (default: frames)\n");
        printf("--output-frame-dpb Convert output to frame mode even if"\
            " field DPB mode used\n");
        printMpeg2Version();
        exit(100);
    }

    remove("pp_out.yuv");
    maxNumFrames = 0;
    for(i = 1; i < argc - 1; i++)
    {
        if(strncmp(argv[i], "-O", 2) == 0)
        {
            strcpy(outFileName, argv[i] + 2);
        }
        else if(strncmp(argv[i], "-N", 2) == 0)
        {
            maxNumFrames = atoi(argv[i] + 2);
        }
        else if (strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if (strncmp(argv[i], "-G", 2) == 0)
            convertTiledOutput = 1;        
        else if(strncmp(argv[i], "-B", 2) == 0)
        {
            numFrameBuffers = atoi(argv[i] + 2);
            if(numFrameBuffers > 16)
                numFrameBuffers = 16;
        }
        else if(strncmp(argv[i], "-X", 2) == 0)
        {
            writeOutput = 0;
        }
        else if(strncmp(argv[i], "-S", 2) == 0)
        {
            fStreamTrace = fopen((argv[i] + 2), "r");
        }
        else if(strncmp(argv[i], "-P", 2) == 0)
        {
            planarOutput = 1;
        }
        else if(strcmp(argv[i], "-Q") == 0)
        {
            skipNonReference = 1;
        }
#ifdef ASIC_TRACE_SUPPORT
        else if(strncmp(argv[i], "-R", 2) == 0)
        {
            useMpeg2Idct = 1;
        }
#endif
        else if(strncmp(argv[i], "-W", 2) == 0)
        {
            wholeStreamMode = 1;
        }
        else if(strcmp(argv[i], "-Y") == 0)
        {
            interlacedField = 1;
        }
        else if(strcmp(argv[i], "-C") == 0)
        {
            cropOutput = 1;
        }
        else if(strcmp(argv[i], "-Z") == 0)
        {
            usePeekOutput = 1;
        }
        else if(strcmp(argv[i], "-I") == 0)
        {
            saveIndex = 1;
        }
        else if(strcmp(argv[i], "--separate-fields-in-dpb") == 0)
        {
            dpbMode = DEC_DPB_INTERLACED_FIELD;
        }        
        else if(strcmp(argv[i], "--output-frame-dpb") == 0)
        {
            convertToFrameDpb = 1;
        }  
        else
        {
            printf("UNKNOWN PARAMETER: %s\n", argv[i]);
            return 1;
        }
    }

    printMpeg2Version();
    /* open data file */
    fIn = fopen(argv[argc - 1], "rb");
    if(fIn == NULL)
    {
        printf("Unable to open input file %s\n", argv[argc - 1]);
        exit(100);
    }
#else
    if(argc < 3)
    {
        printf("\nMPEG-2 Decoder PP Pipelined Testbench\n\n");
        printf("USAGE:\n%s [options] stream.mpeg2 pp.cfg\n", argv[0]);
        printf("-Nn to decode only first n vops of the stream\n");
        printf("-E use tiled reference frame format.\n");
        printf("-Bn to use n frame buffers in decoder\n");
        printf("-X to not to write output picture\n");
        printf("-W whole stream mode - give stream to decoder in one chunk\n");
        printf("-I save index file\n");
        printf("-Q Skip decoding non-reference pictures.\n"); 
        printf("--separate-fields-in-dpb DPB stores interlaced content"\
                    " as fields (default: frames)\n");
        exit(100);
    }

    remove("pp_out.yuv");
    maxNumFrames = 0;
    /* read cmdl parameters */
    for(i = 1; i < argc - 2; i++)
    {
        if(strncmp(argv[i], "-N", 2) == 0)
        {
            maxNumFrames = atoi(argv[i] + 2);
        }
        else if(strncmp(argv[i], "-B", 2) == 0)
        {
            numFrameBuffers = atoi(argv[i] + 2);
            if(numFrameBuffers > 16)
                numFrameBuffers = 16;
        }
        else if (strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if(strncmp(argv[i], "-X", 2) == 0)
        {
            writeOutput = 0;
        }
        else if(strncmp(argv[i], "-W", 2) == 0)
        {
            wholeStreamMode = 1;
        }
        else if(strcmp(argv[i], "-I") == 0)
        {
            saveIndex = 1;
        }
        else if(strcmp(argv[i], "-Q") == 0)
        {
            skipNonReference = 1;
        }
        else if(strcmp(argv[i], "--separate-fields-in-dpb") == 0)
        {
            dpbMode = DEC_DPB_INTERLACED_FIELD;
        }        
        else
        {
            fprintf(stdout, "UNKNOWN PARAMETER: %s\n", argv[i]);
            return 1;
        }
    }

    printMpeg2Version();
    /* open data file */
    fIn = fopen(argv[argc - 2], "rb");
    if(fIn == NULL)
    {
        printf("Unable to open input file %s\n", argv[argc - 2]);
        exit(100);
    }

    /* Print API and build version numbers */
    ppVer = PPGetAPIVersion();
    ppBuild = PPGetBuild();

    /* Version */
    fprintf(stdout,
            "\n8170 PP API v%d.%d - SW build: %d - HW build: %x\n",
            ppVer.major, ppVer.minor, ppBuild.swBuild, ppBuild.hwBuild);
#endif
    
    /* open index file for saving */
    if(saveIndex)
    {
        findex = fopen("stream.cfg", "w");
        if(findex == NULL)
        {
            printf("UNABLE TO OPEN INDEX FILE\n");
            return -1;
        }
    } 
    else
    {
        findex = fopen("stream.cfg", "r");
        if(findex != NULL)
        {
            useIndex = 1;
        }
    }

    /* set test bench configuration */
    TBSetDefaultCfg(&tbCfg);
    fTBCfg = fopen("tb.cfg", "r");
    if(fTBCfg == NULL)
    {
        printf("UNABLE TO OPEN INPUT FILE: \"tb.cfg\"\n");
        printf("USING DEFAULT CONFIGURATION\n");
    }
    else
    {
        fclose(fTBCfg);
        if(TBParseConfig("tb.cfg", TBReadParam, &tbCfg) == TB_FALSE)
            return -1;
        if(TBCheckCfg(&tbCfg) != 0)
            return -1;
    }
    /*TBPrintCfg(&tbCfg); */
    clock_gating = TBGetDecClockGating(&tbCfg);
    data_discard = TBGetDecDataDiscard(&tbCfg);
    latency_comp = tbCfg.decParams.latencyCompensation;
    output_picture_endian = TBGetDecOutputPictureEndian(&tbCfg);
    bus_burst_length = tbCfg.decParams.busBurstLength;
    asic_service_priority = tbCfg.decParams.asicServicePriority;
    output_format = TBGetDecOutputFormat(&tbCfg);
/*#if MD5SUM
    output_picture_endian = DEC_X170_LITTLE_ENDIAN;
    printf("Decoder Output Picture Endian forced to %d\n",
           output_picture_endian);
#endif*/
    printf("Decoder Clock Gating %d\n", clock_gating);
    printf("Decoder Data Discard %d\n", data_discard);
    printf("Decoder Latency Compensation %d\n", latency_comp);
    printf("Decoder Output Picture Endian %d\n", output_picture_endian);
    printf("Decoder Bus Burst Length %d\n", bus_burst_length);
    printf("Decoder Asic Service Priority %d\n", asic_service_priority);
    printf("Decoder Output Format %d\n", output_format);

    seedRnd = tbCfg.tbParams.seedRnd;
    streamHeaderCorrupt = TBGetTBStreamHeaderCorrupt(&tbCfg);
    /* if headers are to be corrupted 
      -> do not wait the picture to finalize before starting stream corruption */
    if (streamHeaderCorrupt)
        picRdy = 1;
    streamTruncate = TBGetTBStreamTruncate(&tbCfg);
    if(strcmp(tbCfg.tbParams.streamBitSwap, "0") != 0)
    {
        streamBitSwap = 1;
    }
    else
    {
        streamBitSwap = 0;
    }
    if(strcmp(tbCfg.tbParams.streamPacketLoss, "0") != 0)
    {
        streamPacketLoss = 1;
    }
    else
    {
        streamPacketLoss = 0;
    }
    disableResync = TBGetTBPacketByPacket(&tbCfg);
    printf("TB Slice by slice %d\n", disableResync);
    printf("TB Seed Rnd %d\n", seedRnd);
    printf("TB Stream Truncate %d\n", streamTruncate);
    printf("TB Stream Header Corrupt %d\n", streamHeaderCorrupt);
    printf("TB Stream Bit Swap %d; odds %s\n",
           streamBitSwap, tbCfg.tbParams.streamBitSwap);
    printf("TB Stream Packet Loss %d; odds %s\n",
           streamPacketLoss, tbCfg.tbParams.streamPacketLoss);

    /* allocate memory for stream buffer. if unsuccessful -> exit */
    streamMem.virtualAddress = NULL;
    streamMem.busAddress = 0;

    length = STREAMBUFFER_BLOCKSIZE;

    rewind(fIn);

    TBInitializeRandom(seedRnd);

    /* check size of the input file -> length of the stream in bytes */
    fseek(fIn, 0L, SEEK_END);
    streamSize = (u32) ftell(fIn);
    rewind(fIn);

    /* sets the stream length to random value */
    if(streamTruncate && !disableResync)
    {
        printf("streamSize %d\n", streamSize);
        ret = TBRandomizeU32(&streamSize);
        if(ret != 0)
        {
            printf("RANDOM STREAM ERROR FAILED\n");
            return -1;
        }
        printf("Randomized streamSize %d\n", streamSize);
    }

#ifdef ASIC_TRACE_SUPPORT
    tmp = openTraceFiles();
    if(!tmp)
    {
        printf("UNABLE TO OPEN TRACE FILES(S)\n");
    }
#endif

    /* Initialize the decoder */
    START_SW_PERFORMANCE;
    decsw_performance();
    ret = Mpeg2DecInit(&decoder, 
                       TBGetDecIntraFreezeEnable( &tbCfg ),
                       numFrameBuffers,
                     tiledOutput | 
        (dpbMode == DEC_DPB_INTERLACED_FIELD ? DEC_DPB_ALLOW_FIELD_ORDERING : 0));

    END_SW_PERFORMANCE;
    decsw_performance();

#ifdef PP_PIPELINE_ENABLED
    /* Initialize the post processer. If unsuccessful -> exit */
    if(pp_startup
       (argv[argc - 1], decoder, PP_PIPELINED_DEC_TYPE_MPEG2, &tbCfg) != 0)
    {
        fprintf(stdout, "PP INITIALIZATION FAILED\n");
        goto end2;
    }
#endif

    if(ret != MPEG2DEC_OK)
    {
        printf("Could not initialize decoder\n");
        goto end2;
    }

    /* Set ref buffer test mode */
    ((DecContainer *) decoder)->refBufferCtrl.testFunction = TBRefbuTestMode;

    if(DWLMallocLinear(((DecContainer *) decoder)->dwl,
                       STREAMBUFFER_BLOCKSIZE, &streamMem) != DWL_OK)
    {
        printf(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
        goto end2;
    }

    /* allocate output buffers if necessary */
    if(allocateBuffers)
    {
        if(AllocatePicBuffers(picBuffer, (DecContainer *) decoder))
            goto end2;
    }
    byteStrmStart = (u8 *) streamMem.virtualAddress;

    DecIn.skipNonReference = skipNonReference;
    DecIn.pStream = byteStrmStart;
    DecIn.streamBusAddress = (u32) streamMem.busAddress;

    if(byteStrmStart == NULL)
    {
        printf(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
        goto end2;
    }

    streamStop = byteStrmStart + length;
    /* NOTE: The registers should not be used outside decoder SW for other
     * than compile time setting test purposes */
    SetDecRegister(((DecContainer *) decoder)->mpeg2Regs, HWIF_DEC_LATENCY,
                   latency_comp);
    SetDecRegister(((DecContainer *) decoder)->mpeg2Regs, HWIF_DEC_CLK_GATE_E,
                   clock_gating);
    /*
    SetDecRegister(((DecContainer *) decoder)->mpeg2Regs, HWIF_DEC_OUT_TILED_E,
                   output_format);
                   */
    SetDecRegister(((DecContainer *) decoder)->mpeg2Regs, HWIF_DEC_OUT_ENDIAN,
                   output_picture_endian);
    SetDecRegister(((DecContainer *) decoder)->mpeg2Regs, HWIF_DEC_MAX_BURST,
                   bus_burst_length);
    if ((DWLReadAsicID() >> 16) == 0x8170U)
    {
        SetDecRegister(((DecContainer *) decoder)->mpeg2Regs, HWIF_PRIORITY_MODE,
                       asic_service_priority);
    }
    SetDecRegister(((DecContainer *) decoder)->mpeg2Regs, HWIF_DEC_DATA_DISC_E,
                   data_discard);

    /* Read what kind of stream is coming */
    START_SW_PERFORMANCE;
    decsw_performance();
    infoRet = Mpeg2DecGetInfo(decoder, &Decinfo);
    END_SW_PERFORMANCE;
    decsw_performance();
    if(infoRet)
    {
        decRet(infoRet);
    }

    pStrmData = (u8 *) DecIn.pStream;

    /* Read sequence headers */
    streamLen = readDecodeUnit(fIn, pStrmData);

    i = StartCode;
    /* decrease 4 because previous function call
     * read the first sequence start code */

    streamLen -= 4;
    DecIn.dataLen = streamLen;
    DecOut.dataLeft = 0;

    printf("Start decoding\n");
    do
    {
        printf("DecIn.dataLen %d\n", DecIn.dataLen);
        DecIn.picId = picId;
        if(ret != MPEG2DEC_STRM_PROCESSED &&
           ret != MPEG2DEC_NONREF_PIC_SKIPPED )
            printf("\nStarting to decode picture ID %d\n", picId);

        /* If enabled, break the stream */
        if(streamBitSwap)
        {
            if((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt)
            {
                /* Picture needs to be ready before corrupting next picture */
                if(picRdy && corruptedBytes <= 0)
                {
                    ret = TBRandomizeBitSwapInStream(DecIn.pStream,
                                                     DecIn.dataLen,
                                                     tbCfg.tbParams.
                                                     streamBitSwap);
                    if(ret != 0)
                    {
                        printf("RANDOM STREAM ERROR FAILED\n");
                        goto end2;
                    }
                    
                    corruptedBytes = DecIn.dataLen;
                    printf("corruptedBytes %d\n", corruptedBytes);
                }
            }
        }

        assert(DecOut.dataLeft == DecIn.dataLen || !DecOut.dataLeft);

        START_SW_PERFORMANCE;
        decsw_performance();
        ret = Mpeg2DecDecode(decoder, &DecIn, &DecOut);
        END_SW_PERFORMANCE;
        decsw_performance();

        decRet(ret);

        /*
         * Choose what to do now based on the decoder return value
         */

        switch (ret)
        {

        case MPEG2DEC_HDRS_RDY:

            /* Set a flag to indicate that headers are ready */
            hdrsRdy = 1;
            TBSetRefbuMemModel( &tbCfg, 
                ((DecContainer *) decoder)->mpeg2Regs,
                &((DecContainer *) decoder)->refBufferCtrl );

            /* Read what kind of stream is coming */
            START_SW_PERFORMANCE;
            decsw_performance();
            infoRet = Mpeg2DecGetInfo(decoder, &Decinfo);
            END_SW_PERFORMANCE;
            decsw_performance();
            if(infoRet)
            {
                decRet(infoRet);
            }
            outp_byte_size =
                (Decinfo.frameWidth * Decinfo.frameHeight * 3) >> 1;
                
            if (Decinfo.interlacedSequence)
                printf("INTERLACED SEQUENCE\n");
                
            dpbMode = Decinfo.dpbMode;

			/* If -O option not used, generate default file name */
            if(outFileName[0] == 0)
            {
                if(planarOutput && cropOutput)
                    sprintf(outFileName, "out_w%dh%d.yuv",
                            Decinfo.codedWidth, Decinfo.codedHeight);
                else
                    sprintf(outFileName, "out_w%dh%d.yuv",
                            Decinfo.frameWidth, Decinfo.frameHeight);
            }

            if(!frameNumber)
            {
                /*disableResync = 0; */
                if(Decinfo.streamFormat == MPEG2)
                    printf("MPEG-2 stream\n");
                else
                    printf("MPEG-1 stream\n");

                printf("Profile and level %d\n",
                       Decinfo.profileAndLevelIndication);
                switch (Decinfo.displayAspectRatio)
                {
                case MPEG2DEC_1_1:
                    printf("Display Aspect ratio 1:1\n");
                    break;
                case MPEG2DEC_4_3:
                    printf("Display Aspect ratio 4:3\n");
                    break;
                case MPEG2DEC_16_9:
                    printf("Display Aspect ratio 16:9\n");
                    break;
                case MPEG2DEC_2_21_1:
                    printf("Display Aspect ratio 2.21:1\n");
                    break;
                }
                printf("Output format %s\n",
                       Decinfo.outputFormat == MPEG2DEC_SEMIPLANAR_YUV420
                       ? "MPEG2DEC_SEMIPLANAR_YUV420" :
                       "MPEG2DEC_TILED_YUV420");
            }
            
            printf("DecOut.dataLeft %d \n", DecOut.dataLeft);
            if(DecOut.dataLeft)
            {
                corruptedBytes -= (DecIn.dataLen - DecOut.dataLeft);
                DecIn.dataLen = DecOut.dataLeft;
                DecIn.pStream = DecOut.pStrmCurrPos;
                DecIn.streamBusAddress = DecOut.strmCurrBusAddress;
            }
            else
            {
                *(u32 *) pStrmData = StartCode;
                DecIn.pStream = (u8 *) pStrmData;
                DecIn.streamBusAddress = (u32) streamMem.busAddress;

                if(strmEnd)
                {
                    /* stream ended */
                    streamLen = 0;
                    DecIn.pStream = NULL;
                }
                else
                {
                    /*u32 streamPacketLossTmp = streamPacketLoss;

                    if(!picRdy)
                        streamPacketLoss = 0;*/
                    streamLen = readDecodeUnit(fIn, pStrmData + 4);
                    /*streamPacketLoss = streamPacketLossTmp;*/
                }
                DecIn.dataLen = streamLen;

            	corruptedBytes = 0;

            }

#ifdef PP_PIPELINE_ENABLED
            pp_set_input_interlaced(Decinfo.interlacedSequence);

            if(pp_update_config
              (decoder, PP_PIPELINED_DEC_TYPE_MPEG2, &tbCfg) == CFG_UPDATE_FAIL)

			{
                fprintf(stdout, "PP CONFIG LOAD FAILED\n");
                goto end2;
			}

/*
            DecIn.enableDeblock = pp_mpeg4_filter_used();
            if(DecIn.enableDeblock == 1)
                printf("Deblocking filter enabled\n");
            else if(DecIn.enableDeblock == 0)
                printf("Deblocking filter disabled\n");
                */
            /* If unspecified at cmd line, use minimum # of buffers, otherwise
             * use specified amount. */
            if(numFrameBuffers == 0)
                pp_number_of_buffers(Decinfo.multiBuffPpSize);
            else
                pp_number_of_buffers(numFrameBuffers);
#endif
            break;

        case MPEG2DEC_PIC_DECODED:
            /* Picture is ready */
            picRdy = 1;
            picId++;

            /* Read what kind of stream is coming */
            infoRet = Mpeg2DecGetInfo(decoder, &Decinfo);
            if(infoRet)
            {
                decRet(infoRet);
            }

            if (usePeekOutput &&
                Mpeg2DecPeek(decoder, &DecPic) == MPEG2DEC_PIC_RDY)
            {
                picDisplayNumber++;
                printf("DECPIC %d, %s\n", DecPic.picId,
                       DecPic.keyPicture ? "key picture,    " :
                       "non key picture,");
                imageData = (u8 *) DecPic.pOutputPicture;

                picSize = DecPic.frameWidth * DecPic.frameHeight * 3 / 2;

                WriteOutput(outFileName, outFileNameTiled, imageData,
                            picDisplayNumber - 1,
                            ((Decinfo.frameWidth + 15) & ~15),
                            ((Decinfo.frameHeight + 15) & ~15),
                            Decinfo.interlacedSequence,
                            DecPic.topField, DecPic.firstField,
                            output_picture_endian, DecPic,
                            Decinfo.codedHeight,
                            DecPic.outputFormat);
            }

            /* loop to get all pictures/fields out from decoder */
            do
            {
                START_SW_PERFORMANCE;
                decsw_performance();
                infoRet = Mpeg2DecNextPicture(decoder, &DecPic, 0);
                END_SW_PERFORMANCE;
                decsw_performance();

                if (!usePeekOutput)
                {
                    /* print result */
                    decNextPictureRet(infoRet);

                    /* Increment display number for every displayed picture */
                    picDisplayNumber++;

                    if(infoRet == MPEG2DEC_PIC_RDY)
                    {
                        /* printf info */
                        printf("PIC %d, %s", DecPic.picId,
                               DecPic.keyPicture ? "key picture,    " :
                               "non key picture,");

                        if(DecPic.fieldPicture)
                            printf(" %s ", DecPic.topField ?
                                "top field.   " : "bottom field.");
                        else
                            printf(" frame picture. ");

                        printTimeCode(&(DecPic.timeCode));
                        if(DecPic.numberOfErrMBs)
                        {
                            printf(", %d/%d error mbs\n",
                                   DecPic.numberOfErrMBs,
                                   (DecPic.frameWidth >> 4) *
                                   (DecPic.frameHeight >> 4));
                            cumulativeErrorMbs += DecPic.numberOfErrMBs;
                        }

                        /* Write output picture to file */
                        imageData = (u8 *) DecPic.pOutputPicture;

                        picSize = DecPic.frameWidth * DecPic.frameHeight * 3 / 2;

#ifndef PP_PIPELINE_ENABLED
                        printf("DecPic.firstField %d\n", DecPic.firstField);
                        WriteOutput(outFileName, outFileNameTiled, imageData,
                                    picDisplayNumber - 1,
                                    ((Decinfo.frameWidth + 15) & ~15),
                                    ((Decinfo.frameHeight + 15) & ~15),
                                    Decinfo.interlacedSequence,
                                    DecPic.topField, DecPic.firstField,
                                    output_picture_endian, DecPic,
                                    Decinfo.codedHeight,
                                    DecPic.outputFormat);
#else

                        HandlePpOutput(frameNumber - 1, DecPic, decoder);
#endif
                    }
                }
            }
            while(infoRet == MPEG2DEC_PIC_RDY);

            frameNumber++;
            vpNum = 0;

            printf("DecOut.dataLeft %d \n", DecOut.dataLeft);
            if(DecOut.dataLeft)
            {
                corruptedBytes -= (DecIn.dataLen - DecOut.dataLeft);
                DecIn.dataLen = DecOut.dataLeft;
                DecIn.pStream = DecOut.pStrmCurrPos;
                DecIn.streamBusAddress = DecOut.strmCurrBusAddress;
            }
            else
            {

                *(u32 *) pStrmData = StartCode;
                DecIn.pStream = (u8 *) pStrmData;
                DecIn.streamBusAddress = (u32) streamMem.busAddress;

                if(strmEnd)
                {
                    streamLen = 0;
                    DecIn.pStream = NULL;
                }
                else
                {
                    /*u32 streamPacketLossTmp = streamPacketLoss;

                    if(!picRdy)
                        streamPacketLoss = 0;*/
                    streamLen = readDecodeUnit(fIn, pStrmData + 4);
                    /*streamPacketLoss = streamPacketLossTmp;*/
                }
                DecIn.dataLen = streamLen;

            	corruptedBytes = 0;

            }

            if(maxNumFrames && (frameNumber >= maxNumFrames))
            {
                printf("\n\nMax num of pictures reached\n\n");
                DecIn.dataLen = 0;
                goto end2;
            }

            break;

        case MPEG2DEC_STRM_PROCESSED:
        case MPEG2DEC_NONREF_PIC_SKIPPED:
            fprintf(stdout,
                    "TB: Frame Number: %u, pic: %d\n", vpNum++, frameNumber);
            /* Used to indicate that picture decoding needs to
             * finalized prior to corrupting next picture */

            /* fallthrough */

        case MPEG2DEC_OK:

            /* Read what kind of stream is coming */
            START_SW_PERFORMANCE;
            decsw_performance();
            infoRet = Mpeg2DecGetInfo(decoder, &Decinfo);
            END_SW_PERFORMANCE;
            decsw_performance();

            if(infoRet)
            {
                decRet(infoRet);
            }

            /*
             **  Write output picture to the file
             */

            /*
             *    Read next decode unit. Because readDecodeUnit
             *   reads VOP start code in previous
             *   function call, Insert this start code
             *   in to first word
             *   of stream buffer, and increase
             *   stream buffer pointer by 4 in
             *   the function call.
             */

            printf("DecOut.dataLeft %d \n", DecOut.dataLeft);
            if(DecOut.dataLeft)
            {
                corruptedBytes -= (DecIn.dataLen - DecOut.dataLeft);
                DecIn.dataLen = DecOut.dataLeft;
                DecIn.pStream = DecOut.pStrmCurrPos;
                DecIn.streamBusAddress = DecOut.strmCurrBusAddress;
            }
            else
            {

                *(u32 *) pStrmData = StartCode;
                DecIn.pStream = (u8 *) pStrmData;
                DecIn.streamBusAddress = (u32) streamMem.busAddress;

                if(strmEnd)
                {
                    streamLen = 0;
                    DecIn.pStream = NULL;
                }
                else
                {
                    /*u32 streamPacketLossTmp = streamPacketLoss;

                    if(!picRdy)
                        streamPacketLoss = 0;*/
                    streamLen = readDecodeUnit(fIn, pStrmData + 4);
                    /*streamPacketLoss = streamPacketLossTmp;*/
                }
                DecIn.dataLen = streamLen;

            	corruptedBytes = 0;

            }

            break;

        case MPEG2DEC_PARAM_ERROR:
            printf("INCORRECT STREAM PARAMS\n");
            goto end2;
            break;

        case MPEG2DEC_STRM_ERROR:
            printf("STREAM ERROR\n");

            printf("DecOut.dataLeft %d \n", DecOut.dataLeft);
            if(DecOut.dataLeft)
            {
                corruptedBytes -= (DecIn.dataLen - DecOut.dataLeft);
                DecIn.dataLen = DecOut.dataLeft;
                DecIn.pStream = DecOut.pStrmCurrPos;
                DecIn.streamBusAddress = DecOut.strmCurrBusAddress;
            }
            else
            {

                *(u32 *) pStrmData = StartCode;
                DecIn.pStream = (u8 *) pStrmData;
                DecIn.streamBusAddress = (u32) streamMem.busAddress;

                if(strmEnd)
                {
                    streamLen = 0;
                    DecIn.pStream = NULL;
                }
                else
                {
                    /*u32 streamPacketLossTmp = streamPacketLoss;

                    if(!picRdy)
                        streamPacketLoss = 0;*/
                    streamLen = readDecodeUnit(fIn, pStrmData + 4);
                    /*streamPacketLoss = streamPacketLossTmp;*/
                }
                DecIn.dataLen = streamLen;

            }
            break;

        default:
            decRet(ret);
            goto end2;
        }
        /*
         * While there is stream
         */

    }
    while(DecIn.dataLen > 0);
  end2:

  /* Output buffered images also... */
    START_SW_PERFORMANCE;
    decsw_performance();
    while(!usePeekOutput &&
          Mpeg2DecNextPicture(decoder, &DecPic, 1) == MPEG2DEC_PIC_RDY)
    {
        /* Increment display number for every displayed picture */
        picDisplayNumber++;

#ifndef PP_PIPELINE_ENABLED
        /* Write output picture to file */
        imageData = (u8 *) DecPic.pOutputPicture;

        picSize = DecPic.frameWidth * DecPic.frameHeight * 3 / 2;

        WriteOutput(outFileName, outFileNameTiled, imageData,
                    picDisplayNumber - 1,
                    ((Decinfo.frameWidth + 15) & ~15),
                    ((Decinfo.frameHeight + 15) & ~15),
                    Decinfo.interlacedSequence,
                    DecPic.topField, DecPic.firstField,
                    output_picture_endian, DecPic, Decinfo.codedHeight,
                    DecPic.outputFormat);
#else
        HandlePpOutput(frameNumber - 1, DecPic, decoder);
#endif
    }
    END_SW_PERFORMANCE;
    decsw_performance();

    START_SW_PERFORMANCE;
    decsw_performance();
    Mpeg2DecGetInfo(decoder, &Decinfo);
    END_SW_PERFORMANCE;
    decsw_performance();

    /*
     * Release the decoder
     */
#ifdef PP_PIPELINE_ENABLED
    pp_close();
#endif
    if(streamMem.virtualAddress)
        DWLFreeLinear(((DecContainer *) decoder)->dwl, &streamMem);

    START_SW_PERFORMANCE;
    decsw_performance();
    Mpeg2DecRelease(decoder);
    END_SW_PERFORMANCE;
    decsw_performance();

    if(Decinfo.frameWidth < 1921)
        printf("\nWidth %d Height %d\n", Decinfo.frameWidth,
               Decinfo.frameHeight);
    if(cumulativeErrorMbs)
    {
        printf("Cumulative errors: %d/%d macroblocks, ",
               cumulativeErrorMbs,
               (Decinfo.frameWidth >> 4) * (Decinfo.frameHeight >> 4) *
               frameNumber);
    }
    printf("decoded %d pictures\n", frameNumber);

    if(fout)
        fclose(fout);

    if(fTiledOutput)
        fclose(fTiledOutput);

#ifdef ASIC_TRACE_SUPPORT
    trace_SequenceCtrl(hwDecPicCount, bFrames);
    trace_MPEG2DecodingTools();
    closeTraceFiles();
#endif
    
    if(saveIndex || useIndex)
    {
        fclose(findex);
    }

    /* Calculate the output size and print it  */
    fout = fopen(outFileName, "rb");
    if(NULL == fout)
    {
        streamLen = 0;
    }
    else
    {
        fseek(fout, 0L, SEEK_END);
        streamLen = (u32) ftell(fout);
        fclose(fout);
    }

#ifndef PP_PIPELINE_ENABLED
    printf("output size %d\n", streamLen);
#endif

    FINALIZE_SW_PERFORMANCE;

    if(cumulativeErrorMbs || !frameNumber)
    {
        printf("ERRORS FOUND\n");
        return (1);
    }
    else
        return (0);

}

/*------------------------------------------------------------------------------
        readDecodeUnit
        Description : search pic start code and read one decode unit at a time
------------------------------------------------------------------------------*/
static u32 readDecodeUnit(FILE * fp, u8 * frameBuffer)
{

    u32 idx = 0, VopStart = 0;
    u8 temp;
    u8 nextPacket = 0;

    StartCode = 0;

    if(stopDecoding)
    {
        printf("Truncated stream size reached -> stop decoding\n");
        return 0;
    }

    /* If enabled, loose the packets (skip this packet first though) */
    if(streamPacketLoss && disableResync)
    {
        u32 ret = 0;

        ret =
            TBRandomizePacketLoss(tbCfg.tbParams.streamPacketLoss, &nextPacket);
        if(ret != 0)
        {
            printf("RANDOM STREAM ERROR FAILED\n");
            return 0;
        }
    }
    
    if(useIndex)
    {
        u32 amount = 0;
        
        /* get next index */
        fscanf(findex, "%llu", &nextIndex);
        amount = nextIndex - curIndex;

        /* read data */
        idx = fread(frameBuffer, 1, amount, fp);
        
        VopStart = 1;
        StartCode = ((frameBuffer[idx - 1] << 24) |
                     (frameBuffer[idx - 2] << 16) |
                     (frameBuffer[idx - 3] << 8) |
                     frameBuffer[idx - 4]);

        /* end of stream */
        if(nextIndex == streamSize)
        {
            strmEnd = 1;
            idx += 4;
        }

        curIndex = nextIndex;
    }
    else
    {
        while(!VopStart)
        {

            fread(&temp, sizeof(u8), 1, fp);

            if(feof(fp))
            {

                fprintf(stdout, "TB: End of stream noticed in readDecodeUnit\n");
                strmEnd = 1;
                idx += 4;
                break;
            }
            /* Reading the whole stream at once must be limited to buffer size */
            if((idx > (length - MPEG2_WHOLE_STREAM_SAFETY_LIMIT)) &&
               wholeStreamMode)
            {

                wholeStreamMode = 0;

            }

            frameBuffer[idx] = temp;

            if(idx >= (frameBuffer == byteStrmStart ? 4 : 3))
            {
                if(!wholeStreamMode)
                {
                    if(disableResync)
                    {
                        /*-----------------------------------
                            Slice by slice
                        -----------------------------------*/
                        if((frameBuffer[idx - 3] == 0x00) &&
                           (frameBuffer[idx - 2] == 0x00) &&
                           (frameBuffer[idx - 1] == 0x01))
                        {
                            if(frameBuffer[idx] > 0x00)
                            {
                                if(frameBuffer[idx] <= 0xAF)
                                {
                                    VopStart = 1;
                                    StartCode = ((frameBuffer[idx] << 24) |
                                                 (frameBuffer[idx - 1] << 16) |
                                                 (frameBuffer[idx - 2] << 8) |
                                                 frameBuffer[idx - 3]);
                                    /*printf("SLICE FOUND\n");*/
                                }
                            }
                            else if(frameBuffer[idx] == 0x00)
                            {
                                VopStart = 1;
                                StartCode = ((frameBuffer[idx] << 24) |
                                         (frameBuffer[idx - 1] << 16) |
                                         (frameBuffer[idx - 2] << 8) |
                                         frameBuffer[idx - 3]);
                                /* MPEG2 start code found */
                            }
                        }
                    }
                    else
                    {
                        /*-----------------------------------
                            MPEG2 Start code
                        -----------------------------------*/
                        if(((frameBuffer[idx - 3] == 0x00) &&
                            (frameBuffer[idx - 2] == 0x00) &&
                            (((frameBuffer[idx - 1] == 0x01) &&
                              (frameBuffer[idx] == 0x00)))))
                        {
                            VopStart = 1;
                            StartCode = ((frameBuffer[idx] << 24) |
                                         (frameBuffer[idx - 1] << 16) |
                                         (frameBuffer[idx - 2] << 8) |
                                         frameBuffer[idx - 3]);
                            /* MPEG2 start code found */
                        }
                    }
                }
            }
            if(idx >= length)
            {
                fprintf(stdout, "idx = %d,lenght = %d \n", idx, length);
                fprintf(stdout, "TB: Out Of Stream Buffer\n");
                break;
            }
            if(idx > strmRew + 128)
            {
                idx -= strmRew;
            }
            idx++;
            /* stop reading if truncated stream size is reached */
            if(streamTruncate && !disableResync)
            {
                if(previousUsed + idx >= streamSize)
                {
                    printf("Stream truncated at %d bytes\n", previousUsed + idx);
                    stopDecoding = 1;   /* next call return 0 size -> exit decoding main loop */
                    break;
                }
            }
        }
    }

    if(saveIndex && !useIndex)
    {
        fprintf(findex, "%llu\n", ftello64(fp));
    }

    traceUsedStream = previousUsed;
    previousUsed += idx;

    /* If we skip this packet */
    if(picRdy && nextPacket && ((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt))
    {
        /* Get the next packet */
        printf("Packet Loss\n");
        return readDecodeUnit(fp, frameBuffer);
    }
    else
    {
        /*printf("READ DECODE UNIT %d\n", idx); */
        printf("No Packet Loss\n");
        if (disableResync && picRdy && streamTruncate 
                && ((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt))
        {
            i32 ret;
            printf("Original packet size %d\n", idx);
            ret = TBRandomizeU32(&idx);
            if(ret != 0)
            {
                printf("RANDOM STREAM ERROR FAILED\n");
                stopDecoding = 1;   /* next call return 0 size -> exit decoding main loop */
                return 0;
            }
            printf("Randomized packet size %d\n", idx);
        }
        return (idx);
    }
}

/*------------------------------------------------------------------------------
        printTimeCode
        Description : Print out time code
------------------------------------------------------------------------------*/

void printTimeCode(Mpeg2DecTime * timecode)
{

    fprintf(stdout, "hours %u, "
            "minutes %u, "
            "seconds %u, "
            "timePictures %u \n",
            timecode->hours,
            timecode->minutes, timecode->seconds, timecode->pictures);
}

/*------------------------------------------------------------------------------
        decRet
        Description : Print out Decoder return values
------------------------------------------------------------------------------*/

void decRet(Mpeg2DecRet ret)
{

    printf("Decode result: ");

    switch (ret)
    {
    case MPEG2DEC_OK:
        printf("MPEG2DEC_OK\n");
        break;
    case MPEG2DEC_STRM_PROCESSED:
        printf("MPEG2DEC_STRM_PROCESSED\n");
        break;
    case MPEG2DEC_NONREF_PIC_SKIPPED:
        printf("MPEG2DEC_NONREF_PIC_SKIPPED\n");
        break;
    case MPEG2DEC_PIC_RDY:
        printf("MPEG2DEC_PIC_RDY\n");
        break;
    case MPEG2DEC_HDRS_RDY:
        printf("MPEG2DEC_HDRS_RDY\n");
        break;
    case MPEG2DEC_PIC_DECODED:
        printf("MPEG2DEC_PIC_DECODED\n");
        break;
    case MPEG2DEC_PARAM_ERROR:
        printf("MPEG2DEC_PARAM_ERROR\n");
        break;
    case MPEG2DEC_STRM_ERROR:
        printf("MPEG2DEC_STRM_ERROR\n");
        break;
    case MPEG2DEC_NOT_INITIALIZED:
        printf("MPEG2DEC_NOT_INITIALIZED\n");
        break;
    case MPEG2DEC_MEMFAIL:
        printf("MPEG2DEC_MEMFAIL\n");
        break;
    case MPEG2DEC_DWL_ERROR:
        printf("MPEG2DEC_DWL_ERROR\n");
        break;
    case MPEG2DEC_HW_BUS_ERROR:
        printf("MPEG2DEC_HW_BUS_ERROR\n");
        break;
    case MPEG2DEC_SYSTEM_ERROR:
        printf("MPEG2DEC_SYSTEM_ERROR\n");
        break;
    case MPEG2DEC_HW_TIMEOUT:
        printf("MPEG2DEC_HW_TIMEOUT\n");
        break;
    case MPEG2DEC_HDRS_NOT_RDY:
        printf("MPEG2DEC_HDRS_NOT_RDY\n");
        break;
    case MPEG2DEC_STREAM_NOT_SUPPORTED:
        printf("MPEG2DEC_STREAM_NOT_SUPPORTED\n");
        break;
    default:
        printf("Other %d\n", ret);
        break;
    }
}

/*------------------------------------------------------------------------------
        decNextPictureRet
        Description : Print out NextPicture return values
------------------------------------------------------------------------------*/
void decNextPictureRet(Mpeg2DecRet ret)
{
    printf("next picture returns: ");

    decRet(ret);
}

/*------------------------------------------------------------------------------
        printMpeg2Version
        Description : Print out decoder version info
------------------------------------------------------------------------------*/

void printMpeg2Version(void)
{

    Mpeg2DecApiVersion decVersion;
    Mpeg2DecBuild decBuild;

    /*
     * Get decoder version info
     */

    decVersion = Mpeg2DecGetAPIVersion();
    printf("\nAPI version:  %d.%d, ", decVersion.major, decVersion.minor);

    decBuild = Mpeg2DecGetBuild();
    printf("sw build nbr: %d, hw build nbr: %x\n\n",
           decBuild.swBuild, decBuild.hwBuild);

}

/*------------------------------------------------------------------------------

    Function name: allocatePicBuffers

    Functional description: Allocates frame buffers

    Inputs:     Mpeg2DecLinearMem * buffer       pointers stored here

    Outputs:    NONE

    Returns:    nonzero if err

------------------------------------------------------------------------------*/

i32 AllocatePicBuffers(Mpeg2DecLinearMem * buffer, DecContainer * container)
{

    u32 offset = (MPEG2_FRAME_BUFFER_SIZE + 0xFFF) & ~(0xFFF);
    u32 i = 0;

#ifndef MPEG2DEC_EXTERNAL_ALLOC_DISABLE

    if(DWLMallocRefFrm(((DecContainer *) container)->dwl,
                       offset * MPEG2_NUM_BUFFERS,
                       (DWLLinearMem_t *) buffer) != DWL_OK)
    {
        printf(("UNABLE TO ALLOCATE OUTPUT BUFFER MEMORY\n"));
        return 1;
    }

    buffer[1].pVirtualAddress = buffer[0].pVirtualAddress + offset / 4;
    buffer[1].busAddress = buffer[0].busAddress + offset;

    buffer[2].pVirtualAddress = buffer[1].pVirtualAddress + offset / 4;
    buffer[2].busAddress = buffer[1].busAddress + offset;

    for(i = 0; i < MPEG2_NUM_BUFFERS; i++)
    {
        printf("buff %d vir %x bus %x\n", i,
               buffer[i].pVirtualAddress, buffer[i].busAddress);
    }

#endif
    return 0;
}

/*------------------------------------------------------------------------------

 Function name:  WriteOutput

  Purpose:
  Write picture pointed by data to file. Size of the
  picture in pixels is indicated by picSize.

------------------------------------------------------------------------------*/
void WriteOutput(char *filename, char *filenameTiled, u8 * data,
                 u32 frameNumber, u32 width, u32 height, u32 interlaced,
                 u32 top, u32 firstField, u32 output_picture_endian,
                 Mpeg2DecPicture DecPicture, u32 codedHeight,
                 u32 tiledMode )
{
    u32 tmp, i, j;
    u32 picSize;
    u8 *p, *ptmp;
    u32 skipLastRow = 0;
    static u32 frameId = 0;
    u8 *rasterScan = NULL;

    if(!writeOutput)
    {
        return;
    }

    picSize = width * height * 3 / 2;

    /* Convert back to raster scan format if decoder outputs 
     * tiled format */
    if(tiledMode && convertTiledOutput)
    {
        rasterScan = (u8*)malloc(picSize);
        if(!rasterScan)
        {
            fprintf(stderr, "error allocating memory for tiled"
                "-->raster conversion!\n");
            return;
        }

        TBTiledToRaster( tiledMode, convertToFrameDpb ? dpbMode : DEC_DPB_DEFAULT, 
            data, rasterScan, width, height );
        TBTiledToRaster( tiledMode, convertToFrameDpb ? dpbMode : DEC_DPB_DEFAULT, 
            data+width*height, rasterScan+width*height, width, height/2 );
        data = rasterScan;
    }
    else if (convertToFrameDpb && (dpbMode != DEC_DPB_FRAME) )
    {

        rasterScan = (u8*)malloc(picSize);
        if(!rasterScan)
        {
            fprintf(stderr, "error allocating memory for tiled"
                "-->raster conversion!\n");
            return;
        }

        TBFieldDpbToFrameDpb( dpbMode, 
            data, rasterScan, width, height );
        TBFieldDpbToFrameDpb( dpbMode,
            data+width*height, rasterScan+width*height, width, height/2 );
        data = rasterScan;    
    }  
    
    if(interlaced && codedHeight <= (height - 16))
    {
        height -= 16;
        skipLastRow = 1;
    }

    /* fout is global file pointer */
    if(fout == NULL)
    {
        /* open output file for writing, can be disabled with define.
         * If file open fails -> exit */
        if(strcmp(filename, "none") != 0)
        {
            fout = fopen(filename, "wb");
            if(fout == NULL)
            {
                printf("UNABLE TO OPEN OUTPUT FILE\n");
                if(rasterScan)
                    free(rasterScan);
                return;
            }
        }
    }

#if 0
    if(fTiledOutput == NULL && tiledOutput)
    {
        /* open output file for writing, can be disabled with define.
         * If file open fails -> exit */
        if(strcmp(filenameTiled, "none") != 0)
        {
            fTiledOutput = fopen(filenameTiled, "wb");
            if(fTiledOutput == NULL)
            {
                printf("UNABLE TO OPEN TILED OUTPUT FILE\n");
                if(rasterScan)
                    free(rasterScan);
                return;
            }
        }
    }
#endif

    if(fout && data)
    {
        if(interlaced && interlacedField)
        {
            /* start of top field */
            p = data;
            /* start of bottom field */
            if(!top)
                p += width;
            else
            {
                if(rasterScan)
                    free(rasterScan);
                return; /* TODO! use "return" ==> match to reference model */
            }

            if(planarOutput)
            {
                /* luma */
                for(i = 0; i < height / 2; i++, p += (width * 2))
                    fwrite(p, 1, width, fout);
                if(skipLastRow)
                    p += 16 * width;
                ptmp = p;
                /* cb */
                for(i = 0; i < height / 4; i++, p += (width * 2))
                    for(j = 0; j < width >> 1; j++)
                        fwrite(p + j * 2, 1, 1, fout);
                /* cr */
                for(i = 0; i < height / 4; i++, ptmp += (width * 2))
                    for(j = 0; j < width >> 1; j++)
                        fwrite(ptmp + 1 + j * 2, 1, 1, fout);
            }
            else
            {
                /* luma */
                for(i = 0; i < height / 2; i++, p += (width * 2))
                    fwrite(p, 1, width, fout);
                if(skipLastRow)
                    p += 16 * width;
                /* chroma */
                for(i = 0; i < height / 4; i++, p += (width * 2))
                    fwrite(p, 1, width, fout);
            }
        }
        else    /* progressive */
        {
#ifndef ASIC_TRACE_SUPPORT
            u8 *picCopy = NULL;
#endif
            /*printf("firstField %d\n");*/
            
            if(interlaced && !interlacedField && firstField)
                return;

            /*
             * printf("PIC %d, %s", DecPicture.picId,
             * DecPicture.
             * keyPicture ? "key picture" : "non key picture");
             * if(DecPicture.fieldPicture)
             * {
             * printf(", %s Field",
             * DecPicture.topField ? "top" : "bottom");
             * }
             * printf(" bPicture %d\n", DecPicture.bPicture);
             * printf("numberOfErrMBs %d\n", DecPicture.numberOfErrMBs); */

            if((DecPicture.fieldPicture && !firstField) ||
               !DecPicture.fieldPicture)
            {
                printf("Output picture %d\n", frameId);
                /* Decoder without pp does not write out fields but a 
                 * frame containing both fields */
                /* PP output is written field by field */

#ifndef ASIC_TRACE_SUPPORT
                if(output_picture_endian == DEC_X170_BIG_ENDIAN)
                {
                    picCopy = (u8 *) malloc(picSize);
                    if(NULL == picCopy)
                    {
                        printf("MALLOC FAILED @ %s %d", __FILE__, __LINE__);
                        if(rasterScan)
                            free(rasterScan);
                        return;
                    }
                    memcpy(picCopy, data, picSize);
                    TbChangeEndianess(picCopy, picSize);
                    data = picCopy;
                }
#endif

#ifdef MD5SUM
                TBWriteFrameMD5Sum(fout, data, picSize, frameId);

                /* tiled */
#if 0
                if(tiledOutput)
                {
                    /* round up to next multiple of 16 */
                    u32 tmpWidth = (width + 15) & ~15;
                    u32 tmpHeight = (height + 15) & ~15;

                    TbWriteTiledOutput(fTiledOutput, data, tmpWidth >> 4,
                                       tmpHeight >> 4, frameId,
                                       1 /* write md5sum */ ,
                                       1 /* semi-planar data */ );
                }
#endif
#else

                /* this presumes output has system endianess */
                if(planarOutput)
                {
                    if(!cropOutput)
                    {
			tmp = width * height;
                        fwrite(data, 1, tmp, fout);
                        p = skipLastRow ? data + tmp + 16 * width : data + tmp;
                        for(i = 0; i < tmp / 4; i++)
                            fwrite(p + i * 2, 1, 1, fout);
                        for(i = 0; i < tmp / 4; i++)
                            fwrite(p + 1 + i * 2, 1, 1, fout);
                    }
                    else /* cropped output */
                    {
                        tmp = width * height;
                        p = data;
                        for(i = 0; i < DecPicture.codedHeight; i++)
                        {
                            fwrite(p, 1, DecPicture.codedWidth, fout);
                            p += DecPicture.frameWidth;
                        }
                        p = data + tmp;
                        for(i = 0; i < DecPicture.codedHeight / 2; i++)
                        {
                            for(j = 0; j < DecPicture.codedWidth / 2; j++)
                                fwrite(p + 2 * j, 1, 1, fout);
                            p += DecPicture.frameWidth;
                        }
                        p = data + tmp + 1;
                        for(i = 0; i < DecPicture.codedHeight / 2; i++)
                        {
                            for(j = 0; j < DecPicture.codedWidth / 2; j++)
                                fwrite(p + 2 * j, 1, 1, fout);
                            p += DecPicture.frameWidth;
                        }
		    }
                }
                else if(!skipLastRow)   /* semi-planar */
                    fwrite(data, 1, picSize, fout);
                else
                {
                    tmp = width * height;
                    fwrite(data, 1, tmp, fout);
                    fwrite(data + tmp + 16 * width, 1, tmp / 2, fout);
                }

                /* tiled */
#if 0
                if(tiledOutput)
                {
                    /* round up to next multiple of 16 */
                    u32 tmpWidth = (width + 15) & ~15;
                    u32 tmpHeight = (height + 15) & ~15;

                    TbWriteTiledOutput(fTiledOutput, data, tmpWidth >> 4,
                                       tmpHeight >> 4, frameId,
                                       0 /* write md5sum */ ,
                                       1 /* semi-planar data */ );
                }
#endif
#endif
#ifndef ASIC_TRACE_SUPPORT
                if(output_picture_endian == DEC_X170_BIG_ENDIAN)
                {
                    free(picCopy);
                }
#endif
                frameId++;
            }
        }
    }

    if(rasterScan)
        free(rasterScan);

}

/*------------------------------------------------------------------------------

    Function name:  WriteOutputLittleEndian

     Purpose:
     Write picture pointed by data to file. Size of the
     picture in pixels is indicated by picSize. The data
     is converted to little endian.

------------------------------------------------------------------------------*/
void WriteOutputLittleEndian(u8 * data, u32 picSize)
{
    u32 chunks = 0;
    u32 i = 0;
    u32 word = 0;

    chunks = picSize / 4;
    for(i = 0; i < chunks; ++i)
    {
        word = data[0];
        word <<= 8;
        word |= data[1];
        word <<= 8;
        word |= data[2];
        word <<= 8;
        word |= data[3];
        fwrite(&word, 4, 1, fout);
        data += 4;
    }

    if(picSize % 4 == 0)
    {
        return;
    }
    else if(picSize % 4 == 1)
    {
        word = data[0];
        word <<= 24;
        fwrite(&word, 1, 1, fout);
    }
    else if(picSize % 4 == 2)
    {
        word = data[0];
        word <<= 8;
        word |= data[1];
        word <<= 16;
        fwrite(&word, 2, 1, fout);
    }
    else if(picSize % 4 == 3)
    {
        word = data[0];
        word <<= 8;
        word |= data[1];
        word <<= 8;
        word |= data[2];
        word <<= 8;
        fwrite(&word, 3, 1, fout);
    }
}

/*------------------------------------------------------------------------------

    Function name:  Mpeg2DecTrace

    Purpose:
        Example implementation of Mpeg2DecTrace function. Prototype of this
        function is given in mpeg2decapi.h. This implementation appends
        trace messages to file named 'dec_api.trc'.

------------------------------------------------------------------------------*/
void Mpeg2DecTrace(const char *string)
{
    FILE *fp;

    fp = fopen("dec_api.trc", "at");

    if(!fp)
        return;

    fwrite(string, 1, strlen(string), fp);
    fwrite("\n", 1, 1, fp);

    fclose(fp);
}

/*------------------------------------------------------------------------------

    Function name: writeOutputPicture

    Functional description: write output picture

    Inputs: vop number, picture description struct, instance

    Outputs:

    Returns:    MP4DecRet

------------------------------------------------------------------------------*/

#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 vopNumber, Mpeg2DecPicture DecPicture, Mpeg2DecInst decoder)
{
    PPResult res;

    res = pp_check_combined_status();

    if(res == PP_OK)
    {
        pp_write_output(vopNumber, DecPicture.fieldPicture,
                        DecPicture.topField);
        pp_read_blend_components(((DecContainer *) decoder)->ppInstance);
    }
    if(pp_update_config
       (decoder, PP_PIPELINED_DEC_TYPE_MPEG2, &tbCfg) == CFG_UPDATE_FAIL)

    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
    }

}
#endif
