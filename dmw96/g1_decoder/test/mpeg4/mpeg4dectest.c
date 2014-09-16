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
--  Abstract : MPEG-4 Decoder Testbench
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
#ifndef MP4DEC_EXTERNAL_ALLOC_DISABLE
#include <fcntl.h>
#include <sys/mman.h>
#endif

#include "dwl.h"
#include "mp4decapi.h"
#ifdef USE_EFENCE
#include "efence.h"
#endif

#include "mp4dechwd_container.h"
#include "regdrv.h"

#ifdef PP_PIPELINE_ENABLED
#include "ppapi.h"
#include "pptestbench.h"
#endif

#include "tb_cfg.h"
#include "tb_tiled.h"

#ifdef MD5SUM
#include "tb_md5.h"
#endif

#include "tb_sw_performance.h"
#ifdef ASIC_TRACE_SUPPORT
#include "trace.h"
#endif

#define DEFAULT -1
#define VOP_START_CODE 0xb6010000

/* Size of stream buffer */
#define STREAMBUFFER_BLOCKSIZE 2*2097151
#define MP4_WHOLE_STREAM_SAFETY_LIMIT (10*10*1024)

/* local function prototypes */

void printTimeCode(MP4DecTime * timecode);
static u32 readDecodeUnit(FILE * fp, u8 * frameBuffer, void *decInst);
static void Error(char *format, ...);
void decRet(MP4DecRet ret);
void printMP4Version(void);
void GetUserData(MP4DecInst pDecInst,
                 MP4DecInput DecIn, MP4DecUserDataType type);

static MP4DecRet writeOutputPicture(char *filename, char *filenameTiled,
                                    MP4DecInst decoder, u32 outp_byte_size,
                                    u32 vopNumber, u32 output_picture_endian,
                                    MP4DecRet ret, u32 end,
                                    u32 frameWidth, u32 frameHeight);

#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 vopNumber, MP4DecPicture DecPicture,
                    MP4DecInst decoder);
#endif

void decsw_performance(void);
u32 MP4GetResyncLength(MP4DecInst decInst, u8 * pStrm);

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
u8 disableH263 = 0;
u32 cropOutput = 0;
u8 disableResync = 1;
u8 strmEnd = 0;
u8 *streamStop = NULL;
u32 strmOffset = 4;

u32 streamSize = 0;
u32 stopDecoding = 0;
u32 streamTruncate = 0;
u32 streamPacketLoss = 0;
u32 picRdy = 0;
u32 hdrsRdy = 0;
u32 streamHeaderCorrupt = 0;
TBCfg tbCfg;

/* Give stream to decode as one chunk */
u32 wholeStreamMode = 0;
u32 cumulativeErrorMbs = 0;

u32 usePeekOutput = 0;
u32 skipNonReference = 0;

u32 planarOutput = 0;
u32 secondField = 1;         /* for field pictures, flag that
                              * whole frame ready after this field */
u32 customWidth = 0;
u32 customHeight = 0;
u32 customDimensions = 0; /* Implicates that raw bitstream does not carry 
                           * frame dimensions */
u32 noStartCodes = 0;     /* If raw bitstream doesn't carry startcodes */

MP4DecStrmFmt strmFmt = MP4DEC_MPEG4;
u32 tiledOutput = DEC_REF_FRM_RASTER_SCAN;
u32 convertTiledOutput = 0;
u32 dpbMode = DEC_DPB_DEFAULT;
u32 convertToFrameDpb = 0;

FILE *findex = NULL;
u32 saveIndex = 0;
u32 useIndex = 0;
off64_t curIndex = 0;
off64_t nextIndex = 0;

#ifdef ASIC_TRACE_SUPPORT
extern u32 hwDecPicCount;
extern u32 useReferenceIdct;
extern u32 gHwVer;
#endif

#ifdef MPEG4_EVALUATION
extern u32 gHwVer;
#endif

