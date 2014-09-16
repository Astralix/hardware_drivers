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
--  Abstract : RV decoder testbench
--
------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>

#include "rvdecapi.h"

#ifdef PP_PIPELINE_ENABLED
#include "ppapi.h"
#include "pptestbench.h"
#endif

#include "rv_container.h"

#include "rm_parse.h"
#include "rv_depack.h"
#include "rv_decode.h"

#include "hantro_rv_test.h"

#include "tb_cfg.h"
#include "tb_tiled.h"

#include "regdrv.h"

#ifdef ASIC_TRACE_SUPPORT
#include "trace.h"
extern u32 hwDecPicCount;
extern u32 gHwVer;
#endif

#ifdef MD5SUM
#include "tb_md5.h"
#endif

#include "tb_sw_performance.h"

#define RM2YUV_INITIAL_READ_SIZE	16

#define MB_MULTIPLE(x)  (((x)+15)&~15)

u32 maxCodedWidth = 0;
u32 maxCodedHeight = 0;
u32 maxFrameWidth = 0;
u32 maxFrameHeight = 0;
u32 enableFramePicture = 1;
u8 *frameBuffer;
FILE *frameOut = NULL;
char outFileName[256] = "";
char outFileNameTiled[256] = "out_tiled.yuv";
u32 disableOutputWriting = 0;
u32 planarOutput = 0;
u32 frameNumber = 0;
u32 numberOfWrittenFrames = 0;
FILE *fTiledOutput = NULL;
u32 numFrameBuffers = 0;
u32 tiledOutput = DEC_REF_FRM_RASTER_SCAN;
u32 convertTiledOutput = 0;

void FramePicture( u8 *pIn, i32 inWidth, i32 inHeight,
                   i32 inFrameWidth, i32 inFrameHeight,
                   u8 *pOut, i32 outWidth, i32 outHeight );

u32 rvInput = 0;

u32 usePeekOutput = 0;
u32 skipNonReference = 0;

void decsw_performance(void)
{
}


#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 frameNumber, RvDecPicture DecPicture,
                    RvDecInst decoder);
#endif

#ifdef RV_TIME_TRACE
	#include "../timer/timer.h"
	#define TIMER_MAX_COUNTER	0xFFFFFFFF
	#define TIMER_FACTOR_MSECOND 1000
	u32 ulStartTime_DEC, ulStopTime_DEC;	/* HW decode time (us) */
	u32 ulStartTime_LCD, ulStopTime_LCD;	/* display time (us) */
	u32 ulStartTime_Full, ulStopTime_Full;	/* total time (us), including parser(sw), driver(sw) & decoder(hw) */
	double fMaxTime_DEC, fTime_DEC, fTotalTime_DEC;
	double fMaxTime_LCD, fTime_LCD, fTotalTime_LCD;
	double fMaxTime_Full, fTime_Full, fTotalTime_Full;
	char strTimeInfo[0x100];
#endif

#ifdef RV_DISPLAY
	#include "../../video/v830videoType.h"
	#include "../../video/highapi/hbridge.h"
	#include "../../video/driver/dipp.h"
	#include "../../video/panel/panel.h"

	static TSize g_srcimg;
	static TSize g_dstimg;
	static u32 g_bufmode = LBUF_TWOBUF;
	static u32 g_delayTime = 0;
#endif

#ifdef RV_ERROR_SIM
	#include "../../tools/random.h"
	static u32 g_randomerror = 0;
#endif

#define DWLFile FILE

#define DWL_SEEK_CUR    SEEK_CUR
#define DWL_SEEK_END    SEEK_END
#define DWL_SEEK_SET    SEEK_SET

#define DWLfopen(filename,mode)                 fopen(filename, mode)
#define DWLfread(buf,size,_size_t,filehandle)   fread(buf,size,_size_t,filehandle)
#define DWLfwrite(buf,size,_size_t,filehandle)  fwrite(buf,size,_size_t,filehandle)
#define DWLftell(filehandle)                    ftell(filehandle)
#define DWLfseek(filehandle,offset,whence)      fseek(filehandle,offset,whence)
#define DWLfrewind(filehandle)                  rewind(filehandle)
#define DWLfclose(filehandle)                   fclose(filehandle)
#define DWLfeof(filehandle)                     feof(filehandle)

#define DEBUG_PRINT(msg)    printf msg
#define ERROR_PRINT(msg)
#undef ASSERT
#define ASSERT(s)
u32 bFrames = 0;


TBCfg tbCfg;
u32 clock_gating = DEC_X170_INTERNAL_CLOCK_GATING;
u32 data_discard = DEC_X170_DATA_DISCARD_ENABLE;
u32 latency_comp = DEC_X170_LATENCY_COMPENSATION;
u32 output_picture_endian = DEC_X170_OUTPUT_PICTURE_ENDIAN;
u32 bus_burst_length = DEC_X170_BUS_BURST_LENGTH;
u32 asic_service_priority = DEC_X170_ASIC_SERVICE_PRIORITY;
u32 output_format = DEC_X170_OUTPUT_FORMAT;

u32 pic_big_endian_size = 0;
u8* pic_big_endian = NULL;

u32 streamTruncate = 0;
u32 streamPacketLoss = 0;
u32 streamHeaderCorrupt = 0;
u32 hdrsRdy = 0;

u32 seedRnd = 0;
u32 streamBitSwap = 0;
i32 corruptedBytes = 0;  /*  */
u32 picRdy = 0;
u32 packetize = 0;


static u32 g_iswritefile = 0;
static u32 g_isdisplay = 0;
static u32 g_displaymode = 0;
static u32 g_bfirstdisplay = 0;
static u32 g_rotationmode = 0;
static u32 g_blayerenable = 0;
static u32 g_islog2std = 0;
static u32 g_islog2file = 0;
static DWLFile *g_fpTimeLog = NULL;

static RvDecInst g_RVDecInst = NULL;

typedef struct
{
	char         filePath[0x100];
	DWLFile*     fpOut;
	rv_depack*   pDepack;
	rv_decode*   pDecode;
	BYTE*        pOutFrame;
	UINT32       ulOutFrameSize;
	UINT32       ulWidth;
	UINT32       ulHeight;
	UINT32       ulNumInputFrames;
	UINT32       ulNumOutputFrames;
	UINT32       ulMaxNumDecFrames;
	UINT32       ulStartFrameID;	/* 0-based */
	UINT32       bDecEnd;
	UINT32       ulTotalTime;		/* ms */
} rm2yuv_info;

static HX_RESULT rv_frame_available(void* pAvail, UINT32 ulSubStreamNum, rv_frame* pFrame);
static HX_RESULT rv_decode_stream_decode(rv_decode* pFrontEnd, RvDecPicture* pOutput);
static void rv_decode_frame_flush(rm2yuv_info *pInfo, RvDecPicture *pOutput);

static UINT32 rm_io_read(void* pUserRead, BYTE* pBuf, UINT32 ulBytesToRead);
static void rm_io_seek(void* pUserRead, UINT32 ulOffset, UINT32 ulOrigin);
static void* rm_memory_malloc(void* pUserMem, UINT32 ulSize);
static void rm_memory_free(void* pUserMem, void* ptr);
static void* rm_memory_create_ncnb(void* pUserMem, UINT32 ulSize);
static void rm_memory_destroy_ncnb(void* pUserMem, void* ptr);

static void parse_path(const char *path, char *dir, char *file);
static void rm2yuv_error(void* pError, HX_RESULT result, const char* pszMsg);
static void printDecodeReturn(i32 retval);
static void printRvVersion(void);
static void displayOnScreen(RvDecPicture *pDecPicture);
static void RV_Time_Init(i32 islog2std, i32 islog2file);
static void RV_Time_Full_Reset(void);
static void RV_Time_Full_Pause(void);
static void RV_Time_Dec_Reset(void);
static void RV_Time_Dec_Pause(u32 picid, u32 pictype);

static u32 maxNumPics = 0;
static u32 picsDecoded = 0;

/*-----------------------------------------------------------------------------------------------------
function:
 	demux input rm file, decode rv stream threrein, and display on LCD or write to file if necessary
 
input:
	filename:		the file path and name to be display
	startframeid:	the beginning frame of the whole input stream to be decoded
	maxnumber:		the maximal number of frames to be decoded
	iswritefile:	write the decoded frame to local file or not, 0: no, 1: yes
	isdisplay:		send the decoded data to display device or not, 0: no, 1: yes
	displaymode:	display mode on LCD, 0: auto size, 1: full screen
	rotationmode:	rotation mode on LCD, 0: normal, 1: counter-clockwise, 2: clockwise
	islog2std:		output time log to console
	islog2file:		output time log to file
	errconceal:		error conceal mode (reserved)

return: 
	NULL
------------------------------------------------------------------------------------------------------*/ 
u32 NextPacket(u8 ** pStrm, u8* streamStop, u32 isRv8)
{
    u32 index;
    u32 maxIndex;
    u32 zeroCount;
    u8 byte;
    static u32 prevIndex = 0;
    static u8 *stream = NULL;
    u8 nextPacket = 0;
    u8 *p;

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

    zeroCount = 0;

    /* Search stream for next start code prefix */
    /*lint -e(716) while(1) used consciously */
    p = stream + 1;
    while(p < streamStop)
    {
        /* RV9 */
        if (isRv8)
        {
            if (p[0] == 0   && p[1] == 0   && p[2] == 1  &&
                !(p[3]&170) && !(p[4]&170) && !(p[5]&170) && (p[6]&170) == 2)
                break;
        }
        else
        {
            if (p[0] == 85  && p[1] == 85  && p[2] == 85  && p[3] == 85 &&
                !(p[4]&170) && !(p[5]&170) && !(p[6]&170) && (p[7]&170) == 2)
                break;
        }
        p++;

    }

    index = p - stream;
    /* Store pointer to the beginning of the packet */
    *pStrm = stream;
    prevIndex = index;

    return (index);
}

/* Stuff for parsing/reading RealVideo streams */
typedef struct
{
    FILE *fid;
    u32 w,h;

    u32 len;
    u32 isRv8;

    u32 numFrameSizes;
    u32 frameSizes[18];

    u32 numSegments;
    RvDecSliceInfo sliceInfo[128];

} rvParser_t;

u32 RvParserInit(rvParser_t *parser, char *file)
{

    u32 tmp;
    u8 buff[100];

    parser->fid = fopen(file, "rb");
    if (parser->fid == NULL)
    {
        return (HXR_FAIL);
    }

    fread(buff, 1, 4, parser->fid);

    /* length */
    tmp = (buff[0] << 24) |
          (buff[1] << 16) |
          (buff[2] <<  8) |
          (buff[3] <<  0);

    tmp = fread(buff, 1, tmp-4, parser->fid);
    if (tmp <= 0)
        return(HXR_FAIL);

    /* 32b len
     * 32b 'VIDO'
     * 32b 'RV30'/'RV40'
     * 16b width
     * 16b height
     * 80b stuff */

    if (strncmp(buff+4, "RV30", 4) == 0)
        parser->isRv8 = 1;
    else
        parser->isRv8 = 0;

    parser->w = (buff[8] << 8) | buff[9];
    parser->h = (buff[10] << 8) | buff[11];

    if (parser->isRv8)
    {
        u32 i;
        u8 *p = buff + 22; /* header size 26 - four byte length field */
        parser->numFrameSizes = 1 + (p[1] & 0x7);
        p += 8;
        parser->frameSizes[0] = parser->w;
        parser->frameSizes[1] = parser->h;
        for (i = 1; i < parser->numFrameSizes; i++)
        {
            parser->frameSizes[2*i + 0] = (*p++) << 2;
            parser->frameSizes[2*i + 1] = (*p++) << 2;
        }
    }
}

void RvParserRelease(rvParser_t *parser)
{

    u32 tmp;
    u8 buff[100];

    if (parser->fid)
        fclose(parser->fid);
    parser->fid = NULL;

}

u32 RvParserReadFrame(rvParser_t *parser, u8 *p)
{

    u32 i, tmp;
    u8 buff[256];

    tmp = fread(buff, 1, 20, parser->fid);
    if (tmp < 20)
        return (HXR_FAIL);
    parser->len = (buff[0] << 24) | (buff[1] << 16) | (buff[2] << 8) | buff[3];
    parser->numSegments =
        (buff[16] << 24) | (buff[17] << 16) | (buff[18] << 8) | buff[19];

    for (i = 0; i < parser->numSegments; i++)
    {
        tmp = fread(buff, 1, 8, parser->fid);
        parser->sliceInfo[i].isValid =
            (buff[0] << 24) | (buff[1] << 16) | (buff[2] << 8) | buff[3];
        parser->sliceInfo[i].offset =
            (buff[4] << 24) | (buff[5] << 16) | (buff[6] << 8) | buff[7];
    }
    
    tmp = fread(p, 1, parser->len, parser->fid);

    if (tmp > 0)
        return HXR_OK;
    else
        return HXR_FAIL;

}

void rv_mode(const char *filename,
                char *filenamePpCfg,
                int startframeid, 
                int maxnumber, 
                int iswritefile, 
                int isdisplay, 
                int displaymode, 
                int rotationmode, 
                int islog2std, 
                int islog2file,
                int blayerenable,
                int errconceal,
                int randomerror)
{
	HX_RESULT           retVal         = HXR_OK;
	UINT32              i              = 0;
	char                rv_outfilename[256];
    UINT8               size[9] = {0,1,1,2,2,3,3,3,3};

	RvDecRet rvRet;	
    RvDecInput decInput;
	RvDecOutput decOutput;
    RvDecPicture pOutput[1];
	RvDecInfo decInfo;
    rvParser_t*         parser         = HXNULL;
    u32 streamLen;
    DWLLinearMem_t streamMem;
    DWLFile *fpOut = HXNULL;
    DWLmemset(rv_outfilename, 0, sizeof(rv_outfilename));

#ifdef ASIC_TRACE_SUPPORT
    if (!openTraceFiles())
        DEBUG_PRINT(("UNABLE TO OPEN TRACE FILE(S)\n"));
#endif

    /* Initialize all static global variables */
	g_RVDecInst = NULL;
	g_iswritefile = iswritefile;
	g_isdisplay = isdisplay;
	g_displaymode = displaymode;
	g_rotationmode = rotationmode;
	g_bfirstdisplay = 0;
	g_blayerenable = blayerenable;

#ifdef RV_ERROR_SIM
	g_randomerror = randomerror;
#endif

	islog2std = islog2std;
	islog2file = islog2file;

	RV_Time_Init(islog2std, islog2file);

    parser = (rvParser_t *)malloc(sizeof(rvParser_t));
    memset(parser, 0, sizeof(rvParser_t));

    retVal = RvParserInit(parser, (char *)filename);
    
	START_SW_PERFORMANCE;
    decsw_performance();
	rvRet = RvDecInit(&g_RVDecInst, 0,
        size[parser->numFrameSizes], parser->frameSizes, !parser->isRv8,
        parser->w, parser->h,
        numFrameBuffers, tiledOutput );
    END_SW_PERFORMANCE;
    decsw_performance();
	if (rvRet != RVDEC_OK)
	{
		ERROR_PRINT(("RVDecInit fail! \n"));
		goto cleanup;
	}

    /* Set ref buffer test mode */
    ((DecContainer *) g_RVDecInst)->refBufferCtrl.testFunction = TBRefbuTestMode;

#ifdef PP_PIPELINE_ENABLED
    /* Initialize the post processer. If unsuccessful -> exit */
    if(pp_startup
       (filenamePpCfg, g_RVDecInst, PP_PIPELINED_DEC_TYPE_RV, &tbCfg) != 0)
    {
        DEBUG_PRINT(("PP INITIALIZATION FAILED\n"));
        goto cleanup;
    }

    if(pp_update_config
       (g_RVDecInst, PP_PIPELINED_DEC_TYPE_RV, &tbCfg) == CFG_UPDATE_FAIL)

    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
        goto cleanup;
    }
