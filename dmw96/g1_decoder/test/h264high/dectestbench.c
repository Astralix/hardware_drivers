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
--  Abstract : Command line interface for H.264 decoder
--
------------------------------------------------------------------------------*/

#include "h264decapi.h"
#include "dwl.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef ASIC_TRACE_SUPPORT
#include "trace.h"
#endif

#include "h264hwd_container.h"
#include "tb_cfg.h"
#include "tb_tiled.h"
#include "regdrv.h"

#ifdef PP_PIPELINE_ENABLED
#include "ppapi.h"
#include "pptestbench.h"
#endif

#ifdef MD5SUM
#include "tb_md5.h"
#endif

#include "tb_sw_performance.h"

#ifdef _ENABLE_2ND_CHROMA
#include "h264decapi_e.h"
#endif

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

/* Debug prints */
#undef DEBUG_PRINT
#define DEBUG_PRINT(argv) printf argv

#define NUM_RETRY   100 /* how many retries after HW timeout */
#define ALIGNMENT_MASK 7

void WriteOutput(char *filename, char *filenameTiled, u8 * data, u32 picSize,
                 u32 picWidth, u32 picHeight, u32 frameNumber, u32 monoChrome,
                 u32 view, u32 tiledMode);
u32 NextPacket(u8 ** pStrm);
u32 NextPacketFromFile(u8 ** pStrm);
u32 CropPicture(u8 * pOutImage, u8 * pInImage,
                u32 frameWidth, u32 frameHeight, H264CropParams * pCropParams,
                u32 monoChrome);
static void printDecodeReturn(i32 retval);

#ifdef PP_PIPELINE_ENABLED
static void HandlePpOutput(u32 picDisplayNumber, u32 viewId);
#endif

u32 fillBuffer(u8 *stream);
/* Global variables for stream handling */
u8 *streamStop = NULL;
u32 packetize = 0;
u32 nalUnitStream = 0;
FILE *foutput = NULL, *foutput2 = NULL;
FILE *fTiledOutput = NULL;
FILE *fchroma2 = NULL;

FILE *findex = NULL;

/* stream start address */
u8 *byteStrmStart;
u32 traceUsedStream = 0;

/* SW/SW testing, read stream trace file */
FILE *fStreamTrace = NULL;

/* output file writing disable */
u32 disableOutputWriting = 0;
u32 retry = 0;

u32 clock_gating = DEC_X170_INTERNAL_CLOCK_GATING;
u32 data_discard = DEC_X170_DATA_DISCARD_ENABLE;
u32 latency_comp = DEC_X170_LATENCY_COMPENSATION;
u32 output_picture_endian = DEC_X170_OUTPUT_PICTURE_ENDIAN;
u32 bus_burst_length = DEC_X170_BUS_BURST_LENGTH;
u32 asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
u32 output_format = DEC_X170_OUTPUT_FORMAT;

u32 streamTruncate = 0;
u32 streamPacketLoss = 0;
u32 streamHeaderCorrupt = 0;
u32 hdrsRdy = 0;
u32 picRdy = 0;
TBCfg tbCfg;

u32 longStream = 0;
FILE *finput;
u32 planarOutput = 0;
/*u32 tiledOutput = 0;*/

u32 tiledOutput = DEC_REF_FRM_RASTER_SCAN;
u32 dpbMode = DEC_DPB_DEFAULT;
u32 convertTiledOutput = 0;

u32 usePeekOutput = 0;
u32 enableMvc = 0;
u32 mvcSeparateViews = 0;
u32 skipNonReference = 0;
u32 convertToFrameDpb = 0;

/* variables for indexing */
u32 saveIndex = 0;
u32 useIndex = 0;
off64_t curIndex = 0;
off64_t nextIndex = 0;
/* indicate when we save index */
u8 saveFlag = 0;

u32 bFrames;

char *grey_chroma = NULL;
size_t grey_chroma_size = 0;

char *pic_big_endian = NULL;
size_t pic_big_endian_size = 0;

#ifdef ASIC_TRACE_SUPPORT
extern u32 hwDecPicCount;
extern u32 gHwVer;
extern u32 h264HighSupport;
#endif

#ifdef H264_EVALUATION
extern u32 gHwVer;
extern u32 h264HighSupport;
#endif

#ifdef ADS_PERFORMANCE_SIMULATION

volatile u32 tttttt = 0;

void trace_perf()
{
    tttttt++;
}

#undef START_SW_PERFORMANCE
#undef END_SW_PERFORMANCE

#define START_SW_PERFORMANCE trace_perf();
#define END_SW_PERFORMANCE trace_perf();

#endif

H264DecInst decInst;

/*------------------------------------------------------------------------------

    Function name:  main

    Purpose:
        main function of decoder testbench. Provides command line interface
        with file I/O for H.264 decoder. Prints out the usage information
        when executed without arguments.

------------------------------------------------------------------------------*/