int main(int argc, char **argv)
{
    FILE *fTBCfg;
    u8 *pStrmData = 0;
    u32 maxNumVops;

    u32 i, tmp, streamLen = 0;
    u32 outp_byte_size;
    u32 timeResolution;
    u32 timeHundreds;
    u32 vopNumber = 0, vpNum = 0;
    u32 rlcMode = 0;
    u32 numFrameBuffers = 0;

    /*
     * Decoder API structures
     */
    MP4DecInst decoder;
    MP4DecRet ret;
    MP4DecRet infoRet;
    MP4DecRet tmpRet;
    MP4DecInput DecIn;
    MP4DecOutput DecOut;
    MP4DecInfo Decinfo;
    MP4DecPicture DecPicture;
    DWLLinearMem_t streamMem;
    u32 picId = 0;

    char outFileName[256] = "";
    char outFileNameTiled[256] = "out_tiled.yuv";

    FILE *fIn = NULL;
    FILE *fConfig = NULL;
    FILE *fPipMask = NULL;

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

#ifdef MPEG4_EVALUATION_8170
    gHwVer = 8170;
#elif MPEG4_EVALUATION_8190
    gHwVer = 8190;
#elif MPEG4_EVALUATION_9170
    gHwVer = 9170;
#elif MPEG4_EVALUATION_9190
    gHwVer = 9190;
#elif MPEG4_EVALUATION_G1
    gHwVer = 10000;
#endif    
    
#ifndef PP_PIPELINE_ENABLED
    if(argc < 2)
    {

        printf("\nx170 MPEG-4 Decoder Testbench\n\n");
        printf("USAGE:\n%s [options] stream.mpeg4\n", argv[0]);
        printf("-Ooutfile write output to \"outfile\" (default out.yuv)\n");
        printf("-Nn to decode only first n vops of the stream\n");
        printf("-X to not to write output picture\n");
        printf("-Bn to use n frame buffers in decoder\n");
        printf("-Sfile.hex stream control trace file\n");
#ifdef ASIC_TRACE_SUPPORT
        printf("-R use reference decoder IDCT (sw/sw integration only)\n");
#endif
        printf("-W whole stream mode - give stream to decoder in one chunk\n");
        printf("-P write planar output\n");
        printf("-I save index file\n");
        printf("-E use tiled reference frame format.\n");
        printf("-G convert tiled output pictures to raster scan\n");
        printf("-F decode Sorenson Spark stream\n");
        printf
            ("-C crop output picture to real picture dimensions (only planar)\n");
        printf("-J decode DivX4 or DivX5 stream\n");
        printf("-D<width>x<height> decode DivX3 stream of resolution width x height\n");		    
        printf("-Q Skip decoding non-reference pictures.\n"); 
        printf("-Z output pictures using MP4DecPeek() function\n");
        printf("--separate-fields-in-dpb DPB stores interlaced content"\
                    " as fields (default: frames)\n");
        printf("--output-frame-dpb Convert output to frame mode even if"\
            " field DPB mode used\n");
        printMP4Version();
        exit(100);
    }

    maxNumVops = 0;
    for(i = 1; i < argc - 1; i++)
    {
        if(strncmp(argv[i], "-O", 2) == 0)
        {
            strcpy(outFileName, argv[i] + 2);
        }
        else if(strncmp(argv[i], "-N", 2) == 0)
        {
            maxNumVops = atoi(argv[i] + 2);
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
        else if(strncmp(argv[i], "-B", 2) == 0)
        {
            numFrameBuffers = atoi(argv[i] + 2);
            if(numFrameBuffers > 16)
                numFrameBuffers = 16;
        }
#ifdef ASIC_TRACE_SUPPORT
        else if(strncmp(argv[i], "-R", 2) == 0)
        {
            useReferenceIdct = 1;
        }
#endif
        else if(strncmp(argv[i], "-W", 2) == 0)
        {
            wholeStreamMode = 1;
        }
        else if (strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if (strncmp(argv[i], "-G", 2) == 0)
            convertTiledOutput = 1;        
        else if(strcmp(argv[i], "-F") == 0)
        {
            strmFmt = MP4DEC_SORENSON;
        }
        else if(strcmp(argv[i], "-J") == 0)
        {
            strmFmt = MP4DEC_CUSTOM_1;
        }
        else if(strcmp(argv[i], "-Z") == 0)
        {
            usePeekOutput = 1;
        }
        else if(strcmp(argv[i], "-Q") == 0)
        {
            skipNonReference = 1;
        }
        else if(strncmp(argv[i], "-D", 2) == 0)
        {
            u32 frameWidth, frameHeight;
            customDimensions = 1;
            noStartCodes = 1;
            tmp = sscanf(argv[i]+2, "%dx%d", &customWidth, &customHeight );
            if( tmp != 2 )
            {
                printf("MALFORMED WIDTHxHEIGHT: %s\n", argv[i]+2);
                return 1;
            }
            strmOffset = 0;
            frameWidth = (customWidth+15)&~15;
            frameHeight = (customHeight+15)&~15;
            outp_byte_size =
                (frameWidth * frameHeight * 3) >> 1;
            /* If -O option not used, generate default file name */
            if(outFileName[0] == 0)
            {
                if(planarOutput && cropOutput)
                    sprintf(outFileName, "out_w%dh%d.yuv",
                            customWidth, customHeight);
                else
                    sprintf(outFileName, "out_w%dh%d.yuv",
                            frameWidth, frameHeight );
            }
        }
        else if(strcmp(argv[i], "-C") == 0)
        {
            cropOutput = 1;
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

    printMP4Version();
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
        printf("\nMPEG-4 Decoder PP Pipelined Testbench\n\n");
        printf("USAGE:\n%s [options] stream.mpeg4 pp.cfg\n", argv[0]);
        printf("-Nn to decode only first n vops of the stream\n");
        printf("-X to not to write output picture\n");
        printf("-W whole stream mode - give stream to decoder in one chunk\n");
        printf("-Bn to use n frame buffers in decoder\n");
        printf("-I save index file\n");
        printf("-E use tiled reference frame format.\n");
        printf("-F decode Sorenson Spark stream\n");
        printf("-J decode DivX4 or DivX5 stream\n");
        printf("-D<width>x<height> decode DivX3 stream of resolution width x height\n");		    
        printf("-Q Skip decoding non-reference pictures.\n"); 
        printf("--separate-fields-in-dpb DPB stores interlaced content"\
                    " as fields (default: frames)\n");
        exit(100);
    }

    remove("pp_out.yuv");
    maxNumVops = 0;
    /* read cmdl parameters */
    for(i = 1; i < argc - 2; i++)
    {
        if(strncmp(argv[i], "-N", 2) == 0)
        {
            maxNumVops = atoi(argv[i] + 2);
        }
        else if(strncmp(argv[i], "-X", 2) == 0)
        {
            writeOutput = 0;
        }
        else if(strncmp(argv[i], "-B", 2) == 0)
        {
            numFrameBuffers = atoi(argv[i] + 2);
            if(numFrameBuffers > 16)
                numFrameBuffers = 16;
        }
        else if(strncmp(argv[i], "-W", 2) == 0)
        {
            wholeStreamMode = 1;
        }
        else if (strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if(strcmp(argv[i], "-F") == 0)
        {
            strmFmt = MP4DEC_SORENSON;
        }
        else if(strcmp(argv[i], "-J") == 0)
        {
            strmFmt = MP4DEC_CUSTOM_1;
        }
        else if(strcmp(argv[i], "-Q") == 0)
        {
            skipNonReference = 1;
        }
        else if(strncmp(argv[i], "-D", 2) == 0)
        {
            u32 frameWidth, frameHeight;
            customDimensions = 1;
            noStartCodes = 1;
            tmp = sscanf(argv[i]+2, "%dx%d", &customWidth, &customHeight );
            if( tmp != 2 )
            {
                printf("MALFORMED WIDTHxHEIGHT: %s\n", argv[i]+2);
                return 1;
            }
            strmOffset = 0;
            frameWidth = (customWidth+15)&~15;
            frameHeight = (customHeight+15)&~15;
            outp_byte_size =
                (frameWidth * frameHeight * 3) >> 1;
            /* If -O option not used, generate default file name */
            if(outFileName[0] == 0)
            {
                if(planarOutput && cropOutput)
                    sprintf(outFileName, "out_w%dh%d.yuv",
                            customWidth, customHeight);
                else
                    sprintf(outFileName, "out_w%dh%d.yuv",
                            frameWidth, frameHeight );
            }
        }
        else if(strcmp(argv[i], "-I") == 0)
        {
            saveIndex = 1;
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

    printMP4Version();
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
            "\nX170 PP API v%d.%d - SW build: %d - HW build: %x\n",
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

    rlcMode = TBGetDecRlcModeForced(&tbCfg);
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
    printf("Decoder RLC %d\n", rlcMode);
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
    disableResync = !TBGetTBPacketByPacket(&tbCfg);
    printf("TB Packet by Packet  %d\n", !disableResync);
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
    if(streamTruncate && disableResync)
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
#endif
    if(!tmp)
    {
        printf("UNABLE TO OPEN TRACE FILES(S)\n");
    }

    /*
     * Initialize the decoder
     */
    START_SW_PERFORMANCE;
    decsw_performance();
    ret = MP4DecInit(&decoder, strmFmt,
                     TBGetDecIntraFreezeEnable( &tbCfg ), 
                     numFrameBuffers,
                     tiledOutput | 
        (dpbMode == DEC_DPB_INTERLACED_FIELD ? DEC_DPB_ALLOW_FIELD_ORDERING : 0));
    END_SW_PERFORMANCE;
    decsw_performance();

    if(ret != MP4DEC_OK)
    {
        decoder = NULL;
        printf("Could not initialize decoder\n");
        goto end;
    }    

    /* Set ref buffer test mode */
    ((DecContainer *) decoder)->refBufferCtrl.testFunction = TBRefbuTestMode;

    /* Check if we have to supply decoder with custom frame dimensions */
    if(customDimensions)
    {
        START_SW_PERFORMANCE;
        decsw_performance();
        MP4DecSetInfo( decoder, customWidth, customHeight );
        END_SW_PERFORMANCE;
        decsw_performance();
        TBSetRefbuMemModel( &tbCfg, 
            ((DecContainer *) decoder)->mp4Regs,
            &((DecContainer *) decoder)->refBufferCtrl );
    }
    
    DecIn.enableDeblock = 0;
#ifdef PP_PIPELINE_ENABLED
    /* Initialize the post processer. If unsuccessful -> exit */
    if(pp_startup
       (argv[argc - 1], decoder, PP_PIPELINED_DEC_TYPE_MPEG4, &tbCfg) != 0)
    {
        fprintf(stdout, "PP INITIALIZATION FAILED\n");
        goto end;
    }

#endif

    if(DWLMallocLinear(((DecContainer *) decoder)->dwl,
                       STREAMBUFFER_BLOCKSIZE, &streamMem) != DWL_OK)
    {
        printf(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
        goto end;
    }

    byteStrmStart = (u8 *) streamMem.virtualAddress;

    DecIn.pStream = byteStrmStart;
    DecIn.streamBusAddress = (u32) streamMem.busAddress;


    streamStop = byteStrmStart + length;
    /* NOTE: The registers should not be used outside decoder SW for other
     * than compile time setting test purposes */
    SetDecRegister(((DecContainer *) decoder)->mp4Regs, HWIF_DEC_LATENCY,
                   latency_comp);
    SetDecRegister(((DecContainer *) decoder)->mp4Regs, HWIF_DEC_CLK_GATE_E,
                   clock_gating);
    /*
    SetDecRegister(((DecContainer *) decoder)->mp4Regs, HWIF_DEC_OUT_TILED_E,
                   output_format);
                   */
    SetDecRegister(((DecContainer *) decoder)->mp4Regs, HWIF_DEC_OUT_ENDIAN,
                   output_picture_endian);
    SetDecRegister(((DecContainer *) decoder)->mp4Regs, HWIF_DEC_MAX_BURST,
                   bus_burst_length);
    if ((DWLReadAsicID() >> 16) == 0x8170U)
    {
        SetDecRegister(((DecContainer *) decoder)->mp4Regs, HWIF_PRIORITY_MODE,
                       asic_service_priority);
    }
    SetDecRegister(((DecContainer *) decoder)->mp4Regs, HWIF_DEC_DATA_DISC_E,
                   data_discard);

    if(rlcMode)
    {
        printf("RLC mode forced\n");
        /*Force the decoder into RLC mode */
        ((DecContainer *) decoder)->rlcMode = 1;
    }

    /* Read what kind of stream is coming */
    START_SW_PERFORMANCE;
    decsw_performance();
    infoRet = MP4DecGetInfo(decoder, &Decinfo);
    END_SW_PERFORMANCE;
    decsw_performance();
    if(infoRet)
    {
        decRet(infoRet);
    }


#ifdef PP_PIPELINE_ENABLED
    if(customDimensions)
    {

        if(pp_update_config
           (decoder, PP_PIPELINED_DEC_TYPE_MPEG4, &tbCfg) == CFG_UPDATE_FAIL)

        {
            fprintf(stdout, "PP CONFIG LOAD FAILED\n");
        }
        /* If unspecified at cmd line, use minimum # of buffers, otherwise
         * use specified amount. */
        if(numFrameBuffers == 0)
            pp_number_of_buffers(Decinfo.multiBuffPpSize);
        else
            pp_number_of_buffers(numFrameBuffers);
    }
#endif
/*
    fout = fopen(outFileName, "wb");
    if(fout == NULL)
    {

        printf("Could not open output file\n");
        goto end2;
    }*/
#ifdef ASIC_TRACE_SUPPORT
    if(1 == infoRet)
        trace_mpeg4DecodingTools.shortVideo = 1;
#endif

    pStrmData = (u8 *) DecIn.pStream;
    DecIn.skipNonReference = skipNonReference;

    /* Read vop headers */

    do
    {
        streamLen = readDecodeUnit(fIn, pStrmData, decoder);
    } while (!feof(fIn) && streamLen && streamLen <= 4);

    i = StartCode;
    /* decrease 4 because previous function call
     * read the first VOP start code */

    if( !noStartCodes )
        streamLen -= 4;
    DecIn.dataLen = streamLen;
    DecOut.dataLeft = 0;
    printf("Start decoding\n");
    do
    {
        /*printf("DecIn.dataLen %d\n", DecIn.dataLen);*/
        DecIn.picId = picId;
        if(ret != MP4DEC_STRM_PROCESSED &&
           ret != MP4DEC_NONREF_PIC_SKIPPED)
            printf("Starting to decode picture ID %d\n", picId);

        if(rlcMode)
        {
            printf("RLC mode forced \n");
            /*Force the decoder into RLC mode */
            ((DecContainer *) decoder)->rlcMode = 1;
        }

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
        ret = MP4DecDecode(decoder, &DecIn, &DecOut);
        END_SW_PERFORMANCE;
        decsw_performance();

        decRet(ret);

        /*
         * Choose what to do now based on the decoder return value
         */

        switch (ret)
        {

        case MP4DEC_HDRS_RDY:
        case MP4DEC_DP_HDRS_RDY:

            /* Set a flag to indicate that headers are ready */
            hdrsRdy = 1;
            TBSetRefbuMemModel( &tbCfg, ((DecContainer *) decoder)->mp4Regs,
                &((DecContainer *) decoder)->refBufferCtrl );

            /* Read what kind of stream is coming */
            START_SW_PERFORMANCE;
            decsw_performance();
            infoRet = MP4DecGetInfo(decoder, &Decinfo);
            END_SW_PERFORMANCE;
            decsw_performance();
            if(infoRet)
            {
                decRet(infoRet);
            }
            
            dpbMode = Decinfo.dpbMode;
            
            outp_byte_size =
                (Decinfo.frameWidth * Decinfo.frameHeight * 3) >> 1;

            if (Decinfo.interlacedSequence)
                printf("INTERLACED SEQUENCE\n");

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

            if(!vopNumber)
            {
                /*disableResync = 1; */
                disableH263 = !Decinfo.streamFormat;
                if(Decinfo.streamFormat)
                    printf("%s stream\n",
                           Decinfo.streamFormat ==
                           1 ? "MPEG-4 short video" : "h.263");
                else
                    printf("MPEG-4 stream\n");

                printf("Profile and level %d\n",
                       Decinfo.profileAndLevelIndication);
                printf("Pixel Aspect ratio %d : %d\n",
                       Decinfo.parWidth, Decinfo.parHeight);
                printf("Output format %s\n",
                       Decinfo.outputFormat == MP4DEC_SEMIPLANAR_YUV420
                       ? "MP4DEC_SEMIPLANAR_YUV420" : "MP4DEC_TILED_YUV420");
            }
            else
            {
                tmpRet = writeOutputPicture(outFileName, outFileNameTiled, decoder,
                                        outp_byte_size,
                                        vopNumber,
                                        output_picture_endian,
                                        ret, 0, Decinfo.frameWidth,
                                        Decinfo.frameHeight);
            }
            /* get user data if present.
             * Valid for only current stream buffer */
            if(Decinfo.userDataVOSLen != 0)
                GetUserData(decoder, DecIn, MP4DEC_USER_DATA_VOS);
            if(Decinfo.userDataVISOLen != 0)
                GetUserData(decoder, DecIn, MP4DEC_USER_DATA_VISO);
            if(Decinfo.userDataVOLLen != 0)
                GetUserData(decoder, DecIn, MP4DEC_USER_DATA_VOL);

            /*printf("DecOut.dataLeft %d \n", DecOut.dataLeft);*/
            if(DecOut.dataLeft && !noStartCodes)
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
                    streamLen = readDecodeUnit(fIn, pStrmData + strmOffset, decoder);
                    if(!feof(fIn) && streamLen && streamLen <= 4 &&
                        !noStartCodes )
                    {
                        for ( i = 0 ; i < 4 ; ++i )
                            pStrmData[i] = pStrmData[4+i];
                        streamLen = readDecodeUnit(fIn, pStrmData + strmOffset, decoder);
                    }
                    /*streamPacketLoss = streamPacketLossTmp;*/
                }
                DecIn.dataLen = streamLen;
                DecOut.dataLeft = 0; 

            	corruptedBytes = 0;

            }

#ifdef PP_PIPELINE_ENABLED
            pp_set_input_interlaced(Decinfo.interlacedSequence);

            if(pp_update_config
               (decoder, PP_PIPELINED_DEC_TYPE_MPEG4, &tbCfg) == CFG_UPDATE_FAIL)

            {
                fprintf(stdout, "PP CONFIG LOAD FAILED\n");
                goto end2;
            }
            DecIn.enableDeblock = pp_mpeg4_filter_used();
            if(DecIn.enableDeblock == 1)
                printf("Deblocking filter enabled\n");
            else if(DecIn.enableDeblock == 0)
                printf("Deblocking filter disabled\n");
            
            /* If unspecified at cmd line, use minimum # of buffers, otherwise
             * use specified amount. */
            if(numFrameBuffers == 0)
                pp_number_of_buffers(Decinfo.multiBuffPpSize);
            else
                pp_number_of_buffers(numFrameBuffers);

#endif
            break;

        case MP4DEC_PIC_DECODED:
            /* Picture is now ready */
            picRdy = 1;
            /* Read what kind of stream is coming */
            picId++;
            START_SW_PERFORMANCE;
            decsw_performance();
            infoRet = MP4DecGetInfo(decoder, &Decinfo);
            END_SW_PERFORMANCE;
            decsw_performance();
            if(infoRet)
            {
                decRet(infoRet);
            }

            tmpRet = writeOutputPicture(outFileName, outFileNameTiled, decoder,
                                        outp_byte_size,
                                        vopNumber,
                                        output_picture_endian,
                                        ret, 0, Decinfo.frameWidth,
                                        Decinfo.frameHeight);

            vopNumber++;
            vpNum = 0;

            /*printf("DecOut.dataLeft %d \n", DecOut.dataLeft);*/
            if(DecOut.dataLeft && !noStartCodes)
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

                    printf("READ NEXT DECODE UNIT\n");

                    if(!picRdy)
                        streamPacketLoss = 0;*/
                    streamLen = readDecodeUnit(fIn, pStrmData + strmOffset, decoder);
                    if(!feof(fIn) && streamLen && streamLen <= 4 &&
                        !noStartCodes)
                    {
                        for ( i = 0 ; i < 4 ; ++i )
                            pStrmData[i] = pStrmData[4+i];
                        streamLen = readDecodeUnit(fIn, pStrmData + strmOffset, decoder);
                    }
                    /*streamPacketLoss = streamPacketLossTmp;*/
                }
                DecIn.dataLen = streamLen;
                DecOut.dataLeft = 0; 

            	corruptedBytes = 0;

            }

            if(maxNumVops && (vopNumber >= maxNumVops))
            {
                printf("\n\nMax num of pictures reached\n\n");
                DecIn.dataLen = 0;
                goto end2;
            }

            break;

        case MP4DEC_STRM_PROCESSED:
        case MP4DEC_NONREF_PIC_SKIPPED:
            fprintf(stdout,
                    "TB: Video packet Number: %u, vop: %d\n", vpNum++,
                    vopNumber);
            /* Used to indicate that picture decoding needs to
             * finalized prior to corrupting next picture */

            /* fallthrough */

        case MP4DEC_OK:

            if((ret == MP4DEC_OK) && (StartCode == VOP_START_CODE))
            {
                fprintf(stdout, "\nTB: ...::::!! The decoder missed"
                        "a VOP startcode !!::::...\n\n");
            }

            /* Read what kind of stream is coming */
            START_SW_PERFORMANCE;
            decsw_performance();
            infoRet = MP4DecGetInfo(decoder, &Decinfo);
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

            /*printf("DecOut.dataLeft %d \n", DecOut.dataLeft);*/
            if(DecOut.dataLeft && !noStartCodes)
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
                    streamLen = readDecodeUnit(fIn, pStrmData + strmOffset, decoder);
                    if(!feof(fIn) && streamLen && streamLen <= 4 &&
                        !noStartCodes)
                    {
                        for ( i = 0 ; i < 4 ; ++i )
                            pStrmData[i] = pStrmData[4+i];
                        streamLen = readDecodeUnit(fIn, pStrmData + strmOffset, decoder);
                    }
                    /*streamPacketLoss = streamPacketLossTmp;*/
                }
                DecIn.dataLen = streamLen;
                DecOut.dataLeft = 0; 

            	corruptedBytes = 0;

            }

            break;

        case MP4DEC_VOS_END:
            printf("Video object seq end\n");
            /*DecIn.dataLen = 0;*/
            /*printf("DecOut.dataLeft %d \n", DecOut.dataLeft);*/
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
                    streamLen = readDecodeUnit(fIn, pStrmData + strmOffset, decoder);
                    if(!feof(fIn) && streamLen && streamLen <= 4 &&
                        !noStartCodes)
                    {
                        for ( i = 0 ; i < 4 ; ++i )
                            pStrmData[i] = pStrmData[4+i];
                        streamLen = readDecodeUnit(fIn, pStrmData + strmOffset, decoder);
                    }                    /*streamPacketLoss = streamPacketLossTmp;*/
                }
                DecIn.dataLen = streamLen;
                DecOut.dataLeft = 0; 

            	corruptedBytes = 0;
            }

            break;

        case MP4DEC_PARAM_ERROR:
            printf("INCORRECT STREAM PARAMS\n");
            goto end2;
            break;

        case MP4DEC_STRM_ERROR:
            /* Used to indicate that picture decoding needs to
             * finalized prior to corrupting next picture
            picRdy = 0;*/
            printf("STREAM ERROR\n");
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
                streamLen = readDecodeUnit(fIn, pStrmData + strmOffset, decoder);
                if(!feof(fIn) && streamLen && streamLen <= 4 &&
                        !noStartCodes)
                {
                    for ( i = 0 ; i < 4 ; ++i )
                        pStrmData[i] = pStrmData[4+i];
                    streamLen = readDecodeUnit(fIn, pStrmData + strmOffset, decoder);
                }
                /*streamPacketLoss = streamPacketLossTmp;*/
            }
            DecIn.dataLen = streamLen;
            DecOut.dataLeft = 0; 
            /*goto end2; */
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

    do
    {
        tmpRet = writeOutputPicture(outFileName, outFileNameTiled,
                                    decoder, outp_byte_size,
                                    vopNumber, output_picture_endian, ret, 1,
                                    Decinfo.frameWidth, Decinfo.frameHeight);
    }
    while(tmpRet == MP4DEC_PIC_RDY);

    START_SW_PERFORMANCE;
    decsw_performance();
    MP4DecGetInfo(decoder, &Decinfo);
    END_SW_PERFORMANCE;
    decsw_performance();

end:    
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
    if (decoder != NULL) MP4DecRelease(decoder);
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
               vopNumber);
    }
    printf("decoded %d pictures\n", vopNumber);

    if(fIn)
        fclose(fIn);

    if(fout)
        fclose(fout);

    if(fTiledOutput)
        fclose(fTiledOutput);

