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
-
-  Description : ...
-
------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <time.h>

#include "vp6decapi.h"

#ifdef PP_PIPELINE_ENABLED
#include "ppapi.h"
#include "pptestbench.h"
#endif

#include "tb_cfg.h"
#include "tb_tiled.h"
#include "vp6hwd_container.h"

#include "regdrv.h"

#ifdef MD5SUM
#include "md5.h"
#endif

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

/* Size of stream buffer */
#define STREAMBUFFER_BLOCKSIZE 2*2097151
DWLLinearMem_t streamMem;

u32 asicTraceEnabled; /* control flag for trace generation */
#ifdef ASIC_TRACE_SUPPORT
extern u32 asicTraceEnabled; /* control flag for trace generation */
/* Stuff to enable ref buffer model support */
#include "../../../system/models/golden/ref_bufferd.h"
/* Ref buffer support stuff ends */
#endif

#ifdef VP6_EVALUATION
/* Stuff to enable ref buffer model support */
#include "../../../system/models/golden/ref_bufferd.h"
/* Ref buffer support stuff ends */
#endif

#ifdef VP6_EVALUATION
extern u32 gHwVer;
#endif

/* for tracing */
u32 bFrames=0;


#define DEBUG_PRINT(...) printf(__VA_ARGS__)

#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 picNumber, VP6DecPicture  DecPicture,
                    VP6DecInst decoder);
#endif

const char *const short_options = "HO:N:mMtPAB:EG";

u32 inputStreamBusAddress = 0;

u32 clock_gating = DEC_X170_INTERNAL_CLOCK_GATING;
u32 data_discard = DEC_X170_DATA_DISCARD_ENABLE;
u32 latency_comp = DEC_X170_LATENCY_COMPENSATION;
u32 output_picture_endian = DEC_X170_OUTPUT_PICTURE_ENDIAN;
u32 bus_burst_length = DEC_X170_BUS_BURST_LENGTH;
u32 asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
u32 output_format = DEC_X170_OUTPUT_FORMAT;

u32 pic_big_endian_size = 0;
u8* pic_big_endian = NULL;

u32 seedRnd = 0;
u32 streamBitSwap = 0;
i32 corruptedBytes = 0;  /*  */
u32 streamTruncate = 0;
u32 streamPacketLoss = 0;
u32 streamHeaderCorrupt = 0;
u32 hdrsRdy = 0;
u32 picRdy = 0;
u32 numBuffers = 3;
u32 tiledOutput = DEC_REF_FRM_RASTER_SCAN;
u32 convertTiledOutput = 0;

u32 usePeekOutput = 0;

const struct option long_options[] = {
    {"help", 0, NULL, 'H'},
    {"output", 1, NULL, 'O'},
    {"lastPic", 1, NULL, 'N'},
    {"md5-partial", 0, NULL, 'm'},
    {"md5-total", 0, NULL, 'M'},
    {"trace", 0, NULL, 't'},
    {"planar", 0, NULL, 'P'},
    {"alpha", 0, NULL, 'A'},
    {"buffers", 1, NULL, 'B'},
    {"tiled-mode", 0, NULL, 'E'},
    {"convert-tiled", 0, NULL, 'G'},
    {NULL, 0, NULL, 0}
};

typedef struct
{
    const char *input;
    const char *output;
    const char *ppCfg;
    int lastFrame;
    int md5;
    int planar;
    int alpha;
    int f,d,s;
} options_s;

options_s options;

TBCfg tbCfg;

void print_usage(const char *prog)
{
    fprintf(stdout, "Usage:\n%s [options] <stream.vp6>\n\n", prog);

    fprintf(stdout, "    -H       --help              Print this help.\n");

    fprintf(stdout,
            "    -O[file] --output            Write output to specified file. [out.yuv]\n");

    fprintf(stdout,
            "    -N[n]    --lastFrame         Forces decoding to stop after n frames; -1 sets no limit.[-1]\n");
    fprintf(stdout,
            "    -m       --md5-partial       Write MD5 checksum for each picture (YUV not written).\n");
    fprintf(stdout,
            "    -M       --md5-total         Write MD5 checksum of whole sequence (YUV not written).\n");
    fprintf(stdout,
            "    -P       --planar            Write planar output\n");
    fprintf(stdout,
            "    -E       --tiled-mode        Signal decoder to use tiled reference frame format\n");
    fprintf(stdout,
            "    -G       --convert-tiled     Convert tiled mode output pictures into raster scan format\n");
    fprintf(stdout,
            "    -A       --alpha             Stream contains alpha channelt\n");
    fprintf(stdout,
            "    -Bn                          Use n frame buffers in decoder\n");
    fprintf(stdout,
            "    -Z                           Output pictures using VP6DecPeek() function\n");

    fprintf(stdout, "\n");
}

