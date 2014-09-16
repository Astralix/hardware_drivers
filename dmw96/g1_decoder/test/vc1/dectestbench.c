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
--  Abstract : Command line interface for VC-1 decoder
--
------------------------------------------------------------------------------*/
#include "vc1decapi.h"
#include "dwl.h"
#include "trace.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef USE_EFENCE
#include "efence.h"
#endif

#include "vc1hwd_container.h"
#include "regdrv.h"
#include "tb_cfg.h"
#include "tb_tiled.h"

#ifdef PP_PIPELINE_ENABLED
#include "pptestbench.h"
#include "ppapi.h"
#endif

#ifdef MD5SUM
#include "tb_md5.h"
#endif

#include "tb_sw_performance.h"

#ifdef ASIC_TRACE_SUPPORT
#include "trace.h"
#endif

/* Define RCV format metadata max size. Standard specifies 44 bytes, add one 
 * to support some non-compliant streams */
#define RCV_METADATA_MAX_SIZE   (44+1)

void VC1DecTrace(const char *string){
    printf("%s\n", string);
}

/*------------------------------------------------------------------------------
Module defines
------------------------------------------------------------------------------*/

#define VC1_MAX_STREAM_SIZE  DEC_X170_MAX_STREAM>>1

/* Debug prints */
#define DEBUG_PRINT(argv) printf argv

/*void decsw_performance(void)  __attribute__((noinline));*/
void decsw_performance(void);

void WriteOutput(char *filename, char *filenameTiled, u8 * data, u32 frameNumber, 
                 u32 width, u32 height, u32 interlaced, u32 top, u32 firstField,
                 u32 tiledMode );
u32 NextPacket(u8 ** pStrm);
u32 CropPicture(u8 *pOutImage, u8 *pInImage,
                u32 picWidth, u32 picHeight, u32 outWidth, u32 outHeight );

void FramePicture( u8 *pIn, i32 inWidth, i32 inHeight,
                   i32 inFrameWidth, i32 inFrameHeight,
                   u8 *pOut, i32 outWidth, i32 outHeight );
u32 fillBuffer(u8 *stream);

/* Global variables for stream handling */
u32 rcvV2;
u32 rcvMetadataSize = 0;
u8 *streamStop = NULL;
FILE *foutput = NULL;
FILE *fTiledOutput = NULL;
FILE *finput;

i32 DecodeRCV(u8 *stream, u32 strmLen, VC1DecMetaData *metaData);
u32 DecodeFrameLayerData(u8 *stream);
static void vc1DecPrintReturnValue(VC1DecRet ret);
static u32 GetNextDuSize(const u8* stream, const u8* streamStart, 
                         u32 strmLen, u32 *skippedBytes);

/* stream start address */
u8 *byteStrmStart;

u32 enableFramePicture = 0;
u32 numberOfWrittenFrames = 0;
u32 numFrameBuffers = 0;
/* index */
u32 saveIndex = 0;
u32 useIndex = 0;
FILE *fIndex = NULL;

off64_t curIndex = 0;
off64_t nextIndex = 0;
off64_t lastStreamPos = 0;

/* SW/SW testing, read stream trace file */
FILE * fStreamTrace = NULL;

u32 disableOutputWriting = 0;
u32 displayOrder = 0;
u32 clock_gating = DEC_X170_INTERNAL_CLOCK_GATING;
u32 data_discard = DEC_X170_DATA_DISCARD_ENABLE;
u32 latency_comp = DEC_X170_LATENCY_COMPENSATION;
u32 output_picture_endian = DEC_X170_OUTPUT_PICTURE_ENDIAN;
u32 bus_burst_length = DEC_X170_BUS_BURST_LENGTH;
u32 asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
u32 output_format = DEC_X170_OUTPUT_FORMAT;
u32 planarOutput = 0;
u32 interlacedField = 0;
u32 streamPacketLoss = 0;
u32 streamTruncate = 0;
u32 streamHeaderCorrupt = 0;
u32 hdrsRdy = 0;
u32 picRdy = 0;
u32 sliceUdInPacket = 0;
u32 usePeekOutput = 0;
u32 skipNonReference = 0;
u32 sliceMode = 0;

u32 tiledOutput = DEC_REF_FRM_RASTER_SCAN;
u32 dpbMode = DEC_DPB_DEFAULT;
u32 dumpDpbContents = 1;
u32 convertTiledOutput = 0;
u32 convertToFrameDpb = 0;

TBCfg tbCfg;
/* for tracing */
u32 bFrames=0;
#ifdef ASIC_TRACE_SUPPORT
extern u32 hwDecPicCount;
extern u32 gHwVer;
#endif

#ifdef VC1_EVALUATION
extern u32 gHwVer;
#endif

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
    u32 strmLen = 0;
    u32 picSize = 0;
    u32 maxPicSize;
    VC1DecInst decInst;
    VC1DecRet ret;
    VC1DecRet tmpret;
    VC1DecInput decInput;
    VC1DecOutput decOutput;
    VC1DecPicture decPicture;
    VC1DecMetaData metaData;
    VC1DecInfo info;
    DWLLinearMem_t streamMem;
    DWLHwConfig_t hwConfig;
    u8* tmpStrm = 0;
    u32 picId = 0;

    u32 picDecodeNumber = 0;
    u32 picDisplayNumber = 0;
    u32 numErrors = 0;
    u32 cropDisplay = 0;
    u32 disableOutputReordering = 0;

    u32 codedPicWidth = 0;
    u32 codedPicHeight = 0;

    FILE *fTBCfg;
    
    u32 seedRnd = 0;
    u32 streamBitSwap = 0;
    u32 advanced = 0;
    u32 skippedBytes = 0;
    off64_t streamPos = 0; /* For advanced profile long streams */

#ifdef PP_PIPELINE_ENABLED
    PPApiVersion ppVer;
    PPBuild ppBuild;
#endif

#ifdef ASIC_TRACE_SUPPORT
    gHwVer = 8190; /* default to 8190 mode */
#endif 

#ifdef VC1_EVALUATION_8170
    gHwVer = 8170;
#elif VC1_EVALUATION_8190
    gHwVer = 8190;
#elif VC1_EVALUATION_9170
    gHwVer = 9170;
#elif VC1_EVALUATION_9190
    gHwVer = 9190;
#elif VC1_EVALUATION_G1
    gHwVer = 10000;
#endif

    char outFileName[256] = "";
    char outFileNameTiled[256] = "out_tiled.yuv";
    u32 longStream = 0;
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

    INIT_SW_PERFORMANCE;

    {
        VC1DecApiVersion decApi;
        VC1DecBuild decBuild;

        /* Print API version number */
        decApi = VC1DecGetAPIVersion();
        decBuild = VC1DecGetBuild();
        DWLReadAsicConfig(&hwConfig);
        DEBUG_PRINT((
            "\n8170 VC-1 Decoder API v%d.%d - SW build: %d - HW build: %x\n",
            decApi.major, decApi.minor, decBuild.swBuild, decBuild.hwBuild));
        DEBUG_PRINT((
             "HW Supports video decoding up to %d pixels,\n",
             hwConfig.maxDecPicWidth));

        if(hwConfig.ppSupport)
            DEBUG_PRINT((
             "Maximum Post-processor output size %d pixels\n\n",
             hwConfig.maxPpOutPicWidth));
        else
            DEBUG_PRINT(("Post-Processor not supported\n\n"));
    }

#ifndef PP_PIPELINE_ENABLED
    /* Check that enough command line arguments given, if not -> print usage
    * information out */
    if(argc < 2)
    {
        DEBUG_PRINT(("Usage: %s [options] file.rcv\n", argv[0]));
        DEBUG_PRINT(("\t-Nn forces decoding to stop after n pictures\n"));
        DEBUG_PRINT(("\t-Ooutfile write output to \"outfile\" (default out_wxxxhyyy.yuv)\n"));
        DEBUG_PRINT(("\t-X Disable output file writing\n"));
        DEBUG_PRINT(("\t-C display cropped image (default decoded image)\n"));
        DEBUG_PRINT(("\t-Sfile.hex stream control trace file\n"));
        DEBUG_PRINT(("\t-L enable support for long streams.\n"));
        DEBUG_PRINT(("\t-P write planar output.\n"));
        DEBUG_PRINT(("\t-Bn to use n frame buffers in decoder\n"));
        DEBUG_PRINT(("\t-I save index file\n"));
        DEBUG_PRINT(("\t-E use tiled reference frame format.\n"));
        DEBUG_PRINT(("\t-G convert tiled output pictures to raster scan\n"));
        DEBUG_PRINT(("\t-Y Write output as Interlaced Fields (instead of Frames).\n"));
        DEBUG_PRINT(("\t-F Enable frame picture writing in multiresolutin output.\n"));        
        DEBUG_PRINT(("\t-Q Skip decoding non-reference pictures.\n")); 
        DEBUG_PRINT(("\t-Z output pictures using VC1DecPeek() function\n"));
        DEBUG_PRINT(("\t--separate-fields-in-dpb DPB stores interlaced content"\
                    " as fields (default: frames)\n"));
        DEBUG_PRINT(("\t--output-frame-dpb Convert output to frame mode even if"\
            " field DPB mode used\n"));

        return 0;
    }
    /* set stream pointers to null */
    streamMem.virtualAddress = NULL;
    streamMem.busAddress = 0;

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
        else if(strncmp(argv[i], "-S", 2) == 0)
        {
            fStreamTrace = fopen(argv[argc - 2], "r");
        }
        else if(strcmp(argv[i], "-L") == 0)
        {
            longStream = 1;
        }
        else if(strcmp(argv[i], "-P") == 0)
        {
            planarOutput = 1;
            enableFramePicture = 1;
        }
        else if (strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if (strncmp(argv[i], "-G", 2) == 0)
            convertTiledOutput = 1;        
        else if(strcmp(argv[i], "-Y") == 0)
        {
            interlacedField = 1;
        }
        else if(strcmp(argv[i], "-F") == 0)
        {
            enableFramePicture = 1;
        }        
        else if(strncmp(argv[i], "-B", 2) == 0)
        {
            numFrameBuffers = atoi(argv[i] + 2);
        }
        else if(strcmp(argv[i], "-Z") == 0)
        {
            usePeekOutput = 1;
        }
        else if(strcmp(argv[i], "-Q") == 0)
        {
            skipNonReference = 1;
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
            DEBUG_PRINT(("UNKNOWN PARAMETER: %s\n", argv[i]));
            return 1;
        }

    }

    /* open input file for reading, file name given by user. If file open
    * fails -> exit */
    finput = fopen(argv[argc - 1], "rb");
    if(finput == NULL)
    {
        DEBUG_PRINT(("UNABLE TO OPEN INPUT FILE: %s\n", argv[argc - 1]));
        return -1;
    }