#ifdef ASIC_TRACE_SUPPORT
    trace_SequenceCtrl(hwDecPicCount, bFrames);
    trace_MPEG4DecodingTools();
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

    FINALIZE_SW_PERFORMANCE;

    if(cumulativeErrorMbs || !vopNumber)
    {
        printf("ERRORS FOUND\n");
        return (1);
    }
    else
        return (0);
}


/*------------------------------------------------------------------------------
        readDecodeUnitNoSc
        Description : search VOP start code  or video
        packet start code and read one decode unit at a time
------------------------------------------------------------------------------*/
static u32 readDecodeUnitNoSc(FILE * fp, u8 * frameBuffer, void *decInst)
{
    u8  sizeTmp[4];
    u32 size, tmp;
    static off64_t offset = 0;
    static u32 count = 0;

    if(useIndex)
    {
        fscanf(findex, "%llu", &nextIndex);
        if(feof(findex))
        {
            strmEnd = 1;
            return 0;  
        }

        if(ftello64(fp) != nextIndex)
        {
            fseek(fp, nextIndex, SEEK_SET);
        }

        fread(&sizeTmp, sizeof(u8), 4, fp);
        
    }
    else
    {
        /* skip "00dc" from frame beginning (may signal video chunk start code).
         * also skip "0000" in case stream contains zero-size packets */
        for(;;)
        {
            fseeko64( fp, offset, SEEK_SET );
            if (fread( &sizeTmp, sizeof(u8), 4, fp ) != 4)
                break;
            if( ( sizeTmp[0] == '0' &&
                  sizeTmp[1] == '0' &&
                  sizeTmp[2] == 'd' &&
                  sizeTmp[3] == 'c' ) ||
                ( sizeTmp[0] == 0x0 &&
                  sizeTmp[1] == 0x0 &&
                  sizeTmp[2] == 0x0 &&
                  sizeTmp[3] == 0x0 ) )
            {
                offset += 4;
                continue;
            }
            break;
        }
    }

    size = (sizeTmp[0]) +
           (sizeTmp[1] << 8) +
           (sizeTmp[2] << 16) +
           (sizeTmp[3] << 24);

    if( size == -1 )
    {
        strmEnd = 1;
        return 0;
    }

    if(saveIndex && !useIndex)
    {
        fprintf(findex, "%llu\n", offset);
    }

    tmp = fread( frameBuffer, sizeof(u8), size, fp );
    if( size != tmp )
    {
        strmEnd = 1;
        return 0;
    }

    offset += size + 4;
    return size;

}