int getoptions(int argc, char **argv, options_s * opts)
{
    int next_opt;

#ifdef ASIC_TRACE_SUPPORT
    asicTraceEnabled = 0;
#endif /* ASIC_TRACE_SUPPORT */

    do
    {
        next_opt = getopt_long(argc, argv, short_options, long_options, NULL);
        if(next_opt == -1)
            return 0;

        switch (next_opt)
        {
        case 'H':
            print_usage(argv[0]);
            exit(0);
        case 'O':
            opts->output = optarg;
            break;
        case 'N':
            opts->lastFrame = atoi(optarg);
            break;
        case 'm':
            opts->md5 = 1;
            break;
        case 'M':
            opts->md5 = 2;
            break;
        case 'P':
            opts->planar = 1;
            break;
        case 'E':
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
            break;
        case 'G':
            convertTiledOutput = 1;
            break;
        case 'B':
            numBuffers = atoi(optarg);
            break;
        case 'A':
            opts->alpha = 1;
            break;
        case 't':
#ifdef ASIC_TRACE_SUPPORT
            asicTraceEnabled = 2;
#else
            fprintf(stdout, "\nWARNING! Trace generation not supported!\n");
#endif
            break;

        case 'Z':
            usePeekOutput = 1;
            break;

        default:
            fprintf(stderr, "Unknown option\n");
        }
    }
    while(1);
}

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#define leRushort( P, V) { \
	register ushort v = (uchar) *P++; \
	(V) = v + ( (ushort) (uchar) *P++ << 8); \
}
#define leRushortF( F, V) { \
	char b[2], *p = b; \
	fread( (void *) b, 1, 2, F); \
	leRushort( p, V); \
}

#define leRulong( P, V) { \
	register ulong v = (uchar) *P++; \
	v += (ulong) (uchar) *P++ << 8; \
	v += (ulong) (uchar) *P++ << 16; \
	(V) = v + ( (ulong) (uchar) *P++ << 24); \
}
#define leRulongF( F, V) { \
	char b[4] = {0,0,0,0}, *p = b; \
    V = 0; \
	fread( (void *) b, 1, 4, F); \
	leRulong( p, V); \
}

#if 0
#	define leWchar( P, V)  { * ( (char *) P)++ = (char) (V);}
#else
#	define leWchar( P, V)  { *P++ = (char) (V);}
#endif

#define leWshort( P, V)  { \
	register short v = (V); \
	leWchar( P, v) \
	leWchar( P, v >> 8) \
}
#define leWshortF( F, V) { \
	char b[2], *p = b; \
	leWshort( p, V); \
	fwrite( (void *) b, 1, 2, F); \
}

#define leWlong( P, V)  { \
	register long v = (V); \
	leWchar( P, v) \
	leWchar( P, v >> 8) \
	leWchar( P, v >> 16) \
	leWchar( P, v >> 24) \
}
#define leWlongF( F, V)  { \
	char b[4], *p = b; \
	leWlong( p, V); \
	fwrite( (void *) b, 1, 4, F); \
}

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

int readCompressedFrame(FILE * fp)
{
    ulong frameSize = 0;

    leRulongF(fp, frameSize);

    if( frameSize >= STREAMBUFFER_BLOCKSIZE )
    {
        /* too big a frame */
        return 0;
    }

    fread(streamMem.virtualAddress, 1, frameSize, fp);

    return (int) frameSize;
}

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

