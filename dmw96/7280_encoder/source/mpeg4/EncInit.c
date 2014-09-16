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
--  Abstract :
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: EncInit.c,v $
--  $Revision: 1.11 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "EncInit.h"
#include "ewl.h"
#include "encasiccontroller.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
#define MP4ENC_MIN_ENC_WIDTH       96
#define MP4ENC_MAX_ENC_WIDTH       1280
#define MP4ENC_MIN_ENC_HEIGHT      96
#define MP4ENC_MAX_ENC_HEIGHT      1280

#define MP4ENC_MAX_MBS_PER_PIC     5120 /* 1280x1024 */
#define MP4ENC_MAX_MBS_PER_PIC_H263     1620    /* 720x576 */
/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static bool_e SetHdr(instance_s * inst, MP4EncStrmType strmType);
static bool_e SetParameter(instance_s * inst, const MP4EncCfg * pEncCfg);
static bool_e CheckParameter(instance_s * inst);

/*------------------------------------------------------------------------------

    EncCheckCfg

    Function checks that the configuration is valid.

    Input   pEncCfg Pointer to configuration structure.

    Return  ENCHW_OK      The configuration is valid.
            ENCHW_NOK     Some of the parameters in configuration are not valid.

------------------------------------------------------------------------------*/
bool_e EncCheckCfg(const MP4EncCfg * pEncCfg)
{
    u32 mbs;

    ASSERT(pEncCfg);

    mbs = ((pEncCfg->height + 15) / 16) * ((pEncCfg->width + 15) / 16);

    /* Encoded image width limits, multiple of 4 */
    if(pEncCfg->width < MP4ENC_MIN_ENC_WIDTH ||
       pEncCfg->width > MP4ENC_MAX_ENC_WIDTH || (pEncCfg->width & 0x3) != 0)
        return ENCHW_NOK;

    /* Encoded image height limits, multiple of 2 */
    if(pEncCfg->height < MP4ENC_MIN_ENC_HEIGHT ||
       pEncCfg->height > MP4ENC_MAX_ENC_HEIGHT || (pEncCfg->height & 0x1) != 0)
        return ENCHW_NOK;

    /* total macroblocks per picture limit */
    if(mbs > MP4ENC_MAX_MBS_PER_PIC)
    {
        return ENCHW_NOK;
    }

    if((pEncCfg->strmType == H263_STRM) && (mbs > MP4ENC_MAX_MBS_PER_PIC_H263))
    {
        return ENCHW_NOK;
    }

    if(pEncCfg->frmRateNum < 1 || pEncCfg->frmRateNum >= (1 << 16))
        return ENCHW_NOK;

    if(pEncCfg->frmRateDenom < 1)
        return ENCHW_NOK;

    /* special allowal of 1000/1001, 0.99 fps by customer request */
    if((pEncCfg->frmRateDenom > pEncCfg->frmRateNum) &&
       !(pEncCfg->frmRateDenom == 1001 && pEncCfg->frmRateNum == 1000))
        return ENCHW_NOK;

    /* Check that stream type is valid */
    if(pEncCfg->strmType > H263_STRM)
        return ENCHW_NOK;

    /* Check that profile and level is valid for MPEG4 stream */
    if((pEncCfg->strmType < H263_STRM) &&
       (pEncCfg->profileAndLevel != MPEG4_SIMPLE_PROFILE_LEVEL_0) &&
       (pEncCfg->profileAndLevel != MPEG4_SIMPLE_PROFILE_LEVEL_0B) &&
       (pEncCfg->profileAndLevel != MPEG4_SIMPLE_PROFILE_LEVEL_1) &&
       (pEncCfg->profileAndLevel != MPEG4_SIMPLE_PROFILE_LEVEL_2) &&
       (pEncCfg->profileAndLevel != MPEG4_SIMPLE_PROFILE_LEVEL_3) &&
       (pEncCfg->profileAndLevel != MPEG4_SIMPLE_PROFILE_LEVEL_4A) &&
       (pEncCfg->profileAndLevel != MPEG4_SIMPLE_PROFILE_LEVEL_5) &&
       (pEncCfg->profileAndLevel != MPEG4_SIMPLE_PROFILE_LEVEL_6) &&
       (pEncCfg->profileAndLevel != 50) &&
       (pEncCfg->profileAndLevel != 51) &&
       (pEncCfg->profileAndLevel != 52) &&
       (pEncCfg->profileAndLevel != MPEG4_ADV_SIMPLE_PROFILE_LEVEL_3) &&
       (pEncCfg->profileAndLevel != MPEG4_ADV_SIMPLE_PROFILE_LEVEL_4) &&
       (pEncCfg->profileAndLevel != MPEG4_ADV_SIMPLE_PROFILE_LEVEL_5))
        return ENCHW_NOK;

    /* Check that profile and level is valid for H263 stream */
    if((pEncCfg->strmType == H263_STRM) &&
       (pEncCfg->profileAndLevel != H263_PROFILE_0_LEVEL_10) &&
       (pEncCfg->profileAndLevel != H263_PROFILE_0_LEVEL_20) &&
       (pEncCfg->profileAndLevel != H263_PROFILE_0_LEVEL_30) &&
       (pEncCfg->profileAndLevel != H263_PROFILE_0_LEVEL_40) &&
       (pEncCfg->profileAndLevel != H263_PROFILE_0_LEVEL_50) &&
       (pEncCfg->profileAndLevel != H263_PROFILE_0_LEVEL_60) &&
       (pEncCfg->profileAndLevel != H263_PROFILE_0_LEVEL_70))
        return ENCHW_NOK;

    /* check HW limitations */
    {
        EWLHwConfig_t cfg = EWLReadAsicConfig();
        /* is MPEG-4 encoding supported */
        if(cfg.mpeg4Enabled == EWL_HW_CONFIG_NOT_SUPPORTED)
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

    EncInit

    Function initializes the Encoder and create new encoder instance.

    Input   pEncCfg     Encoder configuration.
            instAddr    Pointer to instance will be stored in this address

    Return  ENC_OK
            ENC_MEMORY_ERROR 
            ENC_EWL_ERROR
            ENC_EWL_MEMORY_ERROR
            ENC_INVALID_ARGUMENT

------------------------------------------------------------------------------*/
MP4EncRet EncInit(const MP4EncCfg * pEncCfg, instance_s ** instAddr)
{
    instance_s *inst = NULL;
    const void *ewl = NULL;
    u32 mbTotal, asic_mode;
    MP4EncRet ret = ENC_OK;
    EWLInitParam_t param;

    ASSERT(pEncCfg);
    ASSERT(instAddr);

    *instAddr = NULL;

    param.clientType = EWL_CLIENT_TYPE_MPEG4_ENC;

    /* Init EWL */
    if((ewl = EWLInit(&param)) == NULL)
        return ENC_EWL_ERROR;

    /* Encoder instance */
    inst = (instance_s *) EWLcalloc(1, sizeof(instance_s));

    if(inst == NULL)
    {
        ret = ENC_MEMORY_ERROR;
        goto err;
    }

    inst->asic.ewl = ewl;

    EncVosInit(&inst->visualObjectSequence);
    EncVisObInit(&inst->visualObject);
    EncVolInit(&inst->videoObjectLayer);
    EncVopInit(&inst->videoObjectPlane);
    EncSvhInit(&inst->shortVideoHeader);
    EncGoVopInit(&inst->groupOfVideoObjectPlane);

    if(EncAsicControllerInit(&inst->asic) != ENCHW_OK)
    {
        ret = ENC_INVALID_ARGUMENT;
        goto err;
    }

    /* Enable/disable stream headers depending on stream type */
    if(SetHdr(inst, pEncCfg->strmType) != ENCHW_OK)
    {
        ret = ENC_INVALID_ARGUMENT;
        goto err;
    }

    /* Set parameters depending on user config */
    if(SetParameter(inst, pEncCfg) != ENCHW_OK)
    {
        ret = ENC_INVALID_ARGUMENT;
        goto err;
    }

    /* Check parameters */
    if(CheckParameter(inst) != ENCHW_OK)
    {
        ret = ENC_INVALID_ARGUMENT;
        goto err;
    }

    asic_mode = (inst->shortVideoHeader.header ==
                 ENCHW_YES) ? ASIC_H263 : ASIC_MPEG4;

    inst->asic.regs.codingType = asic_mode;

#ifdef MPEG4_HW_VLC_MODE_ENABLED
    if(pEncCfg->strmType == MPEG4_PLAIN_STRM || 
       pEncCfg->strmType == MPEG4_SVH_STRM || 
       pEncCfg->strmType == H263_STRM)
    {
        /* Allocate internal SW/HW shared memories */
        i32 tmp = EncAsicMemAlloc_V2(&inst->asic,
                                     inst->preProcess.lumWidth,
                                     inst->preProcess.lumHeight,
                                     asic_mode);

        if(tmp != ENCHW_OK)
        {

            ret = ENC_EWL_MEMORY_ERROR;
            goto err;
        }

        inst->asic.regs.rlcOutMode = 0; /* use VLC output from HW */
    }
    else
#endif
    {
        mb_s *mb = &inst->macroblock;

        /* Allocate SW memories */
        /* Macroblock header and macroblock coding stuff */
        if(EncMbAlloc(mb) != ENCHW_OK)
        {
            ret = ENC_EWL_MEMORY_ERROR;
            goto err;
        }

        mbTotal = (u32) ((mb->width / 16) * (mb->height / 16));

        /* Allocate internal SW/HW shared memories */
        if(EncAsicMemAlloc(&inst->asic,
                           inst->preProcess.lumWidth,
                           inst->preProcess.lumHeight,
                           ((ENC7280_BUFFER_SIZE_MB * mbTotal) /
                            ENC7280_BUFFER_AMOUNT)) != ENCHW_OK)
        {
            EncMbFree(mb);

            ret = ENC_EWL_MEMORY_ERROR;
            goto err;
        }

        inst->asic.regs.rlcOutMode = 1; /* use RLC output from HW */
    }

    *instAddr = inst;   /* return the instance */

    return ret;

  err:
    if(inst != NULL)
        EWLfree(inst);
    if(ewl != NULL)
        (void) EWLRelease(ewl);

    return ret;
}

/*------------------------------------------------------------------------------

    EncShutdown

    Function frees the encoder instance.

    Input   instance_s *    Pointer to the encoder instance to be freed.
                            After this the pointer is no longer valid.

------------------------------------------------------------------------------*/
void EncShutdown(instance_s * data)
{
    const void *ewl;

    ASSERT(data);

    ewl = data->asic.ewl;

#ifdef MPEG4_HW_VLC_MODE_ENABLED
    if(data->asic.regs.rlcOutMode == 0)
    {
        EncAsicMemFree_V2(&data->asic);
    }
    else
#endif
    {
        EncAsicMemFree(&data->asic);
        EncMbFree(&data->macroblock);
    }

    EWLfree(data);

    (void) EWLRelease(ewl);
}

/*------------------------------------------------------------------------------

    SetHdr

------------------------------------------------------------------------------*/
bool_e SetHdr(instance_s * inst, MP4EncStrmType strmType)
{
    bool_e status = ENCHW_OK;

    ASSERT(inst);

    switch (strmType)
    {
    case MPEG4_PLAIN_STRM:
    case MPEG4_VP_STRM:
    case MPEG4_VP_DP_STRM:
    case MPEG4_VP_DP_RVLC_STRM:
        inst->visualObjectSequence.header = ENCHW_YES;
        inst->visualObject.header = ENCHW_YES;
        inst->videoObject.header = ENCHW_YES;
        inst->videoObjectLayer.header = ENCHW_YES;
        inst->videoObjectPlane.header = ENCHW_YES;
        inst->shortVideoHeader.header = ENCHW_NO;
        inst->groupOfVideoObjectPlane.header = ENCHW_NO;
        break;
    case MPEG4_SVH_STRM:
        inst->visualObjectSequence.header = ENCHW_YES;
        inst->visualObject.header = ENCHW_YES;
        inst->videoObject.header = ENCHW_YES;
        inst->videoObjectLayer.header = ENCHW_NO;
        inst->videoObjectPlane.header = ENCHW_NO;
        inst->shortVideoHeader.header = ENCHW_YES;
        inst->groupOfVideoObjectPlane.header = ENCHW_NO;
        break;
    case H263_STRM:
        inst->visualObjectSequence.header = ENCHW_NO;
        inst->visualObject.header = ENCHW_NO;
        inst->videoObject.header = ENCHW_NO;
        inst->videoObjectLayer.header = ENCHW_NO;
        inst->videoObjectPlane.header = ENCHW_NO;
        inst->shortVideoHeader.header = ENCHW_YES;
        inst->groupOfVideoObjectPlane.header = ENCHW_NO;
        break;
    default:
        status = ENCHW_NOK;
        break;
    }

    return status;
}

/*------------------------------------------------------------------------------

    SetParameter

    Set all parameters in instance to valid values depending on user config.

------------------------------------------------------------------------------*/
bool_e SetParameter(instance_s * inst, const MP4EncCfg * pEncCfg)
{
    i32 width, height;
    vol_s *vol;

    ASSERT(inst != NULL && pEncCfg != NULL);

    vol = &inst->videoObjectLayer;

    /* Visual Object Sequence */
    inst->visualObjectSequence.profile = pEncCfg->profileAndLevel;

    /* Video Object Layer parameters */
    vol->videoObjectLayerWidth = pEncCfg->width;
    vol->videoObjectLayerHeight = pEncCfg->height;
    vol->vopTimeIncRes = pEncCfg->frmRateNum;
    vol->fixedVopTimeInc = pEncCfg->frmRateDenom;

    if((pEncCfg->strmType == MPEG4_VP_DP_STRM) ||
       (pEncCfg->strmType == MPEG4_VP_DP_RVLC_STRM))
        vol->dataPart = ENCHW_YES;
    else
        vol->dataPart = ENCHW_NO;

    if(pEncCfg->strmType == MPEG4_VP_DP_RVLC_STRM)
        vol->rvlc = ENCHW_YES;
    else
        vol->rvlc = ENCHW_NO;

    if((pEncCfg->strmType == MPEG4_VP_STRM) ||
       (pEncCfg->strmType == MPEG4_VP_DP_STRM) ||
       (pEncCfg->strmType == MPEG4_VP_DP_RVLC_STRM))
        vol->resyncMarkerDisable = ENCHW_NO;
    else
        vol->resyncMarkerDisable = ENCHW_YES;

    /* Video Object Plane parameters */
    inst->videoObjectPlane.vopTimeIncRes = vol->vopTimeIncRes;
    inst->videoObjectPlane.hec = ENCHW_NO;
    inst->videoObjectPlane.intraDcVlcThr = 0;

    /* Time code */
    inst->timeCode.vopTimeIncRes = vol->vopTimeIncRes;
    inst->timeCode.timeInc = pEncCfg->frmRateDenom;
    inst->timeCode.fixedVopTimeInc = vol->fixedVopTimeInc;
    inst->timeCode.svHdr = inst->shortVideoHeader.header;
    inst->timeCode.timeCodeHours = 0;
    inst->timeCode.timeCodeMinutes = 0;
    inst->timeCode.timeCodeSecond = 0;
    inst->timeCode.vopTimeInc = 0;
    inst->timeCode.tempRefRem = 0;

    width = 16 * ((vol->videoObjectLayerWidth + 15) / 16);
    height = 16 * ((vol->videoObjectLayerHeight + 15) / 16);

    /* Profile */
    inst->profile.profile = inst->visualObjectSequence.profile;
    inst->profile.videoBufferSize = 1;  /* 1 = Use value from standard VBV model */
    inst->profile.width = width;
    inst->profile.height = height;
    inst->profile.vopTimeIncRes = vol->vopTimeIncRes;
    inst->profile.timeInc = inst->timeCode.timeInc;
    inst->profile.dataPart = vol->dataPart;
    inst->profile.mvRange = 0;
    inst->profile.intraDcVlcThr = inst->videoObjectPlane.intraDcVlcThr;
    inst->profile.mbRc = ENCHW_NO;
    inst->profile.acPred = ENCHW_NO;    /* AC-prediction not supported */

    /* Default video packet size */
    if(vol->resyncMarkerDisable == ENCHW_NO)
        inst->profile.vpSize = 400;
    else
        inst->profile.vpSize = 0;

    /* Setup profile limits based on Profile and Level */
    if(EncInitProfile(&inst->profile) != ENCHW_OK)
        return ENCHW_NOK;

    /* Default bitrate is maximum allowed in profile, limited to 12Mbps */
    inst->profile.bitPerSecond =
        MIN(inst->profile.bitPerSecondMax * 1000, 12000000);

    /* Rate control */
    inst->rateControl.virtualBuffer.bitPerSecond = inst->profile.bitPerSecond;
    inst->rateControl.virtualBuffer.timeInc = inst->timeCode.timeInc;
    inst->rateControl.virtualBuffer.vopTimeIncRes = vol->vopTimeIncRes;
    inst->rateControl.virtualBuffer.setFirstVop = ENCHW_YES;
    inst->rateControl.vopRc = ENCHW_YES;
    inst->rateControl.mbRc = ENCHW_YES;
    inst->rateControl.vopSkip = ENCHW_NO;
    inst->rateControl.qpHdr = 10;
    inst->rateControl.qpHdrPrev = inst->rateControl.qpHdr;
    inst->rateControl.qpMin = 1;
    inst->rateControl.qpMax = 31;
    inst->rateControl.vopTypeCur = IVOP;
    inst->rateControl.vopTypePrev = IVOP;
    inst->rateControl.vopCoded = ENCHW_YES;
    inst->rateControl.mbPerPic = ((width+15) / 16) * ((height+15) / 16);
    inst->rateControl.mbRows = (height + 15) / 16;
    inst->rateControl.coeffCnt = (width / 16) * (height / 16) * 64 * 6;
    inst->rateControl.gopLen = 128;

    inst->rateControl.videoBuffer.videoBufferSize =
        inst->profile.videoBufferSize;
    inst->rateControl.videoBuffer.videoBufferBitCnt =
        inst->profile.videoBufferBitCnt;

    /* Macroblock header */
    inst->macroblock.width = width;
    inst->macroblock.height = height;
    inst->macroblock.svHdr = inst->shortVideoHeader.header;
    inst->macroblock.rvlc = vol->rvlc;
    inst->macroblock.vpSize = (u32) inst->profile.vpSize;
    inst->macroblock.mbPerVop = inst->rateControl.mbPerPic;

    /* Short video header */
    inst->shortVideoHeader.gobPlace = 0;
    inst->shortVideoHeader.videoObjectLayerWidth = vol->videoObjectLayerWidth;
    inst->shortVideoHeader.videoObjectLayerHeight = vol->videoObjectLayerHeight;

    /* Pre processing */
    inst->preProcess.lumWidth = vol->videoObjectLayerWidth;
    inst->preProcess.lumWidthSrc = width;

    inst->preProcess.lumHeight = vol->videoObjectLayerHeight;
    inst->preProcess.lumHeightSrc = vol->videoObjectLayerHeight;

    inst->preProcess.horOffsetSrc = 0;
    inst->preProcess.verOffsetSrc = 0;
    inst->preProcess.mbHeight = 16;

    inst->preProcess.rotation = ROTATE_0;
    inst->preProcess.inputFormat = ENC_YUV420_PLANAR;
    inst->preProcess.videoStab = 0;

    inst->preProcess.colorConversionType = 0;
    EncSetColorConversion(&inst->preProcess, &inst->asic);

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    CheckParameter

------------------------------------------------------------------------------*/
bool_e CheckParameter(instance_s * inst)
{
    /* Check preprocess */
    if(EncPreProcessCheck(&inst->preProcess) != ENCHW_OK)
    {
        return ENCHW_NOK;
    }

    /* H263 */
    if(EncSvhCheck(&inst->shortVideoHeader) != ENCHW_OK)
    {
        return ENCHW_NOK;
    }

    /* Check Profile and Level, not for JPEG */
    if((EncProfileCheck(&inst->profile) != ENCHW_OK))
    {
        return ENCHW_NOK;
    }

    /* Check rate control */
    if(EncRcInit(&inst->rateControl) != ENCHW_OK)
    {
        return ENCHW_NOK;
    }

    return ENCHW_OK;
}
