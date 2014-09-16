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

#include "on2rvdecapi.h"

#ifdef PP_PIPELINE_ENABLED
#include "ppapi.h"
#include "pptestbench.h"
#endif

#include "rv_container.h"

#include "rm_parse.h"
#include "rv_depack.h"
#include "rv_decode.h"

#include "tb_cfg.h"
#include "tb_tiled.h"

#include "regdrv.h"

#ifdef ASIC_TRACE_SUPPORT
#include "trace.h"
extern u32 hwDecPicCount;
extern u32 gHwVer;
#endif

#ifdef RV_EVALUATION
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
u32 tiledOutput = 0;
u32 frameNumber = 0;
u32 numberOfWrittenFrames = 0;
u32 numFrameBuffers = 0;
u32 skipNonReference = 0;
FILE *fTiledOutput = NULL;
void FramePicture( u8 *pIn, i32 inWidth, i32 inHeight,
                   i32 inFrameWidth, i32 inFrameHeight,
                   u8 *pOut, i32 outWidth, i32 outHeight );

#ifdef PP_PIPELINE_ENABLED
void HandlePpOutput(u32 frameNumber, /*RvDecPicture DecPicture,*/
                    RvDecInst decoder);
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
#ifndef ASSERT
#define ASSERT(s)
#endif /* ASSERT */
u32 bFrames = 0;


TBCfg tbCfg;
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
static HX_RESULT rv_decode_stream_decode(u8 *pStrm, On2DecoderInParams *pInput,
    On2DecoderOutParams *pOutput);
static void rv_decode_frame_flush(rm2yuv_info *pInfo, On2DecoderOutParams *pOutput);

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

static u32 maxNumPics = 0;
static u32 picsDecoded = 0;