void writeRawFrame(FILE * fp, unsigned char *buffer, int frameSize, int md5,
    int planar, int width, int height, int tiledMode)
{
    u8 *rasterScan = NULL;
#ifdef MD5SUM
    static struct MD5Context ctx;
#endif

    /* Convert back to raster scan format if decoder outputs 
     * tiled format */
    if(tiledMode && convertTiledOutput)
    {
        rasterScan = (u8*)malloc(frameSize);
        if(!rasterScan)
        {
            fprintf(stderr, "error allocating memory for tiled"
                "-->raster conversion!\n");
            return;
        }

        TBTiledToRaster( tiledMode, DEC_DPB_DEFAULT, buffer, 
            rasterScan, width, height );
        TBTiledToRaster( tiledMode, DEC_DPB_DEFAULT, buffer+width*height, 
            rasterScan+width*height, width, height/2 );
        buffer = rasterScan;
    }

#ifndef ASIC_TRACE_SUPPORT
    if(output_picture_endian == DEC_X170_BIG_ENDIAN)
    {
        if(pic_big_endian_size != frameSize)
        {
            if(pic_big_endian != NULL)
                free(pic_big_endian);

            pic_big_endian = (u8 *) malloc(frameSize);
            if(pic_big_endian == NULL)
            {
                DEBUG_PRINT("MALLOC FAILED @ %s %d", __FILE__, __LINE__);
                if(rasterScan)
                    free(rasterScan);
                return;
            }
            
            pic_big_endian_size = frameSize;
        }

        memcpy(pic_big_endian, buffer, frameSize);
        TbChangeEndianess(pic_big_endian, frameSize);
        buffer = pic_big_endian;
    }
#endif
#ifdef MD5SUM
    if(md5 == 1)
    {
        unsigned char digest[16];
        int i = 0;

        MD5Init(&ctx);
        MD5Update(&ctx, buffer, frameSize);
        MD5Final(digest, &ctx);

        for(i = 0; i < sizeof digest; i++)
        {
            fprintf(fp, "%02X", digest[i]);
        }
        fprintf(fp, "\n");
        if(rasterScan)
            free(rasterScan);
        return;
    }
    else
    {
    }
#endif

    if (!planar)
        fwrite(buffer, frameSize, 1, fp);
    else
    {
        u32 i, tmp;
        tmp = frameSize * 2 / 3;
        fwrite(buffer, 1, tmp, fp);
        for(i = 0; i < tmp / 4; i++)
            fwrite(buffer + tmp + i * 2, 1, 1, fp);
        for(i = 0; i < tmp / 4; i++)
            fwrite(buffer + tmp + 1 + i * 2, 1, 1, fp);

/*
        u32 i, tmp;
        tmp = frameSize*2/3;
        fwrite(buffer, tmp, 1, fp);
        buffer += tmp;
        tmp /= 4;
        for (i = 0; i < tmp; i++)
        {
            fwrite(buffer+i, 1, 1, fp);
            fwrite(buffer+tmp+i, 1, 1, fp);
        }
*/
    }

    if(rasterScan)
        free(rasterScan);

}

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

int decode_file(const options_s * opts)
{
    VP6DecInst decInst;
    VP6DecInput input;
    VP6DecOutput output;
    VP6DecPicture decPic, tmpPic;
    VP6DecRet ret;
    u32 tmp;
    unsigned int currentVideoFrame = 0;

    FILE *inputFile = fopen(opts->input, "rb");
    FILE *outFile = fopen(opts->output, "wb");

    if(inputFile == NULL)
    {
        perror(opts->input);
        return -1;
    }
    if(outFile == NULL)
    {
        perror(opts->output);
        return -1;
    }

#ifdef ASIC_TRACE_SUPPORT
    tmp = openTraceFiles();
    if (!tmp)
    {
        DEBUG_PRINT(("UNABLE TO OPEN TRACE FILE(S)\n"));
    }
#endif

    ret = VP6DecInit(&decInst,
               TBGetDecIntraFreezeEnable( &tbCfg ),
               numBuffers,
               tiledOutput);
    if (ret != VP6DEC_OK)
    {
        printf("DECODER INITIALIZATION FAILED\n");
        goto end;
    }

    /* Set ref buffer test mode */
    ((VP6DecContainer_t *) decInst)->refBufferCtrl.testFunction = TBRefbuTestMode;

#ifdef PP_PIPELINE_ENABLED
    /* Initialize the post processer. If unsuccessful -> exit */
    if(pp_startup
       ((char*)options.ppCfg, decInst, PP_PIPELINED_DEC_TYPE_VP6, &tbCfg) != 0)
    {
        fprintf(stdout, "PP INITIALIZATION FAILED\n");
        goto end;
    }

    if(pp_update_config (decInst, PP_PIPELINED_DEC_TYPE_VP6, &tbCfg) ==
        CFG_UPDATE_FAIL)
    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
        goto end;
    }