#endif

    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_LATENCY,
                   latency_comp);
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_CLK_GATE_E,
                   clock_gating);
    /*
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_OUT_TILED_E,
                   output_format);
    */
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_OUT_ENDIAN,
                   output_picture_endian);
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_MAX_BURST,
                   bus_burst_length);
    if ((DWLReadAsicID() >> 16) == 0x8170U)
    {
        SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_PRIORITY_MODE,
                       asic_service_priority);
    }
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_DATA_DISC_E,
                   data_discard);

	/* Read what kind of stream is coming */
    START_SW_PERFORMANCE;
    decsw_performance();
	rvRet = RvDecGetInfo(g_RVDecInst, &decInfo);
    END_SW_PERFORMANCE;
    decsw_performance();
    if(rvRet)
	{
		printDecodeReturn(rvRet);
	}

#ifdef PP_PIPELINE_ENABLED

    /* If unspecified at cmd line, use minimum # of buffers, otherwise
     * use specified amount. */
    if(numFrameBuffers == 0)
        pp_number_of_buffers(decInfo.multiBuffPpSize);
    else
        pp_number_of_buffers(numFrameBuffers);

#endif

    TBInitializeRandom(seedRnd);

    /* memory for stream buffer */
    streamLen = 10000000;
    if (DWLMallocLinear(((DecContainer *) g_RVDecInst)->dwl,
           streamLen, &streamMem) != DWL_OK)

    decInput.skipNonReference = skipNonReference;
	decInput.picId = 0;
	decInput.timestamp = 0;

	while (RvParserReadFrame(parser, (u8*)streamMem.virtualAddress) == HXR_OK)
	{
        decInput.dataLen = parser->len;
        decInput.pStream = (u8*)streamMem.virtualAddress;
        decInput.streamBusAddress = streamMem.busAddress;

        decInput.sliceInfoNum = parser->numSegments;
        decInput.pSliceInfo = parser->sliceInfo;

        START_SW_PERFORMANCE;
        decsw_performance();
        rvRet = RvDecDecode(g_RVDecInst, &decInput, &decOutput);
        END_SW_PERFORMANCE;
        decsw_performance();
        if (rvRet == RVDEC_HDRS_RDY)
        {
            TBSetRefbuMemModel( &tbCfg, 
                ((DecContainer *) g_RVDecInst)->rvRegs,
                &((DecContainer *) g_RVDecInst)->refBufferCtrl );
            START_SW_PERFORMANCE;
            decsw_performance();
            rvRet = RvDecDecode(g_RVDecInst, &decInput, &decOutput);
            END_SW_PERFORMANCE;
            decsw_performance();
        }

        switch(rvRet) 
        {
            case RVDEC_OK:
            case RVDEC_PIC_DECODED:
                picsDecoded++;   
                START_SW_PERFORMANCE;
                decsw_performance();
                rvRet = RvDecNextPicture(g_RVDecInst, pOutput, 0);
                END_SW_PERFORMANCE;
                decsw_performance();
                if (rvRet == RVDEC_PIC_RDY)
                {
                    u8 *rasterScan = NULL;

                    /* Convert back to raster scan format if decoder outputs 
                     * tiled format */
                    if(pOutput->outputFormat && convertTiledOutput)
                    {
                        rasterScan = (u8*)malloc(pOutput->frameWidth*
                            pOutput->frameHeight*3/2);
                        if(!rasterScan)
                        {
                            fprintf(stderr, "error allocating memory for tiled"
                                "-->raster conversion!\n");
                            return;
                        }

                        TBTiledToRaster( pOutput->outputFormat, DEC_DPB_DEFAULT,
                            pOutput->pOutputPicture, rasterScan, 
                            pOutput->frameWidth, pOutput->frameHeight );
                        TBTiledToRaster( pOutput->outputFormat, DEC_DPB_DEFAULT, 
                                pOutput->pOutputPicture+
                                pOutput->frameWidth*
                                pOutput->frameHeight, 
                            rasterScan+pOutput->frameWidth*pOutput->frameHeight, 
                            pOutput->frameWidth, 
                            pOutput->frameHeight/2 );
                        pOutput->pOutputPicture = rasterScan;
                    }

                    /* TODO: size change */
                    if (fpOut == HXNULL)
                    {
                        char flnm[0x100] = "";
                        if( *outFileName == '\0' )
                        {
                            sprintf(outFileName, "out_w%dh%d.yuv",
                                /*pInfo->filePath,*/ 
                                MB_MULTIPLE(pOutput->frameWidth), 
                                MB_MULTIPLE(pOutput->frameHeight));
                        }

#if 0
                        if(fTiledOutput == NULL && tiledOutput)
                        {
                            /* open output file for writing, can be disabled with define.
                             * If file open fails -> exit */
                            if(strcmp(outFileNameTiled, "none") != 0)
                            {
                                fTiledOutput = fopen(outFileNameTiled, "wb");
                                if(fTiledOutput == NULL)
                                {
                                    printf("UNABLE TO OPEN TILED OUTPUT FILE\n");
                                    return;
                                }
                            }
                        }
#endif

                        fpOut = DWLfopen(outFileName, "w");
                        if(enableFramePicture && !frameOut)
                        {
                            u32 tmpWidth, tmpHeight;
                            maxCodedWidth = pOutput->frameWidth;
                            maxCodedHeight = pOutput->frameHeight;
                            maxFrameWidth = MB_MULTIPLE( maxCodedWidth );
                            maxFrameHeight = MB_MULTIPLE( maxCodedHeight );
                            sprintf(flnm, "frame.yuv",
                                /*pInfo->filePath,*/ maxCodedWidth, maxCodedHeight);
                            frameOut = DWLfopen(flnm, "w");
                            frameBuffer = (u8*)malloc(maxFrameWidth*maxFrameHeight*3/2);
                        }
                    }
#ifdef PP_PIPELINE_ENABLED
                    HandlePpOutput(frameNumber - 1, *pOutput, g_RVDecInst);
#else
                    if (fpOut)
                    {
#ifndef ASIC_TRACE_SUPPORT
                          u8* buffer = pOutput->pOutputPicture;
                          if(output_picture_endian == DEC_X170_BIG_ENDIAN)
                          {
                              u32 frameSize = MB_MULTIPLE(pOutput->frameWidth)* MB_MULTIPLE(pOutput->frameWidth)*3/2;
                              if(pic_big_endian_size != frameSize)
                              {
                                    if(pic_big_endian != NULL)
                                        free(pic_big_endian);

                                    pic_big_endian = (u8 *) malloc(frameSize);
                                    if(pic_big_endian == NULL)
                                    {
                                        DEBUG_PRINT(("MALLOC FAILED @ %s %d", __FILE__, __LINE__));
                                        if(rasterScan)
                                            free(rasterScan);
                                        return;
                                    }
            
                                pic_big_endian_size = frameSize;
                                }

                                memcpy(pic_big_endian, buffer, frameSize);
                                TbChangeEndianess(pic_big_endian, frameSize);
                                pOutput->pOutputPicture = pic_big_endian;
                            }
#endif
#ifdef MD5SUM
                        u32 tmp = maxCodedWidth * maxCodedHeight, i;
                        FramePicture( pOutput->pOutputPicture,
                            maxCodedWidth, maxCodedHeight,
                            maxFrameWidth, maxFrameHeight,
                            frameBuffer, maxCodedWidth, maxCodedHeight );

                        /* TODO: frame counter */
                        if( planarOutput )
                        {
                            TBWriteFrameMD5Sum(fpOut, frameBuffer,
                                maxCodedWidth*
                                maxCodedHeight*3/2, 1);
                        }
                        else
                        {
                            TBWriteFrameMD5Sum(fpOut, pOutput->pOutputPicture,
                                MB_MULTIPLE(pOutput->frameWidth)*
                                MB_MULTIPLE(pOutput->frameHeight)*3/2, 1);
                        }

                        /* tiled */
#if 0
                        if (tiledOutput)
                        {
                            /* round up to next multiple of 16 */
                            u32 tmpWidth = MB_MULTIPLE(pOutput->frameWidth);
                            u32 tmpHeight = MB_MULTIPLE(pOutput->frameHeight);
                              
                            TbWriteTiledOutput(fTiledOutput, pOutput->pOutputPicture, tmpWidth >> 4, tmpHeight >> 4, 
                                    numberOfWrittenFrames, 1 /* write md5sum */, 1 /* semi-planar data */ );
                        }
#endif
#else               
                        if( !planarOutput )
                        {
                            DWLfwrite(pOutput->pOutputPicture, 1, 
                                MB_MULTIPLE(pOutput->frameWidth)*
                                MB_MULTIPLE(pOutput->frameHeight)*3/2, fpOut);
                        }
                        else
                        {
                            u32 tmp = MB_MULTIPLE(pOutput->frameWidth)*
                                      MB_MULTIPLE(pOutput->frameHeight), i;
                            DWLfwrite(pOutput->pOutputPicture, 1, tmp, fpOut);
                            for(i = 0; i < tmp / 4; i++)
                                DWLfwrite(pOutput->pOutputPicture + tmp + i * 2, 1, 1, fpOut);
                            for(i = 0; i < tmp / 4; i++)
                                DWLfwrite(pOutput->pOutputPicture + tmp + 1 + i * 2, 1, 1, fpOut);
                        }
                        if( enableFramePicture )
                        {
                            u32 tmp = maxCodedWidth * maxCodedHeight, i;
                            FramePicture( pOutput->pOutputPicture,
                                maxCodedWidth, maxCodedHeight,
                                maxFrameWidth, maxFrameHeight,
                                frameBuffer, maxCodedWidth, maxCodedHeight );

                            DWLfwrite(frameBuffer, 1, 3*tmp/2, frameOut);
                        }

#if 0
                        /* tiled */
                        if (tiledOutput)
                        {
                            /* round up to next multiple of 16 */
                            u32 tmpWidth = MB_MULTIPLE(pOutput->frameWidth );
                            u32 tmpHeight = MB_MULTIPLE(pOutput->frameHeight );
                              
                            TbWriteTiledOutput(fTiledOutput, pOutput->pOutputPicture, tmpWidth >> 4, tmpHeight >> 4, 
                                    numberOfWrittenFrames, 0 /* not write md5sum */, 1 /* semi-planar data */ );
                        }
#endif

#endif                    
                        numberOfWrittenFrames++;
                    
#ifndef ASIC_TRACE_SUPPORT
                        pOutput->pOutputPicture = buffer;
#endif
                    }      
#endif
                    if(rasterScan)
                        free(rasterScan);
                }
                break;

            case RVDEC_HDRS_RDY:
                TBSetRefbuMemModel( &tbCfg, 
                    ((DecContainer *) g_RVDecInst)->rvRegs,
                    &((DecContainer *) g_RVDecInst)->refBufferCtrl );

                /* Read what kind of stream is coming */
                START_SW_PERFORMANCE;
                decsw_performance();
                rvRet = RvDecGetInfo(g_RVDecInst, &decInfo);
                END_SW_PERFORMANCE;
                decsw_performance();
                if(rvRet)
                {
                    printDecodeReturn(rvRet);
                }

#ifdef PP_PIPELINE_ENABLED

                /* If unspecified at cmd line, use minimum # of buffers, otherwise
                 * use specified amount. */
                if(numFrameBuffers == 0)
                    pp_number_of_buffers(decInfo.multiBuffPpSize);
                else
                    pp_number_of_buffers(numFrameBuffers);

#endif
                printf("*********************** RvDecGetInfo()/RAW ******************\n");
                printf("decInfo.frameWidth %d\n",(decInfo.frameWidth << 4));
                printf("decInfo.frameHeight %d\n",(decInfo.frameHeight << 4));
                printf("decInfo.codedWidth %d\n",decInfo.codedWidth);
                printf("decInfo.codedHeight %d\n",decInfo.codedHeight);
                if(decInfo.outputFormat == RVDEC_SEMIPLANAR_YUV420)
                    printf("decInfo.outputFormat RVDEC_SEMIPLANAR_YUV420\n");
                else
                    printf("decInfo.outputFormat RVDEC_TILED_YUV420\n");
                printf("******************* RAW MODE ********************************\n");

                break;

            case RVDEC_NONREF_PIC_SKIPPED:
                break;

            default:			
                /*
                * RVDEC_NOT_INITIALIZED
                * RVDEC_PARAM_ERROR
                * RVDEC_STRM_ERROR
                * RVDEC_STRM_PROCESSED
                * RVDEC_SYSTEM_ERROR
                * RVDEC_MEMFAIL			 
                * RVDEC_HW_TIMEOUT
                * RVDEC_HW_RESERVED
                * RVDEC_HW_BUS_ERROR
                */
                DEBUG_PRINT(("Fatal Error, Decode end!\n"));
                retVal = HXR_FAIL;
                break;
        }

        if (maxNumPics && picsDecoded >= maxNumPics)
            break;
	}

	/* Output the last picture in decoded picture memory pool */
    START_SW_PERFORMANCE;
    decsw_performance();
	rvRet = RvDecNextPicture(g_RVDecInst, pOutput, 1);
    END_SW_PERFORMANCE;
    decsw_performance();
	if (rvRet == RVDEC_PIC_RDY)
	{
        u8 *rasterScan = NULL;
        /* Convert back to raster scan format if decoder outputs 
         * tiled format */
        if(pOutput->outputFormat && convertTiledOutput)
        {
            rasterScan = (u8*)malloc(pOutput->frameWidth*
                pOutput->frameHeight*3/2);
            if(!rasterScan)
            {
                fprintf(stderr, "error allocating memory for tiled"
                    "-->raster conversion!\n");
                return;
            }

            TBTiledToRaster( pOutput->outputFormat, DEC_DPB_DEFAULT, 
                pOutput->pOutputPicture, rasterScan, 
                pOutput->frameWidth, pOutput->frameHeight );
            TBTiledToRaster( pOutput->outputFormat, DEC_DPB_DEFAULT,
                    pOutput->pOutputPicture+
                    pOutput->frameWidth*
                    pOutput->frameHeight, 
                rasterScan+pOutput->frameWidth*pOutput->frameHeight, 
                pOutput->frameWidth, 
                pOutput->frameHeight/2 );
            pOutput->pOutputPicture = rasterScan;
        }

        /* TODO: size change */
        if (fpOut == HXNULL)
        {
            char flnm[0x100] = "";
            if( *outFileName == '\0' )
            {
                sprintf(outFileName, "out_w%dh%d.yuv",
                    /*pInfo->filePath,*/ 
                    MB_MULTIPLE(pOutput->frameWidth), 
                    MB_MULTIPLE(pOutput->frameHeight));
            }

#if 0
            if(fTiledOutput == NULL && tiledOutput)
            {
                /* open output file for writing, can be disabled with define.
                 * If file open fails -> exit */
                if(strcmp(outFileNameTiled, "none") != 0)
                {
                    fTiledOutput = fopen(outFileNameTiled, "wb");
                    if(fTiledOutput == NULL)
                    {
                        printf("UNABLE TO OPEN TILED OUTPUT FILE\n");
                        return;
                    }
                }
            }
#endif

            fpOut = DWLfopen(outFileName, "w");
        }
#ifdef PP_PIPELINE_ENABLED
        HandlePpOutput(frameNumber - 1, *pOutput, g_RVDecInst);
#else
        if (fpOut)
        {
#ifndef ASIC_TRACE_SUPPORT
            u8* buffer = pOutput->pOutputPicture;
            if(output_picture_endian == DEC_X170_BIG_ENDIAN)
            {
                u32 frameSize = MB_MULTIPLE(pOutput->frameWidth)*MB_MULTIPLE(pOutput->frameWidth)*3/2;
                if(pic_big_endian_size != frameSize)
                {
                    if(pic_big_endian != NULL)
                        free(pic_big_endian);

                    pic_big_endian = (u8 *) malloc(frameSize);
                    if(pic_big_endian == NULL)
                    {
                        DEBUG_PRINT(("MALLOC FAILED @ %s %d", __FILE__, __LINE__));
                        return;
                    }
            
                    pic_big_endian_size = frameSize;
                }

                memcpy(pic_big_endian, buffer, frameSize);
                TbChangeEndianess(pic_big_endian, frameSize);
                pOutput->pOutputPicture = pic_big_endian;
            }
#endif    
#ifdef MD5SUM
            u32 tmp = maxCodedWidth * maxCodedHeight, i;
            FramePicture( pOutput->pOutputPicture,
                maxCodedWidth, maxCodedHeight,
                maxFrameWidth, maxFrameHeight,
                frameBuffer, maxCodedWidth, maxCodedHeight );

            /* TODO: frame counter */
            if( planarOutput )
            {
                TBWriteFrameMD5Sum(fpOut, frameBuffer,
                    maxCodedWidth*
                    maxCodedHeight*3/2, 1);
            }
            else
            {
            /* TODO: frame counter */
                TBWriteFrameMD5Sum(fpOut, pOutput->pOutputPicture,
                        MB_MULTIPLE(pOutput->frameWidth)*
                        MB_MULTIPLE(pOutput->frameHeight)*3/2, 1);
            }
#if 0
            /* tiled */
            if (tiledOutput)
            {
                /* round up to next multiple of 16 */
                u32 tmpWidth = MB_MULTIPLE(pOutput->frameWidth);
                u32 tmpHeight = MB_MULTIPLE(pOutput->frameHeight);
                  
                TbWriteTiledOutput(fTiledOutput, pOutput->pOutputPicture, tmpWidth >> 4, tmpHeight >> 4, 
                        numberOfWrittenFrames, 1 /* write md5sum */, 1 /* semi-planar data */ );
            }
#endif
#else                
            if( !planarOutput )
            {
                DWLfwrite(pOutput->pOutputPicture, 1, 
                    MB_MULTIPLE(pOutput->frameWidth)*
                    MB_MULTIPLE(pOutput->frameHeight)*3/2, fpOut);
            }
            else
            {
                u32 tmp = MB_MULTIPLE(pOutput->frameWidth)*
                    MB_MULTIPLE(pOutput->frameHeight), i;
                DWLfwrite(pOutput->pOutputPicture, 1, tmp, fpOut);
                for(i = 0; i < tmp / 4; i++)
                    DWLfwrite(pOutput->pOutputPicture + tmp + i * 2, 1, 1, fpOut);
                for(i = 0; i < tmp / 4; i++)
                    DWLfwrite(pOutput->pOutputPicture + tmp + 1 + i * 2, 1, 1, fpOut);
            }

            if( enableFramePicture )
            {
                u32 tmp = maxCodedWidth * maxCodedHeight, i;
                FramePicture( pOutput->pOutputPicture,
                    maxCodedWidth, maxCodedHeight,
                    maxFrameWidth, maxFrameHeight,
                    frameBuffer, maxCodedWidth, maxCodedHeight );

                DWLfwrite(frameBuffer, 1, 3*tmp/2, frameOut);
                /*
                for(i = 0; i < tmp / 4; i++)
                    DWLfwrite(frameBuffer + tmp + i * 2, 1, 1, frameOut);
                for(i = 0; i < tmp / 4; i++)
                    DWLfwrite(frameBuffer + tmp + 1 + i * 2, 1, 1, frameOut);
                    */
            }

#if 0
            /* tiled */
            if (tiledOutput)
            {
                /* round up to next multiple of 16 */
                u32 tmpWidth = MB_MULTIPLE(pOutput->frameWidth + 15);
                u32 tmpHeight = MB_MULTIPLE(pOutput->frameHeight + 15);
                  
                TbWriteTiledOutput(fTiledOutput, pOutput->pOutputPicture, tmpWidth >> 4, tmpHeight >> 4, 
                        numberOfWrittenFrames, 0 /* not write md5sum */, 1 /* semi-planar data */ );
            }
#endif

#endif

            numberOfWrittenFrames++;
            
#ifndef ASIC_TRACE_SUPPORT
           pOutput->pOutputPicture = buffer;
#endif
        }
#endif
        if(rasterScan)
            free(rasterScan);
		
		/* update frame number */
		frameNumber++;
	}

    DWLFreeLinear(((DecContainer *) g_RVDecInst)->dwl, &streamMem);

    if (pic_big_endian != NULL)
        free(pic_big_endian);

	TIME_PRINT(("\n"));