/*------------------------------------------------------------------------------
        readDecodeUnit
        Description : search VOP start code  or video
        packet start code and read one decode unit at a time
------------------------------------------------------------------------------*/
static u32 readDecodeUnit(FILE * fp, u8 * frameBuffer, void *decInst)
{

    u32 idx = 0, VopStart = 0;
    u8 temp;
    u8 nextPacket = 0;
    static u32 resyncMarkerLength = 0;

    if( noStartCodes )
    {
        return readDecodeUnitNoSc(fp, frameBuffer, decInst);
    }

    StartCode = 0;

    if(stopDecoding)
    {
        printf("Truncated stream size reached -> stop decoding\n");
        return 0;
    }

    /* If enabled, lose the packets (skip this packet first though) */
    if(streamPacketLoss && !disableResync)
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
        
        /* read index */
        fscanf(findex, "%llu", &nextIndex);
        amount = nextIndex - curIndex;

        idx = fread(frameBuffer, 1, amount, fp);

        /* start code */
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
            if((idx > (length - MP4_WHOLE_STREAM_SAFETY_LIMIT)) && wholeStreamMode)
            {

                wholeStreamMode = 0;

            }

            frameBuffer[idx] = temp;

            if(idx >= 3)
            {

                if(!wholeStreamMode)
                {

                    /*-----------------------------------
                        H263 Start code

                    -----------------------------------*/
                    if((strmFmt == MP4DEC_SORENSON) && (idx >= 7))
                    {
                        if((frameBuffer[idx - 3] == 0x00) &&
                           (frameBuffer[idx - 2] == 0x00) &&
                           ((frameBuffer[idx - 1] & 0xF8) == 0x80))
                        {
                            VopStart = 1;
                            StartCode = ((frameBuffer[idx] << 24) |
                                         (frameBuffer[idx - 1] << 16) |
                                         (frameBuffer[idx - 2] << 8) |
                                         frameBuffer[idx - 3]);
                        }
                    }
                    else if(!disableH263 && (idx >= 7))
                    {
                        if((frameBuffer[idx - 3] == 0x00) &&
                           (frameBuffer[idx - 2] == 0x00) &&
                           ((frameBuffer[idx - 1] & 0xFC) == 0x80))
                        {
                            VopStart = 1;
                            StartCode = ((frameBuffer[idx] << 24) |
                                         (frameBuffer[idx - 1] << 16) |
                                         (frameBuffer[idx - 2] << 8) |
                                         frameBuffer[idx - 3]);
                        }
                    }
                    /*-----------------------------------
                        MPEG4 Start code
                    -----------------------------------*/
                    if(((frameBuffer[idx - 3] == 0x00) &&
                        (frameBuffer[idx - 2] == 0x00) &&
                        (((frameBuffer[idx - 1] == 0x01) &&
                          (frameBuffer[idx] == 0xB6)))))
                    {
                        VopStart = 1;
                        StartCode = ((frameBuffer[idx] << 24) |
                                     (frameBuffer[idx - 1] << 16) |
                                     (frameBuffer[idx - 2] << 8) |
                                     frameBuffer[idx - 3]);
                        /* if MPEG4 start code found,
                         * no need to look for H263 start code */
                        disableH263 = 1;
                        resyncMarkerLength = 0;

                    }

                    /*-----------------------------------
                        resync_marker
                    -----------------------------------*/

                    else if(disableH263 && !disableResync)
                    {
                        if((frameBuffer[idx - 3] == 0x00) &&
                           (frameBuffer[idx - 2] == 0x00) &&
                           (frameBuffer[idx - 1] > 0x01))
                        {

                            if(resyncMarkerLength == 0)
                            {
                                resyncMarkerLength = MP4GetResyncLength(decInst,
                                                                        frameBuffer);
                            }
                            if((frameBuffer[idx - 1] >> (24 - resyncMarkerLength))
                               == 0x1)
                            {
                                VopStart = 1;
                                StartCode = ((frameBuffer[idx] << 24) |
                                             (frameBuffer[idx - 1] << 16) |
                                             (frameBuffer[idx - 2] << 8) |
                                             frameBuffer[idx - 3]);
                            }
                        }
                    }
                    /*-----------------------------------
                        VOS end code
                    -----------------------------------*/
                    if(idx >= 7)
                    {
                        if(((frameBuffer[idx - 3] == 0x00) &&
                            (frameBuffer[idx - 2] == 0x00) &&
                            (((frameBuffer[idx - 1] == 0x01) &&
                              (frameBuffer[idx] == 0xB1)))))
                        {

                            /* stream end located */
                            VopStart = 1;
                            StartCode = ((frameBuffer[idx] << 24) |
                                         (frameBuffer[idx - 1] << 16) |
                                         (frameBuffer[idx - 2] << 8) |
                                         frameBuffer[idx - 3]);
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
            if(streamTruncate && disableResync)
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
        return readDecodeUnit(fp, frameBuffer, decInst);
    }
    else
    {
        /*printf("No Packet Loss\n");*/
        if (!disableResync && picRdy && streamTruncate && ((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt))
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

void printTimeCode(MP4DecTime * timecode)
{

    fprintf(stdout, "hours %u, "
            "minutes %u, "
            "seconds %u, "
            "timeIncr %u, "
            "timeRes %u",
            timecode->hours,
            timecode->minutes,
            timecode->seconds, timecode->timeIncr, timecode->timeRes);
}

/*------------------------------------------------------------------------------
        decRet
        Description : Print out Decoder return values
------------------------------------------------------------------------------*/

void decRet(MP4DecRet ret)
{

    printf("Decode result: ");

    switch (ret)
    {
    case MP4DEC_OK:
        printf("MP4DEC_OK\n");
        break;
    case MP4DEC_STRM_PROCESSED:
        printf("MP4DEC_STRM_PROCESSED\n");
        break;
    case MP4DEC_NONREF_PIC_SKIPPED:
        printf("MP4DEC_NONREF_PIC_SKIPPED\n");
        break;
    case MP4DEC_PIC_RDY:
        printf("MP4DEC_PIC_RDY\n");
        break;
    case MP4DEC_HDRS_RDY:
        printf("MP4DEC_HDRS_RDY\n");
        break;
    case MP4DEC_DP_HDRS_RDY:
        printf("MP4DEC_DP_HDRS_RDY\n");
        break;
    case MP4DEC_PARAM_ERROR:
        printf("MP4DEC_PARAM_ERROR\n");
        break;
    case MP4DEC_STRM_ERROR:
        printf("MP4DEC_STRM_ERROR\n");
        break;
    case MP4DEC_NOT_INITIALIZED:
        printf("MP4DEC_NOT_INITIALIZED\n");
        break;
    case MP4DEC_MEMFAIL:
        printf("MP4DEC_MEMFAIL\n");
        break;
    case MP4DEC_DWL_ERROR:
        printf("MP4DEC_DWL_ERROR\n");
        break;
    case MP4DEC_HW_BUS_ERROR:
        printf("MP4DEC_HW_BUS_ERROR\n");
        break;
    case MP4DEC_SYSTEM_ERROR:
        printf("MP4DEC_SYSTEM_ERROR\n");
        break;
    case MP4DEC_HW_TIMEOUT:
        printf("MP4DEC_HW_TIMEOUT\n");
        break;
    case MP4DEC_HDRS_NOT_RDY:
        printf("MP4DEC_HDRS_NOT_RDY\n");
        break;
    case MP4DEC_PIC_DECODED:
        printf("MP4DEC_PIC_DECODED\n");
        break;
    case MP4DEC_FORMAT_NOT_SUPPORTED:
        printf("MP4DEC_FORMAT_NOT_SUPPORTED\n");
        break;
    case MP4DEC_STRM_NOT_SUPPORTED:
        printf("MP4DEC_STRM_NOT_SUPPORTED\n");
        break;
    default:
        printf("Other %d\n", ret);
        break;
    }
}

/*------------------------------------------------------------------------------

    Function name: printMP4Version

    Functional description: Print version info

    Inputs:

    Outputs:    NONE

    Returns:    NONE

------------------------------------------------------------------------------*/
void printMP4Version(void)
{

    MP4DecApiVersion decVersion;
    MP4DecBuild decBuild;

    /*
     * Get decoder version info
     */

    decVersion = MP4DecGetAPIVersion();
    printf("\nAPI version:  %d.%d, ", decVersion.major, decVersion.minor);

    decBuild = MP4DecGetBuild();
    printf("sw build nbr: %d, hw build nbr: %x\n\n",
           decBuild.swBuild, decBuild.hwBuild);

}

/*------------------------------------------------------------------------------

    Function name: GetUserData

    Functional description: This function is used to get user data from the
                            decoder.

    Inputs:     MP4DecInst pDecInst       decoder instance
                MP4DecUserDataType type   user data type to read

    Outputs:    NONE

    Returns:    NONE

------------------------------------------------------------------------------*/
void GetUserData(MP4DecInst pDecInst,
                 MP4DecInput DecIn, MP4DecUserDataType type)
{
    u32 tmp;
    MP4DecUserConf userDataConfig;
    MP4DecInfo decInfo;
    u8 *data = NULL;
    u32 size = 0;

    /* get info from the decoder */
    tmp = MP4DecGetInfo(pDecInst, &decInfo);
    if(tmp != 0)
    {
        printf(("ERROR, exiting...\n"));
    }
    switch (type)
    {
    case MP4DEC_USER_DATA_VOS:
        size = decInfo.userDataVOSLen;
        data = (u8 *) calloc(size + 1, sizeof(u8));
        userDataConfig.pUserDataVOS = data;
        userDataConfig.userDataVOSMaxLen = size;
        break;
    case MP4DEC_USER_DATA_VISO:
        size = decInfo.userDataVISOLen;
        data = (u8 *) calloc(size + 1, sizeof(u8));
        userDataConfig.pUserDataVISO = data;
        userDataConfig.userDataVISOMaxLen = size;
        break;
    case MP4DEC_USER_DATA_VOL:
        size = decInfo.userDataVOLLen;
        data = (u8 *) calloc(size + 1, sizeof(u8));
        userDataConfig.pUserDataVOL = data;
        userDataConfig.userDataVOLMaxLen = size;
        break;
    case MP4DEC_USER_DATA_GOV:
        size = decInfo.userDataGOVLen;
        data = (u8 *) calloc(size + 1, sizeof(u8));
        userDataConfig.pUserDataGOV = data;
        userDataConfig.userDataGOVMaxLen = size;

        printf("VOS user data size: %d\n", size);
        break;
    default:
        break;
    }
    userDataConfig.userDataType = type;

    /* get user data */
    tmp = MP4DecGetUserData(pDecInst, &DecIn, &userDataConfig);
    if(tmp != 0)
    {
        printf("ERROR, exiting...\n");
    }

    /* print user data */
    if(type == MP4DEC_USER_DATA_VOS)
        printf("VOS user data: %s\n", data);
    else if(type == MP4DEC_USER_DATA_VISO)
        printf("VISO user data: %s\n", data);
    else if(type == MP4DEC_USER_DATA_VOL)
        printf("VOL user data: %s\n", data);
    else if(type == MP4DEC_USER_DATA_GOV)
    {
        printf("\nGOV user data: %s\n", data);
        fflush(stdout);
    }
    /* free allocated memory */
    if(data)
        free(data);
}

/*------------------------------------------------------------------------------

    Function name: writeOutputPicture

    Functional description: write output picture

    Inputs:

    Outputs:

    Returns:    MP4DecRet

------------------------------------------------------------------------------*/

MP4DecRet writeOutputPicture(char *filename, char *filenameTiled,
                             MP4DecInst decoder, u32 outp_byte_size,
                             u32 vopNumber, u32 output_picture_endian,
                             MP4DecRet ret, u32 end,
                             u32 frameWidth, u32 frameHeight)
{
    u8 *pYuvOut = 0;
    MP4DecRet tmpRet;
    MP4DecPicture DecPicture, tmpPicture;
    u32 i, tmp;
    u8 *rasterScan = NULL;

    if (end && usePeekOutput)
        return MP4DEC_OK;
    do
    {
        decsw_performance();
        if (usePeekOutput)
        {
            tmpRet = MP4DecPeek(decoder, &DecPicture);
            while (MP4DecNextPicture(decoder, &tmpPicture, 0) == MP4DEC_PIC_RDY);
        }
        else
            tmpRet = MP4DecNextPicture(decoder, &DecPicture, end ? 1 : 0);
        decsw_performance();

        printf("next picture returns:");
        decRet(tmpRet);
        /* foutput is global file pointer */
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
                    return;
                }
            }
        }

        /* Convert back to raster scan format if decoder outputs 
         * tiled format */
        if(DecPicture.outputFormat != DEC_OUT_FRM_RASTER_SCAN && convertTiledOutput)
        {
            rasterScan = (u8*)malloc(outp_byte_size);
            if(!rasterScan)
            {
                fprintf(stderr, "error allocating memory for tiled"
                    "-->raster conversion!\n");
                return;
            }

            TBTiledToRaster( DecPicture.outputFormat, convertToFrameDpb ? dpbMode : DEC_DPB_DEFAULT, 
                (u8*)DecPicture.pOutputPicture, rasterScan, frameWidth, frameHeight );
            TBTiledToRaster( DecPicture.outputFormat, convertToFrameDpb ? dpbMode : DEC_DPB_DEFAULT, 
                (u8*)DecPicture.pOutputPicture+frameWidth*frameHeight, 
                rasterScan+frameWidth*frameHeight, frameWidth, frameHeight/2 );
            DecPicture.pOutputPicture = rasterScan;
        }
        else if (convertToFrameDpb && (dpbMode != DEC_DPB_FRAME) )
        {

            rasterScan = (u8*)malloc(outp_byte_size);
            if(!rasterScan)
            {
                fprintf(stderr, "error allocating memory for tiled"
                    "-->raster conversion!\n");
                return;
            }

            TBFieldDpbToFrameDpb( dpbMode, 
                (u8*)DecPicture.pOutputPicture, rasterScan, frameWidth, frameHeight );
            TBFieldDpbToFrameDpb( dpbMode,
                (u8*)DecPicture.pOutputPicture+frameWidth*frameHeight, 
                rasterScan+frameWidth*frameHeight, frameWidth, frameHeight/2 );
            DecPicture.pOutputPicture = rasterScan;
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
                    return;
                }
            }
        }
#endif

        if( fout == NULL && fTiledOutput == NULL)
            continue;

        if(tmpRet == MP4DEC_PIC_RDY)
        {
            printf("PIC %d, %s", DecPicture.picId,
                   DecPicture.
                   keyPicture ? "key picture,    " : "non key picture,");

            if(DecPicture.fieldPicture)
                printf(" %s ",
                       DecPicture.topField ? "top field.   " : "bottom field.");
            else
                printf(" frame picture. ");

            printTimeCode(&(DecPicture.timeCode));
            if(DecPicture.nbrOfErrMBs)
            {
                printf(", %d/%d error mbs\n",
                       DecPicture.nbrOfErrMBs,
                       (frameWidth >> 4) * (frameHeight >> 4));
                cumulativeErrorMbs += DecPicture.nbrOfErrMBs;
            }
            else
            {
                printf("\n");

            }

            if((DecPicture.fieldPicture && secondField) ||
               !DecPicture.fieldPicture)
            {                    
                /* Decoder without pp does not write out fields but a
                 * frame containing both fields */
                /* PP output is written field by field */
#ifndef PP_PIPELINE_ENABLED
                if(DecPicture.fieldPicture)
                    secondField = 0;
#endif

                fflush(stdout);
                pYuvOut = (u8 *) DecPicture.pOutputPicture;
                if(writeOutput)
                {
#ifndef PP_PIPELINE_ENABLED
#ifndef ASIC_TRACE_SUPPORT
                    u8 *picCopy = NULL;
#endif
                    assert(pYuvOut != 0);
                    assert(fout != 0);

#ifndef ASIC_TRACE_SUPPORT
                    if(output_picture_endian ==
                       DEC_X170_BIG_ENDIAN)
                    {
                        picCopy = (u8 *) malloc(outp_byte_size);
                        if(NULL == picCopy)
                        {
                            printf("MALLOC FAILED @ %s %d", __FILE__, __LINE__);
                            if(rasterScan)
                                free(rasterScan);
                            return;
                        }
                        memcpy(picCopy, pYuvOut, outp_byte_size);
                        TbChangeEndianess(picCopy, outp_byte_size);
                        pYuvOut = picCopy;
                    }
#endif
                    /* Write  MD5 checksum instead of the frame */
#ifdef MD5SUM
                    TBWriteFrameMD5Sum(fout, pYuvOut,
                                       outp_byte_size, vopNumber);

                    /* tiled */
#if 0
                    if(tiledOutput)
                    {
                        assert(frameWidth % 16 == 0);
                        assert(frameHeight % 16 == 0);
                        TbWriteTiledOutput(fTiledOutput, pYuvOut,
                                           frameWidth >> 4, frameHeight >> 4,
                                           vopNumber, 1 /* write md5sum */ ,
                                           1 /* semi-planar data */ );
                    }
#endif
#else
                    if(!planarOutput)
                    {
                        fwrite(pYuvOut, 1, outp_byte_size, fout);
                        /* tiled */
                        if(tiledOutput)
                        {
                            assert(frameWidth % 16 == 0);
                            assert(frameHeight % 16 == 0);
                            TbWriteTiledOutput(fTiledOutput, pYuvOut,
                                               frameWidth >> 4,
                                               frameHeight >> 4, vopNumber,
                                               0 /* write md5sum */ ,
                                               1 /* semi-planar data */ );
                        }
                    }
                    else if(!cropOutput)
                    {
                        tmp = outp_byte_size * 2 / 3;
                        fwrite(pYuvOut, 1, tmp, fout);
                        for(i = 0; i < tmp / 4; i++)
                            fwrite(pYuvOut + tmp + i * 2, 1, 1, fout);
                        for(i = 0; i < tmp / 4; i++)
                            fwrite(pYuvOut + tmp + 1 + i * 2, 1, 1, fout);
                    }
                    else
                    {
                        u32 j;
                        u8 *p;

                        tmp = outp_byte_size * 2 / 3;
                        p = pYuvOut;
                        for(i = 0; i < DecPicture.codedHeight; i++)
                        {
                            fwrite(p, 1, DecPicture.codedWidth, fout);
                            p += DecPicture.frameWidth;
                        }
                        p = pYuvOut + tmp;
                        for(i = 0; i < DecPicture.codedHeight / 2; i++)
                        {
                            for(j = 0; j < DecPicture.codedWidth / 2; j++)
                                fwrite(p + 2 * j, 1, 1, fout);
                            p += DecPicture.frameWidth;
                        }
                        p = pYuvOut + tmp + 1;
                        for(i = 0; i < DecPicture.codedHeight / 2; i++)
                        {
                            for(j = 0; j < DecPicture.codedWidth / 2; j++)
                                fwrite(p + 2 * j, 1, 1, fout);
                            p += DecPicture.frameWidth;
                        }
                    }
#endif

#ifndef ASIC_TRACE_SUPPORT
                    if(output_picture_endian ==
                       DEC_X170_BIG_ENDIAN)
                    {
                        free(picCopy);
                    }
#endif
#else
                    HandlePpOutput(vopNumber, DecPicture, decoder);

#endif
                }

            }
            else if(DecPicture.fieldPicture)
            {
                secondField = 1;
            }

        }

    }
    while(!usePeekOutput && tmpRet == MP4DEC_PIC_RDY);

    if(rasterScan)
        free(rasterScan);

    return tmpRet;
}

/*------------------------------------------------------------------------------

    Function name: writeOutputPicture

    Functional description: write output picture

    Inputs: vop number, picture description struct, instance

    Outputs:

    Returns:    MP4DecRet

------------------------------------------------------------------------------*/

#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 vopNumber, MP4DecPicture DecPicture, MP4DecInst decoder)
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
       (decoder, PP_PIPELINED_DEC_TYPE_MPEG4, &tbCfg) == CFG_UPDATE_FAIL)

    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
    }

}
#endif

/*------------------------------------------------------------------------------

    Function name: MP4DecTrace

    Functional description: API trace implementation

    Inputs: string

    Outputs:

    Returns: void

------------------------------------------------------------------------------*/
void MP4DecTrace(const char *string)
{
    printf("%s", string);
}

/*------------------------------------------------------------------------------

    Function name: decsw_performance

    Functional description: breakpoint for performance

    Inputs:  void

    Outputs:

    Returns: void

------------------------------------------------------------------------------*/
void decsw_performance(void)
{
}