#else
    /* Print API and build version numbers */
    ppVer = PPGetAPIVersion ();
    ppBuild = PPGetBuild ();

    /* set stream pointers to null */
    streamMem.virtualAddress = NULL;
    streamMem.busAddress = 0;

    /* Version */
    DEBUG_PRINT( (
        "\nX170 PP API v%d.%d - SW build: %d - HW build: %x\n",
        ppVer.major, ppVer.minor, ppBuild.swBuild, ppBuild.hwBuild));

    /* Check that enough command line arguments given, if not -> print usage
     * information out */
    if(argc < 3)
    {
        DEBUG_PRINT(("Usage: %s [-Nn] [-X] [-L] file.rcv pp.cfg\n", argv[0]));
        DEBUG_PRINT(("\t-Nn forces decoding to stop after n pictures\n"));
        DEBUG_PRINT(("\t-X disable output file writing\n"));
        DEBUG_PRINT(("\t-Bn to use n frame buffers in decoder\n"));
        DEBUG_PRINT(("\t-L enable support for long streams.\n"));
        DEBUG_PRINT(("\t-I save index file\n"));
        DEBUG_PRINT(("\t-E use tiled reference frame format.\n"));
        DEBUG_PRINT(("\t-Q Skip decoding non-reference pictures.\n")); 
        DEBUG_PRINT(("\t--separate-fields-in-dpb DPB stores interlaced content"\
                    " as fields (default: frames)\n"));       
        return 0;
    }

    remove("pp_out.yuv");
    
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
        else if(strcmp(argv[i], "-L") == 0)
        {
            longStream = 1;
        }
        else if (strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if(strncmp(argv[i], "-B", 2) == 0)
        {
            numFrameBuffers = atoi(argv[i] + 2);
            if(numFrameBuffers > 16)
                numFrameBuffers = 16;
        }
        else if(strcmp(argv[i], "-Q") == 0)
        {
            skipNonReference = 1;
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
            DEBUG_PRINT(("UNKNOWN PARAMETER: %s\n", argv[i]));
            return -1;
        }
    }

    /* open data file */
    finput = fopen(argv[argc - 2], "rb");
    if(finput == NULL)
    {
        DEBUG_PRINT(("UNABLE TO OPEN INPUT FILE: %s\n", argv[argc - 2]));
        return -1;
    }
#endif
#ifdef ASIC_TRACE_SUPPORT
    tmp = openTraceFiles();
    if (!tmp)
    {
        DEBUG_PRINT(("UNABLE TO OPEN TRACE FILE(S)\n"));
    }
#endif
    if(saveIndex)
    {
        fIndex = fopen("stream.cfg", "w");
        if(fIndex == NULL)
        {
            DEBUG_PRINT(("UNABLE TO OPEN INDEX FILE: \"stream.cfg\"\n"));
            return -1;
        }
    }
    else
    {
        fIndex = fopen("stream.cfg", "r");
        if(fIndex != NULL)
        {
            useIndex = 1;
        }
    }
    /* set test bench configuration */
    TBSetDefaultCfg(&tbCfg);
    fTBCfg = fopen("tb.cfg", "r");
    if (fTBCfg == NULL)
    {
        DEBUG_PRINT(("UNABLE TO OPEN INPUT FILE: \"tb.cfg\"\n"));
        DEBUG_PRINT(("USING DEFAULT CONFIGURATION\n"));
    }
    else
    {
        fclose(fTBCfg);
        if (TBParseConfig("tb.cfg", TBReadParam, &tbCfg) == TB_FALSE)
            return -1;
        if (TBCheckCfg(&tbCfg) != 0)
            return -1;
    }
    /*TBPrintCfg(&tbCfg);*/

    clock_gating = TBGetDecClockGating(&tbCfg);
    data_discard = TBGetDecDataDiscard(&tbCfg);
    latency_comp = tbCfg.decParams.latencyCompensation;
    output_picture_endian = TBGetDecOutputPictureEndian(&tbCfg);
    bus_burst_length = tbCfg.decParams.busBurstLength;
    asic_service_priority = tbCfg.decParams.asicServicePriority;
    output_format = TBGetDecOutputFormat(&tbCfg);
/*#if MD5SUM
    output_picture_endian = DEC_X170_LITTLE_ENDIAN;
    printf("Decoder Output Picture Endian forced to %d\n", output_picture_endian);
#endif*/

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
      -> do not wait the picture to finalize before starting stream corruption */
    if (streamHeaderCorrupt)
        picRdy = 1;
    streamTruncate = TBGetTBStreamTruncate(&tbCfg);
    sliceMode = TBGetTBPacketByPacket(&tbCfg);
    sliceUdInPacket = TBGetTBSliceUdInPacket(&tbCfg);
    if (strcmp(tbCfg.tbParams.streamBitSwap, "0") != 0)
    {
        streamBitSwap = 1;
    }
    else
    {
        streamBitSwap = 0;
    }
    if (strcmp(tbCfg.tbParams.streamPacketLoss, "0") != 0)
    {
        streamPacketLoss = 1;
    }
    else
    {
        streamPacketLoss = 0;
    }
    DEBUG_PRINT(("TB Seed Rnd %d\n", seedRnd));
    DEBUG_PRINT(("TB Stream Truncate %d\n", streamTruncate));
    DEBUG_PRINT(("TB Stream Header Corrupt %d\n", streamHeaderCorrupt));
    DEBUG_PRINT(("TB Packet By Packet %d\n", sliceMode));
    DEBUG_PRINT(("TB Slice User Data In Packet %d\n", sliceUdInPacket));
    DEBUG_PRINT(("TB Stream Bit Swap %d; odds %s\n", 
                streamBitSwap, tbCfg.tbParams.streamBitSwap));
    DEBUG_PRINT(("TB Stream Packet Loss %d; odds %s\n",
            streamPacketLoss, tbCfg.tbParams.streamPacketLoss));

    tmpStrm = (u8*) malloc(100);
    if (NULL == tmpStrm) {
        DEBUG_PRINT(("MALLOC FAILED\n"));
        return -1;
    }

    /* read metadata (max size (5+4)*4 bytes, 
     * DecodeRCV checks that size is at
     * least RCV_METADATA_MAX_SIZE bytes) */
    fread(tmpStrm, sizeof(u8), RCV_METADATA_MAX_SIZE, finput);
    rewind(finput);
    
    /* Advanced profile if startcode prefix found */
    if ( (tmpStrm[0] == 0x00) && (tmpStrm[1] == 0x00) && (tmpStrm[2] == 0x01) )
        advanced = 1;

    /* reset metadata structure */
    DWLmemset(&metaData, 0, sizeof(metaData));

    if (!advanced)
    {
        /* decode row coded video header (coded metadata). DecodeRCV function reads
        * image dimensions from struct A, struct C information is parsed by
        * VC1DecUnpackMetaData function */
        tmp = DecodeRCV(tmpStrm, RCV_METADATA_MAX_SIZE, &metaData);
        if (tmp != 0)
        {
            DEBUG_PRINT(("DECODING RCV FAILED\n"));
            free(tmpStrm);
            return -1;
        }
    }
    else
    {
        metaData.profile = 8;
    }

    TBInitializeRandom(seedRnd);

    if (!advanced && streamHeaderCorrupt && streamBitSwap)
    {
        DEBUG_PRINT(("metaData[0] %x\n", *(tmpStrm+8)));
        DEBUG_PRINT(("metaData[1] %x\n", *(tmpStrm+9)));
        DEBUG_PRINT(("metaData[2] %x\n", *(tmpStrm+10)));
        DEBUG_PRINT(("metaData[3] %x\n", *(tmpStrm+11)));
        tmp = TBRandomizeBitSwapInStream(tmpStrm+8, 4, tbCfg.tbParams.streamBitSwap);
        if (tmp != 0)
        {
            DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
            free(tmpStrm);
            return -1;
        }
        DEBUG_PRINT(("Randomized metaData[0] %x\n", *(tmpStrm+8)));
        DEBUG_PRINT(("Randomized metaData[1] %x\n", *(tmpStrm+9)));
        DEBUG_PRINT(("Randomized metaData[2] %x\n", *(tmpStrm+10)));
        DEBUG_PRINT(("Randomized metaData[3] %x\n", *(tmpStrm+11)));
    }

    if (!advanced)
    {
        decsw_performance();
		START_SW_PERFORMANCE
        tmp = VC1DecUnpackMetaData(tmpStrm+8, 4, &metaData);
		END_SW_PERFORMANCE
        decsw_performance();
        if (tmp != VC1DEC_OK)
        {
            DEBUG_PRINT(("UNPACKING META DATA FAILED\n"));
            free(tmpStrm);
            return -1;
        }
        DEBUG_PRINT(("metaData.vsTransform %d\n", metaData.vsTransform));
        DEBUG_PRINT(("metaData.overlap %d\n", metaData.overlap));
        DEBUG_PRINT(("metaData.syncMarker %d\n", metaData.syncMarker));
        DEBUG_PRINT(("metaData.quantizer %d\n", metaData.quantizer));
        DEBUG_PRINT(("metaData.frameInterp %d\n", metaData.frameInterp));
        DEBUG_PRINT(("metaData.maxBframes %d\n", metaData.maxBframes));
        DEBUG_PRINT(("metaData.fastUvMc %d\n", metaData.fastUvMc));
        DEBUG_PRINT(("metaData.extendedMv %d\n", metaData.extendedMv));
        DEBUG_PRINT(("metaData.multiRes %d\n", metaData.multiRes));
        DEBUG_PRINT(("metaData.rangeRed %d\n", metaData.rangeRed));
        DEBUG_PRINT(("metaData.dquant %d\n", metaData.dquant));
        DEBUG_PRINT(("metaData.loopFilter %d\n", metaData.loopFilter));
        DEBUG_PRINT(("metaData.profile %d\n", metaData.profile));
        
        hdrsRdy = 1;
    }
    if (!advanced && streamHeaderCorrupt)
    {
        u32 rndValue;
        /* randomize picture width */
        DEBUG_PRINT(("metaData.maxCodedWidth %d\n", metaData.maxCodedWidth));
        rndValue = 1920 + 48;
        tmp = TBRandomizeU32(&rndValue);
        if (tmp != 0)
        {
            DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
            free(tmpStrm);
            return -1;
        }
        metaData.maxCodedWidth = rndValue & (~0x15);
        DEBUG_PRINT(("Randomized metaData.maxCodedWidth %d\n", metaData.maxCodedWidth));

        /* randomize picture height */
        DEBUG_PRINT(("metaData.maxCodedHeight %d\n", metaData.maxCodedHeight));
        rndValue = 1920 + 48;
        tmp = TBRandomizeU32(&rndValue);
        if (tmp != 0)
        {
            DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
            free(tmpStrm);
            return -1;
        }
        metaData.maxCodedHeight = rndValue & (~0x15);
        DEBUG_PRINT(("Randomized metaData.maxCodedHeight %d\n", metaData.maxCodedHeight));
    }

    /* initialize decoder. If unsuccessful -> exit */
    decsw_performance();
    START_SW_PERFORMANCE;
    {
        DecDpbFlags flags = 0;
        if( tiledOutput )   flags |= DEC_REF_FRM_TILED_DEFAULT;
        if( dpbMode == DEC_DPB_INTERLACED_FIELD )
            flags |= DEC_DPB_ALLOW_FIELD_ORDERING;
        ret = VC1DecInit(&decInst, &metaData,
                         TBGetDecIntraFreezeEnable( &tbCfg ),
                         numFrameBuffers,
                         flags);
    }
    END_SW_PERFORMANCE;
    decsw_performance();

    if (ret != VC1DEC_OK)
    {
        DEBUG_PRINT(("DECODER INITIALIZATION FAILED\n"));
        goto end;
    }

    /* Set ref buffer test mode */
    ((decContainer_t *) decInst)->refBufferCtrl.testFunction = TBRefbuTestMode;
    TBSetRefbuMemModel( &tbCfg, 
        ((decContainer_t *) decInst)->vc1Regs,
        &((decContainer_t *) decInst)->refBufferCtrl );

    decInput.skipNonReference = skipNonReference;