cleanup:
		
    RvParserRelease(parser);

	/* Close the output file */
	if (fpOut)
	{
		DWLfclose(fpOut);
	}

    if(fTiledOutput)
    {
        DWLfclose(fTiledOutput);
        fTiledOutput = NULL;
    }

	/* Release the decoder instance */
	if (g_RVDecInst)
	{
        START_SW_PERFORMANCE;
        decsw_performance();
		RvDecRelease(g_RVDecInst);
        END_SW_PERFORMANCE;
        decsw_performance();
	}

    FINALIZE_SW_PERFORMANCE;
    
#ifdef ASIC_TRACE_SUPPORT
    trace_SequenceCtrl(hwDecPicCount, 0);
    trace_RvDecodingTools();
    closeTraceFiles();
#endif

}

void raw_mode(DWLFile* fpIn,
              char *filenamePpCfg,
              int startframeid, 
              int maxnumber, 
              int iswritefile, 
              int isdisplay, 
              int displaymode, 
              int rotationmode, 
              int islog2std, 
              int islog2file,
              int blayerenable,
              int errconceal,
              int randomerror)
{
    HX_RESULT           retVal         = HXR_OK;
    INT32               lBytesRead     = 0;
    UINT32              ulNumStreams   = 0;
    UINT32              i              = 0;
    UINT16              usStreamNum    = 0;
    UINT32              ulOutFrameSize = 0;
    UINT32              ulFramesPerSec = 0;
    char                rv_outfilename[256];
    char                tmpStream[4];

    RvDecRet rvRet;	
    RvDecOutput decOutput;
    RvDecInput decInput;
    RvDecInfo decInfo;
    RvDecPicture pOutput[1];
    DWLLinearMem_t sliceInfo;

    DWLLinearMem_t streamMem;
    u32 streamLen;
    u8 *streamStop;
    u8 *ptmpstream;
    u32 tmp;
    u32 isRv8;

    DWLFile *fpOut = HXNULL;
    DWLmemset(rv_outfilename, 0, sizeof(rv_outfilename));

    DEBUG_PRINT(("Start decode ...\n"));

    DWLfread(tmpStream, sizeof(u8), 4, fpIn);
    DWLfseek(fpIn, 0, DWL_SEEK_SET);
    /* RV9 raw stream */
    if (tmpStream[0] == 0 && tmpStream[1] == 0 &&
        tmpStream[2] == 1)
    {
        START_SW_PERFORMANCE;
        decsw_performance();
        rvRet = RvDecInit(&g_RVDecInst, 0, 0, 0, 0, 0, 0,
                          numFrameBuffers, tiledOutput);
        END_SW_PERFORMANCE;
        decsw_performance();
        isRv8 = 1;
    }
    else
    {
        START_SW_PERFORMANCE;
        decsw_performance();
        rvRet = RvDecInit(&g_RVDecInst, 0, 0, 0, 1, 0, 0,
                            numFrameBuffers, tiledOutput);
        END_SW_PERFORMANCE;
        decsw_performance();
        isRv8 = 0;
    }

    if (rvRet != RVDEC_OK)
    {
    	ERROR_PRINT(("RVDecInit fail! \n"));
    	return;
    }

#ifdef PP_PIPELINE_ENABLED
    /* Initialize the post processer. If unsuccessful -> exit */
    if(pp_startup
       (filenamePpCfg, g_RVDecInst, PP_PIPELINED_DEC_TYPE_RV, &tbCfg) != 0)
    {
        DEBUG_PRINT(("PP INITIALIZATION FAILED\n"));
        return;
    }

    if(pp_update_config
       (g_RVDecInst, PP_PIPELINED_DEC_TYPE_RV, &tbCfg) == CFG_UPDATE_FAIL)

    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
        return;
    }