#endif

    SetDecRegister(((VP6DecContainer_t *) decInst)->vp6Regs, HWIF_DEC_LATENCY,
                   latency_comp);
    SetDecRegister(((VP6DecContainer_t *) decInst)->vp6Regs, HWIF_DEC_CLK_GATE_E,
                   clock_gating);
    /*
    SetDecRegister(((VP6DecContainer_t *) decInst)->vp6Regs, HWIF_DEC_OUT_TILED_E,
                   output_format);
                   */
    SetDecRegister(((VP6DecContainer_t *) decInst)->vp6Regs, HWIF_DEC_OUT_ENDIAN,
                   output_picture_endian);
    SetDecRegister(((VP6DecContainer_t *) decInst)->vp6Regs, HWIF_DEC_MAX_BURST,
                   bus_burst_length);
    if ((DWLReadAsicID() >> 16) == 0x8170U)
    {
        SetDecRegister(((VP6DecContainer_t *) decInst)->vp6Regs, HWIF_PRIORITY_MODE,
                       asic_service_priority);
    }
    SetDecRegister(((VP6DecContainer_t *) decInst)->vp6Regs, HWIF_DEC_DATA_DISC_E,
                   data_discard);

    /* allocate memory for stream buffer. if unsuccessful -> exit */
    streamMem.virtualAddress = NULL;
    streamMem.busAddress = 0;

    if(DWLMallocLinear(((VP6DecContainer_t *) decInst)->dwl,
                       STREAMBUFFER_BLOCKSIZE, &streamMem) != DWL_OK)
    {
        printf(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
        exit(-1);
    }

    while(!feof(inputFile) &&
          currentVideoFrame < (unsigned int) opts->lastFrame)
    {
        fflush(stdout);

        input.dataLen = readCompressedFrame(inputFile);
        input.pStream = (u8*)streamMem.virtualAddress;
        input.streamBusAddress = (u32)streamMem.busAddress;

        if(input.dataLen == 0)
            break;

        if( opts->alpha )
        {
            if( input.dataLen < 3 )
                break;
            input.dataLen -= 3;
            input.pStream += 3;
            input.streamBusAddress += 3;
        }

        if(streamTruncate && picRdy && (hdrsRdy || streamHeaderCorrupt))
        {
            i32 ret;

            ret = TBRandomizeU32(&input.dataLen);
            if(ret != 0)
            {
                DEBUG_PRINT("RANDOM STREAM ERROR FAILED\n");
                return 0;
            }
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
                        TBRandomizeBitSwapInStream(input.pStream,
                                                   input.dataLen,
                                                   tbCfg.tbParams.
                                                   streamBitSwap);
                    if(ret != 0)
                    {
                        DEBUG_PRINT("RANDOM STREAM ERROR FAILED\n");
                        goto end;
                    }

                    corruptedBytes = input.dataLen;
                }
            }
        }

        do
        {
            ret = VP6DecDecode(decInst, &input, &output);
            /* printf("VP6DecDecode retruned: %d\n", ret); */

            if(ret == VP6DEC_HDRS_RDY)
            {
                VP6DecInfo decInfo;

                TBSetRefbuMemModel( &tbCfg, 
                    ((VP6DecContainer_t *) decInst)->vp6Regs,
                    &((VP6DecContainer_t *) decInst)->refBufferCtrl );

                hdrsRdy = 1;

                ret = VP6DecGetInfo(decInst, &decInfo);
                if (ret != VP6DEC_OK)
                {
                    DEBUG_PRINT("ERROR in getting stream info!\n");
                    goto end;
                }

                fprintf(stdout, "\n"
                        "Stream info: vp6Version  = 6.%d\n"
                        "             vp6Profile  = %s\n"
                        "             coded size  = %d x %d\n"
                        "             scaled size = %d x %d\n",
                        decInfo.vp6Version - 6,
                        decInfo.vp6Profile == 0 ? "SIMPLE" : "ADVANCED",
                        decInfo.frameWidth, decInfo.frameHeight,
                        decInfo.scaledWidth, decInfo.scaledHeight);

                ret = VP6DEC_HDRS_RDY;  /* restore */
                fprintf(stdout, "\nPicture      ");
            }

        }
        while(ret == VP6DEC_HDRS_RDY);

        if(ret == VP6DEC_PIC_DECODED)
        {
            const char ss[4] = { '-', '\\', '|', '/' };

            picRdy = 1;

            if (usePeekOutput)
                ret = VP6DecPeek(decInst, &decPic);
            else
                ret = VP6DecNextPicture(decInst, &decPic, 0);
            if (ret == VP6DEC_PIC_RDY)
            {
#ifndef PP_PIPELINE_ENABLED
                writeRawFrame(outFile, (unsigned char *) decPic.pOutputFrame,
                              decPic.frameWidth * decPic.frameHeight * 3 / 2,
                              opts->md5, opts->planar,
                              decPic.frameWidth, decPic.frameHeight, 
                              decPic.outputFormat);
#else
                HandlePpOutput(currentVideoFrame, decPic, decInst);
#endif
            }
            if (usePeekOutput)
                ret = VP6DecNextPicture(decInst, &tmpPic, 0);

            currentVideoFrame++;

            fprintf(stdout, "\b\b\b\b\b%4d ", currentVideoFrame);

        }

        corruptedBytes = 0;
    }

    /* last picture from the buffer */
    if (!usePeekOutput)
    {
        ret = VP6DecNextPicture(decInst, &decPic, 1);
        if (ret == VP6DEC_PIC_RDY)
        {
#ifndef PP_PIPELINE_ENABLED
            writeRawFrame(outFile, (unsigned char *) decPic.pOutputFrame,
                          decPic.frameWidth * decPic.frameHeight * 3 / 2,
                          opts->md5, opts->planar,
                          decPic.frameWidth, decPic.frameHeight, 
                          decPic.outputFormat);
#else
            HandlePpOutput(currentVideoFrame, decPic, decInst);
#endif
        }
    }