int main(int argc, char **argv)
{

    u32 i, tmp;
    u32 maxNumPics = 0;
    u8 *imageData;
    u8 *tmpImage = NULL;
    long int strmLen;
    u32 picSize = 0;
    H264DecRet ret;
    H264DecInput decInput;
    H264DecOutput decOutput;
    H264DecPicture decPicture;
    H264DecInfo decInfo;
    DWLLinearMem_t streamMem;
    i32 dwlret;

    u32 picDecodeNumber = 0;
    u32 picDisplayNumber = 0;
    u32 numErrors = 0;
    u32 cropDisplay = 0;
    u32 disableOutputReordering = 0;
    u32 useDisplaySmoothing = 0;
    u32 rlcMode = 0;
    u32 mbErrorConcealment = 0;
    u32 forceWholeStream = 0;
    const u8 *ptmpstream = NULL;
    u32 streamWillEnd = 0;

#ifdef PP_PIPELINE_ENABLED
    PPApiVersion ppVer;
    PPBuild ppBuild;
#endif

    FILE *fTBCfg;
    u32 seedRnd = 0;
    u32 streamBitSwap = 0;
    i32 corruptedBytes = 0;  /*  */

    char outFileName[256] = "";
    char outFileNameTiled[256] = "out_tiled.yuv";

#ifdef ASIC_TRACE_SUPPORT
    gHwVer = 8190; /* default to 8190 mode */
    h264HighSupport = 1;
#endif 

#ifdef H264_EVALUATION_8170
    gHwVer = 8170;
    h264HighSupport = 0;    
#elif H264_EVALUATION_8190
    gHwVer = 8190;
    h264HighSupport = 1;
#elif H264_EVALUATION_9170
    gHwVer = 9170;
    h264HighSupport = 1;    
#elif H264_EVALUATION_9190
    gHwVer = 9190;
    h264HighSupport = 1;    
#elif H264_EVALUATION_G1
    gHwVer = 10000;
    h264HighSupport = 1;    
#endif
    

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

    streamMem.virtualAddress = NULL;
    streamMem.busAddress = 0;    
    
    
    INIT_SW_PERFORMANCE;
    
    {
        H264DecApiVersion decApi;
        H264DecBuild decBuild;

        /* Print API version number */
        decApi = H264DecGetAPIVersion();
        decBuild = H264DecGetBuild();
        DEBUG_PRINT(("\nX170 H.264 Decoder API v%d.%d - SW build: %d - HW build: %x\n\n", decApi.major, decApi.minor, decBuild.swBuild, decBuild.hwBuild));
    }
        
#ifndef PP_PIPELINE_ENABLED
    /* Check that enough command line arguments given, if not -> print usage
     * information out */
    if(argc < 2)
    {
        DEBUG_PRINT(("Usage: %s [options] file.h264\n", argv[0]));
        DEBUG_PRINT(("\t-Nn forces decoding to stop after n pictures\n"));
        DEBUG_PRINT(("\t-Ooutfile write output to \"outfile\" (default out_wxxxhyyy.yuv)\n"));
        DEBUG_PRINT(("\t-X Disable output file writing\n"));
        DEBUG_PRINT(("\t-C display cropped image (default decoded image)\n"));
        DEBUG_PRINT(("\t-R disable DPB output reordering\n"));
        DEBUG_PRINT(("\t-J enable double DPB for smooth display\n"));
        DEBUG_PRINT(("\t-Sfile.hex stream control trace file\n"));
        DEBUG_PRINT(("\t-W disable packetizing even if stream does not fit to buffer.\n"
                     "\t   Only the first 0x1FFFFF bytes of the stream are decoded.\n"));
        DEBUG_PRINT(("\t-E use tiled reference frame format.\n"));
        DEBUG_PRINT(("\t-G convert tiled output pictures to raster scan\n"));
        DEBUG_PRINT(("\t-L enable support for long streams\n"));
        DEBUG_PRINT(("\t-P write planar output\n"));
        DEBUG_PRINT(("\t-I save index file\n"));
/*        DEBUG_PRINT(("\t-T write tiled output (out_tiled.yuv) by converting raster scan output\n"));*/
        DEBUG_PRINT(("\t-Z output pictures using H264DecPeek() function\n"));
        DEBUG_PRINT(("\t--separate-fields-in-dpb DPB stores interlaced content"\
                    " as fields (default: frames)\n"));        
        DEBUG_PRINT(("\t--output-frame-dpb Convert output to frame mode even if"\
            " field DPB mode used\n"));
#ifdef ASIC_TRACE_SUPPORT
        DEBUG_PRINT(("\t-F force 8170 mode in 8190 HW model (baseline configuration forced)\n"));
        DEBUG_PRINT(("\t-B force Baseline configuration to 8190 HW model\n"));        
        DEBUG_PRINT(("\t-Q Skip decoding non-reference pictures.\n")); 
#endif	
        return 0;
    }

    /* read command line arguments */
    for(i = 1; i < (u32) (argc - 1); i++)
    {
        if(strncmp(argv[i], "-N", 2) == 0)
        {
            maxNumPics = (u32) atoi(argv[i] + 2);
        }
        else if(strncmp(argv[i], "-O", 2) == 0)
        {
            strcpy(outFileName, argv[i] + 2);
        }
        else if(strcmp(argv[i], "-X") == 0)
        {
            disableOutputWriting = 1;
        }
        else if(strcmp(argv[i], "-C") == 0)
        {
            cropDisplay = 1;
        }
        else if (strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if (strncmp(argv[i], "-G", 2) == 0)
            convertTiledOutput = 1;        
        else if(strcmp(argv[i], "-R") == 0)
        {
            disableOutputReordering = 1;
        }
        else if(strcmp(argv[i], "-J") == 0)
        {
            useDisplaySmoothing = 1;
        }
        else if(strncmp(argv[i], "-S", 2) == 0)
        {
            fStreamTrace = fopen((argv[i] + 2), "r");
        }
        else if(strcmp(argv[i], "-W") == 0)
        {
            forceWholeStream = 1;
        }
        else if(strcmp(argv[i], "-L") == 0)
        {
            longStream = 1;
        }
        else if(strcmp(argv[i], "-P") == 0)
        {
            planarOutput = 1;
        }
        else if(strcmp(argv[i], "-I") == 0)
        {
            saveIndex = 1;
        }
        else if(strcmp(argv[i], "-M") == 0)
        {
            enableMvc = 1;
        }
        else if(strcmp(argv[i], "-V") == 0)
        {
            mvcSeparateViews = 1;
        }
        else if(strcmp(argv[i], "-Q") == 0)
        {
            skipNonReference = 1;
        }
        else if(strcmp(argv[i], "--separate-fields-in-dpb") == 0)
        {
            dpbMode = DEC_DPB_INTERLACED_FIELD;
        }       
        else if(strcmp(argv[i], "--output-frame-dpb") == 0)
        {
            convertToFrameDpb = 1;
        }          
#ifdef ASIC_TRACE_SUPPORT
        else if(strcmp(argv[i], "-F") == 0)
        {
            gHwVer = 8170;
            h264HighSupport = 0;
            printf("\n\nForce 8170 mode to HW model!!!\n\n");
        }
        else if(strcmp(argv[i], "-B") == 0)
        {
            h264HighSupport = 0;
            printf("\n\nForce Baseline configuration to 8190 HW model!!!\n\n");
        }
#endif
        else if(strcmp(argv[i], "-Z") == 0)
        {
            usePeekOutput = 1;
        }
        else
        {
            DEBUG_PRINT(("UNKNOWN PARAMETER: %s\n", argv[i]));
            return 1;
        }
    }

    /* open input file for reading, file name given by user. If file open
     * fails -> exit */
    finput = fopen(argv[argc - 1], "rb");
    if(finput == NULL)
    {
        DEBUG_PRINT(("UNABLE TO OPEN INPUT FILE\n"));
        return -1;
    }

#else
    /* Print API and build version numbers */
    ppVer = PPGetAPIVersion();
    ppBuild = PPGetBuild();

    /* Version */
    fprintf(stdout,
            "\nX170 PP API v%d.%d - SW build: %d - HW build: %x\n\n",
            ppVer.major, ppVer.minor, ppBuild.swBuild, ppBuild.hwBuild);

    /* Check that enough command line arguments given, if not -> print usage
     * information out */
    if(argc < 3)
    {
        DEBUG_PRINT(("Usage: %s [options] file.h264  pp.cfg\n",
                     argv[0]));
        DEBUG_PRINT(("\t-Nn forces decoding to stop after n pictures\n"));
        DEBUG_PRINT(("\t-X Disable output file writing\n"));
        DEBUG_PRINT(("\t-R disable DPB output reordering\n"));
        DEBUG_PRINT(("\t-J enable double DPB for smooth display\n"));
        DEBUG_PRINT(("\t-W disable packetizing even if stream does not fit to buffer.\n"
                     "\t   NOTE: Useful only for debug purposes.\n"));
        DEBUG_PRINT(("\t-L enable support for long streams.\n"));
        DEBUG_PRINT(("\t-I save index file\n"));
        DEBUG_PRINT(("\t-E use tiled reference frame format.\n"));
        DEBUG_PRINT(("\t--separate-fields-in-dpb DPB stores interlaced content"\
                    " as fields (default: frames)\n"));        
#ifdef ASIC_TRACE_SUPPORT
        DEBUG_PRINT(("\t-F force 8170 mode in 8190 HW model (baseline configuration forced)\n"));
        DEBUG_PRINT(("\t-B force Baseline configuration to 8190 HW model\n"));                
        DEBUG_PRINT(("\t-Q Skip decoding non-reference pictures.\n"));         
#endif
        return 0;
    }

    /* read command line arguments */
    for(i = 1; i < (u32) (argc - 2); i++)
    {
        if(strncmp(argv[i], "-N", 2) == 0)
        {
            maxNumPics = (u32) atoi(argv[i] + 2);
        }
        else if(strcmp(argv[i], "-X") == 0)
        {
            disableOutputWriting = 1;
        }
        else if(strcmp(argv[i], "-R") == 0)
        {
            disableOutputReordering = 1;
        }
        else if(strcmp(argv[i], "-J") == 0)
        {
            useDisplaySmoothing = 1;
        }
        else if (strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if(strcmp(argv[i], "-W") == 0)
        {
            forceWholeStream = 1;
        }
        else if(strcmp(argv[i], "-L") == 0)
        {
            longStream = 1;
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
#ifdef ASIC_TRACE_SUPPORT
        else if(strcmp(argv[i], "-F") == 0)
        {
            gHwVer = 8170;
            h264HighSupport = 0;
            printf("\n\nForce 8170 mode to HW model!!!\n\n");
        }
        else if(strcmp(argv[i], "-B") == 0)
        {
            h264HighSupport = 0;
            printf("\n\nForce Baseline configuration to 8190 HW model!!!\n\n");
        }        
#endif
        else if(strcmp(argv[i], "-M") == 0)
        {
            enableMvc = 1;
        }
        else if(strcmp(argv[i], "-V") == 0)
        {
            mvcSeparateViews = 1;
        }
        else
        {
            DEBUG_PRINT(("UNKNOWN PARAMETER: %s\n", argv[i]));
            return 1;
        }
    }
    
    /* open data file */
    finput = fopen(argv[argc - 2], "rb");
    if(finput == NULL)
    {
        DEBUG_PRINT(("Unable to open input file %s\n", argv[argc - 2]));
        exit(100);
    }
#endif 
    /* open index file for saving */
    if(saveIndex)
    {
        findex = fopen("stream.cfg", "w");
        if(findex == NULL)
        {
            DEBUG_PRINT(("UNABLE TO OPEN INDEX FILE\n"));
            return -1;
        }
    } 
    /* try open index file */
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
        DEBUG_PRINT(("UNABLE TO OPEN INPUT FILE: \"tb.cfg\"\n"));
        DEBUG_PRINT(("USING DEFAULT CONFIGURATION\n"));
    }
    else
    {
        fclose(fTBCfg);
        if(TBParseConfig("tb.cfg", TBReadParam, &tbCfg) == TB_FALSE)
            return -1;
        if(TBCheckCfg(&tbCfg) != 0)
            return -1;

#ifdef ASIC_TRACE_SUPPORT
        if (gHwVer != 8170)
            gHwVer = tbCfg.decParams.hwVersion;
#endif
    }
    /*TBPrintCfg(&tbCfg); */
    mbErrorConcealment = 0; /* TBGetDecErrorConcealment(&tbCfg); */
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
    DEBUG_PRINT(("Decoder Macro Block Error Concealment %d\n",
                 mbErrorConcealment));
    DEBUG_PRINT(("Decoder RLC %d\n", rlcMode));
    DEBUG_PRINT(("Decoder Clock Gating %d\n", clock_gating));
    DEBUG_PRINT(("Decoder Data Discard %d\n", data_discard));
    DEBUG_PRINT(("Decoder Latency Compensation %d\n", latency_comp));
    DEBUG_PRINT(("Decoder Output Picture Endian %d\n", output_picture_endian));
    DEBUG_PRINT(("Decoder Bus Burst Length %d\n", bus_burst_length));
    DEBUG_PRINT(("Decoder Asic Service Priority %d\n", asic_service_priority));
    DEBUG_PRINT(("Decoder Output Format %d\n", output_format));

    seedRnd = tbCfg.tbParams.seedRnd;
    streamHeaderCorrupt = TBGetTBStreamHeaderCorrupt(&tbCfg);
    /* if headers are to be corrupted 
     * -> do not wait the picture to finalize before starting stream corruption */
    if(streamHeaderCorrupt)
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

    packetize = TBGetTBPacketByPacket(&tbCfg);
    nalUnitStream = TBGetTBNalUnitStream(&tbCfg);
    DEBUG_PRINT(("TB Packet by Packet  %d\n", packetize));
    DEBUG_PRINT(("TB Nal Unit Stream %d\n", nalUnitStream));
    DEBUG_PRINT(("TB Seed Rnd %d\n", seedRnd));
    DEBUG_PRINT(("TB Stream Truncate %d\n", streamTruncate));
    DEBUG_PRINT(("TB Stream Header Corrupt %d\n", streamHeaderCorrupt));
    DEBUG_PRINT(("TB Stream Bit Swap %d; odds %s\n", streamBitSwap,
                 tbCfg.tbParams.streamBitSwap));
    DEBUG_PRINT(("TB Stream Packet Loss %d; odds %s\n", streamPacketLoss,
                 tbCfg.tbParams.streamPacketLoss));

    {
        remove("regdump.txt");
        remove("mbcontrol.hex");
        remove("intra4x4_modes.hex");
        remove("motion_vectors.hex");
        remove("rlc.hex");
        remove("picture_ctrl_dec.trc");
    }

#ifdef ASIC_TRACE_SUPPORT
    /* open tracefiles */
    tmp = openTraceFiles();
    if(!tmp)
    {
        DEBUG_PRINT(("UNABLE TO OPEN TRACE FILE(S)\n"));
    }
    if(nalUnitStream)
        trace_h264DecodingTools.streamMode.nalUnitStrm = 1;
    else
        trace_h264DecodingTools.streamMode.byteStrm = 1;
#endif

    /* initialize decoder. If unsuccessful -> exit */
    START_SW_PERFORMANCE;
    {
        DecDpbFlags flags = 0;
        if( tiledOutput )   flags |= DEC_REF_FRM_TILED_DEFAULT;
        if( dpbMode == DEC_DPB_INTERLACED_FIELD )
            flags |= DEC_DPB_ALLOW_FIELD_ORDERING;
        ret =
            H264DecInit(&decInst,
                        disableOutputReordering, 
                        TBGetDecIntraFreezeEnable( &tbCfg ),
                        useDisplaySmoothing, flags );
    }
    END_SW_PERFORMANCE;
    if(ret != H264DEC_OK)
    {
        DEBUG_PRINT(("DECODER INITIALIZATION FAILED\n"));
        goto end;
    }    

    /* configure decoder to decode both views of MVC stereo high streams */
    if (enableMvc)
        H264DecSetMvc(decInst);

    /* Set ref buffer test mode */
    ((decContainer_t *) decInst)->refBufferCtrl.testFunction = TBRefbuTestMode;

#ifdef PP_PIPELINE_ENABLED
    /* Initialize the post processer. If unsuccessful -> exit */
    if(pp_startup
       (argv[argc - 1], decInst, PP_PIPELINED_DEC_TYPE_H264, &tbCfg) != 0)
    {
        DEBUG_PRINT(("PP INITIALIZATION FAILED\n"));
        goto end;
    }

    if(pp_update_config
       (decInst, PP_PIPELINED_DEC_TYPE_H264, &tbCfg) == CFG_UPDATE_FAIL)

    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
        goto end;
    }
#endif

    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_LATENCY,
                   latency_comp);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_CLK_GATE_E,
                   clock_gating);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_OUT_TILED_E,
                   output_format);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_OUT_ENDIAN,
                   output_picture_endian);
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_MAX_BURST,
                   bus_burst_length);
    if ((DWLReadAsicID() >> 16) == 0x8170U)
    {
        SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_PRIORITY_MODE,
                       asic_service_priority);
    }
    SetDecRegister(((decContainer_t *) decInst)->h264Regs, HWIF_DEC_DATA_DISC_E,
                   data_discard);

    if(rlcMode)
    {
        /*Force the decoder into RLC mode */
        ((decContainer_t *) decInst)->forceRlcMode = 1;
        ((decContainer_t *) decInst)->rlcMode = 1;
        ((decContainer_t *) decInst)->tryVlc = 0;
    }