#endif

    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_LATENCY,
                   latency_comp);
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_CLK_GATE_E,
                   clock_gating);
    /*
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_OUT_TILED_E,
                   output_format);
                   */
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_OUT_ENDIAN,
                   output_picture_endian);
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_MAX_BURST,
                   bus_burst_length);
    if ((DWLReadAsicID() >> 16) == 0x8170U)
    {
        SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_PRIORITY_MODE,
                       asic_service_priority);
    }
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_DATA_DISC_E,
                   data_discard);

    TBInitializeRandom(seedRnd);

    streamLen = 10000000;
    if(DWLMallocLinear(((DecContainer *) g_RVDecInst)->dwl,
           streamLen, &streamMem) != DWL_OK)
    {
        DEBUG_PRINT(("UNABLE TO ALLOCATE STREAM BUFFER MEMORY\n"));
        return;
    }

    streamLen = DWLfread(streamMem.virtualAddress, sizeof(u8), streamLen, fpIn);
    streamStop = (u8*)streamMem.virtualAddress+streamLen;

    decInput.skipNonReference = skipNonReference;
	decInput.pStream = (u8*)streamMem.virtualAddress;
	decInput.streamBusAddress = streamMem.busAddress;
	decInput.picId = 0;
	decInput.timestamp = 0;

    ptmpstream = decInput.pStream;
    tmp = NextPacket((u8**)(&decInput.pStream),streamStop, isRv8);
	decInput.dataLen = tmp;
	decInput.streamBusAddress += (u32)(decInput.pStream-ptmpstream);

	decInput.sliceInfoNum = 0;
    sliceInfo.virtualAddress = NULL;
    sliceInfo.busAddress = 0;
    
	decInput.pSliceInfo = NULL;

	/* Now keep getting packets until we receive an error */
	do
	{
        START_SW_PERFORMANCE;
        decsw_performance();
        rvRet = RvDecDecode(g_RVDecInst, &decInput, &decOutput);
        END_SW_PERFORMANCE;
        decsw_performance();
        if (rvRet == RVDEC_HDRS_RDY)
        {
            TBSetRefbuMemModel( &tbCfg, 
                ((DecContainer *) g_RVDecInst)->rvRegs,
                &((DecContainer *) g_RVDecInst)->refBufferCtrl );
            START_SW_PERFORMANCE;
            decsw_performance();
            rvRet = RvDecDecode(g_RVDecInst, &decInput, &decOutput);
            END_SW_PERFORMANCE;
            decsw_performance();
        }

        switch(rvRet) 
        {
            case RVDEC_OK:
            case RVDEC_PIC_DECODED:
                picsDecoded++;   
                START_SW_PERFORMANCE;
                decsw_performance();
                rvRet = RvDecNextPicture(g_RVDecInst, pOutput, 0);
                END_SW_PERFORMANCE;
                decsw_performance();
                if (rvRet == RVDEC_PIC_RDY)
                {
                    u8 *rasterScan = NULL;
                    /* Convert back to raster scan format if decoder outputs 
                     * tiled format */
                    if(pOutput->outputFormat && convertTiledOutput)
                    {
                        rasterScan = (u8*)malloc(pOutput->frameWidth*
                            pOutput->frameHeight*3/2);
                        if(!rasterScan)
                        {
                            fprintf(stderr, "error allocating memory for tiled"
                                "-->raster conversion!\n");
                            return;
                        }

                        TBTiledToRaster( pOutput->outputFormat, DEC_DPB_DEFAULT, 
                            pOutput->pOutputPicture, rasterScan, 
                            pOutput->frameWidth, pOutput->frameHeight );
                        TBTiledToRaster( pOutput->outputFormat, DEC_DPB_DEFAULT, 
                                pOutput->pOutputPicture+
                                pOutput->frameWidth*
                                pOutput->frameHeight, 
                            rasterScan+pOutput->frameWidth*pOutput->frameHeight, 
                            pOutput->frameWidth, 
                            pOutput->frameHeight/2 );
                        pOutput->pOutputPicture = rasterScan;
                    }
                    /* TODO: size change */
                    if (fpOut == HXNULL)
                    {
                        char flnm[0x100] = "";
                        if( *outFileName == '\0' )
                        {
                            sprintf(outFileName, "out_w%dh%d.yuv",
                                /*pInfo->filePath,*/ 
                                MB_MULTIPLE(pOutput->frameWidth), 
                                MB_MULTIPLE(pOutput->frameHeight));
                        }

#if 0
                        if(fTiledOutput == NULL && tiledOutput)
                        {
                            /* open output file for writing, can be disabled with define.
                             * If file open fails -> exit */
                            if(strcmp(outFileNameTiled, "none") != 0)
                            {
                                fTiledOutput = fopen(outFileNameTiled, "wb");
                                if(fTiledOutput == NULL)
                                {
                                    printf("UNABLE TO OPEN TILED OUTPUT FILE\n");
                                    return;
                                }
                            }
                        }
#endif

                        fpOut = DWLfopen(outFileName, "w");
                        if(enableFramePicture && !frameOut)
                        {
                            u32 tmpWidth, tmpHeight;
                            maxCodedWidth = pOutput->frameWidth;
                            maxCodedHeight = pOutput->frameHeight;
                            maxFrameWidth = MB_MULTIPLE( maxCodedWidth );
                            maxFrameHeight = MB_MULTIPLE( maxCodedHeight );
                            sprintf(flnm, "frame.yuv",
                                /*pInfo->filePath,*/ maxCodedWidth, maxCodedHeight);
                            frameOut = DWLfopen(flnm, "w");
                            frameBuffer = (u8*)malloc(maxFrameWidth*maxFrameHeight*3/2);
                        }
                    }
#ifdef PP_PIPELINE_ENABLED
                    HandlePpOutput(frameNumber - 1, *pOutput, g_RVDecInst);
#else
                    if (fpOut)
                    {
#ifndef ASIC_TRACE_SUPPORT
                          u8* buffer = pOutput->pOutputPicture;
                          if(output_picture_endian == DEC_X170_BIG_ENDIAN)
                          {
                              u32 frameSize = MB_MULTIPLE(pOutput->frameWidth)* MB_MULTIPLE(pOutput->frameWidth)*3/2;
                              if(pic_big_endian_size != frameSize)
                              {
                                    if(pic_big_endian != NULL)
                                        free(pic_big_endian);

                                    pic_big_endian = (u8 *) malloc(frameSize);
                                    if(pic_big_endian == NULL)
                                    {
                                        DEBUG_PRINT(("MALLOC FAILED @ %s %d", __FILE__, __LINE__));
                                        return;
                                    }
            
                                pic_big_endian_size = frameSize;
                                }

                                memcpy(pic_big_endian, buffer, frameSize);
                                TbChangeEndianess(pic_big_endian, frameSize);
                                pOutput->pOutputPicture = pic_big_endian;
                            }
#endif
#ifdef MD5SUM
                        u32 tmp = maxCodedWidth * maxCodedHeight, i;
                        FramePicture( pOutput->pOutputPicture,
                            maxCodedWidth, maxCodedHeight,
                            maxFrameWidth, maxFrameHeight,
                            frameBuffer, maxCodedWidth, maxCodedHeight );

                        /* TODO: frame counter */
                        if( planarOutput )
                        {
                            TBWriteFrameMD5Sum(fpOut, frameBuffer,
                                maxCodedWidth*
                                maxCodedHeight*3/2, 1);
                        }
                        else
                        {
                            TBWriteFrameMD5Sum(fpOut, pOutput->pOutputPicture,
                                MB_MULTIPLE(pOutput->frameWidth)*
                                MB_MULTIPLE(pOutput->frameHeight)*3/2, 1);
                        }

#if 0
                        /* tiled */
                        if (tiledOutput)
                        {
                            /* round up to next multiple of 16 */
                            u32 tmpWidth = MB_MULTIPLE(pOutput->frameWidth);
                            u32 tmpHeight = MB_MULTIPLE(pOutput->frameHeight);
                              
                            TbWriteTiledOutput(fTiledOutput, pOutput->pOutputPicture, tmpWidth >> 4, tmpHeight >> 4, 
                                    numberOfWrittenFrames, 1 /* write md5sum */, 1 /* semi-planar data */ );
                        }
#endif 
#else               
                        if( !planarOutput )
                        {
                            DWLfwrite(pOutput->pOutputPicture, 1, 
                                MB_MULTIPLE(pOutput->frameWidth)*
                                MB_MULTIPLE(pOutput->frameHeight)*3/2, fpOut);
                        }
                        else
                        {
                            u32 tmp = MB_MULTIPLE(pOutput->frameWidth)*
                                      MB_MULTIPLE(pOutput->frameHeight), i;
                            DWLfwrite(pOutput->pOutputPicture, 1, tmp, fpOut);
                            for(i = 0; i < tmp / 4; i++)
                                DWLfwrite(pOutput->pOutputPicture + tmp + i * 2, 1, 1, fpOut);
                            for(i = 0; i < tmp / 4; i++)
                                DWLfwrite(pOutput->pOutputPicture + tmp + 1 + i * 2, 1, 1, fpOut);
                        }
                        if( enableFramePicture )
                        {
                            u32 tmp = maxCodedWidth * maxCodedHeight, i;
                            FramePicture( pOutput->pOutputPicture,
                                maxCodedWidth, maxCodedHeight,
                                maxFrameWidth, maxFrameHeight,
                                frameBuffer, maxCodedWidth, maxCodedHeight );

                            DWLfwrite(frameBuffer, 1, 3*tmp/2, frameOut);
                            /*
                            for(i = 0; i < tmp / 4; i++)
                                DWLfwrite(frameBuffer + tmp + i * 2, 1, 1, frameOut);
                            for(i = 0; i < tmp / 4; i++)
                                DWLfwrite(frameBuffer + tmp + 1 + i * 2, 1, 1, frameOut);
                                */
                        }

#if 0
                        /* tiled */
                        if (tiledOutput)
                        {
                            /* round up to next multiple of 16 */
                            u32 tmpWidth = MB_MULTIPLE(pOutput->frameWidth );
                            u32 tmpHeight = MB_MULTIPLE(pOutput->frameHeight );
                              
                            TbWriteTiledOutput(fTiledOutput, pOutput->pOutputPicture, tmpWidth >> 4, tmpHeight >> 4, 
                                    numberOfWrittenFrames, 0 /* not write md5sum */, 1 /* semi-planar data */ );
                        }
#endif

#endif                    
                        numberOfWrittenFrames++;
                    
#ifndef ASIC_TRACE_SUPPORT
                        pOutput->pOutputPicture = buffer;
#endif
                    }      

#endif
                    if(rasterScan)
                        free(rasterScan);

                }
                break;

            case RVDEC_HDRS_RDY:
                TBSetRefbuMemModel( &tbCfg, 
                    ((DecContainer *) g_RVDecInst)->rvRegs,
                    &((DecContainer *) g_RVDecInst)->refBufferCtrl );

                /* Read what kind of stream is coming */
                START_SW_PERFORMANCE;
                decsw_performance();
                rvRet = RvDecGetInfo(g_RVDecInst, &decInfo);
                END_SW_PERFORMANCE;
                decsw_performance();
                if(rvRet)
                {
                    printDecodeReturn(rvRet);
                }

#ifdef PP_PIPELINE_ENABLED

                /* If unspecified at cmd line, use minimum # of buffers, otherwise
                 * use specified amount. */
                if(numFrameBuffers == 0)
                    pp_number_of_buffers(decInfo.multiBuffPpSize);
                else
                    pp_number_of_buffers(numFrameBuffers);

#endif
                printf("*********************** RvDecGetInfo()/RAW ******************\n");
                printf("decInfo.frameWidth %d\n",(decInfo.frameWidth << 4));
                printf("decInfo.frameHeight %d\n",(decInfo.frameHeight << 4));
                printf("decInfo.codedWidth %d\n",decInfo.codedWidth);
                printf("decInfo.codedHeight %d\n",decInfo.codedHeight);
                if(decInfo.outputFormat == RVDEC_SEMIPLANAR_YUV420)
                    printf("decInfo.outputFormat RVDEC_SEMIPLANAR_YUV420\n");
                else
                    printf("decInfo.outputFormat RVDEC_TILED_YUV420\n");
                printf("******************* RAW MODE ********************************\n");

                break;

            case RVDEC_NONREF_PIC_SKIPPED:
                break;

            default:			
                /*
                * RVDEC_NOT_INITIALIZED
                * RVDEC_PARAM_ERROR
                * RVDEC_STRM_ERROR
                * RVDEC_STRM_PROCESSED
                * RVDEC_SYSTEM_ERROR
                * RVDEC_MEMFAIL			 
                * RVDEC_HW_TIMEOUT
                * RVDEC_HW_RESERVED
                * RVDEC_HW_BUS_ERROR
                */
                DEBUG_PRINT(("Fatal Error, Decode end!\n"));
                retVal = HXR_FAIL;
                break;
        }

        ptmpstream = decInput.pStream;
        tmp = NextPacket((u8**)(&decInput.pStream),streamStop, isRv8);
        decInput.dataLen = tmp;
        decInput.streamBusAddress += (u32)(decInput.pStream-ptmpstream);

        if (maxNumPics && picsDecoded >= maxNumPics)
            break;
	} while(decInput.dataLen > 0);

	/* Output the last picture in decoded picture memory pool */
    START_SW_PERFORMANCE;
    decsw_performance();
	rvRet = RvDecNextPicture(g_RVDecInst, pOutput, 1);
    END_SW_PERFORMANCE;
    decsw_performance();
	if (rvRet == RVDEC_PIC_RDY)
	{
        u8 *rasterScan = NULL;
        /* Convert back to raster scan format if decoder outputs 
         * tiled format */
        if(pOutput->outputFormat && convertTiledOutput)
        {
            rasterScan = (u8*)malloc(pOutput->frameWidth*
                pOutput->frameHeight*3/2);
            if(!rasterScan)
            {
                fprintf(stderr, "error allocating memory for tiled"
                    "-->raster conversion!\n");
                return;
            }

            TBTiledToRaster( pOutput->outputFormat, DEC_DPB_DEFAULT, 
                pOutput->pOutputPicture, rasterScan, 
                pOutput->frameWidth, pOutput->frameHeight );
            TBTiledToRaster( pOutput->outputFormat, DEC_DPB_DEFAULT, 
                    pOutput->pOutputPicture+
                    pOutput->frameWidth*
                    pOutput->frameHeight, 
                rasterScan+pOutput->frameWidth*pOutput->frameHeight, 
                pOutput->frameWidth, 
                pOutput->frameHeight/2 );
            pOutput->pOutputPicture = rasterScan;
        }

        /* TODO: size change */
        if (fpOut == HXNULL)
        {
            char flnm[0x100] = "";
            if( *outFileName == '\0' )
            {
                sprintf(outFileName, "out_w%dh%d.yuv",
                    /*pInfo->filePath,*/ 
                    MB_MULTIPLE(pOutput->frameWidth), 
                    MB_MULTIPLE(pOutput->frameHeight));
            }

#if 0
            if(fTiledOutput == NULL && tiledOutput)
            {
                /* open output file for writing, can be disabled with define.
                 * If file open fails -> exit */
                if(strcmp(outFileNameTiled, "none") != 0)
                {
                    fTiledOutput = fopen(outFileNameTiled, "wb");
                    if(fTiledOutput == NULL)
                    {
                        printf("UNABLE TO OPEN TILED OUTPUT FILE\n");
                        return;
                    }
                }
            }
#endif

            fpOut = DWLfopen(outFileName, "w");
        }
#ifdef PP_PIPELINE_ENABLED
        HandlePpOutput(frameNumber - 1, *pOutput, g_RVDecInst);
#else
        if (fpOut)
        {
#ifndef ASIC_TRACE_SUPPORT
            u8* buffer = pOutput->pOutputPicture;
            if(output_picture_endian == DEC_X170_BIG_ENDIAN)
            {
                u32 frameSize = MB_MULTIPLE(pOutput->frameWidth)*MB_MULTIPLE(pOutput->frameWidth)*3/2;
                if(pic_big_endian_size != frameSize)
                {
                    if(pic_big_endian != NULL)
                        free(pic_big_endian);

                    pic_big_endian = (u8 *) malloc(frameSize);
                    if(pic_big_endian == NULL)
                    {
                        DEBUG_PRINT(("MALLOC FAILED @ %s %d", __FILE__, __LINE__));
                        return;
                    }
            
                    pic_big_endian_size = frameSize;
                }

                memcpy(pic_big_endian, buffer, frameSize);
                TbChangeEndianess(pic_big_endian, frameSize);
                pOutput->pOutputPicture = pic_big_endian;
            }
#endif    
#ifdef MD5SUM
            u32 tmp = maxCodedWidth * maxCodedHeight, i;
            FramePicture( pOutput->pOutputPicture,
                maxCodedWidth, maxCodedHeight,
                maxFrameWidth, maxFrameHeight,
                frameBuffer, maxCodedWidth, maxCodedHeight );

            /* TODO: frame counter */
            if( planarOutput )
            {
                TBWriteFrameMD5Sum(fpOut, frameBuffer,
                    maxCodedWidth*
                    maxCodedHeight*3/2, 1);
            }
            else
            {
            /* TODO: frame counter */
                TBWriteFrameMD5Sum(fpOut, pOutput->pOutputPicture,
                        MB_MULTIPLE(pOutput->frameWidth)*
                        MB_MULTIPLE(pOutput->frameHeight)*3/2, 1);
            }

#if 0
            /* tiled */
            if (tiledOutput)
            {
                /* round up to next multiple of 16 */
                u32 tmpWidth = MB_MULTIPLE(pOutput->frameWidth);
                u32 tmpHeight = MB_MULTIPLE(pOutput->frameHeight);
                  
                TbWriteTiledOutput(fTiledOutput, pOutput->pOutputPicture, tmpWidth >> 4, tmpHeight >> 4, 
                        numberOfWrittenFrames, 1 /* write md5sum */, 1 /* semi-planar data */ );
            }
#endif

#else                
            if( !planarOutput )
            {
                DWLfwrite(pOutput->pOutputPicture, 1, 
                    MB_MULTIPLE(pOutput->frameWidth)*
                    MB_MULTIPLE(pOutput->frameHeight)*3/2, fpOut);
            }
            else
            {
                u32 tmp = MB_MULTIPLE(pOutput->frameWidth)*
                    MB_MULTIPLE(pOutput->frameHeight), i;
                DWLfwrite(pOutput->pOutputPicture, 1, tmp, fpOut);
                for(i = 0; i < tmp / 4; i++)
                    DWLfwrite(pOutput->pOutputPicture + tmp + i * 2, 1, 1, fpOut);
                for(i = 0; i < tmp / 4; i++)
                    DWLfwrite(pOutput->pOutputPicture + tmp + 1 + i * 2, 1, 1, fpOut);
            }

            if( enableFramePicture )
            {
                u32 tmp = maxCodedWidth * maxCodedHeight, i;
                FramePicture( pOutput->pOutputPicture,
                    maxCodedWidth, maxCodedHeight,
                    maxFrameWidth, maxFrameHeight,
                    frameBuffer, maxCodedWidth, maxCodedHeight );

                DWLfwrite(frameBuffer, 1, 3*tmp/2, frameOut);
                /*
                for(i = 0; i < tmp / 4; i++)
                    DWLfwrite(frameBuffer + tmp + i * 2, 1, 1, frameOut);
                for(i = 0; i < tmp / 4; i++)
                    DWLfwrite(frameBuffer + tmp + 1 + i * 2, 1, 1, frameOut);
                    */
            }

#if 0
            /* tiled */
            if (tiledOutput)
            {
                /* round up to next multiple of 16 */
                u32 tmpWidth = MB_MULTIPLE(pOutput->frameWidth + 15);
                u32 tmpHeight = MB_MULTIPLE(pOutput->frameHeight + 15);
                  
                TbWriteTiledOutput(fTiledOutput, pOutput->pOutputPicture, tmpWidth >> 4, tmpHeight >> 4, 
                        numberOfWrittenFrames, 0 /* not write md5sum */, 1 /* semi-planar data */ );
            }
#endif

#endif

            numberOfWrittenFrames++;
            
#ifndef ASIC_TRACE_SUPPORT
           pOutput->pOutputPicture = buffer;
#endif
        }
#endif
        if(rasterScan)
            free(rasterScan);
		
		/* update frame number */
		frameNumber++;
	}

    DWLFreeLinear(((DecContainer *) g_RVDecInst)->dwl, &streamMem);

    DWLfclose(fpOut);
    if(fTiledOutput)
        DWLfclose(fTiledOutput);

    if (pic_big_endian != NULL)
        free(pic_big_endian);
}