#ifdef PP_PIPELINE_ENABLED
    /* Initialize the post processor. If unsuccessful -> exit */

    if(pp_startup
        (argv[argc - 1], decInst, PP_PIPELINED_DEC_TYPE_VC1, &tbCfg) != 0)
    {
        fprintf(stdout, "PP INITIALIZATION FAILED\n");
        goto end;
    }

    if(!advanced)
    {
        if (pp_update_config
                (decInst, PP_PIPELINED_DEC_TYPE_VC1, &tbCfg) == CFG_UPDATE_FAIL)
        {
            fprintf(stdout, "PP CONFIG LOAD FAILED\n");
            goto end;
        }

        /* get info */
        decsw_performance();
        VC1DecGetInfo(decInst, &info);
        decsw_performance();
        
        dpbMode = info.dpbMode;        

        /* If unspecified at cmd line, use minimum # of buffers, otherwise
         * use specified amount. */
        if(numFrameBuffers == 0)
            pp_number_of_buffers(info.multiBuffPpSize);
        else
            pp_number_of_buffers(numFrameBuffers);
    }
#endif

    if (!longStream) {
        /* check size of the input file -> length of the stream in bytes */
        fseek(finput, 0L, SEEK_END);
        strmLen = (u32) ftell(finput);
        rewind(finput);

        /* sets the stream length to random value*/
        if (streamTruncate && !sliceMode)
        {
            DEBUG_PRINT(("strmLen %d\n", strmLen));
            tmp = TBRandomizeU32(&strmLen);
            if (tmp != 0)
            {
                DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
                goto end;
            }
            DEBUG_PRINT(("Randomized strmLen %d\n", strmLen));
        }

        /* allocate memory for stream buffer. if unsuccessful -> exit */


        if(DWLMallocLinear(((decContainer_t*)decInst)->dwl, strmLen, &streamMem)
                != DWL_OK )
        {
            DEBUG_PRINT(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
            goto end;
        }

        byteStrmStart = (u8 *) streamMem.virtualAddress;

        if(byteStrmStart == NULL)
        {
            DEBUG_PRINT(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
            goto end;
        }

        /* read input stream from file to buffer and close input file */
        fread(byteStrmStart, sizeof(u8), strmLen, finput);
        fclose(finput);

        /* initialize VC1DecDecode() input structure */
        streamStop = byteStrmStart + strmLen;

        if (!advanced)
        {
            /* size of Sequence layer data structure */
            decInput.pStream = byteStrmStart + ( 4 + 4 * rcvV2 ) *4 + rcvMetadataSize;
            decInput.streamSize = DecodeFrameLayerData((u8*)decInput.pStream);
            decInput.pStream += 4 + 4 * rcvV2; /* size of Frame layer data structure */
            decInput.streamBusAddress = (u32)streamMem.busAddress +
                    (decInput.pStream - byteStrmStart);
        }
        else
        {
            decInput.pStream = byteStrmStart;
            decInput.streamSize = strmLen;
            decInput.streamBusAddress = (u32)streamMem.busAddress;
        }
    }
    /* LONG STREAM */
    else {
        if(DWLMallocLinear(((decContainer_t *) decInst)->dwl,
                VC1_MAX_STREAM_SIZE,
                &streamMem) != DWL_OK ){
            DEBUG_PRINT(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
            goto end;
        }
        byteStrmStart = (u8 *) streamMem.virtualAddress;
        decInput.pStream =  (u8 *) streamMem.virtualAddress;
        decInput.streamBusAddress = (u32)streamMem.busAddress;
        
        if (!advanced)
        {
            /* Read meta data and frame layer data */
            fread(tmpStrm, sizeof(u8), ( 4 + 4 * rcvV2 ) *4 + 4 + 4 * rcvV2 + rcvMetadataSize, finput);
            if (ferror(finput)) {
                DEBUG_PRINT(("STREAM READ ERROR\n"));
                goto end;
            }
            if (feof(finput)) {
                DEBUG_PRINT(("END OF STREAM\n"));
                goto end;
            }

            decInput.streamSize = DecodeFrameLayerData(tmpStrm + ( 4 + 4 * rcvV2 ) *4 + rcvMetadataSize);
            fread( (u8*)decInput.pStream, sizeof(u8), decInput.streamSize, finput );
        }
        else
        {
            if(useIndex)
            {
                decInput.streamSize = fillBuffer((u8*)decInput.pStream);
                strmLen = decInput.streamSize;
            }
            else
            {
                decInput.streamSize = 
                    fread((u8*)decInput.pStream, sizeof(u8), VC1_MAX_STREAM_SIZE, finput);
                strmLen = decInput.streamSize;
            }
        }

        if (ferror(finput)) {
            DEBUG_PRINT(("STREAM READ ERROR\n"));
            goto end;
        }
        if (feof(finput)) {
            DEBUG_PRINT(("STREAM WILL END\n"));
            /*goto end;*/
        }
    }

    if (!advanced)
    {
        /* If -O option not used, generate default file name */
        if (outFileName[0] == 0)
            sprintf(outFileName, "out_w%dh%d.yuv",
            metaData.maxCodedWidth, metaData.maxCodedHeight);
        
        /* max picture size */
        maxPicSize = (
            ( ( metaData.maxCodedWidth + 15 ) & ~15 ) *
            ( ( metaData.maxCodedHeight + 15 ) & ~15 ) ) * 3 / 2;
    }
    
    if( (cropDisplay || metaData.multiRes) && !advanced )
    {
        tmpImage = (u8*)malloc(maxPicSize * sizeof(u8) );
    }

    {
        SetDecRegister(((decContainer_t *) decInst)->vc1Regs,
            HWIF_DEC_LATENCY, latency_comp);
        SetDecRegister(((decContainer_t *) decInst)->vc1Regs,
            HWIF_DEC_CLK_GATE_E, clock_gating);
        /*
        SetDecRegister(((decContainer_t *) decInst)->vc1Regs,
            HWIF_DEC_OUT_TILED_E, output_format);
            */
        SetDecRegister(((decContainer_t *) decInst)->vc1Regs,
            HWIF_DEC_OUT_ENDIAN, output_picture_endian);
        SetDecRegister(((decContainer_t *) decInst)->vc1Regs,
            HWIF_DEC_MAX_BURST, bus_burst_length);
        if ((DWLReadAsicID() >> 16) == 0x8170U)
        {
            SetDecRegister(((decContainer_t *) decInst)->vc1Regs,
                HWIF_PRIORITY_MODE, asic_service_priority);
        }
        SetDecRegister(((decContainer_t *) decInst)->vc1Regs,
            HWIF_DEC_DATA_DISC_E, data_discard);
    }

    /* main decoding loop */
    do
    {
        decInput.picId = picId;

        DEBUG_PRINT(("Starting to decode picture ID %d\n", picId));
        
        /*printf("decInput.streamSize %d\n", decInput.streamSize);*/
        
        if (streamTruncate && picRdy && (hdrsRdy || streamHeaderCorrupt) && (longStream || (!longStream && sliceMode)))
        {
            i32 ret;
            ret = TBRandomizeU32(&decInput.streamSize);
            if(ret != 0)
            {
                DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
                return 0;
            }
            DEBUG_PRINT(("Randomized stream size %d\n", decInput.streamSize));
        }

        /* If enabled, break the stream*/
        if (streamBitSwap)
        {
            if((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt)
            {
                /*if (picRdy && ((picDecodeNumber%2 && corruptedBytes <= 0) == 0))*/
                if (picRdy && corruptedBytes <= 0)
                {
                    tmp = TBRandomizeBitSwapInStream(decInput.pStream,
                                                     decInput.streamSize,
                                                     tbCfg.tbParams.streamBitSwap);
                    if (tmp != 0)
                    {
                        DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
                        goto end;
                    }
                    
                    corruptedBytes = decInput.streamSize;
                    printf("corruptedBytes %d\n", corruptedBytes);
                }
            }
        }
        /* call API function to perform decoding */
        /* if stream size bigger than decoder can handle, skip the frame */
        /* checking error response for oversized pic done in API testing */
        if(decInput.streamSize <= VC1_MAX_STREAM_SIZE){

            decsw_performance();
            START_SW_PERFORMANCE;
            ret = VC1DecDecode(decInst, &decInput, &decOutput);
            END_SW_PERFORMANCE;
            /*DEBUG_PRINT(("decOutput.dataLeft %d\n", decOutput.dataLeft));*/
            decsw_performance();
            DEBUG_PRINT(("VC1DecDecode returned: "));
            vc1DecPrintReturnValue(ret);
            
#if 1            
            /* there is some data left */
            /* if simple or main, test bench does not care of decOutput.dataLeft but rather skips to next picture */
            /* if advanced but long stream mode, stream is read again from file */
            /* if slice mode, test bench does not care of decOutput.dataLeft but also skips to next slice */
            if(decOutput.dataLeft && advanced && !longStream && !sliceMode)
            {
                printf("decOutput.dataLeft %d\n", decOutput.dataLeft);
                corruptedBytes -= (decInput.streamSize - decOutput.dataLeft);
            }
            else
            {
                corruptedBytes = 0;
            }
#endif
        }
        else
        {
            ret = VC1DEC_STRM_PROCESSED;
            DEBUG_PRINT(("Oversized stream for picture, ignoring... \n"));
            break;
        }
        switch(ret)
        {
        case VC1DEC_RESOLUTION_CHANGED:
            TBSetRefbuMemModel( &tbCfg, 
                ((decContainer_t *) decInst)->vc1Regs,
                &((decContainer_t *) decInst)->refBufferCtrl );
            /* get info */
            decsw_performance();
            VC1DecGetInfo(decInst, &info);
            decsw_performance();
            DEBUG_PRINT(("RESOLUTION CHANGED\n"));
            DEBUG_PRINT(("New resolution is %dx%d\n",
                        info.codedWidth, info.codedHeight));

#ifdef PP_PIPELINE_ENABLED
            /* Flush picture buffer before configuring new image size to PP */
            decsw_performance();
            while( VC1DecNextPicture(decInst, &decPicture, 1 ) == VC1DEC_PIC_RDY )
            {
                decsw_performance();

                /* Increment display number for every displayed picture */
                picDisplayNumber++;


                /* write PP output */
                numberOfWrittenFrames++;
                if (!disableOutputWriting)
                {
                    pp_write_output( picDecodeNumber - 1,
                                     decPicture.fieldPicture, 
                                     decPicture.topField );
                }
                decsw_performance();
            }
            decsw_performance();

            if (pp_change_resolution( ((info.codedWidth+7) & ~7),
                                      ((info.codedHeight+7) & ~7), 
                                      &tbCfg) )
            {
                DEBUG_PRINT(("PP CONFIG FAILED!!!\n"));
                goto end;
            }
#endif
            break;

        case VC1DEC_HDRS_RDY:
            /* Set a flag to indicate that headers are ready */
            hdrsRdy = 1;
            TBSetRefbuMemModel( &tbCfg, 
                ((decContainer_t *) decInst)->vc1Regs,
                &((decContainer_t *) decInst)->refBufferCtrl );
                
            if( !longStream )
            {
                tmp = (decOutput.pStreamCurrPos - decInput.pStream);
                streamPos += tmp;
                decInput.pStream = decOutput.pStreamCurrPos;
                decInput.streamBusAddress += tmp;
                if (sliceMode)
                {
                    decInput.streamSize = 
                        GetNextDuSize(decOutput.pStreamCurrPos, 
                                      byteStrmStart, strmLen, &skippedBytes);
                    decInput.pStream += skippedBytes;
                    decInput.streamBusAddress += skippedBytes;
                    streamPos += skippedBytes;
                }
                else
                    decInput.streamSize = decOutput.dataLeft;
            }
            else /* LONG STREAM */
            {
                if(useIndex)
                {
                    if(decOutput.dataLeft != 0)
                    {
                        decInput.streamBusAddress += (decOutput.pStreamCurrPos - decInput.pStream);
                        decInput.streamSize = decOutput.dataLeft;
                        decInput.pStream = decOutput.pStreamCurrPos;
                    }
                    else
                    {
                        decInput.streamBusAddress = (u32)streamMem.busAddress;
                        decInput.pStream =  (u8 *) streamMem.virtualAddress;
                        decInput.streamSize = fillBuffer((u8*)decInput.pStream);
                    }

                }
                else
                {
                    tmp = (decOutput.pStreamCurrPos - decInput.pStream);
                    streamPos += tmp;
                    fseeko64( finput, streamPos, SEEK_SET );
                    decInput.streamSize = 
                        fread((u8*)decInput.pStream, sizeof(u8), VC1_MAX_STREAM_SIZE, finput);
                
                    if (sliceMode)
                    {
                        decInput.streamSize = 
                            GetNextDuSize((u8*)decInput.pStream, 
                                          decInput.pStream, decInput.streamSize, &skippedBytes);
                        streamPos += skippedBytes;
                        fseeko64( finput, streamPos, SEEK_SET );
                        if(saveIndex)
                        {
                            if(decInput.pStream[0] == 0 &&
                               decInput.pStream[1] == 0 &&
                               decInput.pStream[2] == 1)
                            {
                                fprintf(fIndex, "%llu\n", streamPos);
                            }
                        }
                        decInput.streamSize = 
                        fread((u8*)decInput.pStream, sizeof(u8), 
                                    decInput.streamSize, finput);
                    }
                }
                if (ferror(finput)) {
                    DEBUG_PRINT(("STREAM READ ERROR\n"));
                    goto end;
                }
                if (feof(finput)) {
                    DEBUG_PRINT(("STREAM WILL END\n"));
                    /*goto end;*/
                }
            }

            /* get info */
            decsw_performance();
            VC1DecGetInfo(decInst, &info);
            decsw_performance();
            
            dpbMode = info.dpbMode;
            
            if (info.interlacedSequence)
                printf("INTERLACED SEQUENCE\n");
            
            /* max picture size */
            maxPicSize = (info.maxCodedWidth * info.maxCodedHeight * 3)>>1;

            if( (cropDisplay || enableFramePicture) && (tmpImage == NULL) )
                tmpImage = (u8*)malloc(maxPicSize * sizeof(u8) );

            /* If -O option not used, generate default file name */
            if (outFileName[0] == 0)
            {
                if (!info.interlacedSequence || !interlacedField)
                {
                    sprintf(outFileName, "out_w%dh%d.yuv",
                        info.maxCodedWidth, info.maxCodedHeight);
                }
                else
                {
                    sprintf(outFileName, "out_w%dh%d.yuv",
                        info.maxCodedWidth, info.maxCodedHeight>>1);
                }
            }
#ifdef PP_PIPELINE_ENABLED
            pp_set_input_interlaced(info.interlacedSequence);
            if (pp_update_config
                    (decInst, PP_PIPELINED_DEC_TYPE_VC1, &tbCfg) == CFG_UPDATE_FAIL)
            {
                fprintf(stdout, "PP CONFIG LOAD FAILED\n");
                goto end;
            }

            /* If unspecified at cmd line, use minimum # of buffers, otherwise
             * use specified amount. */
            if(numFrameBuffers == 0)
                pp_number_of_buffers(info.multiBuffPpSize);
            else
                pp_number_of_buffers(numFrameBuffers);
#endif
            break;

        case VC1DEC_PIC_DECODED:
            /* Picture is now ready */
            picRdy = 1;
            printf("VC1DEC_PIC_DECODED\n");
            /* get info */
            decsw_performance();
            VC1DecGetInfo(decInst, &info);
            decsw_performance();

            /* Increment decoding number for every decoded picture */
            picDecodeNumber++;
            
            if (!longStream && !advanced) {
                decInput.pStream += decInput.streamSize;
                decInput.streamBusAddress += decInput.streamSize;
            }
    
            if (usePeekOutput &&
                VC1DecPeek(decInst, &decPicture) == VC1DEC_PIC_RDY)
            {
                picDisplayNumber++;
                DEBUG_PRINT(("OUTPUT PICTURE ID \t%d; SIZE %dx%d; ERR MBs %d; %s, ",
                    decPicture.picId, decPicture.codedWidth,
                    decPicture.codedHeight, decPicture.numberOfErrMBs,
                    decPicture.keyPicture ? "(KEYFRAME)" : ""));
                DEBUG_PRINT(("DECODED PIC ID %d\n\n", picId));
                if (codedPicWidth == 0)
                {
                    codedPicWidth = decPicture.codedWidth;
                    codedPicHeight = decPicture.codedHeight;
                }

                /* Write output picture to file */
                imageData = (u8*)decPicture.pOutputPicture;

                picSize = decPicture.frameWidth*decPicture.frameHeight*3/2;

                WriteOutput(outFileName, outFileNameTiled, imageData, 
                            picDisplayNumber - 1,  
                            ( ( decPicture.codedWidth + 15 ) & ~15 ),
                            ( ( decPicture.codedHeight + 15 ) & ~15 ),
                            info.interlacedSequence,
                            decPicture.topField, decPicture.firstField,
                            decPicture.outputFormat);
            }

            /* loop to get all pictures/fields out from decoder */
            do {
                decsw_performance();
                START_SW_PERFORMANCE;
                tmpret = VC1DecNextPicture(decInst, &decPicture, 0 );
                END_SW_PERFORMANCE;
                decsw_performance();
                
                if (!usePeekOutput)
                {
                    /* Increment display number for every displayed picture */
                    picDisplayNumber++;
     
                    DEBUG_PRINT(("VC1DecNextPicture returned: "));
                    vc1DecPrintReturnValue(tmpret);
     
                    if( tmpret == VC1DEC_PIC_RDY )
                    {
                        DEBUG_PRINT(("OUTPUT PICTURE ID \t%d; SIZE %dx%d; ERR MBs %d; %s, ",
                            decPicture.picId, decPicture.codedWidth,
                            decPicture.codedHeight, decPicture.numberOfErrMBs,
                            decPicture.keyPicture ? "(KEYFRAME)" : ""));
                        DEBUG_PRINT(("DECODED PIC ID %d\n\n", picId));
                        if (codedPicWidth == 0)
                        {
                            codedPicWidth = decPicture.codedWidth;
                            codedPicHeight = decPicture.codedHeight;
                        }
     
                        /* Write output picture to file */
                        imageData = (u8*)decPicture.pOutputPicture;
     
#ifndef PP_PIPELINE_ENABLED
     
                        if (cropDisplay &&
                            ( decPicture.frameWidth > decPicture.codedWidth ||
                            decPicture.frameHeight > decPicture.codedHeight ) )
                        {
                            picSize = decPicture.codedWidth *
                                      decPicture.codedHeight*3/2;
                            tmp = CropPicture(tmpImage, imageData,
                                decPicture.frameWidth, decPicture.frameHeight,
                                decPicture.codedWidth, decPicture.codedHeight );
                            if (tmp)
                                return -1;
                            WriteOutput(outFileName, outFileNameTiled, tmpImage, 
                                    picDisplayNumber - 1, decPicture.codedWidth, 
                                    decPicture.codedHeight,
                                    info.interlacedSequence, 
                                    decPicture.topField, decPicture.firstField,
                                    decPicture.outputFormat);
                        }
                        else
                        {
                            picSize = decPicture.frameWidth *
                                      decPicture.frameHeight*3/2;
     
                            if( (enableFramePicture
                                && ( decPicture.codedWidth !=
                                        metaData.maxCodedWidth ||
                                     decPicture.codedHeight !=
                                        metaData.maxCodedHeight ) ) )
                                {
                                    FramePicture( imageData,
                                                  decPicture.codedWidth,
                                                  decPicture.codedHeight,
                                                  decPicture.frameWidth,
                                                  decPicture.frameHeight,
                                                  tmpImage, 
                                                  info.maxCodedWidth,
                                                  info.maxCodedHeight );
                                    WriteOutput(outFileName, outFileNameTiled,
                                                tmpImage, picDisplayNumber - 1,
                                                info.maxCodedWidth, 
                                                info.maxCodedHeight,
                                                info.interlacedSequence,
                                                decPicture.topField,
                                                decPicture.firstField,
                                                decPicture.outputFormat);
                                }
                                else
                                {
                                    WriteOutput(outFileName, outFileNameTiled,
                                                imageData, picDisplayNumber - 1,
                                                ( ( decPicture.codedWidth + 15 ) & ~15 ),
                                                ( ( decPicture.codedHeight + 15 ) & ~15 ),
                                                info.interlacedSequence,
                                                decPicture.topField,
                                                decPicture.firstField,
                                                decPicture.outputFormat);
                                }
     
                        }
#else
                        
                        /* write PP output */
                        numberOfWrittenFrames++;
                        if (!disableOutputWriting)
                        {
                            if (pp_check_combined_status() == PP_OK)
                            {
                                pp_write_output( picDecodeNumber - 1,
                                                 decPicture.fieldPicture, 
                                                 decPicture.topField );
                            }
                            else
                            {
                                fprintf(stderr, "COMBINED STATUS FAILED\n");
                            }
            
                        if (pp_update_config
                            (decInst, PP_PIPELINED_DEC_TYPE_VC1, &tbCfg) == CFG_UPDATE_FAIL)
                        {
                            fprintf(stdout, "PP CONFIG LOAD FAILED\n"); 
                        }
                        }
#endif
                    }
                }
            
            } while (tmpret == VC1DEC_PIC_RDY);

            /* Update pic Id */
            picId++;

            /* If enough pictures decoded -> force decoding to end
            * by setting that no more stream is available */
            if ((maxNumPics && numberOfWrittenFrames == maxNumPics) ||
                decInput.pStream >= streamStop && !longStream)
                decInput.streamSize = 0;
            else
            {
                /* Decode frame layer metadata */
                if( !longStream )
                {
                    if( !advanced )
                    {
                        decInput.streamSize =
                                DecodeFrameLayerData((u8*)decInput.pStream);
                        decInput.pStream += 4 + 4 * rcvV2;
                        if ((decInput.pStream + decInput.streamSize) > streamStop)
                        {
                            decInput.streamSize = streamStop > decInput.pStream ?
                                streamStop-decInput.pStream : 0;
                        }
                        decInput.streamBusAddress += 4 + 4 * rcvV2;
                    }
                    else
                    {
                        tmp = (decOutput.pStreamCurrPos - decInput.pStream);
                        streamPos += tmp;
                        decInput.pStream = decOutput.pStreamCurrPos;
                        decInput.streamBusAddress += tmp;
                        if (sliceMode)
                        {
                            decInput.streamSize = 
                                GetNextDuSize(decOutput.pStreamCurrPos, 
                                              byteStrmStart, strmLen, &skippedBytes);
                            decInput.pStream += skippedBytes;
                            decInput.streamBusAddress += skippedBytes;
                            streamPos += skippedBytes;
                        }
                        else
                            decInput.streamSize = decOutput.dataLeft;
                    }
                }
                else /* LONG STREAM */
                {
                    if (!advanced) {
                        fread(tmpStrm, sizeof(u8),  4 + 4 * rcvV2, finput);
                        if (ferror(finput)) {
                            DEBUG_PRINT(("STREAM READ ERROR\n"));
                            goto end;
                        }
                        if (feof(finput)) {
                            DEBUG_PRINT(("END OF STREAM\n"));
                            decInput.streamSize = 0;
                            continue;
                        }
                        decInput.streamSize = DecodeFrameLayerData(tmpStrm);
                        fread((u8*)decInput.pStream, sizeof(u8), decInput.streamSize, finput);
                    }
                    else
                    {
                        if(useIndex)
                        {
                            if(decOutput.dataLeft != 0)
                            {
                                decInput.streamBusAddress += (decOutput.pStreamCurrPos - decInput.pStream);
                                decInput.streamSize = decOutput.dataLeft;
                                decInput.pStream = decOutput.pStreamCurrPos;
                            }
                            else
                            {
                                decInput.streamBusAddress = (u32)streamMem.busAddress;
                                decInput.pStream =  (u8 *) streamMem.virtualAddress;
                                decInput.streamSize = fillBuffer((u8*)decInput.pStream);
                            }
                        }
                        else
                        { 
                            tmp = (decOutput.pStreamCurrPos - decInput.pStream);
                            streamPos += tmp;
                            fseeko64( finput, streamPos, SEEK_SET );

                            decInput.streamSize = 
                                fread((u8*)decInput.pStream, sizeof(u8), VC1_MAX_STREAM_SIZE, finput);

                            if(saveIndex && !sliceMode)
                            {
                                if(decInput.pStream[0] == 0 &&
                                   decInput.pStream[1] == 0 &&
                                   decInput.pStream[2] == 1)
                                {
                                    fprintf(fIndex, "%llu\n", streamPos);
                                }                                
                            }


                            if (sliceMode)
                            {
                                decInput.streamSize = 
                                    GetNextDuSize((u8*)decInput.pStream, decInput.pStream,
                                              decInput.streamSize, &skippedBytes);
                                streamPos += skippedBytes;
                                fseeko64( finput, streamPos, SEEK_SET );

                                if(saveIndex)
                                {
                                    if(decInput.pStream[0] == 0 &&
                                       decInput.pStream[1] == 0 &&
                                       decInput.pStream[2] == 1)
                                    {
                                        fprintf(fIndex, "%llu\n", streamPos);
                                    }
                                } 
    
                                decInput.streamSize = 
                                fread((u8*)decInput.pStream, sizeof(u8), 
                                        decInput.streamSize, finput);
                            }
                        }
                    }
                    if (ferror(finput)) {
                        DEBUG_PRINT(("STREAM READ ERROR\n"));
                        goto end;
                    }
                    if (feof(finput)) {
                        DEBUG_PRINT(("STREAM WILL END\n"));
                        /*goto end;*/
                    }
                }
            }
            break;

        case VC1DEC_STRM_PROCESSED:
        case VC1DEC_NONREF_PIC_SKIPPED:
            /* Used to indicate that picture decoding needs to finalized prior
               to corrupting next picture 
            picRdy = 0;*/

            /* If enough pictures decoded -> force decoding to end
            * by setting that no more stream is available */
            if ((maxNumPics && numberOfWrittenFrames == maxNumPics) ||
                (decInput.pStream >= streamStop ) && !longStream)
                decInput.streamSize = 0;
            else
            {
                /* Decode frame layer metadata */
                if( !longStream )
                {
                    if( !advanced )
                    {
                        decInput.pStream += decInput.streamSize;
                        decInput.streamBusAddress += decInput.streamSize;

                        decInput.streamSize =
                                DecodeFrameLayerData((u8*)decInput.pStream);
                        decInput.pStream += 4 + 4 * rcvV2;
                        if ((decInput.pStream + decInput.streamSize) > streamStop)
                        {
                            decInput.streamSize = streamStop > decInput.pStream ?
                                streamStop-decInput.pStream : 0;
                        }
                        decInput.streamBusAddress += 4 + 4 * rcvV2;
                    }
                    else
                    {
                        tmp = (decOutput.pStreamCurrPos - decInput.pStream);
                        streamPos += tmp;
                        decInput.pStream = decOutput.pStreamCurrPos;
                        decInput.streamBusAddress += tmp;
                        if (sliceMode)
                        {
                            decInput.streamSize = 
                                GetNextDuSize(decOutput.pStreamCurrPos, 
                                              byteStrmStart, strmLen, &skippedBytes);
                            decInput.pStream += skippedBytes;
                            decInput.streamBusAddress += skippedBytes;
                            streamPos += skippedBytes;
                        }
                        else
                            decInput.streamSize = decOutput.dataLeft;
                    }
                }
                else /* LONG STREAM */
                {
                    if (!advanced) {
                        fread(tmpStrm, sizeof(u8),  4 + 4 * rcvV2, finput);
                        if (ferror(finput)) {
                            DEBUG_PRINT(("STREAM READ ERROR\n"));
                            goto end;
                        }
                        if (feof(finput)) {
                            DEBUG_PRINT(("END OF STREAM\n"));
                            decInput.streamSize = 0;
                            continue;
                        }
                        decInput.streamSize = DecodeFrameLayerData(tmpStrm);
                        fread((u8*)decInput.pStream, sizeof(u8), decInput.streamSize, finput);
                    }
                    else
                    {
                        if(useIndex)
                        {
                            if(decOutput.dataLeft != 0)
                            {
                                decInput.streamBusAddress += (decOutput.pStreamCurrPos - decInput.pStream);
                                decInput.streamSize = decOutput.dataLeft;
                                decInput.pStream = decOutput.pStreamCurrPos;
                            }
                            else
                            {
                                decInput.streamBusAddress = (u32)streamMem.busAddress;
                                decInput.pStream =  (u8 *) streamMem.virtualAddress;
                                decInput.streamSize = fillBuffer((u8*)decInput.pStream);
                            }

                        }
                        else
                        {
                            tmp = (decOutput.pStreamCurrPos - decInput.pStream);
                            streamPos += tmp;
                            fseeko64( finput, streamPos, SEEK_SET );

                            decInput.streamSize = 
                                fread((u8*)decInput.pStream, sizeof(u8), 
                                        VC1_MAX_STREAM_SIZE, finput);
                            if (sliceMode)
                            {
                                decInput.streamSize = 
                                    GetNextDuSize((u8*)decInput.pStream, decInput.pStream,
                                                  decInput.streamSize, &skippedBytes);
                                streamPos += skippedBytes;
                                fseeko64( finput, streamPos, SEEK_SET );

                                if(saveIndex)
                                {
                                    if(decInput.pStream[0] == 0 &&
                                       decInput.pStream[1] == 0 &&
                                       decInput.pStream[2] == 1)
                                    {
                                        fprintf(fIndex, "%llu\n", streamPos);
                                    }
                                 }

                                decInput.streamSize = 
                                fread((u8*)decInput.pStream, sizeof(u8), 
                                        decInput.streamSize, finput);
                            }
                        }
                    }
                    if (ferror(finput)) {
                        DEBUG_PRINT(("STREAM READ ERROR\n"));
                        goto end;
                    }
                    if (feof(finput)) {
                        DEBUG_PRINT(("STREAM WILL END\n"));
                        /*goto end;*/
                    }
                }
            }
            break;

        case VC1DEC_END_OF_SEQ:
            decInput.streamSize = 0;
            break;

        default:
            DEBUG_PRINT(("FATAL ERROR: %d\n", ret));
            goto end;
        }
        /* keep decoding until all data from input stream buffer consumed */
    }
    while(decInput.streamSize > 0);

    printf("STREAM END ENCOUNTERED\n");
    if(saveIndex && advanced)
    {
        tmp = (decOutput.pStreamCurrPos - decInput.pStream);
        streamPos += tmp;
        
        fprintf(fIndex, "%llu\n", streamPos);
    }
    if(saveIndex || useIndex)
    {
        fclose(fIndex);
    }

    /* Output buffered images also... */
    decsw_performance();
    START_SW_PERFORMANCE;
    while( !usePeekOutput &&
         ((maxNumPics && numberOfWrittenFrames < maxNumPics) || !maxNumPics ) &&
             VC1DecNextPicture(decInst, &decPicture, 1 ) == VC1DEC_PIC_RDY )
    {
        END_SW_PERFORMANCE;
        decsw_performance();
        /* Increment display number for every displayed picture */
        picDisplayNumber++;

#if !defined(VC1_EVALUATION_VERSION)
        DEBUG_PRINT(("   BUFFERED %s ID %d OUTPUT %dx%d\n\n",
            decPicture.keyPicture ? "KEYFRAME, " : "", decPicture.picId,
            decPicture.codedWidth,
            decPicture.codedHeight));
#endif

#ifndef PP_PIPELINE_ENABLED

        /* Write output picture to file */
        imageData = (u8*)decPicture.pOutputPicture;

        if (cropDisplay &&
            ( decPicture.frameWidth > decPicture.codedWidth ||
              decPicture.frameHeight > decPicture.codedHeight ) )
        {
            picSize = decPicture.codedWidth*decPicture.codedHeight*3/2;
            tmp = CropPicture(tmpImage, imageData,
                decPicture.frameWidth, decPicture.frameHeight,
                decPicture.codedWidth, decPicture.codedHeight );
            if (tmp)
                return -1;
            WriteOutput(outFileName, outFileNameTiled, tmpImage, 
                        picDisplayNumber - 1, decPicture.codedWidth, 
                        decPicture.codedHeight, info.interlacedSequence, 
                        decPicture.topField, decPicture.firstField,
                        decPicture.outputFormat);
        }
        else
        {
            picSize = decPicture.frameWidth*decPicture.frameHeight*3/2;

            if( (enableFramePicture
                    && ( decPicture.codedWidth != metaData.maxCodedWidth ||
                    decPicture.codedHeight != metaData.maxCodedHeight)))
                {
                    FramePicture( imageData, decPicture.codedWidth,
                                  decPicture.codedHeight,
                                  decPicture.frameWidth,
                                  decPicture.frameHeight,
                                  tmpImage, 
                                  info.maxCodedWidth,
                                  info.maxCodedHeight );
                    WriteOutput(outFileName, outFileNameTiled, tmpImage, 
                                picDisplayNumber - 1, info.maxCodedWidth, 
                                info.maxCodedHeight, info.interlacedSequence, 
                                decPicture.topField,decPicture.firstField,
                                decPicture.outputFormat);
                }
                else
                {
                    WriteOutput(outFileName, outFileNameTiled, imageData, 
                                picDisplayNumber - 1, 
                                ( ( decPicture.codedWidth + 15 ) & ~15 ),
                                ( ( decPicture.codedHeight + 15 ) & ~15 ),
                                info.interlacedSequence, 
                                decPicture.topField, decPicture.firstField,
                                decPicture.outputFormat);
                }
        }

#else
                    
        /* write PP output */
        numberOfWrittenFrames++;
        if (!disableOutputWriting)
        {
            if (pp_check_combined_status() == PP_OK)
            {
                pp_write_output( picDecodeNumber - 1,
                                 decPicture.fieldPicture, 
                                 decPicture.topField );
            }
            else
            {
                fprintf(stderr, "COMBINED STATUS FAILED\n");
            }
	    
	    if (pp_update_config
	        (decInst, PP_PIPELINED_DEC_TYPE_VC1, &tbCfg) == CFG_UPDATE_FAIL)
	    {
	        fprintf(stdout, "PP CONFIG LOAD FAILED\n"); 
	    }
        }
#endif
    decsw_performance();
    START_SW_PERFORMANCE;
    }
    END_SW_PERFORMANCE; 
    decsw_performance();

end:

    DEBUG_PRINT(("\nWidth %d Height %d\n", codedPicWidth, codedPicHeight));

    if( streamMem.virtualAddress != NULL)
        DWLFreeLinear(((decContainer_t *) decInst)->dwl, &streamMem);

    /* release decoder instance */
#ifdef PP_PIPELINE_ENABLED
    pp_close();
#endif
    START_SW_PERFORMANCE;
    VC1DecRelease(decInst);
    END_SW_PERFORMANCE;
    
#ifdef ASIC_TRACE_SUPPORT
    trace_SequenceCtrl(hwDecPicCount, bFrames);
    trace_VC1DecodingTools();
    closeTraceFiles();
#endif
    if(foutput)
        fclose(foutput);
    if( fStreamTrace)
        fclose(fStreamTrace);
    if(fTiledOutput)
        fclose(fTiledOutput);
    if (longStream && finput)
        fclose(finput);

    /* free allocated buffers */
    if (tmpImage)
        free(tmpImage);
    if (tmpStrm)
        free(tmpStrm);

    foutput = fopen(outFileName, "rb");
    if (NULL == foutput)
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
    DEBUG_PRINT(("NUMBER OF WRITTEN FRAMES %d\n", numberOfWrittenFrames));
    
    FINALIZE_SW_PERFORMANCE;

    DEBUG_PRINT(("DECODING DONE\n"));

    if(numErrors || picDecodeNumber == 1)
    {
        DEBUG_PRINT(("ERRORS FOUND in %d out of %d PICTURES\n",
            numErrors, picDecodeNumber));
        return 1;
    }

    return 0;
}

/*------------------------------------------------------------------------------

 Function name:  GetNextDuSize

  Purpose:
    Get stream slice by slice...

------------------------------------------------------------------------------*/
u32 GetNextDuSize(const u8* stream, const u8* streamStart, u32 strmLen, u32 *skippedBytes)
{
    const u8* p;
    u8 byte;
    u32 zero = 0;
    u32 size = 0;
    u32 totalSize = 0;
    u32 scPrefix = 0;
    u32 nextPacket = 0;
    u32 tmp = 0;
    
    *skippedBytes = 0;
    strmLen -= (stream - streamStart);
    p = stream;

    if (strmLen < 3)
        return strmLen;
    
    /* If enabled, loose the packets (skip this packet first though) */
    if (streamPacketLoss)
    {
        u32 ret = 0;
        ret = TBRandomizePacketLoss(tbCfg.tbParams.streamPacketLoss, &nextPacket);
        if (ret != 0)
        {
           printf("RANDOM STREAM ERROR FAILED\n");
           return 0;
        }
    }
    if (picRdy && nextPacket && (hdrsRdy || streamHeaderCorrupt))
    {
        DEBUG_PRINT(("\nPacket Loss\n"));
        tmp = 1;
    }
    /*else
    {
        DEBUG_PRINT(("\nNo Packet Loss\n"));
    } */

    /* Seek start of the next slice */
    while(1)
    {
        byte = *p++;
        size++;
        totalSize++;
        
        if (totalSize >= strmLen)
            return size;
 
        if (!byte)
            zero++;
        else if ( (byte == 0x01) && (zero >=2) )
            scPrefix = 1;
        else if (scPrefix && (byte>=0x0A && byte<=0x0F) && sliceUdInPacket)
        {
            DEBUG_PRINT(("sliceUdInPacket\n"));
            if (tmp)
            {
                zero = 0;
                scPrefix = 0;
                tmp = 0;
            }
            else
            {
                *skippedBytes += (size-4);
                size -= *skippedBytes;
                zero = 0;
                scPrefix = 0;
                
                if (nextPacket)
                    size = 0;
                
                break;
            }
        }
        else if (scPrefix && ((byte>=0x0A && byte<=0x0F) || (byte>=0x1B && byte<=0x1F)) && !sliceUdInPacket)
        {
            DEBUG_PRINT(("No sliceUdInPacket\n"));
            if (tmp)
            {
                zero = 0;
                scPrefix = 0;
                tmp = 0;
            }
            else
            {
                *skippedBytes += (size-4);
                size -= *skippedBytes;
                zero = 0;
                scPrefix = 0;
                
                if (nextPacket)
                    size = 0;
                
                break;
            }
        }
        else
        {
            zero = 0;
            scPrefix = 0;
        }
    }
    zero = 0;
    /* Seek end of the next slice */
    while(1)
    {
        byte = *p++;
        size++;
        totalSize++;
        
        if (totalSize >= strmLen)
            return size;

        if (!byte)
            zero++;
        else if ( (byte == 0x01) && (zero >=2) )
            scPrefix = 1;
        else if (scPrefix && (byte>=0x0A && byte<=0x0F) && sliceUdInPacket)
        {
            DEBUG_PRINT(("sliceUdInPacket\n"));
            size -= 4;
            break;
        }
        else if (scPrefix && ((byte>=0x0A && byte<=0x0F) || (byte>=0x1B && byte<=0x1F)) && !sliceUdInPacket)
        {
            DEBUG_PRINT(("No sliceUdInPacket\n"));
            size -= 4;
            break;
        }
        else
        {
            zero = 0;
            scPrefix = 0;
        }
    }
    
    /*if (picRdy && nextPacket && ((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt)
    {
        DEBUG_PRINT(("\nPacket Loss\n"));
        return GetNextDuSize(stream + size, streamStart + size, strmLen - size, skippedBytes);
    }*/
    /*if (picRdy && streamTruncate && ((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt))
    {
        i32 ret;
        DEBUG_PRINT(("Original packet size %d\n", size));
        ret = TBRandomizeU32(&index);
        if(ret != 0)
        {
            DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
            return 0;
        }
        DEBUG_PRINT(("Randomized packet size %d\n", size));
    }*/
    return size;
}

/*------------------------------------------------------------------------------

 Function name:  WriteOutput

  Purpose:
  Write picture pointed by data to file. Size of the
  picture in pixels is indicated by picSize.

------------------------------------------------------------------------------*/
void WriteOutput(char *filename, char *filenameTiled, u8 * data, u32 frameNumber, 
                 u32 width, u32 height, u32 interlaced, u32 top, u32 firstField,
                 u32 tiledMode)
{
    u32 tmp, i, j;
    u32 picSize;
    u8 *p, *ptmp;
    u8 *rasterScan = NULL;
            
    if (!interlacedField && firstField)
        numberOfWrittenFrames++;

    if(disableOutputWriting != 0)
    {
        return;
    }

    picSize = width * height * 3/2;

    DEBUG_PRINT(("picture %d in display order \n", ++displayOrder));
    /* foutput is global file pointer */
    if(foutput == NULL)
    {
    /* open output file for writing, can be disabled with define.
        * If file open fails -> exit */
        if(strcmp(filename, "none") != 0)
        {
            foutput = fopen(filename, "wb");
            if(foutput == NULL)
            {
                DEBUG_PRINT(("UNABLE TO OPEN OUTPUT FILE\n"));
                return;
            }
        }
    }
    
    
    /* Convert back to raster scan format if decoder outputs 
     * tiled format */
    if(tiledMode && convertTiledOutput)
    {
        u32 effHeight = (height + 15) & (~15);
        rasterScan = (u8*)malloc(width*effHeight*3/2);
        if(!rasterScan)
        {
            fprintf(stderr, "error allocating memory for tiled"
                "-->raster conversion!\n");
            return;
        }
                
        TBTiledToRaster( tiledMode, convertToFrameDpb ? dpbMode : DEC_DPB_DEFAULT, 
            data, rasterScan, width, effHeight );
        TBTiledToRaster( tiledMode, convertToFrameDpb ? dpbMode : DEC_DPB_DEFAULT, 
            data+width*effHeight, 
            rasterScan+width*height, width, effHeight/2 );
        data = rasterScan;
    }
    else if(convertToFrameDpb && (dpbMode != DEC_DPB_DEFAULT))
    {
        u32 effHeight = (height + 15) & (~15);
        rasterScan = (u8*)malloc(width*effHeight*3/2);
        if(!rasterScan)
        {
            fprintf(stderr, "error allocating memory for field"
                "-->frame DPB conversion!\n");
            return;
        }
                
        TBFieldDpbToFrameDpb( dpbMode, data, rasterScan, width, effHeight );
        TBFieldDpbToFrameDpb( dpbMode, data+width*effHeight, 
            rasterScan+width*height, width, effHeight/2 );
        data = rasterScan;   
    }

#if 0    
    if (fTiledOutput == NULL && tiledOutput)
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

    if(foutput && data)
    {
        /* field output */
        if (interlaced && interlacedField)
        {
            /* start of top field */
            p = data;
            /* start of bottom field */
            if (!top) 
                p += width;
            if (planarOutput)
            {
                /* luma */
                for (i = 0; i < height/2; i++, p+=(width*2))
                    fwrite(p, 1, width, foutput);
                ptmp = p;
                /* cb */
                for (i = 0; i < height/4; i++, p+=(width*2))
                    for (j = 0; j < width>>1; j++)
                        fwrite(p+j*2, 1, 1, foutput);
                /* cr */
                for (i = 0; i < height/4; i++, ptmp+=(width*2))
                     for (j = 0; j < width>>1; j++)
                        fwrite(ptmp+1+j*2, 1, 1, foutput);
             }
             else
             {
                /* luma */
                for (i = 0; i < height/2; i++, p+=(width*2))
                    fwrite(p, 1, width, foutput);
                /* chroma */
                for (i = 0; i < height/4; i++, p+=(width*2))
                {
                    fwrite(p, 1, width, foutput);
                }
             }
        }
        else /* frame output */
        {
#ifndef ASIC_TRACE_SUPPORT
            u8* picCopy = NULL;
#endif

            if (interlaced && !interlacedField && firstField)
               return;
               
#ifndef ASIC_TRACE_SUPPORT
            if(output_picture_endian == DEC_X170_BIG_ENDIAN)
            {
                picCopy = (u8*) malloc(picSize);
                if (NULL == picCopy)
                {
                    DEBUG_PRINT(("MALLOC FAILED @ %s %d", __FILE__, __LINE__));
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
           TBWriteFrameMD5Sum(foutput, data, picSize, numberOfWrittenFrames - 1);
            
            /* tiled */
            if (tiledOutput)
            {
                /* round up to next multiple of 16 */
                u32 tmpWidth = (width + 15) & ~15;
                u32 tmpHeight = (height + 15) & ~15;
                  
                TbWriteTiledOutput(fTiledOutput, data, tmpWidth >> 4, tmpHeight >> 4, 
                        numberOfWrittenFrames - 1, 1 /* write md5sum */, 1 /* semi-planar data */ );
            }
#else

            /* this presumes output has system endianess */
            if (planarOutput)
            {
                tmp = picSize * 2 / 3;
                fwrite(data, 1, tmp, foutput);
                for (i = 0; i < tmp/4; i++)
                    fwrite(data+tmp+i*2, 1, 1, foutput);
                for (i = 0; i < tmp/4; i++)
                    fwrite(data+tmp+1+i*2, 1, 1, foutput);
            }
            else /* semi-planar */
                fwrite(data, 1, picSize, foutput);
                    
            /* tiled */
            if (tiledOutput)
            {
                /* round up to next multiple of 16 */
                u32 tmpWidth = (width + 15) & ~15;
                u32 tmpHeight = (height + 15) & ~15;
                
                TbWriteTiledOutput(fTiledOutput, data, tmpWidth >> 4, tmpHeight >> 4,
                        numberOfWrittenFrames - 1, 0 /* not write md5sum */, 1 /* semi-planar data */ );
            }
#endif
         
#ifndef ASIC_TRACE_SUPPORT
            if(output_picture_endian == DEC_X170_BIG_ENDIAN)
            {
                free(picCopy);
            }
#endif
        }
    }

    if(rasterScan)
        free(rasterScan);

}

/*------------------------------------------------------------------------------

    Function name: CropPicture

     Purpose:
     Perform cropping for picture. Input picture pInImage with dimensions
     picWidth x picHeight is cropped into outWidth x outHeight and the
     resulting picture is stored in pOutImage.

------------------------------------------------------------------------------*/
u32 CropPicture(u8 *pOutImage, u8 *pInImage,
                u32 picWidth, u32 picHeight, u32 outWidth, u32 outHeight )
{

    u32 i, j;
    u8 *pOut, *pIn;

    if (pOutImage == NULL || pInImage == NULL ||
        !outWidth || !outHeight ||
        !picWidth || !picHeight)
    {
    /* just to prevent lint warning, returning non-zero will result in
        * return without freeing the memory */
        free(pOutImage);
        return(1);
    }

    /* Calculate starting pointer for luma */
    pIn = pInImage;
    pOut = pOutImage;

    /* Copy luma pixel values */
    for (i = outHeight; i; i--)
    {
        for (j = outWidth; j; j--)
        {
            *pOut++ = *pIn++;
        }
        pIn += picWidth - outWidth;
    }

    outWidth >>= 1;
    outHeight >>= 1;

    /* Calculate starting pointer for cb */
    pIn = pInImage + picWidth*picHeight;

    /* Copy chroma pixel values */
    for (i = outHeight; i; i--)
    {
        for (j = outWidth*2; j; j--)
        {
            *pOut++ = *pIn++;
        }
        pIn += picWidth - outWidth*2;
    }

    return (0);
}

/*------------------------------------------------------------------------------

    Function name:  VC1DecTrace

     Purpose:
     Example implementation of H264DecTrace function. Prototype of this
     function is given in H264DecApi.h. This implementation appends
     trace messages to file named 'dec_api.trc'.

------------------------------------------------------------------------------*/
/*void VC1DecTrace(const char *string)
{
    FILE *fp;

    fp = fopen("dec_api.trc", "at");

    if(!fp)
        return;

    fwrite(string, 1, strlen(string), fp);
    fwrite("\n", 1, 1, fp);

    fclose(fp);
}*/

#define SHOW1(p) (p[0]); p+=1;
#define SHOW2(p) (p[0]) | (p[1]<<8); p+=2;
#define SHOW3(p) (p[0]) | (p[1]<<8) | (p[2]<<16); p+=3;
#define SHOW4(p) (p[0]) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24); p+=4;

#define    BIT0(tmp)  ((tmp & 1)   >>0);
#define    BIT1(tmp)  ((tmp & 2)   >>1);
#define    BIT2(tmp)  ((tmp & 4)   >>2);
#define    BIT3(tmp)  ((tmp & 8)   >>3);
#define    BIT4(tmp)  ((tmp & 16)  >>4);
#define    BIT5(tmp)  ((tmp & 32)  >>5);
#define    BIT6(tmp)  ((tmp & 64)  >>6);
#define    BIT7(tmp)  ((tmp & 128) >>7);

/*------------------------------------------------------------------------------

    Function name:  DecodeFrameLayerData

     Purpose:
     Decodes initialization frame layer from rcv format.

      Returns:
      Frame size in bytes.

------------------------------------------------------------------------------*/
u32 DecodeFrameLayerData(u8 *stream)
{
    u32 tmp = 0;
    u32 timeStamp = 0;
    u32 frameSize = 0;
    u8 *p = stream;

    frameSize = SHOW3(p);
    tmp = SHOW1(p);
    tmp = BIT7(tmp);
    if( rcvV2 )
    {
        timeStamp = SHOW4(p);
        if (tmp == 1)
            DEBUG_PRINT(("INTRA FRAME timestamp: %d size: %d\n",
            timeStamp, frameSize));
        else
            DEBUG_PRINT(("INTER FRAME timestamp: %d size: %d\n",
            timeStamp, frameSize));
    }
    else
    {
        if (tmp == 1)
            DEBUG_PRINT(("INTRA FRAME size: %d\n", frameSize));
        else
            DEBUG_PRINT(("INTER FRAME size: %d\n", frameSize));
    }

    return frameSize;
}

/*------------------------------------------------------------------------------

    Function name:  DecodeRCV

     Purpose:
     Decodes initialization metadata from rcv format.

------------------------------------------------------------------------------*/
i32 DecodeRCV(u8 *stream, u32 strmLen, VC1DecMetaData *metaData)
{
    u32 tmp1 = 0;
    u32 tmp2 = 0;
    u32 tmp3 = 0;
    u32 profile = 0;
    u8 *p;
    p = stream;

    if (strmLen < 9*4+8)
        return -1;

    tmp1 = SHOW3(p);
    DEBUG_PRINT(("\nNUMFRAMES: \t %d\n", tmp1));
    tmp1 = SHOW1(p);
    DEBUG_PRINT(("0xC5: \t\t %0X\n", tmp1));
    if( tmp1 & 0x40 )   rcvV2 = 1;
    else                rcvV2 = 0;
    DEBUG_PRINT(("RCV_VERSION: \t\t %d\n", rcvV2+1 ));

    rcvMetadataSize = SHOW4(p);
    DEBUG_PRINT(("0x04: \t\t %0X\n", rcvMetadataSize));

    /* Decode image dimensions */
    p += rcvMetadataSize;
    tmp1 = SHOW4(p);
    metaData->maxCodedHeight = tmp1;
    tmp1 = SHOW4(p);
    metaData->maxCodedWidth = tmp1;

    return 0;
}

/*------------------------------------------------------------------------------

    FramePicture

        Create frame of max-coded-width*max-coded-height around output image.
        Useful for system model verification, this way we can directly compare
        our output to the DecRef_* output generated by the reference decoder..

------------------------------------------------------------------------------*/
void FramePicture( u8 *pIn, i32 inWidth, i32 inHeight,
                   i32 inFrameWidth, i32 inFrameHeight,
                   u8 *pOut, i32 outWidth, i32 outHeight )
{

/* Variables */

    i32 x, y;

/* Code */

    memset( pOut, 0, outWidth*outHeight*3/2 );

    /* Luma */
    pOut += outWidth * ( outHeight - inHeight );
    for ( y = 0 ; y < inHeight ; ++y )
    {
        pOut += ( outWidth - inWidth );
        for( x = 0 ; x < inWidth; ++x )
            *pOut++ = *pIn++;
        pIn += ( inFrameWidth - inWidth );
    }

    pIn += inFrameWidth * ( inFrameHeight - inHeight );

    inFrameHeight /= 2;
    inFrameWidth /= 2;
    outHeight /= 2;
    outWidth /= 2;
    inHeight /= 2;
    inWidth /= 2;

    /* Chroma */
    pOut += 2 * outWidth * ( outHeight - inHeight );
    for ( y = 0 ; y < inHeight ; ++y )
    {
        pOut += 2 * ( outWidth - inWidth );
        for( x = 0 ; x < 2*inWidth; ++x )
            *pOut++ = *pIn++;
        pIn += 2 * ( inFrameWidth - inWidth );
    }

}

void vc1DecPrintReturnValue(VC1DecRet ret)
{
    switch(ret){
        case VC1DEC_OK:
            DEBUG_PRINT(("VC1DEC_OK\n"));
            break;

        case VC1DEC_HDRS_RDY:
            DEBUG_PRINT(("VC1DEC_HDRS_RDY\n"));
            break;

        case VC1DEC_PIC_RDY:
            DEBUG_PRINT(("VC1DEC_PIC_RDY\n"));
            break;

        case VC1DEC_END_OF_SEQ:
            DEBUG_PRINT(("VC1DEC_END_OF_SEQ\n"));
            break;

        case VC1DEC_PIC_DECODED:
            DEBUG_PRINT(("VC1DEC_PIC_DECODED\n"));
            break;
        
        case VC1DEC_RESOLUTION_CHANGED:
            DEBUG_PRINT(("VC1DEC_RESOLUTION_CHANGED\n"));
            break;

        case VC1DEC_STRM_ERROR:
            DEBUG_PRINT(("VC1DEC_STRM_ERROR\n"));
            break;

        case VC1DEC_STRM_PROCESSED:
            DEBUG_PRINT(("VC1DEC_STRM_PROCESSED\n"));
            break;

        case VC1DEC_PARAM_ERROR:
            DEBUG_PRINT(("VC1DEC_PARAM_ERROR\n"));
            break;

        case VC1DEC_NOT_INITIALIZED:
            DEBUG_PRINT(("VC1DEC_NOT_INITIALIZED\n"));
            break;

        case VC1DEC_MEMFAIL:
            DEBUG_PRINT(("VC1DEC_MEMFAIL\n"));
            break;

        case VC1DEC_INITFAIL:
            DEBUG_PRINT(("VC1DEC_INITFAIL\n"));
            break;

        case VC1DEC_METADATA_FAIL:
            DEBUG_PRINT(("VC1DEC_METADATA_FAIL\n"));
            break;

        case VC1DEC_HW_RESERVED:
            DEBUG_PRINT(("VC1DEC_HW_RESERVED\n"));
            break;

        case VC1DEC_HW_TIMEOUT:
            DEBUG_PRINT(("VC1DEC_HW_TIMEOUT\n"));
            break;

        case VC1DEC_HW_BUS_ERROR:
            DEBUG_PRINT(("VC1DEC_HW_BUS_ERROR\n"));
            break;

        case VC1DEC_SYSTEM_ERROR:
            DEBUG_PRINT(("VC1DEC_SYSTEM_ERROR\n"));
            break;

        case VC1DEC_DWL_ERROR:
            DEBUG_PRINT(("VC1DEC_DWL_ERROR\n"));
            break;

        case VC1DEC_NONREF_PIC_SKIPPED:
            DEBUG_PRINT(("VC1DEC_NONREF_PIC_SKIPPED\n"));
            break;

        default:

            DEBUG_PRINT(("unknown return value!\n"));
    }
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

/*------------------------------------------------------------------------------

    Function name: fillBuffer

------------------------------------------------------------------------------*/
u32 fillBuffer(u8 *stream)
{
    u32 amount = 0;
    u32 dataLen = 0;
    off64_t pos = ftello64(finput);
    
    if(curIndex != pos)
    {
        fseeko64(finput, curIndex, SEEK_SET);
    }

    fscanf(fIndex, "%llu", &nextIndex);
    amount += nextIndex - curIndex;
    curIndex = nextIndex;
    
    /* read data */
    dataLen = fread(stream, 1, amount, finput);

    return dataLen;
}