static codecSegmentInfo sliceInfo[128];

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

	On2RvDecRet rvRet;	
    On2DecoderInit rvInit;
    On2RvMsgSetDecoderRprSizes msg;
    u32 tmpBuf[16];

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

	islog2std = islog2std;
	islog2file = islog2file;

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

    maxCodedWidth = pDecode->ulLargestPels;
    maxCodedHeight = pDecode->ulLargestLines;
    maxFrameWidth = MB_MULTIPLE( maxCodedWidth );
    maxFrameHeight = MB_MULTIPLE( maxCodedHeight );

    rvInit.outtype = 0;
    rvInit.pels = pDecode->ulLargestPels;
    rvInit.lines = pDecode->ulLargestLines;
    rvInit.nPadWidth = rvInit.nPadHeight = rvInit.pad_to_32 = 0;
    rvInit.ulInvariants = 0;
    rvInit.packetization = 0;
    rvInit.ulStreamVersion = pDecode->bIsRV8 ? 0x30200000 : 0x40000000;

    /* initialize decoder */
	rvRet = On2RvDecInit(&rvInit, &g_RVDecInst);
	if (rvRet != ON2RVDEC_OK)
	{
		ERROR_PRINT(("RVDecInit fail! \n"));
		goto cleanup;
	}

    /* override picture buffer amount */
    if(numFrameBuffers)
    {
        rvRet = On2RvDecSetNbrOfBuffers( numFrameBuffers, g_RVDecInst );
        if (rvRet != ON2RVDEC_OK)
        {
            ERROR_PRINT(("Number of buffers failed!\n"));
            goto cleanup;
        }
    }

    
    /* set possible picture sizes for RV8 using
     * ON2RV_MSG_ID_Set_RVDecoder_RPR_Sizes custom message */
    if (pDecode->bIsRV8)
    {
        u32 i;
        msg.message_id = ON2RV_MSG_ID_Set_RVDecoder_RPR_Sizes;
        msg.num_sizes = pDecode->ulNumImageSizes;
        msg.sizes = tmpBuf;
        for (i = 0; i < 16; i++)
            msg.sizes[i] = pDecode->pDimensions[i];
        rvRet = On2RvDecCustomMessage(&msg, g_RVDecInst);
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

    /* If unspecified at cmd line, use minimum # of buffers, otherwise
     * use specified amount. */
    if(numFrameBuffers == 0)
        pp_number_of_buffers(2);
    else
        pp_number_of_buffers(numFrameBuffers);

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

	/* Fill in the width and height */
	info.ulWidth  = (pDecode->ulLargestPels+15)&0xFFFFFFF0;
	info.ulHeight = (pDecode->ulLargestLines+15)&0xFFFFFFF0;

	/* Parse the output file path */
	if(g_iswritefile)
	{
		char flnm[0x100] = "";
		parse_path(filename, info.filePath, NULL);
	}

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

#ifdef ASIC_TRACE_SUPPORT
        if (maxNumPics && picsDecoded >= maxNumPics)
            break;
#endif
	}

    /* last picture */
    {
        On2DecoderOutParams decOutput;
        On2DecoderInParams decInput;
        decInput.flags = ON2RV_DECODE_LAST_FRAME;
        decInput.numDataSegments = 0;
        retVal = rv_decode_stream_decode(tmpBuf, &decInput, &decOutput);

        if (HX_SUCCEEDED(retVal))
        {	
            rv_decode_frame_flush(&info, &decOutput);
        }
    }

    /* Display results */
	DEBUG_PRINT(("Video stream in '%s' decoding complete: %lu input frames, %lu output frames\n",
		(const char*) filename, picsDecoded, numberOfWrittenFrames));

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
		On2RvDecFree(g_RVDecInst);
	}

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
	On2DecoderOutParams decOutput;
	On2DecoderInParams decInput;
    u32 i;

	DWLmemset(&decOutput, 0, sizeof(decOutput));
	DWLmemset(&decInput, 0, sizeof(decInput));
    
	if (pAvail && pFrame)
	{
		/* Get the info pointer */
		rm2yuv_info* pInfo = (rm2yuv_info*) pAvail;
		if (pInfo->pDepack && pInfo->pDecode)
		{
            /* Increment the number of input frames */
            decInput.dataLength = pFrame->ulDataLen;
            decInput.numDataSegments = pFrame->ulNumSegments;
            decInput.pDataSegments = sliceInfo;
            DWLmemset(decInput.pDataSegments, 0, sizeof(sliceInfo));
            for (i = 0; i < decInput.numDataSegments; i++)
            {
                sliceInfo[i].bIsValid = pFrame->pSegment[i].bIsValid;
                sliceInfo[i].ulSegmentOffset = pFrame->pSegment[i].ulOffset;
            }
            decInput.streamBusAddr = pFrame->ulDataBusAddress;
            decInput.timestamp = pFrame->ulTimestamp;
            decInput.skipNonReference = skipNonReference;

            /* Decode frames until there aren't any more */
            do
            {
                decInput.flags = (decOutput.notes & ON2RV_DECODE_MORE_FRAMES) ?
                    ON2RV_DECODE_MORE_FRAMES : 0;

                retVal = rv_decode_stream_decode(pFrame->pData,
                    &decInput, &decOutput);

                if (HX_SUCCEEDED(retVal))
                {	
                    rv_decode_frame_flush(pInfo, &decOutput);
                }
                else
                {
                    pInfo->bDecEnd = 1;
                }
            }
            while (HX_SUCCEEDED(retVal) &&
                (decOutput.notes & ON2RV_DECODE_MORE_FRAMES));

			rv_depack_destroy_frame(pInfo->pDepack, &pFrame);
            picsDecoded++;
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
HX_RESULT rv_decode_stream_decode(u8 *pStrm, On2DecoderInParams *pInput,
    On2DecoderOutParams *pOutput)
{
	HX_RESULT retVal = HXR_FAIL;
	On2RvDecRet rvRet;
    u32 moreData = 1;
	UINT32 i = 0;
    u32 packetLoss = 0;
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
	
    if (streamBitSwap)
    {
        if((hdrsRdy && !streamHeaderCorrupt) || streamHeaderCorrupt)
        {
            /* Picture needs to be ready before corrupting next picture */
            if(picRdy && corruptedBytes <= 0)
            {
                ret = TBRandomizeBitSwapInStream(pStrm,
                pInput->dataLength, tbCfg.tbParams.streamBitSwap);
                if(ret != 0)
                {
                    printf("RANDOM STREAM ERROR FAILED\n");
                    return 0;
                }

                corruptedBytes = pInput->dataLength;
                printf("corruptedBytes %d\n", corruptedBytes);
            }
        }
    }

    do
    {
        moreData = 0;

    	rvRet = On2RvDecDecode(pStrm, NULL, pInput, pOutput, g_RVDecInst);

    	printDecodeReturn(rvRet);

        switch(rvRet)
        {
	    case ON2RVDEC_OK:
                if (pOutput->numFrames)
                {
                    frameNumber++;
                }
                retVal = HXR_OK;
                break;
            default:
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
	write the decoded picture to a local file or display on screen, if neccesary
------------------------------------------------------------------------------*/
void rv_decode_frame_flush(rm2yuv_info *pInfo, On2DecoderOutParams *pOutput)
{
	UINT32 ulWidth, ulHeight, ulOutFrameSize;

	if (pOutput == NULL)
		return;

	/* Is there a valid output frame? */
	if (pOutput->numFrames && !(pOutput->notes & ON2RV_DECODE_DONT_DRAW))
	{
		/* Get the dimensions and size of output frame */
		ulWidth = pOutput->width;
		ulHeight = pOutput->height;
		ulOutFrameSize = ulWidth * ulHeight * 3 / 2;

		/* Check to see if dimensions have changed */
		if (ulWidth != pInfo->ulWidth || ulHeight != pInfo->ulHeight )
		{
			DEBUG_PRINT(("Warning: YUV output dimensions changed from "
				"%lux%lu to %lux%lu at frame #: %lu (in decoding order)\n",
				pInfo->ulWidth,	pInfo->ulHeight, ulWidth, ulHeight, 0));
			/* Don't bother resizing to display dimensions */
			pInfo->ulWidth = ulWidth;
			pInfo->ulHeight = ulHeight;
			pInfo->ulOutFrameSize = ulOutFrameSize;
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
			}
#ifdef PP_PIPELINE_ENABLED
  		    HandlePpOutput(frameNumber - 1, /*tmpPic, */g_RVDecInst);
#else
			if (pInfo->fpOut)
			{
#ifndef MD5SUM
                if( enableFramePicture )
                {
                    u32 tmp = maxCodedWidth * maxCodedHeight, i;
                    FramePicture( pOutput->pOutFrame,
                        pInfo->ulWidth, pInfo->ulHeight,
                        MB_MULTIPLE(pInfo->ulWidth), 
                        MB_MULTIPLE(pInfo->ulHeight), 
                        frameBuffer, maxCodedWidth, maxCodedHeight );

                    DWLfwrite(frameBuffer, 1, 3*tmp/2, frameOut);
                }
#endif

#ifdef MD5SUM
                {
                    u32 tmp = maxCodedWidth * maxCodedHeight, i;
                    FramePicture( pOutput->pOutFrame,
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
                            0);
                    }
                    else
                    {
                    TBWriteFrameMD5Sum(pInfo->fpOut, pOutput->pOutFrame, 
                            MB_MULTIPLE(pInfo->ulWidth)*
                            MB_MULTIPLE(pInfo->ulHeight)*3/2,
                            0); 
                    }                       
                }


                /* tiled */
                if (tiledOutput)
                {
                    /* round up to next multiple of 16 */
                    u32 tmpWidth = MB_MULTIPLE(pInfo->ulWidth);
                    u32 tmpHeight = MB_MULTIPLE(pInfo->ulHeight);
                      
                    TbWriteTiledOutput(fTiledOutput, pOutput->pOutFrame, tmpWidth >> 4, tmpHeight >> 4, 
                            numberOfWrittenFrames, 1 /* write md5sum */, 1 /* semi-planar data */ );
                }
#else                
                if( !planarOutput )
                {
                    DWLfwrite(pOutput->pOutFrame, 1, 
                        MB_MULTIPLE(pInfo->ulWidth)*
                        MB_MULTIPLE(pInfo->ulHeight)*3/2,
                        pInfo->fpOut);
                }
                else
                {
                    u32 tmp = MB_MULTIPLE(pInfo->ulWidth) * 
                              MB_MULTIPLE(pInfo->ulHeight), i;
                    DWLfwrite(pOutput->pOutFrame, 1, tmp, pInfo->fpOut);
                    for(i = 0; i < tmp / 4; i++)
                        DWLfwrite(pOutput->pOutFrame + tmp + i * 2, 1, 1, pInfo->fpOut);
                    for(i = 0; i < tmp / 4; i++)
                        DWLfwrite(pOutput->pOutFrame + tmp + 1 + i * 2, 1, 1, pInfo->fpOut);
                }

                /* tiled */
                if (tiledOutput)
                {
                    /* round up to next multiple of 16 */
                    u32 tmpWidth = MB_MULTIPLE(pInfo->ulWidth);
                    u32 tmpHeight = MB_MULTIPLE(pInfo->ulHeight);
                      
                    TbWriteTiledOutput(fTiledOutput, pOutput->pOutFrame, tmpWidth >> 4, tmpHeight >> 4, 
                            numberOfWrittenFrames, 0 /* not write md5sum */, 1 /* semi-planar data */ );
                }

#endif                
                numberOfWrittenFrames++;
			}
#endif
		}

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
		case ON2RVDEC_OK:
			DEBUG_PRINT(("ON2RVDEC_OK\n"));
			break;
		case ON2RVDEC_INVALID_PARAMETER:
			DEBUG_PRINT(("ON2RVDEC_INVALID_PARAMETER\n"));
			break;
		case ON2RVDEC_OUTOFMEMORY:
			DEBUG_PRINT(("ON2RVDEC_OUTOFMEMORY\n"));
			break;
		case ON2RVDEC_NOTIMPL:
			DEBUG_PRINT(("ON2RVDEC_NOTIMPL\n"));
			break;
		case ON2RVDEC_POINTER:
			DEBUG_PRINT(("ON2RVDEC_POINTER\n"));
			break;
		case ON2RVDEC_FAIL:
			DEBUG_PRINT(("ON2RVDEC_FAIL\n"));
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
	char *ptr = NULL;

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

    /* set test bench configuration */
#ifdef ASIC_TRACE_SUPPORT
    gHwVer = 9170;
#endif

#ifdef RV_EVALUATION_9170
    gHwVer = 9170;
#elif RV_EVALUATION_9190
    gHwVer = 9190;
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
        printf("\t-T write tiled output by converting raster scan output\n");
        printf("\t-Bn to use n frame buffers in decoder\n");
        printf("\t-Q skip decoding non-reference pictures\n");
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
void HandlePpOutput(u32 vopNumber, /*RvDecPicture DecPicture, */RvDecInst decoder)
{
    PPResult res;

    res = pp_check_combined_status();

    if(res == PP_OK)
    {
        pp_write_output(vopNumber, 0 /*DecPicture.fieldPicture*/,
                        0 /*DecPicture.topField*/);
        pp_read_blend_components(((DecContainer *) decoder)->ppInstance);
    }
    if(pp_update_config
       (decoder, PP_PIPELINED_DEC_TYPE_RV, &tbCfg) == CFG_UPDATE_FAIL)

    {
        fprintf(stdout, "PP CONFIG LOAD FAILED\n");
    }

}
#endif