end:

    printf("\rPictures decoded: %4d\n\n", currentVideoFrame);

#ifdef PP_PIPELINE_ENABLED
    pp_close();
#endif

    if(streamMem.virtualAddress)
        DWLFreeLinear(((VP6DecContainer_t *) decInst)->dwl, &streamMem);
    
    VP6DecRelease(decInst);

    fclose(inputFile);
    fclose(outFile);
    if (pic_big_endian != NULL)
        free(pic_big_endian);

    return 0;
}

int decode_test(const options_s * opts)
{
    int ret;

    printf("Starting Decode\n\n");

    ret = decode_file(&options);

    printf("\nDecoding Complete.\n\n");

    return ret;
}

int main(int argc, char *argv[])
{
    int i, ret;
    FILE *fTBCfg;
    const char default_out[] = "out.yuv";
    const char default_pp_out[] = "pp_out.yuv";

#ifdef PP_PIPELINE_ENABLED
    PPApiVersion ppVer;
    PPBuild ppBuild;
#endif

#ifdef VP6_EVALUATION_9190
    gHwVer = 9190;
#elif VP6_EVALUATION_G1
    gHwVer = 10000;
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

#ifndef PP_PIPELINE_ENABLED
    if(argc < 2)
    {
        print_usage(argv[0]);
        exit(0);
    }

    options.output = default_out;
    options.input = argv[argc - 1];
    options.md5 = 0;
    options.lastFrame = -1;
    options.planar = 0;

    getoptions(argc - 1, argv, &options);
#else
    if(argc < 3)
    {
        printf("\nVP6 Decoder PP Pipelined Testbench\n\n");
        printf("USAGE:\n%s [options] stream.vp6 pp.cfg\n", argv[0]);
        printf("-Nn to decode only first n vops of the stream\n");
        printf("-A  stream contains alpha channel\n");
        exit(100);
    }

    options.output = default_pp_out;
    options.input = argv[argc - 2];
    options.ppCfg = argv[argc - 1];
    options.md5 = 0;
    options.lastFrame = -1;
    options.planar = 0;
    options.alpha = 0;
    /* read cmdl parameters */
    for(i = 1; i < argc - 2; i++)
    {
        if(strncmp(argv[i], "-N", 2) == 0)
        {
            options.lastFrame = atoi(argv[i] + 2);
        }
        else if(strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if(strncmp(argv[i], "-A", 2) == 0)
        {
            options.alpha = 1;
        }
        else
        {
            fprintf(stdout, "UNKNOWN PARAMETER: %s\n", argv[i]);
            return 1;
        }
    }

    /* Print API and build version numbers */
    ppVer = PPGetAPIVersion();
    ppBuild = PPGetBuild();

    /* Version */
    fprintf(stdout,
            "\nX170 PP API v%d.%d - SW build: %d - HW build: %x\n",
            ppVer.major, ppVer.minor, ppBuild.swBuild, ppBuild.hwBuild);
#endif

    /* check if traces shall be enabled */
#ifdef ASIC_TRACE_SUPPORT
    {
        char traceString[80];
        FILE *fid = fopen("trace.cfg", "r");
        if (fid)
        {
            /* all tracing enabled if any of the recognized keywords found */
            while(fscanf(fid, "%s\n", traceString) != EOF)
            {
                if (!strcmp(traceString, "toplevel") ||
                    !strcmp(traceString, "all"))
                    asicTraceEnabled = 2;
                else if(!strcmp(traceString, "fpga") ||
                        !strcmp(traceString, "decoding_tools"))
                    if(asicTraceEnabled == 0)
                        asicTraceEnabled = 1;
            }
        }
    }
#endif

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

#ifdef ASIC_TRACE_SUPPORT

/*
    {
        extern refBufferd_t    vp6Refbuffer;
        refBufferd_Reset(&vp6Refbuffer);
    }
    */

#endif /* ASIC_TRACE_SUPPORT */

#ifdef VP6_EVALUATION

/*
    {
        extern refBufferd_t    vp6Refbuffer;
        refBufferd_Reset(&vp6Refbuffer);
    }
    */

#endif /* VP6_EVALUATION */

     clock_gating = TBGetDecClockGating(&tbCfg);
     data_discard = TBGetDecDataDiscard(&tbCfg);
     latency_comp = tbCfg.decParams.latencyCompensation;
     output_picture_endian = TBGetDecOutputPictureEndian(&tbCfg);
     bus_burst_length = tbCfg.decParams.busBurstLength;
     asic_service_priority = tbCfg.decParams.asicServicePriority;
     output_format = TBGetDecOutputFormat(&tbCfg);

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

    TBInitializeRandom(seedRnd);

    {
        VP6DecApiVersion decApi;
        VP6DecBuild decBuild;

        /* Print API version number */
        decApi = VP6DecGetAPIVersion();
        decBuild = VP6DecGetBuild();
        DEBUG_PRINT
            ("\n8190 VP6 Decoder API v%d.%d - SW build: %d - HW build: %x\n\n",
             decApi.major, decApi.minor, decBuild.swBuild, decBuild.hwBuild);
    }

    ret = decode_test(&options);

#ifdef ASIC_TRACE_SUPPORT
    {
        extern u32 hwDecPicCount;
        trace_SequenceCtrl( hwDecPicCount );
    }
#endif /* ASIC_TRACE_SUPPORT */

    return ret;
}

#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 vopNumber, VP6DecPicture DecPicture, VP6DecInst decoder)
{
    PPResult res;

    res = pp_check_combined_status();

    if(res == PP_OK)
    {
        pp_write_output(vopNumber, 0, 0);
        pp_read_blend_components(((VP6DecContainer_t *) decoder)->pp.ppInstance);
    }
    if(pp_update_config
       (decoder, PP_PIPELINED_DEC_TYPE_VP6, &tbCfg) == CFG_UPDATE_FAIL)

    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
    }

}
#endif