void rv_display(const char *filename,
                char *filenamePpCfg,
                int startframeid, 
                int maxnumber, 
                int iswritefile, 
                int isdisplay, 
                int displaymode, 
                int rotationmode, 
                int islog2std, 
                int islog2file,
                int blayerenable,
                int errconceal,
                int randomerror)
{
	HX_RESULT           retVal         = HXR_OK;
	DWLFile*            fpIn           = HXNULL;
	DWLFile*            fpInPpCfg      = HXNULL;
	INT32               lBytesRead     = 0;
	UINT32              ulNumStreams   = 0;
	UINT32              i              = 0;
	UINT16              usStreamNum    = 0;
	UINT32              ulOutFrameSize = 0;
	UINT32              ulFramesPerSec = 0;
	rm_parser*          pParser        = HXNULL;
	rm_stream_header*   pHdr           = HXNULL;
	rm_packet*          pPacket        = HXNULL;
	rv_depack*          pDepack        = HXNULL;
	rv_decode*          pDecode        = HXNULL;
	rv_format_info*     pInfo          = HXNULL;
	rm2yuv_info         info;
	BYTE                ucBuf[RM2YUV_INITIAL_READ_SIZE];
	char                rv_outfilename[256];

#ifdef RV_TIME_TRACE
	UINT32              ulStartTime = TIMER_GET_NOW_TIME();
	UINT32              ulStopTime = 0;
#endif

	RvDecRet rvRet;	
	/*RVDecInstInit RVInstInit;*/
	RvDecPicture decOutput;
	RvDecInfo decInfo;

#ifdef ASIC_TRACE_SUPPORT
    if (!openTraceFiles())
        DEBUG_PRINT(("UNABLE TO OPEN TRACE FILE(S)\n"));
#endif

    /* Initialize all static global variables */
	g_RVDecInst = NULL;
	g_iswritefile = iswritefile;
	g_isdisplay = isdisplay;
	g_displaymode = displaymode;
	g_rotationmode = rotationmode;
	g_bfirstdisplay = 0;
	g_blayerenable = blayerenable;

#ifdef RV_ERROR_SIM
	g_randomerror = randomerror;
#endif

	islog2std = islog2std;
	islog2file = islog2file;

	RV_Time_Init(islog2std, islog2file);

	DWLmemset(rv_outfilename, 0, sizeof(rv_outfilename));

	/* NULL out the info */
	DWLmemset(info.filePath, 0, sizeof(info.filePath));
	info.fpOut             = HXNULL;
	info.pDepack           = HXNULL;
	info.pDecode           = HXNULL;
	info.pOutFrame         = HXNULL;
	info.ulWidth           = 0;
	info.ulHeight          = 0;	
	info.ulOutFrameSize    = 0;
	info.ulNumInputFrames  = 0;
	info.ulNumOutputFrames = 0;
	info.ulMaxNumDecFrames = maxnumber;
	info.ulStartFrameID    = startframeid;
	info.bDecEnd           = 0;
	info.ulTotalTime       = 0;

	/* Open the input file */
#ifdef WIN32
	fpIn = DWLfopen((const char*) filename, "rb");
#else
	fpIn = DWLfopen((const char*) filename, "r");
#endif
	
	DEBUG_PRINT(("filename=%s\n",filename));
	if (!fpIn)
	{
		ERROR_PRINT(("Could not open %s for reading.\n", filename));
		goto cleanup;
	}
	
	/* Read the first few bytes of the file */
	lBytesRead = (INT32) DWLfread((void*) ucBuf, 1, RM2YUV_INITIAL_READ_SIZE, fpIn);
	if (lBytesRead != RM2YUV_INITIAL_READ_SIZE)
	{
		ERROR_PRINT(("Could not read %d bytes at the beginning of %s\n",
			RM2YUV_INITIAL_READ_SIZE, filename));
		goto cleanup;
	}
	/* Seek back to the beginning */
	DWLfseek(fpIn, 0, DWL_SEEK_SET);
	
    /* Make sure this is an .rm file */
    if (!rm_parser_is_rm_file(ucBuf, RM2YUV_INITIAL_READ_SIZE))
	{
		ERROR_PRINT(("%s is not an .rm file.\n", filename));
        raw_mode(fpIn,
			filenamePpCfg,
            startframeid,
            maxnumber,
            iswritefile,
            isdisplay, 
            displaymode,
            rotationmode,
            islog2std,
            islog2file,
            blayerenable,
            errconceal,
            randomerror);
		goto cleanup;
	}

	/* Create the parser struct */
	pParser = rm_parser_create2(HXNULL,
		rm2yuv_error,
		HXNULL,
		rm_memory_malloc,
		rm_memory_free);
	if (!pParser)
	{
		goto cleanup;
	}

	/* Set the file into the parser */
	retVal = rm_parser_init_io(pParser, 
		(void *)fpIn,
		(rm_read_func_ptr)rm_io_read,
		(rm_seek_func_ptr)rm_io_seek);
	if (retVal != HXR_OK)
	{
		goto cleanup;
	}

	/* Read all the headers at the beginning of the .rm file */
	retVal = rm_parser_read_headers(pParser);
	if (retVal != HXR_OK)
	{
		goto cleanup;
	}

	/* Get the number of streams */
	ulNumStreams = rm_parser_get_num_streams(pParser);
	if (ulNumStreams == 0)
	{
		ERROR_PRINT(("Error: rm_parser_get_num_streams() returns 0\n"));
		goto cleanup;
	}

	/* Now loop through the stream headers and find the video */
	for (i = 0; i < ulNumStreams && retVal == HXR_OK; i++)
	{
		retVal = rm_parser_get_stream_header(pParser, i, &pHdr);
		if (retVal == HXR_OK)
		{
			if (rm_stream_is_realvideo(pHdr))
			{
				usStreamNum = (UINT16) i;
				break;
			}
			else
			{
				/* Destroy the stream header */
				rm_parser_destroy_stream_header(pParser, &pHdr);
			}
		}
	}
	
    /* Do we have a RealVideo stream in this .rm file? */
	if (!pHdr)
	{
		ERROR_PRINT(("There is no RealVideo stream in this file.\n"));
		goto cleanup;
	}

	/* Create the RealVideo depacketizer */
	pDepack = rv_depack_create2_ex((void*) &info,
		rv_frame_available,
		HXNULL,
		rm2yuv_error,
		HXNULL,
		rm_memory_malloc,
		rm_memory_free,
		rm_memory_create_ncnb,
		rm_memory_destroy_ncnb);
	if (!pDepack)
	{
		goto cleanup;
	}

	/* Assign the rv_depack pointer to the info struct */
	info.pDepack = pDepack;

	/* Initialize the RV depacketizer with the RealVideo stream header */
	retVal = rv_depack_init(pDepack, pHdr);
	if (retVal != HXR_OK)
	{
		goto cleanup;
	}

	/* Get the bitstream header information, create rv_infor_format struct and init it */
	retVal = rv_depack_get_codec_init_info(pDepack, &pInfo);
	if (retVal != HXR_OK)
	{
		goto cleanup;
	}

	/*
	* Print out the width and height so the user
	* can input this into their YUV-viewing program.
	*/
	//DEBUG_PRINT(("Video in %s has dimensions %lu x %lu pixels (width x height)\n",
	//	(const char*) filename, info.ulWidth, info.ulHeight));

	/*
	* Get the frames per second. This value is in 32-bit
	* fixed point, where the upper 16 bits is the integer
	* part of the fps, and the lower 16 bits is the fractional
	* part. We're going to truncate to integer, so all
	* we need is the upper 16 bits.
	*/
	ulFramesPerSec = pInfo->ufFramesPerSecond >> 16;

	/* Create an rv_decode object */
	pDecode = rv_decode_create2(HXNULL,
		rm2yuv_error,
		HXNULL,
		rm_memory_malloc,
		rm_memory_free);
	if (!pDecode)
	{
		goto cleanup;
	}

	/* Assign the decode object into the rm2yuv_info struct */
	info.pDecode = pDecode;

	/* Init the rv_decode object */
	retVal = rv_decode_init(pDecode, pInfo);
	if (retVal != HXR_OK)
	{
		goto cleanup;
	}

	/* Get the size of an output frame */
	ulOutFrameSize = rv_decode_max_output_size(pDecode);
	if (!ulOutFrameSize)
	{
		goto cleanup;
	}
	info.ulOutFrameSize = ulOutFrameSize;

	DEBUG_PRINT(("Start decode ...\n"));

	/* Init RV decode instance */
	/*RVInstInit.isrv8 = pDecode->bIsRV8;*/
	/*RVInstInit.pctszSize = pDecode->ulPctszSize;*/
	/*RVInstInit.pSizes = pDecode->pDimensions;*/
	/*RVInstInit.maxwidth = pDecode->ulLargestPels;*/
	/*RVInstInit.maxheight = pDecode->ulLargestLines;*/

    maxCodedWidth = pDecode->ulLargestPels;
    maxCodedHeight = pDecode->ulLargestLines;
    maxFrameWidth = MB_MULTIPLE( maxCodedWidth );
    maxFrameHeight = MB_MULTIPLE( maxCodedHeight );

	START_SW_PERFORMANCE;
    decsw_performance();
	rvRet = RvDecInit(&g_RVDecInst, /*&RVInstInit*/0,
        pDecode->ulPctszSize, (u32*)pDecode->pDimensions, !pDecode->bIsRV8,
        pDecode->ulLargestPels, pDecode->ulLargestLines,
        numFrameBuffers, tiledOutput );
    END_SW_PERFORMANCE;
    decsw_performance();
	if (rvRet != RVDEC_OK)
	{
		ERROR_PRINT(("RVDecInit fail! \n"));
		goto cleanup;
	}

#ifdef PP_PIPELINE_ENABLED
    /* Initialize the post processer. If unsuccessful -> exit */
    if(pp_startup
       (filenamePpCfg, g_RVDecInst, PP_PIPELINED_DEC_TYPE_RV, &tbCfg) != 0)
    {
        DEBUG_PRINT(("PP INITIALIZATION FAILED\n"));
        goto cleanup;
    }

    if(pp_update_config
       (g_RVDecInst, PP_PIPELINED_DEC_TYPE_RV, &tbCfg) == CFG_UPDATE_FAIL)

    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
        goto cleanup;
    }
#endif

    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_LATENCY,
                   latency_comp);
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_CLK_GATE_E,
                   clock_gating);
    /*
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_OUT_TILED_E,
                   output_format);
                   */
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_OUT_ENDIAN,
                   output_picture_endian);
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_MAX_BURST,
                   bus_burst_length);
    if ((DWLReadAsicID() >> 16) == 0x8170U)
    {
        SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_PRIORITY_MODE,
                       asic_service_priority);
    }
    SetDecRegister(((DecContainer *) g_RVDecInst)->rvRegs, HWIF_DEC_DATA_DISC_E,
                   data_discard);

	/* Read what kind of stream is coming */
    START_SW_PERFORMANCE;
    decsw_performance();
	rvRet = RvDecGetInfo(g_RVDecInst, &decInfo);
    END_SW_PERFORMANCE;
    decsw_performance();
    if(rvRet)
	{
		printDecodeReturn(rvRet);
	}

#ifdef PP_PIPELINE_ENABLED

    /* If unspecified at cmd line, use minimum # of buffers, otherwise
     * use specified amount. */
    if(numFrameBuffers == 0)
        pp_number_of_buffers(decInfo.multiBuffPpSize);
    else
        pp_number_of_buffers(numFrameBuffers);

#endif

    TBInitializeRandom(seedRnd);

	/* Fill in the width and height */
	info.ulWidth  = (pDecode->ulLargestPels+15)&0xFFFFFFF0;
	info.ulHeight = (pDecode->ulLargestLines+15)&0xFFFFFFF0;

	/* Parse the output file path */
	if(g_iswritefile)
	{
		char flnm[0x100] = "";
		parse_path(filename, info.filePath, NULL);
	}

#ifdef RV_DISPLAY
	if (g_displaymode)
	{
		Panel_GetSize(&g_dstimg);
	}
	else
	{
		g_dstimg.cx = 0;
		g_dstimg.cy = 0;
	}
#endif

	RV_Time_Full_Reset();

	/* Now keep getting packets until we receive an error */
	while (retVal == HXR_OK && !info.bDecEnd)
	{
		/* Get the next packet */
		retVal = rm_parser_get_packet(pParser, &pPacket);
		if (retVal == HXR_OK)
		{
			/* Is this a video packet? */
			if (pPacket->usStream == usStreamNum)
			{
				/*
				* Put the packet into the depacketizer. When frames
				* are available, we will get a callback to
				* rv_frame_available().
				*/
				retVal = rv_depack_add_packet(pDepack, pPacket);
			}
			/* Destroy the packet */
			rm_parser_destroy_packet(pParser, &pPacket);
		}

        if (maxNumPics && picsDecoded >= maxNumPics)
            break;
	}

	TIME_PRINT(("\n"));

	/* Output the last picture in decoded picture memory pool */
	START_SW_PERFORMANCE;
    decsw_performance();
	rvRet = RvDecNextPicture(g_RVDecInst, &decOutput, 1);
    END_SW_PERFORMANCE;
    decsw_performance();
	if (!usePeekOutput && rvRet == RVDEC_PIC_RDY)
	{
        info.pDecode->pOutputParams.width = decOutput.codedWidth;
        info.pDecode->pOutputParams.height = decOutput.codedHeight;
        info.pDecode->pOutputParams.notes &= ~RV_DECODE_DONT_DRAW;

		/* update frame number */
		frameNumber++;
        
		rv_decode_frame_flush(&info, &decOutput);
	}

    /* Display results */
	DEBUG_PRINT(("Video stream in '%s' decoding complete: %lu input frames, %lu output frames\n",
		(const char*) filename, info.ulNumInputFrames, info.ulNumOutputFrames));

#ifdef RV_TIME_TRACE
	ulStopTime = TIMER_GET_NOW_TIME();
	if (ulStopTime <= ulStartTime)
		info.ulTotalTime = (TIMER_MAX_COUNTER-ulStartTime+ulStopTime)/TIMER_FACTOR_MSECOND;
	else
		info.ulTotalTime = (ulStopTime-ulStartTime)/TIMER_FACTOR_MSECOND;

	TIME_PRINT(("Video stream in '%s' decoding complete:\n", (const char*) filename));
	TIME_PRINT(("    width is %d, height is %d\n",info.ulWidth, info.ulHeight));
	TIME_PRINT(("    %lu input frames, %lu output frames\n",info.ulNumInputFrames, info.ulNumOutputFrames));	
	TIME_PRINT(("    total time is %lu ms, avarage time is %lu ms, fps is %6.2f\n",
		info.ulTotalTime, info.ulTotalTime/info.ulNumOutputFrames, 1000.0/(info.ulTotalTime/info.ulNumOutputFrames)));
