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
--  Abstract : Encoder initialization and setup
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: H264Init.c,v $
--  $Revision: 1.10 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "H264Init.h"
#include "enccommon.h"
#include "ewl.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
#define H264ENC_MIN_ENC_WIDTH       96
#define H264ENC_MAX_ENC_WIDTH       1280
#define H264ENC_MIN_ENC_HEIGHT      96
#define H264ENC_MAX_ENC_HEIGHT      1280

#define H264ENC_MAX_MBS_PER_PIC     5120    /* 1280x1024 */

#define H264ENC_MAX_PROFILE_LEVEL   32

#define H264ENC_DEFAULT_QP          26

// as defined in codec.h of omx/omxil_encoder/source/
#define TIME_RESOLUTION 90000

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static bool_e SetParameter(h264Instance_s * inst,
                           const H264EncConfig * pEncCfg);
static bool_e CheckParameter(const h264Instance_s * inst);

/*------------------------------------------------------------------------------

    H264CheckCfg

    Function checks that the configuration is valid.

    Input   pEncCfg Pointer to configuration structure.

    Return  ENCHW_OK      The configuration is valid.
            ENCHW_NOK     Some of the parameters in configuration are not valid.

------------------------------------------------------------------------------*/
bool_e H264CheckCfg(const H264EncConfig * pEncCfg)
{
    ASSERT(pEncCfg);

    if ((pEncCfg->streamType != H264ENC_BYTE_STREAM) &&
        (pEncCfg->streamType != H264ENC_NAL_UNIT_STREAM))
        return ENCHW_NOK;

    /* Encoded image width limits, multiple of 4 */
    if(pEncCfg->width < H264ENC_MIN_ENC_WIDTH ||
       pEncCfg->width > H264ENC_MAX_ENC_WIDTH || (pEncCfg->width & 0x3) != 0)
        return ENCHW_NOK;

    /* Encoded image height limits, multiple of 2 */
    if(pEncCfg->height < H264ENC_MIN_ENC_HEIGHT ||
       pEncCfg->height > H264ENC_MAX_ENC_HEIGHT || (pEncCfg->height & 0x1) != 0)
        return ENCHW_NOK;

    /* total macroblocks per picture limit */
    if(((pEncCfg->height + 15) / 16) * ((pEncCfg->width + 15) / 16) >
       H264ENC_MAX_MBS_PER_PIC)
    {
        return ENCHW_NOK;
    }

    /* Check frame rate */
    // Check against TIME_RESOLUTION in codec.h
    if(pEncCfg->frameRateNum < 1 || pEncCfg->frameRateNum > TIME_RESOLUTION)
        return ENCHW_NOK;

    if(pEncCfg->frameRateDenom < 1)
    {
        return ENCHW_NOK;
    }

    /* special allowal of 1000/1001, 0.99 fps by customer request */
    if(pEncCfg->frameRateDenom > pEncCfg->frameRateNum &&
       !(pEncCfg->frameRateDenom == 1001 && pEncCfg->frameRateNum == 1000))
    {
        return ENCHW_NOK;
    }

    /* check profile&level */
    if((pEncCfg->profileAndLevel > H264ENC_MAX_PROFILE_LEVEL) &&
       (pEncCfg->profileAndLevel != H264ENC_BASELINE_LEVEL_1_b))
    {
        return ENCHW_NOK;
    }

    /* check HW limitations */
    {
        EWLHwConfig_t cfg = EWLReadAsicConfig();
        /* is H.264 encoding supported */
        if(cfg.h264Enabled == EWL_HW_CONFIG_NOT_SUPPORTED)
        {
            return ENCHW_NOK;
        }

        /* max width supported */
        if(cfg.maxEncodedWidth < pEncCfg->width)
        {
            return ENCHW_NOK;
        }
    }

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    H264Init

    Function initializes the Encoder and create new encoder instance.

    Input   pEncCfg     Encoder configuration.
            instAddr    Pointer to instance will be stored in this address

    Return  H264ENC_OK
            H264ENC_MEMORY_ERROR 
            H264ENC_EWL_ERROR
            H264ENC_EWL_MEMORY_ERROR
            H264ENC_INVALID_ARGUMENT

------------------------------------------------------------------------------*/
H264EncRet H264Init(const H264EncConfig * pEncCfg, h264Instance_s ** instAddr)
{
    h264Instance_s *inst = NULL;
    const void *ewl = NULL;

    H264EncRet ret = H264ENC_OK;
    EWLInitParam_t param;

    ASSERT(pEncCfg);
    ASSERT(instAddr);

    *instAddr = NULL;

    param.clientType = EWL_CLIENT_TYPE_H264_ENC;

    /* Init EWL */
    if((ewl = EWLInit(&param)) == NULL)
        return H264ENC_EWL_ERROR;

    /* Encoder instance */
    inst = (h264Instance_s *) EWLcalloc(1, sizeof(h264Instance_s));

    if(inst == NULL)
    {
        ret = H264ENC_MEMORY_ERROR;
        goto err;
    }

    /* Default values */
    H264SeqParameterSetInit(&inst->seqParameterSet);
    H264PicParameterSetInit(&inst->picParameterSet);
    H264SliceInit(&inst->slice);

    /* Set parameters depending on user config */
    if(SetParameter(inst, pEncCfg) != ENCHW_OK)
    {
        ret = H264ENC_INVALID_ARGUMENT;
        goto err;
    }

    /* Check and init the rest of parameters */
    if(CheckParameter(inst) != ENCHW_OK)
    {
        ret = H264ENC_INVALID_ARGUMENT;
        goto err;
    }

    if(H264InitRc(&inst->rateControl) != ENCHW_OK)
    {
        return H264ENC_INVALID_ARGUMENT;
    }

    /* Initialize ASIC */
    inst->asic.ewl = ewl;
    (void) EncAsicControllerInit(&inst->asic);

    /* Allocate internal SW/HW shared memories */
    if(EncAsicMemAlloc_V2(&inst->asic,
                          (u32) inst->preProcess.lumWidth,
                          (u32) inst->preProcess.lumHeight,
                          ASIC_H264) != ENCHW_OK)
    {

        ret = H264ENC_EWL_MEMORY_ERROR;
        goto err;
    }

    *instAddr = inst;

    /* init VUI */
    {
        const h264VirtualBuffer_s *vb = &inst->rateControl.virtualBuffer;

        H264SpsSetVuiTimigInfo(&inst->seqParameterSet,
                               vb->timeScale, vb->unitsInTic);
    }

    if(inst->seqParameterSet.levelIdc >= 31)
        inst->asic.regs.h264Inter4x4Disabled = 1;
    else
        inst->asic.regs.h264Inter4x4Disabled = 0;

    return ret;

  err:
    if(inst != NULL)
        EWLfree(inst);
    if(ewl != NULL)
        (void) EWLRelease(ewl);

    return ret;
}

/*------------------------------------------------------------------------------

    H264Shutdown

    Function frees the encoder instance.

    Input   h264Instance_s *    Pointer to the encoder instance to be freed.
                            After this the pointer is no longer valid.

------------------------------------------------------------------------------*/
void H264Shutdown(h264Instance_s * data)
{
    const void *ewl;

    ASSERT(data);

    ewl = data->asic.ewl;

    EncAsicMemFree_V2(&data->asic);

    EWLfree(data);

    (void) EWLRelease(ewl);
}

/*------------------------------------------------------------------------------

    SetParameter

    Set all parameters in instance to valid values depending on user config.

------------------------------------------------------------------------------*/
bool_e SetParameter(h264Instance_s * inst, const H264EncConfig * pEncCfg)
{
    i32 width, height, tmp, bps;

    ASSERT(inst);

    /* Internal images, next macroblock boundary */
    width = 16 * ((pEncCfg->width + 15) / 16);
    height = 16 * ((pEncCfg->height + 15) / 16);

    /* stream type */
    if(pEncCfg->streamType == H264ENC_BYTE_STREAM)
    {
        inst->asic.regs.h264StrmMode = ASIC_H264_BYTE_STREAM;
        inst->picParameterSet.byteStream = ENCHW_YES;
        inst->seqParameterSet.byteStream = ENCHW_YES;
        inst->rateControl.sei.byteStream = ENCHW_YES;
        inst->slice.byteStream = ENCHW_YES;
    }
    else
    {
        inst->asic.regs.h264StrmMode = ASIC_H264_NAL_UNIT;
        inst->picParameterSet.byteStream = ENCHW_NO;
        inst->seqParameterSet.byteStream = ENCHW_NO;
        inst->rateControl.sei.byteStream = ENCHW_NO;
        inst->slice.byteStream = ENCHW_NO;
    }

    /* Slice */
    inst->slice.sliceSize = 0;

    /* Macroblock */
    inst->mbPerFrame = width / 16 * height / 16;
    inst->mbPerRow = width / 16;
    inst->mbPerCol = height / 16;

    /* Sequence parameter set */
    inst->seqParameterSet.levelIdc = pEncCfg->profileAndLevel;
    inst->seqParameterSet.picWidthInMbsMinus1 = width / 16 - 1;
    inst->seqParameterSet.picHeightInMapUnitsMinus1 = height / 16 - 1;

    /* Set cropping parameters if required */
    if( pEncCfg->width%16 || pEncCfg->height%16 )
    {
        inst->seqParameterSet.frameCropping = ENCHW_YES;
        inst->seqParameterSet.frameCropRightOffset = (width - pEncCfg->width)/2;
        inst->seqParameterSet.frameCropBottomOffset = (height - pEncCfg->height)/2;
    }

    /* Baseline level 1b is indicated with levelIdc == 11 and constraintSet3 */
    if(pEncCfg->profileAndLevel == H264ENC_BASELINE_LEVEL_1_b)
    {
        inst->seqParameterSet.levelIdc = 11;
        inst->seqParameterSet.constraintSet3 = ENCHW_YES;
    }

    /* Get the index for the table of level maximum values */
    tmp = H264GetLevelIndex(inst->seqParameterSet.levelIdc);
    if(tmp == INVALID_LEVEL)
        return ENCHW_NOK;

    inst->seqParameterSet.levelIdx = tmp;

#if 1   /* enforce maximum frame size in level */
    if(inst->mbPerFrame > H264MaxFS[inst->seqParameterSet.levelIdx])
    {
        return ENCHW_NOK;
    }
#endif

#if 0   /* enforce macroblock rate limit in level */
    {
        u32 mb_rate =
            (pEncCfg->frameRateNum * inst->mbPerFrame) /
            pEncCfg->frameRateDenom;

        if(mb_rate > H264MaxMBPS[inst->seqParameterSet.levelIdx])
        {
            return ENCHW_NOK;
        }
    }
#endif

    /* Picture parameter set */
    inst->picParameterSet.picInitQpMinus26 = (i32) H264ENC_DEFAULT_QP - 26;

    /* Rate control setup */

    /* Maximum bitrate for the specified level */
    bps = H264MaxBR[inst->seqParameterSet.levelIdx];

    {
        h264RateControl_s *rc = &inst->rateControl;

        rc->outRateDenom = pEncCfg->frameRateDenom;
        rc->outRateNum = pEncCfg->frameRateNum;
        rc->mbPerPic = (width / 16) * (height / 16);
        rc->mbRows = height / 16;

        {
            h264VirtualBuffer_s *vb = &rc->virtualBuffer;

            vb->bitRate = bps;
            vb->unitsInTic = pEncCfg->frameRateDenom;
            vb->timeScale = pEncCfg->frameRateNum;
            vb->bufferSize = H264MaxCPBS[inst->seqParameterSet.levelIdx];
        }

        rc->hrd = ENCHW_YES;
        rc->picRc = ENCHW_YES;
        rc->mbRc = ENCHW_YES;
        rc->picSkip = ENCHW_NO;

        rc->qpHdr = H264ENC_DEFAULT_QP;
        rc->qpMin = 10;
        rc->qpMax = 51;

        rc->frameCoded = ENCHW_YES;
        rc->sliceTypeCur = ISLICE;
        rc->sliceTypePrev = PSLICE;
        rc->gopLen = 150;
    }

    /* no SEI by default */
    inst->rateControl.sei.enabled = ENCHW_NO;

    /* Pre processing */
    inst->preProcess.lumWidth = pEncCfg->width;
    inst->preProcess.lumWidthSrc =
        H264GetAllowedWidth(pEncCfg->width, H264ENC_YUV420_PLANAR);

    inst->preProcess.lumHeight = pEncCfg->height;
    inst->preProcess.lumHeightSrc = pEncCfg->height;

    inst->preProcess.horOffsetSrc = 0;
    inst->preProcess.verOffsetSrc = 0;
    inst->preProcess.mbHeight = 16;

    inst->preProcess.rotation = ROTATE_0;
    inst->preProcess.inputFormat = H264ENC_YUV420_PLANAR;
    inst->preProcess.videoStab = 0;

    inst->preProcess.colorConversionType = 0;
    EncSetColorConversion(&inst->preProcess, &inst->asic);

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    CheckParameter

------------------------------------------------------------------------------*/
bool_e CheckParameter(const h264Instance_s * inst)
{
    /* Check crop */
    if(EncPreProcessCheck(&inst->preProcess) != ENCHW_OK)
    {
        return ENCHW_NOK;
    }

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    Round the width to the next multiple of 8 or 16 depending on YUV type.

------------------------------------------------------------------------------*/
i32 H264GetAllowedWidth(i32 width, H264EncPictureType inputType)
{
    if(inputType == H264ENC_YUV420_PLANAR)
    {
        /* Width must be multiple of 16 to make
         * chrominance row 64-bit aligned */
        return ((width + 15) / 16) * 16;
    }
    else
    {   /* H264ENC_YUV420_SEMIPLANAR */
        /* H264ENC_YUV422_INTERLEAVED_YUYV */
        /* H264ENC_YUV422_INTERLEAVED_UYVY */
        return ((width + 7) / 8) * 8;
    }
}