#ifdef _ENABLE_2ND_CHROMA
    if (!TBGetDecCh8PixIleavOutput(&tbCfg))
    {
        ((decContainer_t *) decInst)->storage.enable2ndChroma = 0;
    }
    else
    {
        /* cropping not supported when additional chroma output format used */
        cropDisplay = 0;
    }
#endif

    TBInitializeRandom(seedRnd);

    /* check size of the input file -> length of the stream in bytes */
    fseek(finput, 0L, SEEK_END);
    strmLen = ftell(finput);
    rewind(finput);

    decInput.skipNonReference = skipNonReference;

    if(!longStream)
    {
        /* If the decoder can not handle the whole stream at once, force packet-by-packet mode */
        if(!forceWholeStream)
        {
            if(strmLen > DEC_X170_MAX_STREAM)
            {
                packetize = 1;
            }
        }
        else
        {
            if(strmLen > DEC_X170_MAX_STREAM)
            {
                packetize = 0;
                strmLen = DEC_X170_MAX_STREAM;
            }
        }

        /* sets the stream length to random value */
        if(streamTruncate && !packetize && !nalUnitStream)
        {
            DEBUG_PRINT(("strmLen %d\n", strmLen));
            ret = TBRandomizeU32(&strmLen);
            if(ret != 0)
            {
                DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
                goto end;
            }
            DEBUG_PRINT(("Randomized strmLen %d\n", strmLen));
        }

        /* NOTE: The DWL should not be used outside decoder SW.
         * here we call it because it is the easiest way to get
         * dynamically allocated linear memory
         * */
        
        /* allocate memory for stream buffer. if unsuccessful -> exit */        
#ifndef ADS_PERFORMANCE_SIMULATION
        if(DWLMallocLinear(((decContainer_t *) decInst)->dwl,
                           strmLen, &streamMem) != DWL_OK)
        {
            DEBUG_PRINT(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
            goto end;
        }
#else
        streamMem.virtualAddress = malloc(strmLen);
        streamMem.busAddress = (size_t) streamMem.virtualAddress;
#endif

        byteStrmStart = (u8 *) streamMem.virtualAddress;

        if(byteStrmStart == NULL)
        {
            DEBUG_PRINT(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
            goto end;
        }

        /* read input stream from file to buffer and close input file */
        fread(byteStrmStart, 1, strmLen, finput);

        /* initialize H264DecDecode() input structure */
        streamStop = byteStrmStart + strmLen;
        decInput.pStream = byteStrmStart;
        decInput.streamBusAddress = (u32) streamMem.busAddress;

        decInput.dataLen = strmLen;
    }
    else
    {
#ifndef ADS_PERFORMANCE_SIMULATION
        if(DWLMallocLinear(((decContainer_t *) decInst)->dwl,
                           DEC_X170_MAX_STREAM, &streamMem) != DWL_OK)
        {
            DEBUG_PRINT(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
            goto end;
        }
#else
        streamMem.virtualAddress = malloc(DEC_X170_MAX_STREAM);
        streamMem.busAddress = (size_t) streamMem.virtualAddress;

        if(streamMem.virtualAddress == NULL)
        {
            DEBUG_PRINT(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
            goto end;
        }
#endif

        /* initialize H264DecDecode() input structure */
        decInput.pStream = (u8 *) streamMem.virtualAddress;
        decInput.streamBusAddress = (u32) streamMem.busAddress;
    }

    if(longStream && !packetize && !nalUnitStream)
    {
        if(useIndex == 1)
        {
            u32 amount = 0;
            curIndex = 0;

            /* read index */
            fscanf(findex, "%llu", &nextIndex);

            {
                /* check if last index -> stream end */
                u8 byte = 0;
                fread(&byte, 2, 1, findex);
                if(feof(findex))
                {
                    DEBUG_PRINT(("STREAM WILL END\n"));
                    streamWillEnd = 1;
                }
                else
                {
                    fseek(findex, -2, SEEK_CUR);
                }                
            }

            amount = nextIndex - curIndex;
            curIndex = nextIndex;

            /* read first block */
            decInput.dataLen = fread(decInput.pStream, 1, amount, finput);
        }
        else
        {
            decInput.dataLen =
                fread(decInput.pStream, 1, DEC_X170_MAX_STREAM, finput);
        }
        /*DEBUG_PRINT(("BUFFER READ\n")); */
        if(feof(finput))
        {
            DEBUG_PRINT(("STREAM WILL END\n"));
            streamWillEnd = 1;
        }
    }

    else
    {
        if(useIndex)
        {
            if(!nalUnitStream)
                fscanf(findex, "%llu", &curIndex);
        }

        /* get pointer to next packet and the size of packet
         * (for packetize or nalUnitStream modes) */
        ptmpstream = decInput.pStream;
        if((tmp = NextPacket((u8 **) (&decInput.pStream))) != 0)
        {
            decInput.dataLen = tmp;
            decInput.streamBusAddress += (u32) (decInput.pStream - ptmpstream);
        }
    }

    picDecodeNumber = picDisplayNumber = 1;

    /* main decoding loop */
    do
    {
        saveFlag = 1;
        /*DEBUG_PRINT(("decInput.dataLen %d\n", decInput.dataLen));
         * DEBUG_PRINT(("decInput.pStream %d\n", decInput.pStream)); */

        if(streamTruncate && picRdy && (hdrsRdy || streamHeaderCorrupt) &&
           (longStream || (!longStream && (packetize || nalUnitStream))))
        {
            i32 ret;

            ret = TBRandomizeU32(&decInput.dataLen);
            if(ret != 0)
            {
                DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
                return 0;
            }
            DEBUG_PRINT(("Randomized stream size %d\n", decInput.dataLen));
        }

        /* If enabled, break the stream */
        if(streamBitSwap)
        {
            if((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt)
            {
                /* Picture needs to be ready before corrupting next picture */
                if(picRdy && corruptedBytes <= 0)
                {
                    ret =
                        TBRandomizeBitSwapInStream(decInput.pStream,
                                                   decInput.dataLen,
                                                   tbCfg.tbParams.
                                                   streamBitSwap);
                    if(ret != 0)
                    {
                        DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
                        goto end;
                    }

                    corruptedBytes = decInput.dataLen;
                    DEBUG_PRINT(("corruptedBytes %d\n", corruptedBytes));
                }
            }
        }

        /* Picture ID is the picture number in decoding order */
        decInput.picId = picDecodeNumber;

        /* call API function to perform decoding */
        START_SW_PERFORMANCE;
        ret = H264DecDecode(decInst, &decInput, &decOutput);
        END_SW_PERFORMANCE;
        printDecodeReturn(ret);
        switch (ret)
        {
        case H264DEC_STREAM_NOT_SUPPORTED:
            {
                DEBUG_PRINT(("ERROR: UNSUPPORTED STREAM!\n"));
                goto end;
            }
        case H264DEC_HDRS_RDY:
            {
                saveFlag = 0;
                /* Set a flag to indicate that headers are ready */
                hdrsRdy = 1;
                TBSetRefbuMemModel( &tbCfg, 
                    ((decContainer_t *) decInst)->h264Regs,
                    &((decContainer_t *) decInst)->refBufferCtrl );

                /* Stream headers were successfully decoded
                 * -> stream information is available for query now */

                START_SW_PERFORMANCE;
                tmp = H264DecGetInfo(decInst, &decInfo);
                END_SW_PERFORMANCE;
                if(tmp != H264DEC_OK)
                {
                    DEBUG_PRINT(("ERROR in getting stream info!\n"));
                    goto end;
                }

                DEBUG_PRINT(("Width %d Height %d\n",
                             decInfo.picWidth, decInfo.picHeight));

                DEBUG_PRINT(("Cropping params: (%d, %d) %dx%d\n",
                             decInfo.cropParams.cropLeftOffset,
                             decInfo.cropParams.cropTopOffset,
                             decInfo.cropParams.cropOutWidth,
                             decInfo.cropParams.cropOutHeight));

                DEBUG_PRINT(("MonoChrome = %d\n", decInfo.monoChrome));
                DEBUG_PRINT(("Interlaced = %d\n", decInfo.interlacedSequence));
                DEBUG_PRINT(("DPB mode   = %d\n", decInfo.dpbMode));
                DEBUG_PRINT(("Pictures in DPB = %d\n", decInfo.picBuffSize));
                DEBUG_PRINT(("Pictures in Multibuffer PP = %d\n", decInfo.multiBuffPpSize));
                
                dpbMode = decInfo.dpbMode;
#ifdef PP_PIPELINE_ENABLED
                pp_number_of_buffers(decInfo.multiBuffPpSize);
#endif
                if(cropDisplay)
                {
                    /* Cropped frame size in planar YUV 4:2:0 */
                    picSize = decInfo.cropParams.cropOutWidth *
                        decInfo.cropParams.cropOutHeight;
                    if(!decInfo.monoChrome)
                        picSize = (3 * picSize) / 2;
                    tmpImage = malloc(picSize);
                    if(tmpImage == NULL)
                    {
                        DEBUG_PRINT(("ERROR in allocating cropped image!\n"));
                        goto end;
                    }
                }
                else
                {
                    /* Decoder output frame size in planar YUV 4:2:0 */
                    picSize = decInfo.picWidth * decInfo.picHeight;
                    if(!decInfo.monoChrome)
                        picSize = (3 * picSize) / 2;
                    if (usePeekOutput)
                        tmpImage = malloc(picSize);
                }

                DEBUG_PRINT(("videoRange %d, matrixCoefficients %d\n",
                             decInfo.videoRange, decInfo.matrixCoefficients));

                /* If -O option not used, generate default file name */
                if(outFileName[0] == 0)
                    sprintf(outFileName, "out.yuv");

                break;
            }
        case H264DEC_ADVANCED_TOOLS:
            {
                /* ASO/STREAM ERROR was noticed in the stream. The decoder has to
                 * reallocate resources */
                assert(decOutput.dataLeft); /* we should have some data left *//* Used to indicate that picture decoding needs to finalized prior to corrupting next picture */

                /* Used to indicate that picture decoding needs to finalized prior to corrupting next picture
                 * picRdy = 0; */
                break;
            }
        case H264DEC_PIC_DECODED:
        case H264DEC_PENDING_FLUSH:
            /* case H264DEC_FREEZED_PIC_RDY: */
            /* Picture is now ready */
            picRdy = 1;

            /*lint -esym(644,tmpImage,picSize) variable initialized at
             * H264DEC_HDRS_RDY_BUFF_NOT_EMPTY case */

            /* If enough pictures decoded -> force decoding to end
             * by setting that no more stream is available */
            if(picDecodeNumber == maxNumPics)
                decInput.dataLen = 0;

            printf("DECODED PICTURE %d\n", picDecodeNumber);
            /* Increment decoding number for every decoded picture */
            picDecodeNumber++;
            
            if (usePeekOutput && ret == H264DEC_PIC_DECODED &&
                H264DecPeek(decInst, &decPicture) == H264DEC_PIC_RDY)
            {
                static u32 lastField = 0;
                DEBUG_PRINT(("DECPIC %d\n", picDecodeNumber));

                /* Write output picture to file. If output consists of separate fields -> store
                 * whole frame before writing */
                imageData = (u8 *) decPicture.pOutputPicture;
                if (decPicture.fieldPicture)
                {
                    if (lastField == 0)
                        memcpy(tmpImage, imageData, picSize);
                    else
                    {
                        u32 i;
                        u8 *pIn = imageData, *pOut = tmpImage;
                        if (decPicture.topField == 0)
                        {
                            pOut += decPicture.picWidth;
                            pIn += decPicture.picWidth;
                        }
                        tmp = decInfo.monoChrome ?
                            decPicture.picHeight / 2 :
                            3 * decPicture.picHeight / 4;
                        for (i = 0; i < tmp; i++)
                        {
                            memcpy(pOut, pIn, decPicture.picWidth);
                            pIn += 2*decPicture.picWidth;
                            pOut += 2*decPicture.picWidth;
                        }
                    }
                    lastField ^= 1;
                }
                else
                {
                    if (lastField)
                        WriteOutput(outFileName, outFileNameTiled,
                                    tmpImage, picSize,
                                    decInfo.picWidth,
                                    decInfo.picHeight,
                                    picDecodeNumber - 2, decInfo.monoChrome, 0,
                                    decPicture.outputFormat);
                    lastField = 0;
                    memcpy(tmpImage, imageData, picSize);
                }

                if (!lastField)
                    WriteOutput(outFileName, outFileNameTiled,
                                tmpImage, picSize,
                                decInfo.picWidth,
                                decInfo.picHeight,
                                picDecodeNumber - 1, decInfo.monoChrome, 0,
                                decPicture.outputFormat);

            }

            /* use function H264DecNextPicture() to obtain next picture
             * in display order. Function is called until no more images
             * are ready for display */
            START_SW_PERFORMANCE;
            while (H264DecNextPicture(decInst, &decPicture, 0) ==
                  H264DEC_PIC_RDY)
            {
                END_SW_PERFORMANCE;
                if (!usePeekOutput)
                {
                    DEBUG_PRINT(("PIC %d, view %d, type %s", picDisplayNumber,
                                 decPicture.viewId, decPicture.isIdrPicture ? "IDR" : "NON-IDR"));

                    if(picDisplayNumber != decPicture.picId)
                        DEBUG_PRINT((", decoded pic %d", decPicture.picId));

                    if(decPicture.nbrOfErrMBs)
                    {
                        DEBUG_PRINT((", concealed %d", decPicture.nbrOfErrMBs));
                    }

                    if(decPicture.interlaced)
                    {
                        DEBUG_PRINT((", INTERLACED "));
                        if(decPicture.fieldPicture)
                        {
                            DEBUG_PRINT(("FIELD %s",
                                         decPicture.topField ? "TOP" : "BOTTOM"));
                        }
                        else
                        {
                            DEBUG_PRINT(("FRAME"));
                        }
                    }

                    DEBUG_PRINT(("\n"));
                    fflush(stdout);

                    numErrors += decPicture.nbrOfErrMBs;

                    /*lint -esym(644,decInfo) always initialized if pictures
                     * available for display */

                    /* Write output picture to file */
                    imageData = (u8 *) decPicture.pOutputPicture;
                    if(cropDisplay)
                    {
                        tmp = CropPicture(tmpImage, imageData,
                                          decInfo.picWidth, decInfo.picHeight,
                                          &decInfo.cropParams, decInfo.monoChrome);
                        if(tmp)
                        {
                            DEBUG_PRINT(("ERROR in cropping!\n"));
                            goto end;
                        }
                    }
#ifndef PP_PIPELINE_ENABLED
                    WriteOutput(outFileName, outFileNameTiled,
                                cropDisplay ? tmpImage : imageData, picSize,
                                cropDisplay ? decInfo.cropParams.
                                cropOutWidth : decInfo.picWidth,
                                cropDisplay ? decInfo.cropParams.
                                cropOutHeight : decInfo.picHeight,
                                picDisplayNumber - 1, decInfo.monoChrome,
                                decPicture.viewId,
                                decPicture.outputFormat);
#else
                    if(!disableOutputWriting)
                    {
                        HandlePpOutput(picDisplayNumber, decPicture.viewId);
                    }
                    pp_check_combined_status();
                    pp_read_blend_components(((decContainer_t *) decInst)->pp.ppInstance);
                    /* load new cfg if needed (handled in pp testbench) */
                    if(pp_update_config(decInst, PP_PIPELINED_DEC_TYPE_H264, &tbCfg) == CFG_UPDATE_FAIL)
                    {
                        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
                    }                
                    
#endif
                    /* Increment display number for every displayed picture */
                    picDisplayNumber++;
                }

                START_SW_PERFORMANCE;

                if (useDisplaySmoothing && ret != H264DEC_PENDING_FLUSH)
                    break;
            }
            END_SW_PERFORMANCE;
            retry = 0;
            break;

        case H264DEC_STRM_PROCESSED:
        case H264DEC_NONREF_PIC_SKIPPED:
        case H264DEC_STRM_ERROR:
            {
                /* Used to indicate that picture decoding needs to finalized prior to corrupting next picture
                 * picRdy = 0; */

                break;
            }

        case H264DEC_OK:
            /* nothing to do, just call again */
            break;
        case H264DEC_HW_TIMEOUT:
            DEBUG_PRINT(("Timeout\n"));
            goto end;
        default:
            DEBUG_PRINT(("FATAL ERROR: %d\n", ret));
            goto end;
        }

        /* break out of do-while if maxNumPics reached (dataLen set to 0) */
        if(decInput.dataLen == 0)
            break;

        if(longStream && !packetize && !nalUnitStream)
        {
            if(streamWillEnd)
            {
                corruptedBytes -= (decInput.dataLen - decOutput.dataLeft);
                decInput.dataLen = decOutput.dataLeft;
                decInput.pStream = decOutput.pStrmCurrPos;
                decInput.streamBusAddress = decOutput.strmCurrBusAddress;
            }
            else
            {
                if(useIndex == 1)
                {
                    if(decOutput.dataLeft)
                    {
                        decInput.streamBusAddress += (decOutput.pStrmCurrPos - decInput.pStream);
                        corruptedBytes -= (decInput.dataLen - decOutput.dataLeft);
                        decInput.dataLen = decOutput.dataLeft;
                        decInput.pStream = decOutput.pStrmCurrPos;
                    }
                    else
                    {
                        decInput.streamBusAddress = (u32) streamMem.busAddress;
                        decInput.pStream = (u8 *) streamMem.virtualAddress;
                        decInput.dataLen = fillBuffer(decInput.pStream);
                    }
                }
                else
                {
                    if(fseek(finput, -decOutput.dataLeft, SEEK_CUR) == -1)
                    {
                        DEBUG_PRINT(("SEEK FAILED\n"));
                        decInput.dataLen == 0;
                    }
                    else
                    {
                        /* store file index */
                        if(saveIndex && saveFlag)
                        {
                            fprintf(findex, "%llu\n", ftello64(finput));
                        }

                        decInput.dataLen =
                            fread(decInput.pStream, 1, DEC_X170_MAX_STREAM, finput);
                    }
                }
                
                if(feof(finput))
                {
                    DEBUG_PRINT(("STREAM WILL END\n"));
                    streamWillEnd = 1;
                }
                
                corruptedBytes = 0;
            }
        }

        else
        {
            if(decOutput.dataLeft)
            {
                decInput.streamBusAddress += (decOutput.pStrmCurrPos - decInput.pStream);
                corruptedBytes -= (decInput.dataLen - decOutput.dataLeft);
                decInput.dataLen = decOutput.dataLeft;
                decInput.pStream = decOutput.pStrmCurrPos;
            }
            else
            {
                decInput.streamBusAddress = (u32) streamMem.busAddress; 
                decInput.pStream = (u8 *) streamMem.virtualAddress;
                /*u32 streamPacketLossTmp = streamPacketLoss;
                 * 
                 * if(!picRdy)
                 * {
                 * streamPacketLoss = 0;
                 * } */
                ptmpstream = decInput.pStream;

                tmp = ftell(finput);
                decInput.dataLen = NextPacket((u8 **) (&decInput.pStream));
                printf("NextPacket = %d at %d\n", decInput.dataLen, tmp);

                decInput.streamBusAddress +=
                    (u32) (decInput.pStream - ptmpstream);

                /*streamPacketLoss = streamPacketLossTmp; */

                corruptedBytes = 0;
            }
        }

        /* keep decoding until all data from input stream buffer consumed
         * and all the decoded/queued frames are ready */
    }
    while(decInput.dataLen > 0);

    /* store last index */
    if(saveIndex)
    {
        off64_t pos = ftello64(finput) - decOutput.dataLeft;
        fprintf(findex, "%llu\n", pos);
    }

    if(useIndex || saveIndex)
    {
        fclose(findex);
    }

    DEBUG_PRINT(("Decoding ended, flush the DPB\n"));

    /* if output in display order is preferred, the decoder shall be forced
     * to output pictures remaining in decoded picture buffer. Use function
     * H264DecNextPicture() to obtain next picture in display order. Function
     * is called until no more images are ready for display. Second parameter
     * for the function is set to '1' to indicate that this is end of the
     * stream and all pictures shall be output */
    while(!usePeekOutput &&
          H264DecNextPicture(decInst, &decPicture, 1) == H264DEC_PIC_RDY)
    {
        DEBUG_PRINT(("PIC %d, type %s", picDisplayNumber,
                     decPicture.isIdrPicture ? "IDR" : "NON-IDR"));
        if(picDisplayNumber != decPicture.picId)
            DEBUG_PRINT((", decoded pic %d", decPicture.picId));
        if(decPicture.nbrOfErrMBs)
        {
            DEBUG_PRINT((", concealed %d\n", decPicture.nbrOfErrMBs));
        }
        if(decPicture.interlaced)
        {
            DEBUG_PRINT((", INTERLACED "));
            if(decPicture.fieldPicture)
            {
                DEBUG_PRINT(("FIELD %s",
                             decPicture.topField ? "TOP" : "BOTTOM"));
            }
            else
            {
                DEBUG_PRINT(("FRAME"));
            }
        }

        DEBUG_PRINT(("\n"));
        fflush(stdout);

        numErrors += decPicture.nbrOfErrMBs;

        /* Write output picture to file */
        imageData = (u8 *) decPicture.pOutputPicture;
        if(cropDisplay)
        {
            tmp = CropPicture(tmpImage, imageData,
                              decInfo.picWidth, decInfo.picHeight,
                              &decInfo.cropParams, decInfo.monoChrome);
            if(tmp)
            {
                DEBUG_PRINT(("ERROR in cropping!\n"));
                goto end;
            }
        }

#ifndef PP_PIPELINE_ENABLED
        WriteOutput(outFileName, outFileNameTiled,
                    cropDisplay ? tmpImage : imageData, picSize,
                    cropDisplay ? decInfo.cropParams.cropOutWidth : decInfo.
                    picWidth,
                    cropDisplay ? decInfo.cropParams.cropOutHeight : decInfo.
                    picHeight, picDisplayNumber - 1, decInfo.monoChrome,
                    decPicture.viewId,
                    decPicture.outputFormat); 
#else
        if(!disableOutputWriting)
        {
            HandlePpOutput(picDisplayNumber, decPicture.viewId);
        }

        pp_read_blend_components(((decContainer_t *) decInst)->pp.ppInstance);
        pp_check_combined_status();
        /* load new cfg if needed (handled in pp testbench) */
        if(pp_update_config(decInst, PP_PIPELINED_DEC_TYPE_H264, &tbCfg) == CFG_UPDATE_FAIL)
        {
            fprintf(stdout, "PP CONFIG LOAD FAILED\n");
        }        
        
#endif
        /* Increment display number for every displayed picture */
        picDisplayNumber++;
    }

  end:

    byteStrmStart = NULL;
    fflush(stdout);

    if(streamMem.virtualAddress != NULL)
    {
#ifndef ADS_PERFORMANCE_SIMULATION
        if(decInst)
            DWLFreeLinear(((decContainer_t *) decInst)->dwl, &streamMem);
#else
        free(streamMem.virtualAddress);
#endif
    }

#ifdef PP_PIPELINE_ENABLED
    pp_close();
#endif

    /* release decoder instance */
    START_SW_PERFORMANCE;
    H264DecRelease(decInst);
    END_SW_PERFORMANCE;

    if(foutput)
        fclose(foutput);
    if(foutput2)
        fclose(foutput2);
    if(fchroma2)
        fclose(fchroma2);
    if(fTiledOutput)
        fclose(fTiledOutput);
    if(fStreamTrace)
        fclose(fStreamTrace);
    if(finput)
        fclose(finput);

    /* free allocated buffers */
    if(tmpImage != NULL)
        free(tmpImage);
    if(grey_chroma != NULL)
        free(grey_chroma);
    if(pic_big_endian)
        free(pic_big_endian);

    foutput = fopen(outFileName, "rb");
    if(NULL == foutput)
    {
        strmLen = 0;
    }
    else
    {
        fseek(foutput, 0L, SEEK_END);
        strmLen = (u32) ftell(foutput);
        fclose(foutput);
    }

    DEBUG_PRINT(("Output file: %s\n", outFileName));

    DEBUG_PRINT(("OUTPUT_SIZE %d\n", strmLen));

    FINALIZE_SW_PERFORMANCE;

    DEBUG_PRINT(("DECODING DONE\n"));

#ifdef ASIC_TRACE_SUPPORT
    trace_SequenceCtrl(hwDecPicCount, 0);
    trace_H264DecodingTools();
    /* close trace files */
    closeTraceFiles();
#endif

    if(retry > NUM_RETRY)
    {
        return -1;
    }

    if(numErrors || picDecodeNumber == 1)
    {
        DEBUG_PRINT(("ERRORS FOUND %d %d\n", numErrors, picDecodeNumber));
        /*return 1;*/
        return 0;
    }

    return 0;
}

/*------------------------------------------------------------------------------

    Function name:  WriteOutput

    Purpose:
        Write picture pointed by data to file. Size of the
        picture in pixels is indicated by picSize.

------------------------------------------------------------------------------*/
void WriteOutput(char *filename, char *filenameTiled, u8 * data, u32 picSize,
                 u32 picWidth, u32 picHeight, u32 frameNumber, u32 monoChrome,
                 u32 view, u32 tiledMode)
{

    u32 i, tmp;
    FILE **fout;
    char altFileName[256];
    char *fn;
    u8 *rasterScan = NULL;

    if(disableOutputWriting != 0)
    {
        return;
    }

    fout = (view && mvcSeparateViews) ? &foutput2 : &foutput;
    /* foutput is global file pointer */
    if(*fout == NULL)
    {
        if (view && mvcSeparateViews)
        {
            strcpy(altFileName, filename);
            sprintf(altFileName+strlen(altFileName)-4, "_%d.yuv", view);
            fn = altFileName;
        }
        else
            fn = filename;

        /* open output file for writing, can be disabled with define.
         * If file open fails -> exit */
        if(strcmp(filename, "none") != 0)
        {
            *fout = fopen(fn, "wb");
            if(*fout == NULL)
            {
                DEBUG_PRINT(("UNABLE TO OPEN OUTPUT FILE\n"));
                return;
            }
        }
#ifdef _ENABLE_2ND_CHROMA
        if (TBGetDecCh8PixIleavOutput(&tbCfg) && !monoChrome)
        {
            fchroma2 = fopen("out_ch8pix.yuv", "wb");
            if(fchroma2 == NULL)
            {
                DEBUG_PRINT(("UNABLE TO OPEN OUTPUT FILE\n"));
                return;
            }
        }
#endif
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
                DEBUG_PRINT(("UNABLE TO OPEN TILED OUTPUT FILE\n"));
                return;
            }
        }
    }
#endif


    /* Convert back to raster scan format if decoder outputs 
     * tiled format */
    if(tiledMode && convertTiledOutput)
    {
        u32 effHeight = (picHeight + 15) & (~15);
        rasterScan = (u8*)malloc(picWidth*effHeight*3/2);
        if(!rasterScan)
        {
            fprintf(stderr, "error allocating memory for tiled"
                "-->raster conversion!\n");
            return;
        }
        
        TBTiledToRaster( tiledMode, convertToFrameDpb ? dpbMode : DEC_DPB_FRAME, 
            data, rasterScan, picWidth, 
            effHeight );
        if(!monoChrome)
            TBTiledToRaster( tiledMode, convertToFrameDpb ? dpbMode : DEC_DPB_FRAME, 
            data+picWidth*effHeight, 
            rasterScan+picWidth*effHeight, picWidth, effHeight/2 );
        data = rasterScan;
    }
    else if ( convertToFrameDpb && (dpbMode != DEC_DPB_DEFAULT))
    {
        u32 effHeight = (picHeight + 15) & (~15);
        rasterScan = (u8*)malloc(picWidth*effHeight*3/2);
        if(!rasterScan)
        {
            fprintf(stderr, "error allocating memory for tiled"
                "-->raster conversion!\n");
            return;
        }
        
        TBFieldDpbToFrameDpb( convertToFrameDpb ? dpbMode : DEC_DPB_FRAME, 
            data, rasterScan, picWidth, 
            effHeight );
        if(!monoChrome)
            TBFieldDpbToFrameDpb( convertToFrameDpb ? dpbMode : DEC_DPB_FRAME, 
            data+picWidth*effHeight, 
            rasterScan+picWidth*effHeight, picWidth, effHeight/2 );
        data = rasterScan;
    }
    
    if(monoChrome)
    {
        if(grey_chroma_size != (picSize / 2))
        {
            if(grey_chroma != NULL)
                free(grey_chroma);

            grey_chroma = (char *) malloc(picSize / 2);
            if(grey_chroma == NULL)
            {
                DEBUG_PRINT(("UNABLE TO ALLOCATE GREYSCALE CHROMA BUFFER\n"));
                if(rasterScan)
                    free(rasterScan);
                return;
            }
            grey_chroma_size = picSize / 2;
            memset(grey_chroma, 128, grey_chroma_size);
        }
    }

    if(*fout == NULL || data == NULL)
    {
        return;
    }

#ifndef ASIC_TRACE_SUPPORT
    if(output_picture_endian == DEC_X170_BIG_ENDIAN)
    {
        if(pic_big_endian_size != picSize)
        {
            if(pic_big_endian != NULL)
                free(pic_big_endian);

            pic_big_endian = (char *) malloc(picSize);
            if(pic_big_endian == NULL)
            {
                DEBUG_PRINT(("MALLOC FAILED @ %s %d", __FILE__, __LINE__));
                if(rasterScan)
                    free(rasterScan);
                return;
            }
            
            pic_big_endian_size = picSize;
        }

        memcpy(pic_big_endian, data, picSize);
        TbChangeEndianess(pic_big_endian, picSize);
        data = pic_big_endian;
    }
#endif

#ifdef MD5SUM
    TBWriteFrameMD5Sum(*fout, data, picSize, frameNumber);

    /* tiled */
#if 0
    if(tiledOutput && !monoChrome) /* TODO: does not work for monochrome output */
    {
        assert(picWidth % 16 == 0);
        assert(picHeight % 16 == 0);
        TbWriteTiledOutput(fTiledOutput, data, picWidth >> 4,
                           picHeight >> 4, frameNumber,
                           1 /* write md5sum */ ,
                           1 /* semi-planar data */ );
    }
#endif
#else
    /* this presumes output has system endianess */
    if(planarOutput && !monoChrome)
    {
        tmp = picSize * 2 / 3;
        fwrite(data, 1, tmp, *fout);
        for(i = 0; i < tmp / 4; i++)
            fwrite(data + tmp + i * 2, 1, 1, *fout);
        for(i = 0; i < tmp / 4; i++)
            fwrite(data + tmp + 1 + i * 2, 1, 1, *fout);
    }
    else    /* semi-planar */
    {
        fwrite(data, 1, picSize, *fout);
        if(monoChrome)
        {
            fwrite(grey_chroma, 1, grey_chroma_size, *fout);
        }
    }

    /* tiled */
#if 0
    if(tiledOutput && !monoChrome) /* TODO: does not work for monochrome output */
    {
        assert(picWidth % 16 == 0);
        assert(picHeight % 16 == 0);
        TbWriteTiledOutput(fTiledOutput, data, picWidth >> 4,
                           picHeight >> 4, frameNumber,
                           0 /* do not write md5sum */ ,
                           1 /* semi-planar data */ );
    }
#endif
#endif

#ifdef _ENABLE_2ND_CHROMA
    if (TBGetDecCh8PixIleavOutput(&tbCfg) && !monoChrome)
    {
        u8 *pCh;
        u32 tmp;
        H264DecRet ret;

        ret = H264DecNextChPicture(decInst, &pCh, &tmp);
        ASSERT(ret == H264DEC_PIC_RDY);
#ifndef ASIC_TRACE_SUPPORT
        if(output_picture_endian == DEC_X170_BIG_ENDIAN)
        {
            if(pic_big_endian_size != picSize/3)
            {
                if(pic_big_endian != NULL)
                    free(pic_big_endian);

                pic_big_endian = (char *) malloc(picSize/3);
                if(pic_big_endian == NULL)
                {
                    DEBUG_PRINT(("MALLOC FAILED @ %s %d", __FILE__, __LINE__));
                    if(rasterScan)
                        free(rasterScan);
                    return;
                }
            
                pic_big_endian_size = picSize/3;
            }

            memcpy(pic_big_endian, pCh, picSize/3);
            TbChangeEndianess(pic_big_endian, picSize/3);
            pCh = pic_big_endian;
        }
#endif

#ifdef MD5SUM
        TBWriteFrameMD5Sum(fchroma2, pCh, picSize/3, frameNumber);
#else
        fwrite(pCh, 1, picSize/3, fchroma2);
#endif
    }
#endif

    if(rasterScan)
        free(rasterScan);


}

/*------------------------------------------------------------------------------

    Function name: NextPacket

    Purpose:
        Get the pointer to start of next packet in input stream. Uses
        global variables 'packetize' and 'nalUnitStream' to determine the
        decoder input stream mode and 'streamStop' to determine the end
        of stream. There are three possible stream modes:
            default - the whole stream at once
            packetize - a single NAL-unit with start code prefix
            nalUnitStream - a single NAL-unit without start code prefix

        pStrm stores pointer to the start of previous decoder input and is
        replaced with pointer to the start of the next decoder input.

        Returns the packet size in bytes

------------------------------------------------------------------------------*/

u32 NextPacket(u8 ** pStrm)
{
    u32 index;
    u32 maxIndex;
    u32 zeroCount;
    u8 byte;
    static u32 prevIndex = 0;
    static u8 *stream = NULL;
    u8 nextPacket = 0;

    /* Next packet is read from file is long stream support is enabled */
    if(longStream)
    {
        return NextPacketFromFile(pStrm);
    }

    /* For default stream mode all the stream is in first packet */
    if(!packetize && !nalUnitStream)
        return 0;

    /* If enabled, loose the packets (skip this packet first though) */
    if(streamPacketLoss)
    {
        u32 ret = 0;

        ret =
            TBRandomizePacketLoss(tbCfg.tbParams.streamPacketLoss, &nextPacket);
        if(ret != 0)
        {
            DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
            return 0;
        }
    }

    index = 0;

    if(stream == NULL)
        stream = *pStrm;
    else
        stream += prevIndex;

    maxIndex = (u32) (streamStop - stream);

    if(stream > streamStop)
        return (0);

    if(maxIndex == 0)
        return (0);

    /* leading zeros of first NAL unit */
    do
    {
        byte = stream[index++];
    }
    while(byte != 1 && index < maxIndex);

    /* invalid start code prefix */
    if(index == maxIndex || index < 3)
    {
        DEBUG_PRINT(("INVALID BYTE STREAM\n"));
        return 0;
    }

    /* nalUnitStream is without start code prefix */
    if(nalUnitStream)
    {
        stream += index;
        maxIndex -= index;
        index = 0;
    }

    zeroCount = 0;

    /* Search stream for next start code prefix */
    /*lint -e(716) while(1) used consciously */
    while(1)
    {
        byte = stream[index++];
        if(!byte)
            zeroCount++;

        if((byte == 0x01) && (zeroCount >= 2))
        {
            /* Start code prefix has two zeros
             * Third zero is assumed to be leading zero of next packet
             * Fourth and more zeros are assumed to be trailing zeros of this
             * packet */
            if(zeroCount > 3)
            {
                index -= 4;
                zeroCount -= 3;
            }
            else
            {
                index -= zeroCount + 1;
                zeroCount = 0;
            }
            break;
        }
        else if(byte)
            zeroCount = 0;

        if(index == maxIndex)
        {
            break;
        }

    }

    /* Store pointer to the beginning of the packet */
    *pStrm = stream;
    prevIndex = index;

    /* If we skip this packet */
    if(picRdy && nextPacket &&
       ((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt))
    {
        /* Get the next packet */
        DEBUG_PRINT(("Packet Loss\n"));
        return NextPacket(pStrm);
    }
    else
    {
        /* nalUnitStream is without trailing zeros */
        if(nalUnitStream)
            index -= zeroCount;
        /*DEBUG_PRINT(("No Packet Loss\n")); */
        /*if (picRdy && streamTruncate && ((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt))
         * {
         * i32 ret;
         * DEBUG_PRINT(("Original packet size %d\n", index));
         * ret = TBRandomizeU32(&index);
         * if(ret != 0)
         * {
         * DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
         * return 0;
         * }
         * DEBUG_PRINT(("Randomized packet size %d\n", index));
         * } */
        return (index);
    }
}

/*------------------------------------------------------------------------------

    Function name: NextPacketFromFile

    Purpose:
        Get the pointer to start of next NAL-unit in input stream. Uses global 
        variables 'finput' to read the stream and 'packetize' to determine the
        decoder input stream mode.
        
        nalUnitStream a single NAL-unit without start code prefix. If not 
        enabled, a single NAL-unit with start code prefix

        pStrm stores pointer to the start of previous decoder input and is
        replaced with pointer to the start of the next decoder input.

        Returns the packet size in bytes

------------------------------------------------------------------------------*/
u32 NextPacketFromFile(u8 ** pStrm)
{

    u32 index = 0;
    u32 zeroCount;
    u8 byte;
    u8 nextPacket = 0;
    i32 ret = 0;
    u8 firstRead = 1;
    fpos_t strmPos;
    static u8 *stream = NULL;

    /* store the buffer start address for later use */
    if(stream == NULL)
        stream = *pStrm;
    else
        *pStrm = stream;

    /* If enabled, loose the packets (skip this packet first though) */
    if(streamPacketLoss)
    {
        u32 ret = 0;

        ret =
            TBRandomizePacketLoss(tbCfg.tbParams.streamPacketLoss, &nextPacket);
        if(ret != 0)
        {
            DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
            return 0;
        }
    }

    if(fgetpos(finput, &strmPos))
    {
        DEBUG_PRINT(("FILE POSITION GET ERROR\n"));
        return 0;
    }

    if(useIndex == 0)
    {
        /* test end of stream */
        ret = fread(&byte, 1, 1, finput);
        if(ferror(finput))
        {
            DEBUG_PRINT(("STREAM READ ERROR\n"));
            return 0;
        }
        if(feof(finput))
        {
            DEBUG_PRINT(("END OF STREAM\n"));
            return 0;
        }

        /* leading zeros of first NAL unit */
        do
        {
            index++;
            /* the byte is already read to test the end of stream */
            if(!firstRead)
            {
                ret = fread(&byte, 1, 1, finput);
                if(ferror(finput))
                {
                    DEBUG_PRINT(("STREAM READ ERROR\n"));
                    return 0;
                }
            }
            else
            {
                firstRead = 0;
            }
        }
        while(byte != 1 && !feof(finput));

        /* invalid start code prefix */
        if(feof(finput) || index < 3)
        {
            DEBUG_PRINT(("INVALID BYTE STREAM\n"));
            return 0;
        }

        /* nalUnitStream is without start code prefix */
        if(nalUnitStream)
        {
            if(fgetpos(finput, &strmPos))
            {
                DEBUG_PRINT(("FILE POSITION GET ERROR\n"));
                return 0;
            }
            index = 0;
        }

        zeroCount = 0;

        /* Search stream for next start code prefix */
        /*lint -e(716) while(1) used consciously */
        while(1)
        {
            /*byte = stream[index++]; */
            index++;
            ret = fread(&byte, 1, 1, finput);
            if(ferror(finput))
            {
                DEBUG_PRINT(("FILE ERROR\n"));
                return 0;
            }
            if(!byte)
                zeroCount++;

            if((byte == 0x01) && (zeroCount >= 2))
            {
                /* Start code prefix has two zeros
                 * Third zero is assumed to be leading zero of next packet
                 * Fourth and more zeros are assumed to be trailing zeros of this
                 * packet */
                if(zeroCount > 3)
                {
                    index -= 4;
                    zeroCount -= 3;
                }
                else
                {
                    index -= zeroCount + 1;
                    zeroCount = 0;
                }
                break;
            }
            else if(byte)
                zeroCount = 0;

            if(feof(finput))
            {
                --index;
                break;
            }
        }

        /* Store pointer to the beginning of the packet */
        if(fsetpos(finput, &strmPos))
        {
            DEBUG_PRINT(("FILE POSITION SET ERROR\n"));
            return 0;
        }

        if(saveIndex)
        {
            fprintf(findex, "%llu\n", strmPos);
            if(nalUnitStream)
            {
                /* store amount */
                fprintf(findex, "%llu\n", index); 
            }
        }

        /* Read the rewind stream */
        fread(*pStrm, 1, index, finput);
        if(feof(finput))
        {
            DEBUG_PRINT(("TRYING TO READ STREAM BEYOND STREAM END\n"));
            return 0;
        }
        if(ferror(finput))
        {
            DEBUG_PRINT(("FILE ERROR\n"));
            return 0;
        }
    }
    else
    {
        u32 amount = 0;
        u32 fPos = 0;

        if(nalUnitStream)
            fscanf(findex, "%llu", &curIndex);

        /* check position */
        fPos = ftell(finput);
        if(fPos != curIndex)
        {
            fseeko64(finput, curIndex - fPos, SEEK_CUR);
        }

        if(nalUnitStream)
        {
            fscanf(findex, "%llu", &amount);
        }
        else
        {
            fscanf(findex, "%llu", &nextIndex);
            amount = nextIndex - curIndex;
            curIndex = nextIndex;
        }

        fread(*pStrm, 1, amount, finput);
        index = amount;
    }
    /* If we skip this packet */
    if(picRdy && nextPacket &&
       ((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt))
    {
        /* Get the next packet */
        DEBUG_PRINT(("Packet Loss\n"));
        return NextPacket(pStrm);
    }
    else
    {
        /*DEBUG_PRINT(("No Packet Loss\n")); */
        /*if (picRdy && streamTruncate && ((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt))
         * {
         * i32 ret;
         * DEBUG_PRINT(("Original packet size %d\n", index));
         * ret = TBRandomizeU32(&index);
         * if(ret != 0)
         * {
         * DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
         * return 0;
         * }
         * DEBUG_PRINT(("Randomized packet size %d\n", index));
         * } */
        return (index);
    }
}

/*------------------------------------------------------------------------------

    Function name: CropPicture

    Purpose:
        Perform cropping for picture. Input picture pInImage with dimensions
        frameWidth x frameHeight is cropped with pCropParams and the resulting
        picture is stored in pOutImage.

------------------------------------------------------------------------------*/
u32 CropPicture(u8 * pOutImage, u8 * pInImage,
                u32 frameWidth, u32 frameHeight, H264CropParams * pCropParams,
                u32 monoChrome)
{

    u32 i, j;
    u32 outWidth, outHeight;
    u8 *pOut, *pIn;

    if(pOutImage == NULL || pInImage == NULL || pCropParams == NULL ||
       !frameWidth || !frameHeight)
    {
        return (1);
    }

    if(((pCropParams->cropLeftOffset + pCropParams->cropOutWidth) >
        frameWidth) ||
       ((pCropParams->cropTopOffset + pCropParams->cropOutHeight) >
        frameHeight))
    {
        return (1);
    }

    outWidth = pCropParams->cropOutWidth;
    outHeight = pCropParams->cropOutHeight;

    /* Calculate starting pointer for luma */
    pIn = pInImage + pCropParams->cropTopOffset * frameWidth +
        pCropParams->cropLeftOffset;
    pOut = pOutImage;

    /* Copy luma pixel values */
    for(i = outHeight; i; i--)
    {
        for(j = outWidth; j; j--)
        {
            *pOut++ = *pIn++;
        }
        pIn += frameWidth - outWidth;
    }

#if 0   /* planar */
    outWidth >>= 1;
    outHeight >>= 1;

    /* Calculate starting pointer for cb */
    pIn = pInImage + frameWidth * frameHeight +
        pCropParams->cropTopOffset * frameWidth / 4 +
        pCropParams->cropLeftOffset / 2;

    /* Copy cb pixel values */
    for(i = outHeight; i; i--)
    {
        for(j = outWidth; j; j--)
        {
            *pOut++ = *pIn++;
        }
        pIn += frameWidth / 2 - outWidth;
    }

    /* Calculate starting pointer for cr */
    pIn = pInImage + 5 * frameWidth * frameHeight / 4 +
        pCropParams->cropTopOffset * frameWidth / 4 +
        pCropParams->cropLeftOffset / 2;

    /* Copy cr pixel values */
    for(i = outHeight; i; i--)
    {
        for(j = outWidth; j; j--)
        {
            *pOut++ = *pIn++;
        }
        pIn += frameWidth / 2 - outWidth;
    }
#else /* semiplanar */

    if(monoChrome)
        return 0;

    outHeight >>= 1;

    /* Calculate starting pointer for chroma */
    pIn = pInImage + frameWidth * frameHeight +
        (pCropParams->cropTopOffset * frameWidth / 2 +
         pCropParams->cropLeftOffset);

    /* Copy chroma pixel values */
    for(i = outHeight; i; i--)
    {
        for(j = outWidth; j; j -= 2)
        {
            *pOut++ = *pIn++;
            *pOut++ = *pIn++;
        }
        pIn += (frameWidth - outWidth);
    }

#endif

    return (0);
}

/*------------------------------------------------------------------------------

    Function name:  H264DecTrace

    Purpose:
        Example implementation of H264DecTrace function. Prototype of this
        function is given in H264DecApi.h. This implementation appends
        trace messages to file named 'dec_api.trc'.

------------------------------------------------------------------------------*/
void H264DecTrace(const char *string)
{
    FILE *fp;

#if 0
    fp = fopen("dec_api.trc", "at");
#else
    fp = stderr;
#endif

    if(!fp)
        return;

    fprintf(fp, "%s", string);

    if(fp != stderr)
        fclose(fp);
}

/*------------------------------------------------------------------------------

    Function name:  bsdDecodeReturn

    Purpose: Print out decoder return value

------------------------------------------------------------------------------*/
static void printDecodeReturn(i32 retval)
{

    DEBUG_PRINT(("TB: H264DecDecode returned: "));
    switch (retval)
    {

    case H264DEC_OK:
        DEBUG_PRINT(("H264DEC_OK\n"));
        break;
    case H264DEC_NONREF_PIC_SKIPPED:
        DEBUG_PRINT(("H264DEC_NONREF_PIC_SKIPPED\n"));
        break;
    case H264DEC_STRM_PROCESSED:
        DEBUG_PRINT(("H264DEC_STRM_PROCESSED\n"));
        break;
    case H264DEC_PIC_RDY:
        DEBUG_PRINT(("H264DEC_PIC_RDY\n"));
        break;
    case H264DEC_PIC_DECODED:
        DEBUG_PRINT(("H264DEC_PIC_DECODED\n"));
        break;
    case H264DEC_ADVANCED_TOOLS:
        DEBUG_PRINT(("H264DEC_ADVANCED_TOOLS\n"));
        break;
    case H264DEC_HDRS_RDY:
        DEBUG_PRINT(("H264DEC_HDRS_RDY\n"));
        break;
    case H264DEC_STREAM_NOT_SUPPORTED:
        DEBUG_PRINT(("H264DEC_STREAM_NOT_SUPPORTED\n"));
        break;
    case H264DEC_DWL_ERROR:
        DEBUG_PRINT(("H264DEC_DWL_ERROR\n"));
        break;
    case H264DEC_HW_TIMEOUT:
        DEBUG_PRINT(("H264DEC_HW_TIMEOUT\n"));
        break;
    default:
        DEBUG_PRINT(("Other %d\n", retval));
        break;
    }
}

u32 fillBuffer(u8 *stream)
{
    u32 amount = 0;
    u32 dataLen = 0;

    if(curIndex != ftell(finput))
    {
        fseeko64(finput, curIndex, SEEK_SET);
    }

    /* read next index */
    fscanf(findex, "%llu", &nextIndex);
    amount = nextIndex - curIndex;
    curIndex = nextIndex;

    /* read data */
    dataLen = fread(stream, 1, amount, finput);

    return dataLen;
}

#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 picDisplayNumber, u32 viewId)
{

    static char catcmd0[100];
    static char catcmd1[100];
    static char rmcmd[100];
    const char *fname;

    pp_write_output(picDisplayNumber - 1, 0, 0);

    if (enableMvc && mvcSeparateViews)
    {
        if (picDisplayNumber == 1)
        {
            /* determine output file names for base and stereo views */
            char outName[100];
            char ext[10];
            u32 idx;
            fname = pp_get_out_name();
            strcpy(outName, fname);
            idx = strlen(outName);
            while(idx && outName[idx] != '.') idx--;
            strcpy(ext, outName + idx);
            sprintf(outName + idx, "_0%s", ext);
            sprintf(rmcmd, "rm -f %s", outName);
            system(rmcmd);
            sprintf(catcmd0, "cat %s >> %s", fname, outName);
            strcpy(outName, fname);
            sprintf(outName + idx, "_1%s", ext);
            sprintf(rmcmd, "rm -f %s", outName);
            system(rmcmd);
            sprintf(catcmd1, "cat %s >> %s", fname, outName);
            sprintf(rmcmd,   "rm %s", fname);
        }

        if (viewId == 0)
            system(catcmd0);
        else
            system(catcmd1);

        system(rmcmd);
    }

}
#endif