#endif

	/* If the error was just a normal "out-of-packets" error,
	then clean it up */
	if (retVal == HXR_NO_DATA)
	{
		retVal = HXR_OK;
	}

cleanup:
		
	/* Destroy the codec init info */
	if (pInfo)
	{
		rv_depack_destroy_codec_init_info(pDepack, &pInfo);
	}
	/* Destroy the depacketizer */
	if (pDepack)
	{
		rv_depack_destroy(&pDepack);
	}
	/* If we have a packet, destroy it */
	if (pPacket)
	{
		rm_parser_destroy_packet(pParser, &pPacket);
	}
	/* Destroy the stream header */
	if (pHdr)
	{
		rm_parser_destroy_stream_header(pParser, &pHdr);
	}
	/* Destroy the rv_decode object */
	if (pDecode)
	{
		rv_decode_destroy(pDecode);
		pDecode = HXNULL;
	}
	/* Destroy the parser */
	if (pParser)
	{
		rm_parser_destroy(&pParser);
	}
	/* Close the input file */
	if (fpIn)
	{
		DWLfclose(fpIn);
		fpIn = HXNULL;
	}
	/* Close the output file */
	if (info.fpOut)
	{
		DWLfclose(info.fpOut);
		info.fpOut = HXNULL;
	}

    if(fTiledOutput)
    {
        DWLfclose(fTiledOutput);
        fTiledOutput = NULL;
    }

	/* Close the time log file */
	if (g_fpTimeLog)
	{
		DWLfclose(g_fpTimeLog);
		g_fpTimeLog = HXNULL;
	}		
	/* Release the decoder instance */
	if (g_RVDecInst)
	{
        START_SW_PERFORMANCE;
        decsw_performance();
		RvDecRelease(g_RVDecInst);
        END_SW_PERFORMANCE;
        decsw_performance();
	}

    FINALIZE_SW_PERFORMANCE;
    
#ifdef ASIC_TRACE_SUPPORT
    trace_SequenceCtrl(hwDecPicCount, 0);
    trace_RvDecodingTools();
    closeTraceFiles();
#endif

}

/*------------------------------------------------------------------------------
function
	rv_frame_available()

purpose
	If enough data have been extracted from rm packte(s) for an entire frame, 
	this function will be called to decode the available stream data.
------------------------------------------------------------------------------*/

HX_RESULT rv_frame_available(void* pAvail, UINT32 ulSubStreamNum, rv_frame* pFrame)
{	
	HX_RESULT retVal = HXR_FAIL;
	RvDecPicture decOutput;

	if (pAvail && pFrame)
	{
		/* Get the info pointer */
		rm2yuv_info* pInfo = (rm2yuv_info*) pAvail;
		if (pInfo->pDepack && pInfo->pDecode)
		{
			/* Put the frame into rv_decode */
			retVal = rv_decode_stream_input(pInfo->pDecode, pFrame);
			if (HX_SUCCEEDED(retVal))
			{
				/* Increment the number of input frames */
				pInfo->ulNumInputFrames++;

				/* Decode frames until there aren't any more */
				do
				{
					/* skip all B-Frames before Frame g_startframeid */
					if (pInfo->pDecode->pInputFrame->usSequenceNum < pInfo->ulStartFrameID
						&& pInfo->pDecode->pInputFrame->ucCodType == 3/*RVDEC_TRUEBPIC*/)
					{
						retVal = HXR_OK;
						break;
					}
					retVal = rv_decode_stream_decode(pInfo->pDecode, &decOutput);
					RV_Time_Full_Pause();
					RV_Time_Full_Reset();

					if (HX_SUCCEEDED(retVal))
					{	
						rv_decode_frame_flush(pInfo, &decOutput);
					}
					else
					{
						pInfo->bDecEnd = 1;
					}
				}
				while (HX_SUCCEEDED(retVal) && rv_decode_more_frames(pInfo->pDecode));
			}
foo:
			rv_depack_destroy_frame(pInfo->pDepack, &pFrame);
		}
	}

	return retVal;
}


/*------------------------------------------------------------------------------
function
	rv_decode_stream_decode()

purpose
	Calls the decoder backend (HW) to decode issued frame 
	and output a decoded frame if any possible.

return
	Returns zero on success, negative result indicates failure.

------------------------------------------------------------------------------*/
HX_RESULT rv_decode_stream_decode(rv_decode *pFrontEnd, RvDecPicture *pOutput)
{
	HX_RESULT retVal = HXR_FAIL;
	RvDecRet rvRet;
	RvDecInput decInput;
	RvDecInfo decInfo;
	RvDecOutput decOutput;
	DecContainer *pDecCont = (DecContainer *)g_RVDecInst;
    RvDecPicture tmpOut;
	const void *dwl = pDecCont->dwl;
    u32 moreData = 1;
	UINT32 size = 0;
	UINT32 i = 0;
    u32 packetLoss = 0;
    RvDecSliceInfo sliceInfo[128];
    u32 ret = 0;

    /* If enabled, loose the packets (skip this packet first though) */
    if(streamPacketLoss)
    {

        ret =
            TBRandomizePacketLoss(tbCfg.tbParams.streamPacketLoss, &packetLoss);
        if(ret != 0)
        {
            DEBUG_PRINT(("RANDOM STREAM ERROR FAILED\n"));
            return 0;
        }
    }

    if( packetLoss )
        return HXR_OK;
	
#ifdef RV_ERROR_SIM
	DWLFile *fp = NULL;
	char flnm[0x100] = "";
	
	if (g_randomerror)
	{
		CreateError((char *)(pFrontEnd->pInputFrame->pData + 100), (int)(pFrontEnd->pInputParams.dataLength), 20);
		sprintf(flnm, "stream_%d.bin", pFrontEnd->pInputFrame->usSequenceNum);
		fp = DWLfopen(flnm, "w");
		if(fp)
		{
			DWLfwrite(pFrontEnd->pInputFrame->pData, 1, pFrontEnd->pInputParams.dataLength, fp);
			DWLfclose(fp);
		}
	}
#endif

    decInput.skipNonReference = skipNonReference;
	decInput.pStream = pFrontEnd->pInputFrame->pData;
	decInput.streamBusAddress = pFrontEnd->pInputFrame->ulDataBusAddress;
	decInput.picId = pFrontEnd->pInputFrame->usSequenceNum;
	decInput.timestamp = pFrontEnd->pInputFrame->ulTimestamp;
	decInput.dataLen = pFrontEnd->pInputParams.dataLength;
	decInput.sliceInfoNum = pFrontEnd->pInputParams.numDataSegments+1;
	/* Configure sliceInfo, which will be accessed by HW */
	size = sizeof(sliceInfo);
	decInput.pSliceInfo = sliceInfo;
	DWLmemset(decInput.pSliceInfo, 0, size);
	for (i = 0; i < decInput.sliceInfoNum; i++)
	{
		sliceInfo[i].offset = pFrontEnd->pInputParams.pDataSegments[i].ulOffset;
        sliceInfo[i].isValid = pFrontEnd->pInputParams.pDataSegments[i].bIsValid;
	}

    if (streamBitSwap)
    {
        if((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt)
        {
            /* Picture needs to be ready before corrupting next picture */
            if(picRdy && corruptedBytes <= 0)
            {
                ret = TBRandomizeBitSwapInStream(decInput.pStream,
                decInput.dataLen, tbCfg.tbParams.streamBitSwap);
                if(ret != 0)
                {
                    printf("RANDOM STREAM ERROR FAILED\n");
                    return 0;
                }

                corruptedBytes = decInput.dataLen;
                printf("corruptedBytes %d\n", corruptedBytes);
            }
        }
    }

    do
    {
        moreData = 0;

        printf("\n%d\n",decInput.timestamp);
		printf("\n%d\n",decInput.picId);

    	RV_Time_Dec_Reset();
        START_SW_PERFORMANCE;
        decsw_performance();
    	rvRet = RvDecDecode(pDecCont, &decInput, &decOutput);
        END_SW_PERFORMANCE;
        decsw_performance();
		RV_Time_Dec_Pause(0,0);

    	printDecodeReturn(rvRet);
        pFrontEnd->pOutputParams.notes &= ~RV_DECODE_MORE_FRAMES;

        switch(rvRet)
        {
            case RVDEC_HDRS_RDY:
                TBSetRefbuMemModel( &tbCfg, 
                    ((DecContainer *) g_RVDecInst)->rvRegs,
                    &((DecContainer *) g_RVDecInst)->refBufferCtrl );
                moreData = 1; 
				
                /* Read what kind of stream is coming */
                START_SW_PERFORMANCE;
                decsw_performance();
                rvRet = RvDecGetInfo(g_RVDecInst, &decInfo);
                END_SW_PERFORMANCE;
                decsw_performance();
                if(rvRet)
                {
                    printDecodeReturn(rvRet);
                }

#ifdef PP_PIPELINE_ENABLED

                /* If unspecified at cmd line, use minimum # of buffers, otherwise
                 * use specified amount. */
                if(numFrameBuffers == 0)
                    pp_number_of_buffers(decInfo.multiBuffPpSize);
                else
                    pp_number_of_buffers(numFrameBuffers);

#endif            
     	        printf("*********************** RvDecGetInfo() **********************\n");
                printf("decInfo.frameWidth %d\n",(decInfo.frameWidth << 4));
                printf("decInfo.frameHeight %d\n",(decInfo.frameHeight << 4));
                printf("decInfo.codedWidth %d\n",decInfo.codedWidth);
                printf("decInfo.codedHeight %d\n",decInfo.codedHeight);
                if(decInfo.outputFormat == RVDEC_SEMIPLANAR_YUV420)
                    printf("decInfo.outputFormat RVDEC_SEMIPLANAR_YUV420\n");
                else
                    printf("decInfo.outputFormat RVDEC_TILED_YUV420\n");
                printf("*************************************************************\n");

                hdrsRdy = 1;
                break;
            
	    case RVDEC_STRM_PROCESSED:
        case RVDEC_NONREF_PIC_SKIPPED:
                /* In case picture size changes we need to reinitialize memory model */
                TBSetRefbuMemModel( &tbCfg, 
                    ((DecContainer *) g_RVDecInst)->rvRegs,
                    &((DecContainer *) g_RVDecInst)->refBufferCtrl );
                START_SW_PERFORMANCE;
                decsw_performance();
                rvRet = RvDecNextPicture(g_RVDecInst, pOutput, 0);
                END_SW_PERFORMANCE;
                decsw_performance();
                if (rvRet == RVDEC_PIC_RDY)
                {
                    pFrontEnd->pOutputParams.width = pOutput->codedWidth;
                    pFrontEnd->pOutputParams.height = pOutput->codedHeight;
                    pFrontEnd->pOutputParams.notes &= ~RV_DECODE_DONT_DRAW;
                    pFrontEnd->pOutputParams.notes |= RV_DECODE_MORE_FRAMES;
                    retVal = HXR_OK;

					/* update frame number */
					frameNumber++;
                }
                else if (rvRet == RVDEC_OK)
                {
                    pFrontEnd->pOutputParams.notes |= RV_DECODE_DONT_DRAW;
                    retVal = HXR_OK;
                }
                else
                {
                    retVal = HXR_FAIL;
                }
                
                break;

            case RVDEC_OK:
            case RVDEC_PIC_DECODED:
                picsDecoded++;
                if (usePeekOutput)
                {
                    rvRet = RvDecPeek(g_RVDecInst, pOutput);
                }
                else
                {
                    START_SW_PERFORMANCE;
                    decsw_performance();
                    rvRet = RvDecNextPicture(g_RVDecInst, pOutput, 0);
                    END_SW_PERFORMANCE;
                    decsw_performance();
                }

                if (rvRet == RVDEC_PIC_RDY)
                {
                    pFrontEnd->pOutputParams.width = pOutput->codedWidth;
                    pFrontEnd->pOutputParams.height = pOutput->codedHeight;
                    pFrontEnd->pOutputParams.notes &= ~RV_DECODE_DONT_DRAW;
                    retVal = HXR_OK;
                    
                    /* update frame number */
                    frameNumber++;
                }
                else if (rvRet == RVDEC_OK)
                {
                    pFrontEnd->pOutputParams.notes |= RV_DECODE_DONT_DRAW;
                    retVal = HXR_OK;
                }
                else
                {
                    DEBUG_PRINT(("Error, Decode end!\n"));
                    retVal = HXR_FAIL;
                }

                if (usePeekOutput)
                    rvRet = RvDecNextPicture(g_RVDecInst, &tmpOut, 0);
                    
                picRdy = 1;
                corruptedBytes = 0;
                break;
            default:
                /*
                * RVDEC_NOT_INITIALIZED
                * RVDEC_PARAM_ERROR
                * RVDEC_STRM_ERROR
                * RVDEC_STRM_PROCESSED
                * RVDEC_SYSTEM_ERROR
                * RVDEC_MEMFAIL			 
                * RVDEC_HW_TIMEOUT
                * RVDEC_HW_RESERVED
                * RVDEC_HW_BUS_ERROR
                */
                DEBUG_PRINT(("Fatal Error, Decode end!\n"));
                retVal = HXR_FAIL;
                break;
        }
    }
    while(moreData);

	return retVal;
}

/*------------------------------------------------------------------------------
function
	rv_decode_frame_flush()

purpose
	update rm2yuv_info
	write the decoded picture to a local file or display on screen, if neccesary
------------------------------------------------------------------------------*/
void rv_decode_frame_flush(rm2yuv_info *pInfo, RvDecPicture *pOutput)
{
	UINT32 ulWidth, ulHeight, ulOutFrameSize;

	if (pInfo == NULL || pOutput == NULL)
		return;

	/* Is there a valid output frame? */
	if (rv_decode_frame_valid(pInfo->pDecode))
	{
        u8 *rasterScan = NULL;

        DEBUG_PRINT(("picid= %d timestamp= %d\n", pOutput->picId, 0));

        /* Convert back to raster scan format if decoder outputs 
         * tiled format */
        if(pOutput->outputFormat && convertTiledOutput)
        {
            rasterScan = (u8*)malloc(pOutput->frameWidth*
                pOutput->frameHeight*3/2);
            if(!rasterScan)
            {
                fprintf(stderr, "error allocating memory for tiled"
                    "-->raster conversion!\n");
                return;
            }

            TBTiledToRaster( pOutput->outputFormat, DEC_DPB_DEFAULT, 
                pOutput->pOutputPicture, rasterScan, 
                pOutput->frameWidth, pOutput->frameHeight );
            TBTiledToRaster( pOutput->outputFormat, DEC_DPB_DEFAULT, 
                    pOutput->pOutputPicture+
                    pOutput->frameWidth*
                    pOutput->frameHeight, 
                rasterScan+pOutput->frameWidth*pOutput->frameHeight, 
                pOutput->frameWidth, 
                pOutput->frameHeight/2 );
            pOutput->pOutputPicture = rasterScan;
        }

		/* Increment the number of output frames */
		pInfo->ulNumOutputFrames++;

		/* Get the dimensions and size of output frame */
		rv_decode_get_output_dimensions(pInfo->pDecode, &ulWidth, &ulHeight);
		ulOutFrameSize = rv_decode_get_output_size(pInfo->pDecode);

		/* Check to see if dimensions have changed */
		if (ulWidth != pInfo->ulWidth || ulHeight != pInfo->ulHeight )
		{
			DEBUG_PRINT(("Warning: YUV output dimensions changed from "
				"%lux%lu to %lux%lu at frame #: %lu (in decoding order)\n",
				pInfo->ulWidth,	pInfo->ulHeight, ulWidth, ulHeight, pOutput->picId));
			/* Don't bother resizing to display dimensions */
			pInfo->ulWidth = ulWidth;
			pInfo->ulHeight = ulHeight;
			pInfo->ulOutFrameSize = ulOutFrameSize;
            /* close the opened output file handle if necessary */
            /*
            if (pInfo->fpOut)
            {
                DWLfclose(pInfo->fpOut);
                pInfo->fpOut = HXNULL;
            }
            */
            /* clear the flag of first display */
            g_bfirstdisplay = 0;
		}
		
        if(g_iswritefile)
		{
			if (pInfo->fpOut == HXNULL)
			{
				char flnm[0x100] = "";
                if(enableFramePicture && !frameOut)
                {
    				sprintf(flnm, "frame.yuv",
    					/*pInfo->filePath,*/ maxCodedWidth, maxCodedHeight);
                    frameOut = DWLfopen(flnm, "w");
                    frameBuffer = (u8*)malloc(maxFrameWidth*maxFrameHeight*3/2);
                }
                if( *outFileName == '\0' )
                {
                    sprintf(outFileName, "out_w%dh%d.yuv",
                        /*pInfo->filePath,*/ 
                        MB_MULTIPLE(pInfo->ulWidth), 
                        MB_MULTIPLE(pInfo->ulHeight));
                }
				pInfo->fpOut = DWLfopen(outFileName, "w");

#if 0
                if(fTiledOutput == NULL && tiledOutput)
                {
                    /* open output file for writing, can be disabled with define.
                     * If file open fails -> exit */
                    if(strcmp(outFileNameTiled, "none") != 0)
                    {
                        fTiledOutput = fopen(outFileNameTiled, "wb");
                        if(fTiledOutput == NULL)
                        {
                            printf("UNABLE TO OPEN TILED OUTPUT FILE\n");
                            return;
                        }
                    }
                }

#endif
			}
#ifdef PP_PIPELINE_ENABLED
		    HandlePpOutput(frameNumber - 1, *pOutput, g_RVDecInst);
#else
			if (pInfo->fpOut)
			{
#ifndef ASIC_TRACE_SUPPORT
                u8* buffer = pOutput->pOutputPicture;
                if(output_picture_endian == DEC_X170_BIG_ENDIAN)
                {
                    u32 frameSize = MB_MULTIPLE(pInfo->ulWidth)*MB_MULTIPLE(pInfo->ulHeight)*3/2;
                    if(pic_big_endian_size != frameSize)
                    {
                        if(pic_big_endian != NULL)
                            free(pic_big_endian);

                        pic_big_endian = (u8 *) malloc(frameSize);
                        if(pic_big_endian == NULL)
                        {
                            DEBUG_PRINT(("MALLOC FAILED @ %s %d", __FILE__, __LINE__));
                            if(rasterScan)
                                free(rasterScan);
                            return;
                        }
            
                        pic_big_endian_size = frameSize;
                    }

                    memcpy(pic_big_endian, buffer, frameSize);
                    TbChangeEndianess(pic_big_endian, frameSize);
                    pOutput->pOutputPicture = pic_big_endian;
                }
#endif
#ifndef MD5SUM
                if( enableFramePicture )
                {
                    u32 tmp = maxCodedWidth * maxCodedHeight, i;
                    FramePicture( pOutput->pOutputPicture,
                        pInfo->ulWidth, pInfo->ulHeight,
                        MB_MULTIPLE(pInfo->ulWidth), 
                        MB_MULTIPLE(pInfo->ulHeight), 
                        frameBuffer, maxCodedWidth, maxCodedHeight );

                    DWLfwrite(frameBuffer, 1, 3*tmp/2, frameOut);
                    /*
                    for(i = 0; i < tmp / 4; i++)
                        DWLfwrite(frameBuffer + tmp + i * 2, 1, 1, frameOut);
                    for(i = 0; i < tmp / 4; i++)
                        DWLfwrite(frameBuffer + tmp + 1 + i * 2, 1, 1, frameOut);
                        */
                }
#endif

#ifdef MD5SUM
                {
                    u32 tmp = maxCodedWidth * maxCodedHeight, i;
                    FramePicture( pOutput->pOutputPicture,
                        pInfo->ulWidth, pInfo->ulHeight,
                        MB_MULTIPLE(pInfo->ulWidth), 
                        MB_MULTIPLE(pInfo->ulHeight), 
                        frameBuffer, maxCodedWidth, maxCodedHeight );

                    /* TODO: frame counter */
                    if(planarOutput)
                    {
                        TBWriteFrameMD5Sum(pInfo->fpOut, frameBuffer,
                            maxCodedWidth*
                            maxCodedHeight*3/2, 
                            pInfo->ulNumOutputFrames);
                    }
                    else
                    {
                    TBWriteFrameMD5Sum(pInfo->fpOut, pOutput->pOutputPicture, 
                            MB_MULTIPLE(pInfo->ulWidth)*
                            MB_MULTIPLE(pInfo->ulHeight)*3/2,
                            pInfo->ulNumOutputFrames); 
                    }                       
                }


#if 0
                /* tiled */
                if (tiledOutput)
                {
                    /* round up to next multiple of 16 */
                    u32 tmpWidth = MB_MULTIPLE(pInfo->ulWidth);
                    u32 tmpHeight = MB_MULTIPLE(pInfo->ulHeight);
                      
                    TbWriteTiledOutput(fTiledOutput, pOutput->pOutputPicture, tmpWidth >> 4, tmpHeight >> 4, 
                            numberOfWrittenFrames, 1 /* write md5sum */, 1 /* semi-planar data */ );
                }
#endif

#else                
                if( !planarOutput )
                {
                    DWLfwrite(pOutput->pOutputPicture, 1, 
                        MB_MULTIPLE(pInfo->ulWidth)*
                        MB_MULTIPLE(pInfo->ulHeight)*3/2,
                        pInfo->fpOut);
                }
                else
                {
                    u32 tmp = MB_MULTIPLE(pInfo->ulWidth) * 
                              MB_MULTIPLE(pInfo->ulHeight), i;
                    DWLfwrite(pOutput->pOutputPicture, 1, tmp, pInfo->fpOut);
                    for(i = 0; i < tmp / 4; i++)
                        DWLfwrite(pOutput->pOutputPicture + tmp + i * 2, 1, 1, pInfo->fpOut);
                    for(i = 0; i < tmp / 4; i++)
                        DWLfwrite(pOutput->pOutputPicture + tmp + 1 + i * 2, 1, 1, pInfo->fpOut);
                }

#if 0
                /* tiled */
                if (tiledOutput)
                {
                    /* round up to next multiple of 16 */
                    u32 tmpWidth = MB_MULTIPLE(pInfo->ulWidth);
                    u32 tmpHeight = MB_MULTIPLE(pInfo->ulHeight);
                      
                    TbWriteTiledOutput(fTiledOutput, pOutput->pOutputPicture, tmpWidth >> 4, tmpHeight >> 4, 
                            numberOfWrittenFrames, 0 /* not write md5sum */, 1 /* semi-planar data */ );
                }
#endif

#endif                
                numberOfWrittenFrames++;
                
#ifndef ASIC_TRACE_SUPPORT
                pOutput->pOutputPicture = buffer;
#endif
			}
#endif
		}

#ifdef RV_DISPLAY
		g_srcimg.cx = pOutput->frameWidth;	// 16-pixel up rounding
		g_srcimg.cy = pOutput->frameHeight;	// 16-pixel up rounding
		if(g_isdisplay)
		{
			displayOnScreen(pOutput);
		}
#endif

		DEBUG_PRINT(("Get frame %d.\n", pInfo->ulNumOutputFrames));
		if (pInfo->ulMaxNumDecFrames && (pInfo->ulNumOutputFrames >= pInfo->ulMaxNumDecFrames))
		{
			pInfo->bDecEnd = 1;
		}

        if(rasterScan)
            free(rasterScan);
	}

}

/*------------------------------------------------------------------------------
function
	rm2yuv_error

purpose
	print error message
------------------------------------------------------------------------------*/
void rm2yuv_error(void* pError, HX_RESULT result, const char* pszMsg)
{
	pError = pError;
	result = result;
	pszMsg = pszMsg;
	ERROR_PRINT(("rm2yuv_error pError=0x%08x result=0x%08x msg=%s\n", pError, result, pszMsg));
}

/*------------------------------------------------------------------------------
function
	printDecodeReturn

purpose
	Print out decoder return value
------------------------------------------------------------------------------*/
static void printDecodeReturn(i32 retval)
{
	DEBUG_PRINT(("TB: RvDecDecode returned: "));
	switch(retval){
		case RVDEC_OK:
			DEBUG_PRINT(("RVDEC_OK\n"));
			break;
        case RVDEC_PIC_DECODED:
            DEBUG_PRINT(("RVDEC_PIC_DECODED\n"));
            break;
        case RVDEC_NONREF_PIC_SKIPPED:
			DEBUG_PRINT(("RVDEC_NONREF_PIC_SKIPPED\n"));
            break;
		case RVDEC_STRM_PROCESSED:
			DEBUG_PRINT(("RVDEC_STRM_PROCESSED\n"));
			break;
		case RVDEC_STRM_ERROR:
			DEBUG_PRINT(("RVDEC_STRM_ERROR\n"));
			break;
		case RVDEC_HW_TIMEOUT:
			DEBUG_PRINT(("RVDEC_HW_TIMEOUT\n"));
			break;
		case RVDEC_HW_BUS_ERROR:
			DEBUG_PRINT(("RVDEC_HW_BUS_ERROR\n"));
			break;
		case RVDEC_HW_RESERVED:
			DEBUG_PRINT(("RVDEC_HW_RESERVED\n"));
			break;
		case RVDEC_PARAM_ERROR:
			DEBUG_PRINT(("RVDEC_PARAM_ERROR\n"));
			break;
		case RVDEC_NOT_INITIALIZED:
			DEBUG_PRINT(("RVDEC_NOT_INITIALIZED\n"));
			break;
		case RVDEC_SYSTEM_ERROR:
			DEBUG_PRINT(("RVDEC_SYSTEM_ERROR\n"));
			break;
		default:
			DEBUG_PRINT(("Other %d\n", retval));
			break;
	}
}

/*------------------------------------------------------------------------------
function
	parse_path

purpose
	parse filename and path

parameter
	path(I):	full path of input file
	dir(O):		directory path
	file(O):	file name
-----------------------------------------------------------------------------*/
void parse_path(const char *path, char *dir, char *file)
{
	int len = 0;
	const char *ptr = NULL;

	if (path == NULL)
		return;

	len = strlen(path);
	if (len == 0)
		return;

	ptr = path + len - 1;    // point to the last char
	while (ptr != path)
	{
		if ((*ptr == '\\') || (*ptr == '/'))
			break;

		ptr--;
	}

	if (file != NULL)
	{
		if (ptr != path)
			strcpy(file, ptr + 1);
		else
			strcpy(file, ptr);
	}

	if ((ptr != path) && (dir != NULL))
	{
		len = ptr - path + 1;    // with the "/" or "\"
		DWLmemcpy(dir, path, len);
		dir[len] = '\0';
	}
}

/*------------------------------------------------------------------------------
function
	rm_io_read

purpose
	io read interface for rm parse/depack
-----------------------------------------------------------------------------*/
UINT32 rm_io_read(void* pUserRead, BYTE* pBuf, UINT32 ulBytesToRead)
{
	UINT32 ulRet = 0;

	if (pUserRead && pBuf && ulBytesToRead)
	{
		/* The void* is a DWLFile* */
		DWLFile *fp = (DWLFile *) pUserRead;
		/* Read the number of bytes requested */
		ulRet = (UINT32) DWLfread(pBuf, 1, ulBytesToRead, fp);
	}

	return ulRet;
}

/*------------------------------------------------------------------------------
function
	rm_io_seek

purpose
	io seek interface for rm parse/depack
-----------------------------------------------------------------------------*/
void rm_io_seek(void* pUserRead, UINT32 ulOffset, UINT32 ulOrigin)
{
	if (pUserRead)
	{
		/* The void* is a DWLFile* */
		DWLFile *fp = (DWLFile *) pUserRead;
		/* Do the seek */
		DWLfseek(fp, ulOffset, ulOrigin);
	}
}

/*------------------------------------------------------------------------------
function
	rm_memory_malloc

purpose
	memory (sw only) allocation interface for rm parse/depack
-----------------------------------------------------------------------------*/
void* rm_memory_malloc(void* pUserMem, UINT32 ulSize)
{
	pUserMem = pUserMem;

	return (void*)DWLmalloc(ulSize);
}

/*------------------------------------------------------------------------------
function
	rm_memory_free

purpose
	memory (sw only) free interface for rm parse/depack
-----------------------------------------------------------------------------*/
void rm_memory_free(void* pUserMem, void* ptr)
{
	pUserMem = pUserMem;

	DWLfree(ptr);
}

/*------------------------------------------------------------------------------
function
	rm_memory_create_ncnb

purpose
	memory (sw/hw share) allocation interface for rm parse/depack
-----------------------------------------------------------------------------*/
void* rm_memory_create_ncnb(void* pUserMem, UINT32 ulSize)
{
	DWLLinearMem_t *pBuf = NULL;
	DecContainer *pDecCont = (DecContainer *)g_RVDecInst;
	const void *dwl = NULL;

	pUserMem = pUserMem;

	ASSERT(pDecCont != NULL && pDecCont->dwl != NULL);
	
	pBuf = (DWLLinearMem_t *)DWLmalloc(sizeof(DWLLinearMem_t));
	if (pBuf == NULL)
		return NULL;
	
	dwl = pDecCont->dwl;
	if (DWLMallocLinear(dwl, ulSize, pBuf))
	{
		DWLfree(pBuf);
		return NULL;
	}
	else
	{
		return pBuf;
	}
}

/*------------------------------------------------------------------------------
function
	rm_memory_destroy_ncnb

purpose
	memory (sw/hw share) release interface for rm parse/depack
-----------------------------------------------------------------------------*/
void rm_memory_destroy_ncnb(void* pUserMem, void* ptr)
{
	DWLLinearMem_t *pBuf = NULL;
	DecContainer *pDecCont = (DecContainer *)g_RVDecInst;
	const void *dwl = NULL;

	pUserMem = pUserMem;

	ASSERT(pDecCont != NULL && pDecCont->dwl != NULL);

	if (ptr == NULL)
		return;

	pBuf = *(DWLLinearMem_t **)ptr;

	if (pBuf == NULL)
		return;

	dwl = pDecCont->dwl;

	DWLFreeLinear(dwl, pBuf);
	DWLfree(pBuf);
	*(DWLLinearMem_t **)ptr = NULL;
}

/*------------------------------------------------------------------------------
Function name:  RvDecTrace()

Functional description:
	This implementation appends trace messages to file named 'dec_api.trc'.

Argument:
	string - trace message, a null terminated string
------------------------------------------------------------------------------*/

void RvDecTrace(const char *str)
{
	DWLFile *fp = NULL;

	fp = DWLfopen("dec_api.trc", "a");
	if(!fp)
		return;

	DWLfwrite(str, 1, strlen(str), fp);
	DWLfwrite("\n", 1, 1, fp);

	DWLfclose(fp);
}

void displayOnScreen(RvDecPicture *pDecPicture)
{
	pDecPicture = pDecPicture;

#ifdef RV_TIME_TRACE
	ulStopTime_Full = TIMER_GET_NOW_TIME();
	if (ulStopTime_Full <= ulStartTime_Full)
		fTime_Full += (double)((TIMER_MAX_COUNTER-ulStartTime_Full+ulStopTime_Full)/TIMER_FACTOR_MSECOND);
	else
		fTime_Full += (double)((ulStopTime_Full-ulStartTime_Full)/TIMER_FACTOR_MSECOND);

	fTime_LCD = 0.0;
	ulStartTime_LCD = TIMER_GET_NOW_TIME();
#endif

	VIM_DISPLAY((g_srcimg, g_dstimg, (char *)pDecPicture->pData, &g_bfirstdisplay, 
		g_delayTime, g_rotationmode, g_blayerenable));

#ifdef RV_TIME_TRACE
	ulStopTime_LCD = TIMER_GET_NOW_TIME();
	if (ulStopTime_LCD <= ulStartTime_LCD)
		fTime_LCD += (double)((TIMER_MAX_COUNTER-ulStartTime_LCD+ulStopTime_LCD)/TIMER_FACTOR_MSECOND);
	else
		fTime_LCD += (double)((ulStopTime_LCD-ulStartTime_LCD)/TIMER_FACTOR_MSECOND);

	if(fTime_LCD > fMaxTime_LCD)
		fMaxTime_LCD = fTime_LCD;

	fTotalTime_LCD += fTime_LCD;

	if (g_islog2std)
	{
		TIME_PRINT((" | \tDisplay:  ID= %-6d  PicType= %-4d  time= %-6.0f ms\t", pDecPicture->picId, 0, fTime_LCD));
	}
	if (g_islog2file && g_fpTimeLog) 
	{
		sprintf(strTimeInfo, " | \tDisplay:  ID= %-6d  PicType= %-4d  time= %-6.0f ms\t", pDecPicture->picId, 0, fTime_LCD);
		DWLfwrite(strTimeInfo, 1, strlen(strTimeInfo), g_fpTimeLog);
	}

	ulStartTime_Full = TIMER_GET_NOW_TIME();
#endif
}

void RV_Time_Init(i32 islog2std, i32 islog2file)
{
#ifdef RV_TIME_TRACE
	g_islog2std = islog2std;
	g_islog2file = islog2file;
	
	if (g_islog2file)
	{
	#ifdef WIN32
		g_fpTimeLog = DWLfopen("rvtime.log", "wb");
	#else
		g_fpTimeLog = DWLfopen("rvtime.log", "w");
	#endif
	}

	fMaxTime_DEC = 0.0;
	fMaxTime_LCD = 0.0;
	fMaxTime_Full = 0.0;
	fTotalTime_DEC = 0.0;
	fTotalTime_LCD = 0.0;
	fTotalTime_Full = 0.0;
#endif
}

void RV_Time_Full_Reset(void)
{
#ifdef RV_TIME_TRACE
	fTime_Full = 0.0;
	ulStartTime_Full = TIMER_GET_NOW_TIME();
#endif
}

void RV_Time_Full_Pause(void)
{
#ifdef RV_TIME_TRACE
	ulStopTime_Full = TIMER_GET_NOW_TIME();
	if (ulStopTime_Full <= ulStartTime_Full)
		fTime_Full += (double)((TIMER_MAX_COUNTER-ulStartTime_Full+ulStopTime_Full)/TIMER_FACTOR_MSECOND);
	else
		fTime_Full += (double)((ulStopTime_Full-ulStartTime_Full)/TIMER_FACTOR_MSECOND);

	if(fTime_Full > fMaxTime_Full)
		fMaxTime_Full = fTime_Full;

	fTotalTime_Full += fTime_Full;

	if (g_islog2std)
	{
		TIME_PRINT((" | \tFullTime= %-6.0f ms", fTime_Full));
	}				
	if (g_islog2file && g_fpTimeLog) 
	{
		sprintf(strTimeInfo, " | \tFullTime= %-6.0f ms", fTime_Full);
		DWLfwrite(strTimeInfo, 1, strlen(strTimeInfo), g_fpTimeLog);
	}				
#endif
}

void RV_Time_Dec_Reset(void)
{
#ifdef RV_TIME_TRACE
	fTime_DEC = 0.0;
	ulStartTime_DEC = TIMER_GET_NOW_TIME();
#endif
}

void RV_Time_Dec_Pause(u32 picid, u32 pictype)
{
#ifdef RV_TIME_TRACE
	ulStopTime_Full = TIMER_GET_NOW_TIME();
	if (ulStopTime_Full <= ulStartTime_Full)
		fTime_Full += (double)((TIMER_MAX_COUNTER-ulStartTime_Full+ulStopTime_Full)/TIMER_FACTOR_MSECOND);
	else
		fTime_Full += (double)((ulStopTime_Full-ulStartTime_Full)/TIMER_FACTOR_MSECOND);

	ulStopTime_DEC = TIMER_GET_NOW_TIME();
	if (ulStopTime_DEC <= ulStartTime_DEC)
		fTime_DEC += (double)((TIMER_MAX_COUNTER-ulStartTime_DEC+ulStopTime_DEC)/TIMER_FACTOR_MSECOND);
	else
		fTime_DEC += (double)((ulStopTime_DEC-ulStartTime_DEC)/TIMER_FACTOR_MSECOND);

	if(fTime_DEC > fMaxTime_DEC)
		fMaxTime_DEC = fTime_DEC;

	fTotalTime_DEC += fTime_DEC;

	if (g_islog2std)
	{
		TIME_PRINT(("\nDecode:  ID= %-6d  PicType= %-4d  time= %-6.0f ms\t", picid, pictype, fTime_DEC));
	}
	if (g_islog2file && g_fpTimeLog) 
	{
		sprintf(strTimeInfo, "\nDecode:  ID= %-6d  PicType= %-4d  time= %-6.0f ms\t", picid, pictype, fTime_DEC);
		DWLfwrite(strTimeInfo, 1, strlen(strTimeInfo), g_fpTimeLog);
	}

	ulStartTime_Full = TIMER_GET_NOW_TIME();
#endif
}

int main(int argc, char *argv[])
{
	int startframeid = 0;
	int maxnumber = 1000000000;
	int iswritefile = 1;
	int isdisplay = 0;
	int displaymode = 0;
	int rotationmode = 0;
	int islog2std = 1;
	int islog2file = 0;
	int blayerenable = 0;
	int errconceal = 0;
	int randomerror = 0;
    int i;
    FILE *fTBCfg;

    INIT_SW_PERFORMANCE;

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


    /* set test bench configuration */
#ifdef ASIC_TRACE_SUPPORT
    gHwVer = 9170;
#endif

#ifndef PP_PIPELINE_ENABLED
	if (argc < 2)
    {
        printf("\nREAL Decoder Testbench\n");
		printf("Usage: %s [options] stream.rm\n", argv[0]);
        printf("\t-Nn forces decoding to stop after n pictures\n");
        printf("\t-Ooutfile write output to \"outfile\" (default out_wxxxhyyy.yuv)\n");
        printf("\t-X disable output file writing\n");
        printf("\t-P write planar output\n");
        printf("\t-E use tiled reference frame format.\n");
        printf("\t-G convert tiled output pictures to raster scan\n");
        printf("\t-Bn to use n frame buffers in decoder\n");
        printf("\t-Q skip decoding non-reference pictures\n");
        printf("\t-Z output pictures using RvDecPeek() function\n");
        printf("\t-V input file in RealVideo format\n");
		printRvVersion();
		return -1;
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
        else if(strcmp(argv[i], "-P") == 0)
        {
            planarOutput = 1;
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
        else if(strcmp(argv[i], "-V") == 0)
        {
            rvInput = 1;
        }
        else if(strcmp(argv[i], "-Z") == 0)
        {
            usePeekOutput = 1;
        }
        else if(strcmp(argv[i], "-Q") == 0)
        {
            skipNonReference = 1;
        }
        else
        {
            DEBUG_PRINT(("UNKNOWN PARAMETER: %s\n", argv[i]));
        }

    }

	printRvVersion();
#else
if (argc < 3)
    {
        printf("\nREAL Decoder PP Pipelined Testbench\n\n");
		printf("Usage:\n%s [options] stream.rm pp.cfg\n", argv[0]);
        printf("\t-Nn forces decoding to stop after n pictures\n");
        printf("\t-E use tiled reference frame format.\n");
        printf("\t-X disable output file writing\n");
        printf("\t-Bn to use n frame buffers in decoder\n");
        printf("\t-Q skip decoding non-reference pictures\n");

		return -1;
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
        else if(strcmp(argv[i], "-P") == 0)
        {
            planarOutput = 1;
        }
        else if(strncmp(argv[i], "-B", 2) == 0)
        {
            numFrameBuffers = atoi(argv[i] + 2);
            if(numFrameBuffers > 16)
                numFrameBuffers = 16;
        }
        else if (strncmp(argv[i], "-E", 2) == 0)
            tiledOutput = DEC_REF_FRM_TILED_DEFAULT;
        else if(strcmp(argv[i], "-Q") == 0)
        {
            skipNonReference = 1;
        }
        else
        {
            DEBUG_PRINT(("UNKNOWN PARAMETER: %s\n", argv[i]));
        }
    }
	printRvVersion();
#endif

    TBSetDefaultCfg(&tbCfg);
    fTBCfg = fopen("tb.cfg", "r");
    if(fTBCfg == NULL)
    {
        DEBUG_PRINT(("UNABLE TO OPEN INPUT FILE: \"tb.cfg\"\n"));
        DEBUG_PRINT(("USING DEFAULT CONFIGURATION\n"));
        tbCfg.decParams.hwVersion = 9170;
    }
    else
    {
        fclose(fTBCfg);
        if(TBParseConfig("tb.cfg", TBReadParam, &tbCfg) == TB_FALSE)
            return -1;
        if(TBCheckCfg(&tbCfg) != 0)
            return -1;

#ifdef ASIC_TRACE_SUPPORT
        gHwVer = tbCfg.decParams.hwVersion;
#endif
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
    DEBUG_PRINT(("TB Packet by Packet  %d\n", packetize));
    DEBUG_PRINT(("TB Seed Rnd %d\n", seedRnd));
    DEBUG_PRINT(("TB Stream Truncate %d\n", streamTruncate));
    DEBUG_PRINT(("TB Stream Header Corrupt %d\n", streamHeaderCorrupt));
    DEBUG_PRINT(("TB Stream Bit Swap %d; odds %s\n", streamBitSwap,
                 tbCfg.tbParams.streamBitSwap));
    DEBUG_PRINT(("TB Stream Packet Loss %d; odds %s\n", streamPacketLoss,
                 tbCfg.tbParams.streamPacketLoss));

    if (rvInput)
    {
        rv_mode(argv[argc-1],
            NULL,
            startframeid,
            maxnumber,
            iswritefile,
            isdisplay, 
            displaymode,
            rotationmode,
            islog2std,
            islog2file,
            blayerenable,
            errconceal,
            randomerror);
        if(frameOut)
        {
            fclose(frameOut);
            free(frameBuffer);
        }
        return 0;
    }
#ifndef PP_PIPELINE_ENABLED
	rv_display(argv[argc-1],
		NULL,
		startframeid,
		maxnumber,
		iswritefile,
		isdisplay, 
		displaymode,
		rotationmode,
		islog2std,
		islog2file,
		blayerenable,
		errconceal,
		randomerror);
#else
	rv_display(argv[argc-2],
		argv[argc-1],
		startframeid,
		maxnumber,
		iswritefile,
		isdisplay, 
		displaymode,
		rotationmode,
		islog2std,
		islog2file,
		blayerenable,
		errconceal,
		randomerror);
#endif


    if(frameOut)
    {
        fclose(frameOut);
        free(frameBuffer);
    }

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
    u8 *pOutCb, *pOutCr;

/* Code */

    memset( pOut, 128, outWidth*outHeight*3/2 );

    /* Luma */
    for ( y = 0 ; y < inHeight ; ++y )
    {
        for( x = 0 ; x < inWidth; ++x )
            *pOut++ = *pIn++;
        pIn += ( inFrameWidth - inWidth );
        pOut += ( outWidth - inWidth );
    }

    pIn += inFrameWidth * ( inFrameHeight - inHeight );
    pOut += outWidth * ( outHeight - inHeight );

    inFrameHeight /= 2;
    inFrameWidth /= 2;
    outHeight /= 2;
    outWidth /= 2;
    inHeight /= 2;
    inWidth /= 2;

    /* Chroma */
    /*pOut += 2 * outWidth * ( outHeight - inHeight );*/
    pOutCb = pOut;
    pOutCr = pOut + outWidth*outHeight;
    for ( y = 0 ; y < inHeight ; ++y )
    {
        for( x = 0 ; x < inWidth; ++x )
        {
            *pOutCb++ = *pIn++;
            *pOutCr++ = *pIn++;
        }
        pIn += 2 * ( inFrameWidth - inWidth );
        pOutCb += ( outWidth - inWidth );
        pOutCr += ( outWidth - inWidth );
    }

}

/*------------------------------------------------------------------------------
        printRvVersion
        Description : Print out decoder version info
------------------------------------------------------------------------------*/

void printRvVersion(void)
{

    RvDecApiVersion decVersion;
    RvDecBuild decBuild;

    /*
     * Get decoder version info
     */

    decVersion = RvDecGetAPIVersion();
    printf("\nAPI version:  %d.%d, ", decVersion.major, decVersion.minor);

    decBuild = RvDecGetBuild();
    printf("sw build nbr: %d, hw build nbr: %x\n\n",
           decBuild.swBuild, decBuild.hwBuild);

}

#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 vopNumber, RvDecPicture DecPicture, RvDecInst decoder)
{
    PPResult res;

    res = pp_check_combined_status();

    if(res == PP_OK)
    {
        pp_write_output(vopNumber, 0, 0);
        pp_read_blend_components(((DecContainer *) decoder)->ppInstance);
    }
    if(pp_update_config
       (decoder, PP_PIPELINED_DEC_TYPE_RV, &tbCfg) == CFG_UPDATE_FAIL)

    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
    }

}
#endif


