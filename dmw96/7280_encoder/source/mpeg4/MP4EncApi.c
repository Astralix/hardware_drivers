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
--  Abstract :   MPEG4 Encoder API
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: MP4EncApi.c,v $
--  $Revision: 1.41 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
       Version Information
------------------------------------------------------------------------------*/

#define MP4ENC_API_MAJOR_VERSION 2
#define MP4ENC_API_MINOR_VERSION 0

#define MP4ENC_BUILD_MAJOR 2
#define MP4ENC_BUILD_MINOR 5
#define MP4ENC_BUILD_REVISION 0

#define MP4ENC_SW_BUILD ((MP4ENC_BUILD_MAJOR * 1000000) + \
(MP4ENC_BUILD_MINOR * 1000) + MP4ENC_BUILD_REVISION)

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "mp4encapi.h"
#include "EncInit.h"
#include "EncInstance.h"
#include "EncCodeVop.h"
#include "enccommon.h"
#include "encasiccontroller.h"
#include "EncRateControl.h"

#ifdef VIDEOSTAB_ENABLED
#include "vidstabcommon.h"
#endif

#include "EncTestId.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

#define EVALUATION_LIMIT 300     max number of frames to encode 

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/* Parameter limits */
#define MP4ENCSTRMSTART_MIN_BUF     32
#define MP4ENCSTRMENCODE_MIN_BUF    4096
#define MP4ENC_MAX_VP_SIZE          60000
#define MP4ENC_MAX_PP_INPUT_WIDTH      1920
#define MP4ENC_MAX_PP_INPUT_HEIGHT     1920
#define MP4ENC_MAX_BITRATE             (16000*1000)

/* Tracing macro */
#ifdef MP4ENC_TRACE
#define APITRACE(str) MP4Enc_Trace(str)
#else
#define APITRACE(str)
#endif

#define MPEG4_BUS_ADDRESS_VALID(bus_address)  (((bus_address) != 0) && \
                                              ((bus_address & 0x07) == 0))

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void GetTimeCode(const timeCode_s * timeCode, MP4EncOut * pEncOut);
static void MP4GetGobSizes(const instance_s * inst, u32 * pGobSizeBuf);
static i32 VSCheckSize(u32 inputWidth, u32 inputHeight, u32 stabilizedWidth,
                       u32 stabilizedHeight);

/*------------------------------------------------------------------------------
 Function name : MP4EncGetApiVersion
 Description   : Return the API version info
 Return type   : MP4EncApiVersion 
------------------------------------------------------------------------------*/
MP4EncApiVersion MP4EncGetApiVersion(void)
{
    MP4EncApiVersion ver;

    ver.major = MP4ENC_API_MAJOR_VERSION;
    ver.minor = MP4ENC_API_MINOR_VERSION;

    APITRACE("MP4EncGetApiVersion# ENCHW_OK");

    return ver;
}

/*------------------------------------------------------------------------------
    Function name : MP4EncGetBuild
    Description   : Return the SW and HW build information
    
    Return type   : MP4EncBuild 
    Argument      : void
------------------------------------------------------------------------------*/
MP4EncBuild MP4EncGetBuild(void)
{
    MP4EncBuild ver;

    ver.swBuild = MP4ENC_SW_BUILD;
    ver.hwBuild = EWLReadAsicID();

    APITRACE("MP4EncGetBuild# ENCHW_OK");

    return (ver);
}

/*------------------------------------------------------------------------------
 Function name : MP4EncInit
 Description   : Initialize an encoder instance and returns it to application
 Return type   : MP4EncRet 
 Argument      : pEncCfg - initialization parameters
                 instAddr - where to save the created instance
------------------------------------------------------------------------------*/
MP4EncRet MP4EncInit(const MP4EncCfg * pEncCfg, MP4EncInst * instAddr)
{
    MP4EncRet ret;
    instance_s *pInternalEncInst = NULL;

    APITRACE("MP4EncInit#");

    /* check that right shift on negative numbers is performed signed */
    /*lint -save -e* following check causes multiple lint messages */
#if (((-1) >> 1) != (-1))
#error Right bit-shifting (>>) does not preserve the sign
#endif
    /*lint -restore */

    /* Check for illegal inputs */
    if(pEncCfg == NULL || instAddr == NULL)
    {
        APITRACE("MP4EncInit: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check that configuration is valid */
    if(EncCheckCfg(pEncCfg) == ENCHW_NOK)
    {
        APITRACE("MP4EncInit: ERROR Invalid configuration");
        return ENC_INVALID_ARGUMENT;
    }

    /* Initialize encoder instance and allocate memories */
    ret = EncInit(pEncCfg, &pInternalEncInst);

    if(ret != ENC_OK)
    {
        APITRACE("MP4EncInit: ERROR Initialization failed");
        return ret;
    }

#ifdef INTERNAL_TEST
    /* Configure the encoder instance according to the test vector */
    EncConfigureTestBeforeStream(pInternalEncInst);
#endif

    /* Status == INIT   Initialization succesful */
    pInternalEncInst->encStatus = ENCSTAT_INIT;

    pInternalEncInst->inst = pInternalEncInst;  /* this is used as a checksum */

    *instAddr = (MP4EncInst) pInternalEncInst;

    APITRACE("MP4EncInit: ENCHW_OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
 Function name : MP4EncRelease
 Description   : Releases encoder instance and all associated resource
 Return type   : MP4EncRet 
 Argument      : inst - the instance to be released
------------------------------------------------------------------------------*/
MP4EncRet MP4EncRelease(MP4EncInst inst)
{
    instance_s *pEncInst = (instance_s *) inst;

    APITRACE("MP4EncRelease#");

    /* Check for illegal inputs */
    if(pEncInst == NULL)
    {
        APITRACE("MP4EncRelease: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncRelease: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

#ifdef TRACE_STREAM
    EncCloseStreamTrace();
#endif

    /* Free allocated user data */
    if(pEncInst->visualObjectSequence.userData.header == ENCHW_YES)
        EncUserDataFree(&pEncInst->visualObjectSequence.userData);

    if(pEncInst->visualObject.userData.header == ENCHW_YES)
        EncUserDataFree(&pEncInst->visualObject.userData);

    if(pEncInst->videoObjectLayer.userData.header == ENCHW_YES)
        EncUserDataFree(&pEncInst->videoObjectLayer.userData);

    if(pEncInst->groupOfVideoObjectPlane.userData.header == ENCHW_YES)
        EncUserDataFree(&pEncInst->groupOfVideoObjectPlane.userData);

    pEncInst->inst = NULL;
    /* Free all allocated memories */
    EncShutdown(pEncInst);

    pEncInst = NULL;

    APITRACE("MP4EncRelease: ENCHW_OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
 Function name : MP4EncSetCodingCtrl
 Description   : Sets encoding parameters
 Return type   : MP4EncRet 
 Argument      : inst - the instance in use
                 pCodeParams - user provided parameters
------------------------------------------------------------------------------*/
MP4EncRet MP4EncSetCodingCtrl(MP4EncInst inst,
                              const MP4EncCodingCtrl * pCodeParams)
{
    instance_s *pEncInst = (instance_s *) inst;

    APITRACE("MP4EncSetCodingCtrl#");

    /* Check for illegal inputs */
    if((pEncInst == NULL) || (pCodeParams == NULL))
    {
        APITRACE("MP4EncSetCodingCtrl: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncSetCodingCtrl: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

    /* Check for invalid values */
    if(pEncInst->videoObjectLayer.resyncMarkerDisable == ENCHW_NO)
    {
        profile_s profile = pEncInst->profile;

        profile.vpSize = pCodeParams->vpSize;

        /* Profile limits VP size */
        if(EncProfileCheck(&profile) != ENCHW_OK)
        {
            APITRACE("MP4EncSetCodingCtrl: ERROR VP size above limit");
            return ENC_INVALID_ARGUMENT;
        }

        /* Min and max size for the video packet */
        if(pCodeParams->vpSize < 1 || pCodeParams->vpSize > MP4ENC_MAX_VP_SIZE)
        {
            APITRACE("MP4EncSetCodingCtrl: ERROR Unsupported VP size");
            return ENC_INVALID_ARGUMENT;
        }
    }

    /* GOV, only for MPEG-4 stream */
    if(pEncInst->videoObjectPlane.header == ENCHW_YES)
    {
        if(pCodeParams->insGOV != 0)
            pEncInst->groupOfVideoObjectPlane.header = ENCHW_YES;
        else
            pEncInst->groupOfVideoObjectPlane.header = ENCHW_NO;
    }

    /* HEC, only for VP stream */
    if(pEncInst->videoObjectLayer.resyncMarkerDisable == ENCHW_NO)
    {
        if(pCodeParams->insHEC != 0)
            pEncInst->videoObjectPlane.hec = ENCHW_YES;
        else
            pEncInst->videoObjectPlane.hec = ENCHW_NO;
    }

    /* GOB bitmask, only for SVH/H.263 stream, mask 24 LSB */
    if(pEncInst->shortVideoHeader.header == ENCHW_YES)
        pEncInst->shortVideoHeader.gobPlace = (pCodeParams->insGOB & 0xFFFFFF);

    /* VP size */
    if(pEncInst->videoObjectLayer.resyncMarkerDisable == ENCHW_NO)
    {
        pEncInst->profile.vpSize = pCodeParams->vpSize;
        pEncInst->macroblock.vpSize = (u32) pCodeParams->vpSize;
    }

    APITRACE("MP4EncSetCodingCtrl: ENCHW_OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
 Function name : MP4EncGetCodingCtrl
 Description   : Returns current encoding parameters
 Return type   : MP4EncRet 
 Argument      : inst - the instance in use
                    pCodeParams - palce where parameters are returned
------------------------------------------------------------------------------*/
MP4EncRet MP4EncGetCodingCtrl(MP4EncInst inst, MP4EncCodingCtrl * pCodeParams)
{
    const instance_s *pEncInst = (instance_s *) inst;

    APITRACE("MP4EncGetCodingCtrl#");

    /* Check for illegal inputs */
    if((pEncInst == NULL) || (pCodeParams == NULL))
    {
        APITRACE("MP4EncGetCodingCtrl: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncGetCodingCtrl: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

    /* VP size */
    pCodeParams->vpSize = (i32) pEncInst->macroblock.vpSize;
    /* GOB */
    pCodeParams->insGOB = pEncInst->shortVideoHeader.gobPlace;
    /* HEC */
    pCodeParams->insHEC = pEncInst->videoObjectPlane.hec;
    /* GOV */
    pCodeParams->insGOV = pEncInst->groupOfVideoObjectPlane.header;

    APITRACE("MP4EncGetCodingCtrl: ENCHW_OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
 Function name : MP4EncSetRateCtrl
 Description   : Sets rate control parameters
 Return type   : MP4EncRet 
 Argument      : inst - the instance in use
                    pRateCtrl - user provided parameters
------------------------------------------------------------------------------*/
MP4EncRet MP4EncSetRateCtrl(MP4EncInst inst, const MP4EncRateCtrl * pRateCtrl)
{
    instance_s *pEncInst = (instance_s *) inst;
    profile_s profile;
    rateControl_s *rc;
    u32 custom_vbv;
    i32 bps, tmp;

    APITRACE("MP4EncSetRateCtrl#");

    /* Check for illegal inputs */
    if((pEncInst == NULL) || (pRateCtrl == NULL))
    {
        APITRACE("MP4EncSetRateCtrl: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncSetRateCtrl: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

    /* Check for invalid input values */
    if(pRateCtrl->qpMin < 1 || pRateCtrl->qpMin > 31 ||
       pRateCtrl->qpMax < 1 || pRateCtrl->qpMax > 31 ||
       pRateCtrl->qpMax < pRateCtrl->qpMin)
    {
        APITRACE("MP4EncSetRateCtrl: ERROR Invalid QP min/max");
        return ENC_INVALID_ARGUMENT;
    }

    if((pRateCtrl->qpHdr != -1) &&
      (pRateCtrl->qpHdr < 1 || pRateCtrl->qpHdr > 31 ||
       (u32)pRateCtrl->qpHdr < pRateCtrl->qpMin ||
       (u32)pRateCtrl->qpHdr > pRateCtrl->qpMax))
    {
        APITRACE("MP4EncSetRateCtrl: ERROR QP out of range");
        return ENC_INVALID_ARGUMENT;
    }

    if(pRateCtrl->gopLen < 1 || pRateCtrl->gopLen > 128)
    {
        APITRACE("MP4EncSetRateCtrl: ERROR Invalid GOP length");
        return ENC_INVALID_ARGUMENT;
    }

    /* Bitrate affects only when rate control is enabled */
    if((pRateCtrl->vopRc || pRateCtrl->mbRc ||
        pRateCtrl->vopSkip || pRateCtrl->vbv) &&
       (pRateCtrl->bitPerSecond < 10000 ||
        pRateCtrl->bitPerSecond > MP4ENC_MAX_BITRATE))
    {
        APITRACE("MP4EncSetRateCtrl: ERROR Invalid bitPerSecond");
        return ENC_INVALID_ARGUMENT;
    }

    rc = &pEncInst->rateControl;

    /* Saturates really high settings */
    bps = pRateCtrl->bitPerSecond;
    /* bits per unpacked frame */
    tmp = 3 * 8 * rc->mbPerPic * 256 / 2;
    /* bits per second */
    tmp = EncCalculate(tmp, rc->virtualBuffer.vopTimeIncRes, 
                            rc->virtualBuffer.vopTimeInc);
    if (bps > (tmp / 2))
        bps = tmp / 2;

    /* Check for invalid bitrate */
    profile = pEncInst->profile;
    profile.bitPerSecond = bps;
    if(pRateCtrl->vbv != 0 && EncProfileCheck(&profile) != ENCHW_OK)
    {
        APITRACE("MP4EncSetRateCtrl: ERROR Bitrate above profile limit");
        return ENC_INVALID_ARGUMENT;
    }

    if(pRateCtrl->vbv > 1 &&
       pRateCtrl->vbv != (pEncInst->profile.videoBufferSize >> 14))
    {
        custom_vbv = 1; /* signal to insert VOL/VBV control params */
    }
    else
    {
        custom_vbv = 0;
    }

    /* Set the video buffer size, 1 == use the maximum value in profile */
    pEncInst->profile.videoBufferSize = pRateCtrl->vbv;

    /* Calculate the videoBufferSize in bits */
    (void) EncInitProfile(&pEncInst->profile);

    pEncInst->profile.bitPerSecond = bps;

    /* Set the parameters to rate control */
    if(pRateCtrl->vbv != 0)
        rc->videoBuffer.vbv = ENCHW_YES;
    else
        rc->videoBuffer.vbv = ENCHW_NO;

    rc->videoBuffer.videoBufferSize = pEncInst->profile.videoBufferSize;
    rc->videoBuffer.videoBufferBitCnt = pEncInst->profile.videoBufferBitCnt;

    if(pRateCtrl->vopRc != 0)
        rc->vopRc = ENCHW_YES;
    else
        rc->vopRc = ENCHW_NO;

    if(pRateCtrl->mbRc != 0)
        rc->mbRc = ENCHW_YES;
    else
        rc->mbRc = ENCHW_NO;

    if(pRateCtrl->vopSkip != 0)
        rc->vopSkip = ENCHW_YES;
    else
        rc->vopSkip = ENCHW_NO;

    rc->qpHdr = pRateCtrl->qpHdr;
    rc->qpMin = pRateCtrl->qpMin;
    rc->qpMax = pRateCtrl->qpMax;
    rc->virtualBuffer.bitPerSecond = bps;
    rc->gopLen = pRateCtrl->gopLen;

    /* Rate control init depends if this is before the first VOP */
    if(pEncInst->encStatus == ENCSTAT_INIT ||
       pEncInst->encStatus == ENCSTAT_START_STREAM)
    {
        rc->virtualBuffer.setFirstVop = ENCHW_YES;
    }

    (void) EncRcInit(rc);   /* all parameters were checked before */

    /* add VBV info to VOL header */
    if(rc->videoBuffer.vbv == ENCHW_YES && custom_vbv != 0)
    {
        u32 tmp;
        volControlParameter_s *vcp =
            &pEncInst->videoObjectLayer.volControlParameter;

        pEncInst->videoObjectLayer.isVolControlParameter = ENCHW_YES;
        vcp->vbvParameter = ENCHW_YES;

        /* bit rate in 400 bits units (30-bit) */
        tmp = ((pEncInst->profile.bitPerSecond + 399) / 400) & 0x3fffffff;

        vcp->firstHalfBitRate = tmp >> 15;
        vcp->latterHalfBitRate = tmp & 0x7fffU;

        /* vbv buffer size in 16384 bits units (18-bit) */
        tmp = pEncInst->profile.videoBufferSize >> 14;

        vcp->firstHalfVbvBufferSize = tmp >> 3;
        vcp->latterHalfVbvBufferSize = tmp & 0x07U;

        /* vbv buffer occupancy in 64 bits units (26-bit) */
        tmp = pEncInst->profile.videoBufferBitCnt >> 6;

        vcp->firstHalfVbvOccupancy = tmp >> 15;
        vcp->latterHalfVbvOccupancy = tmp & 0x7fffU;
    }

    APITRACE("MP4EncSetRateCtrl: ENCHW_OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
 Function name : MP4EncGetRateCtrl
 Description   : Return current rate control parameters
 Return type   : MP4EncRet 
 Argument      : inst - the instance in use
                 pRateCtrl - place where parameters are returned
------------------------------------------------------------------------------*/
MP4EncRet MP4EncGetRateCtrl(MP4EncInst inst, MP4EncRateCtrl * pRateCtrl)
{
    const instance_s *pEncInst = (instance_s *) inst;
    const rateControl_s *rc;

    APITRACE("MP4EncGetRateCtrl#");

    /* Check for illegal inputs */
    if((pEncInst == NULL) || (pRateCtrl == NULL))
    {
        APITRACE("MP4EncGetRateCtrl: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncGetRateCtrl: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

    rc = &pEncInst->rateControl;

    pRateCtrl->vopRc = rc->vopRc;
    pRateCtrl->mbRc = rc->mbRc;
    pRateCtrl->vopSkip = rc->vopSkip;
    pRateCtrl->qpHdr = rc->qpHdr;
    pRateCtrl->qpMin = rc->qpMin;
    pRateCtrl->qpMax = rc->qpMax;
    pRateCtrl->bitPerSecond = rc->virtualBuffer.bitPerSecond;
    pRateCtrl->gopLen = rc->gopLen;

    /* Return the video buffer size in units of 16384 bits */
    pRateCtrl->vbv = rc->videoBuffer.videoBufferSize / 16384;

    APITRACE("MP4EncGetRateCtrl: ENCHW_OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
 Function name : MP4EncSetUsrData
 Description   : Set user data into the stream
 Return type   : MP4EncRet 
 Argument      : inst - the instance in use
 Argument      : u8 * pBuf  - user data buffer
 Argument      : length - amount of data in bytes
 Argument      : type - type of user data
------------------------------------------------------------------------------*/
MP4EncRet MP4EncSetUsrData(MP4EncInst inst, const u8 * pBuf, u32 length,
                           MP4EncUsrDataType type)
{
    instance_s *pEncInst = (instance_s *) inst;
    bool_e stat;
    userData_s *userData;

    APITRACE("MP4EncSetUsrData#");

    /* Check for illegal inputs */
    if(pEncInst == NULL)
    {
        APITRACE("MP4EncSetUsrData: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncSetUsrData: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

    /* Choose correct user data structure */
    switch (type)
    {
    case MPEG4_VOS_USER_DATA:
        userData = &pEncInst->visualObjectSequence.userData;
        break;
    case MPEG4_VO_USER_DATA:
        userData = &pEncInst->visualObject.userData;
        break;
    case MPEG4_VOL_USER_DATA:
        userData = &pEncInst->videoObjectLayer.userData;
        break;
    case MPEG4_GOV_USER_DATA:
        userData = &pEncInst->groupOfVideoObjectPlane.userData;
        break;
    default:
        APITRACE("MP4EncSetUsrData: ERROR Invalid type");
        return ENC_INVALID_ARGUMENT;
    }

    /* Check for invalid input values */
    if((pBuf == NULL) || (length == 0))
    {
        /* disable user data */
        if(userData->data != NULL)
            EncUserDataFree(userData);
    }
    else
    {
        /* Allocate memory for user data and copy the data */
        stat = EncUserDataAlloc(userData, pBuf, length);

        if(stat != ENCHW_OK)
        {
            APITRACE("MP4EncSetUsrData: ERROR Memory allocation failed");
            return ENC_MEMORY_ERROR;
        }
    }

    APITRACE("MP4EncSetUsrData: ENCHW_OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
 Function name : MP4EncStrmStart
 Description   : Starts a new stream
 Return type   : MP4EncRet 
 Argument      : inst - encoder instance
 Argument      : pEncIn - user provided input parameters
                 pEncOut - place where output info is returned
------------------------------------------------------------------------------*/
MP4EncRet MP4EncStrmStart(MP4EncInst inst, const MP4EncIn * pEncIn,
                          MP4EncOut * pEncOut)
{
    instance_s *pEncInst = (instance_s *) inst;
    stream_s stream;

    APITRACE("MP4EncStrmStart#");

    /* Check for illegal inputs */
    if((pEncInst == NULL) || (pEncIn == NULL) || (pEncOut == NULL))
    {
        APITRACE("MP4EncStrmStart: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncStrmStart: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

    /* Check status */
    if(pEncInst->encStatus != ENCSTAT_INIT)
    {
        APITRACE("MP4EncStrmStart: ERROR Invalid status");
        return ENC_INVALID_STATUS;
    }

    /* Check for invalid input values */
    if((pEncIn->pOutBuf == NULL) ||
       (pEncIn->outBufSize < MP4ENCSTRMSTART_MIN_BUF))
    {
        APITRACE("MP4EncStrmStart: ERROR Invalid input");
        return ENC_INVALID_ARGUMENT;
    }

    /* Set stream buffer, the size has been checked */
    (void) EncSetBuffer(&stream, (u8 *) pEncIn->pOutBuf,
                        (u32) pEncIn->outBufSize);

#ifdef TRACE_STREAM
    /* Open stream tracing */
    EncOpenStreamTrace("stream.trc");

    traceStream.frameNum = 0;
    traceStream.id = 0; /* Stream generated by SW */
    traceStream.bitCnt = 0;  /* New frame */
#endif

    /* Write stream headers */
    EncVosHrd(&stream, &pEncInst->visualObjectSequence);
    EncVisObHdr(&stream, &pEncInst->visualObject);
    EncVidObHdr(&stream, &pEncInst->videoObject);
    EncVolHdr(&stream, &pEncInst->videoObjectLayer);

    if(stream.overflow == ENCHW_YES)
    {
        pEncOut->strmSize = 0;
        APITRACE("MP4EncStrmStart: ERROR Output buffer too small");
        return ENC_OUTPUT_BUFFER_OVERFLOW;
    }

    /* Bytes generated */
    pEncOut->strmSize = (i32) stream.byteCnt;

    /* Status == START_STREAM   Stream started */
    pEncInst->encStatus = ENCSTAT_START_STREAM;

#ifdef VIDEOSTAB_ENABLED
    /* new stream so reset the stabilization */
    VSAlgReset(&pEncInst->vsSwData);
#endif

    APITRACE("MP4EncStrmStart: ENCHW_OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
 Function name : MP4EncStrmEncode
 Description   : Encodes a new picture
 Return type   : MP4EncRet 
 Argument      : inst - encoder instance
 Argument      : pEncIn - user provided input parameters
                 pEncOut - place where output info is returned
------------------------------------------------------------------------------*/
MP4EncRet MP4EncStrmEncode(MP4EncInst inst, const MP4EncIn * pEncIn,
                           MP4EncOut * pEncOut)
{
    instance_s *pEncInst = (instance_s *) inst;
    vop_s *vop;
    govop_s *govop;
    svh_s *svh;
    mb_s *mb;
    asicData_s *asic;
    rateControl_s *rc;
    timeCode_s *timeCode;
    timeCode_s timeCodeTmp;
    stream_s stream;
    vopType_e vopType;
    encodeFrame_e ret;

    APITRACE("MP4EncStrmEncode#");

    /* Check for illegal inputs */
    if((pEncInst == NULL) || (pEncIn == NULL) || (pEncOut == NULL))
    {
        APITRACE("MP4EncStrmEncode: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncStrmEncode: ERROR Existing instance");
        return ENC_INSTANCE_ERROR;
    }

    /* Check status, INIT and ERROR not allowed */
    if((pEncInst->encStatus != ENCSTAT_START_STREAM) &&
       (pEncInst->encStatus != ENCSTAT_START_VOP) &&
       (pEncInst->encStatus != ENCSTAT_NEW_REF_IMG))
    {
        APITRACE("MP4EncStrmEncode: ERROR Invalid status");
        return ENC_INVALID_STATUS;
    }

    /* Check for invalid input values */
    if(pEncInst->asic.regs.rlcOutMode == 0 &&
       (!MPEG4_BUS_ADDRESS_VALID(pEncIn->outBufBusAddress)))
    {
        APITRACE("MP4EncStrmEncode: ERROR Invalid input");
        return ENC_INVALID_ARGUMENT;
    }

    if((pEncIn->pOutBuf == NULL) ||
       (pEncIn->vopType > (u32) PREDICTED_VOP) ||
       (pEncIn->outBufSize < MP4ENCSTRMENCODE_MIN_BUF))
    {
        APITRACE("MP4EncStrmEncode: ERROR Invalid input");
        return ENC_INVALID_ARGUMENT;
    }

    switch (pEncInst->preProcess.inputFormat)
    {
    case ENC_YUV420_PLANAR:
        if(!MPEG4_BUS_ADDRESS_VALID(pEncIn->busChromaV))
        {
            APITRACE("MP4EncStrmEncode: ERROR Invalid input busChromaU");
            return ENC_INVALID_ARGUMENT;
        }
        /* fall through */
    case ENC_YUV420_SEMIPLANAR:
        if(!MPEG4_BUS_ADDRESS_VALID(pEncIn->busChromaU))
        {
            APITRACE("MP4EncStrmEncode: ERROR Invalid input busChromaU");
            return ENC_INVALID_ARGUMENT;
        }
        /* fall through */
    case ENC_YUV422_INTERLEAVED_YUYV:
    case ENC_YUV422_INTERLEAVED_UYVY:
    case ENC_RGB565:
    case ENC_BGR565:
    case ENC_RGB555:
    case ENC_BGR555:
    case ENC_RGB444:
    case ENC_BGR444:
        if(!MPEG4_BUS_ADDRESS_VALID(pEncIn->busLuma))
        {
            APITRACE("MP4EncStrmEncode: ERROR Invalid input busLuma");
            return ENC_INVALID_ARGUMENT;
        }
        break;
    default:
        APITRACE("MP4EncStrmEncode: ERROR Invalid input format");
        return ENC_INVALID_ARGUMENT;
    }

    if(pEncInst->preProcess.videoStab)
    {
        if(!MPEG4_BUS_ADDRESS_VALID(pEncIn->busLumaStab))
        {
            APITRACE("MP4EncStrmEncode: ERROR Invalid input busLumaStab");
            return ENC_INVALID_ARGUMENT;
        }
    }

    vop = &pEncInst->videoObjectPlane;
    govop = &pEncInst->groupOfVideoObjectPlane;
    svh = &pEncInst->shortVideoHeader;
    mb = &pEncInst->macroblock;
    rc = &pEncInst->rateControl;
    timeCode = &pEncInst->timeCode;
    asic = &pEncInst->asic;

    /* Clear the output structure */
    pEncOut->vopType = NOTCODED_VOP;
    pEncOut->strmSize = 0;
    GetTimeCode(timeCode, pEncOut);

    /* Clear the first element of the video packet size table */
    if(pEncIn->pVpBuf != NULL && pEncIn->vpBufSize >= sizeof(*pEncIn->pVpBuf))
        pEncIn->pVpBuf[0] = 0;

#ifdef EVALUATION_LIMIT
    /* Check for evaluation limit */
    if(pEncInst->frameCnt >= EVALUATION_LIMIT)
    {
        APITRACE("MP4EncStrmEncode: Evaluation limit exceeded");
        return ENC_OK;
    }
#endif

    vopType = (vopType_e) pEncIn->vopType;

    /* Status affects the VOP type */
    if((pEncInst->encStatus == ENCSTAT_START_STREAM) ||
       (pEncInst->encStatus == ENCSTAT_NEW_REF_IMG))
    {
        pEncInst->encStatus = ENCSTAT_NEW_REF_IMG;
        vopType = IVOP;
    }

    /* Set stream buffer, the size has been checked */
    (void) EncSetBuffer(&stream, (u8 *) pEncIn->pOutBuf,
                        (u32) pEncIn->outBufSize);

#ifdef TRACE_STREAM
    traceStream.id = 0; /* Stream generated by SW */
    traceStream.bitCnt = 0;  /* New frame */
    traceStream.frameNum = pEncInst->frameCnt;
#endif

    /* GoVop */
    if(govop->header == ENCHW_YES)
    {
        EncGoVopHdr(&stream, govop);

        if(stream.overflow == ENCHW_YES)
        {
            APITRACE("MP4EncStrmEncode: ERROR Output buffer too small");
            return ENC_OUTPUT_BUFFER_OVERFLOW;
        }

        pEncOut->strmSize = (i32) stream.byteCnt;
        pEncOut->vopType = NOTCODED_VOP;
        govop->header = ENCHW_NO;
        pEncInst->encStatus = ENCSTAT_NEW_REF_IMG;
        APITRACE("MP4EncStrmEncode: ENCHW_OK, GOV header");
        return ENC_GOV_READY;
    }

    /* Vop time increment */
    EncTimeIncrement(timeCode, pEncIn->timeIncr);
    EncCopyTimeCode(&timeCodeTmp, timeCode);

    /* VOP type before rate control */
    rc->vopTypeCur = vopType;
    rc->vopCoded = ENCHW_YES;
    EncBeforeVopRc(rc, timeCode->timeInc);

    /* Rate control may choose to skip the VOP */
    if(rc->vopCoded != ENCHW_YES)
    {
        APITRACE("MP4EncStrmEncode: ENCHW_OK, VOP skipped");
        return ENC_VOP_READY;
    }

    /* Final time code */
    EncTimeCode(timeCode, govop);

    /* Round control */
    vop->roundControl ^= 1;
    if((svh->header == ENCHW_YES) && (svh->plusHeader == ENCHW_NO))
    {
        vop->roundControl = 0;
    }
    svh->roundControl = vop->roundControl;

    ASSERT(vop->roundControl == 0 || vop->roundControl == 1);

    /* Set ASIC input image */
    asic->regs.inputLumBase = pEncIn->busLuma;
    asic->regs.inputCbBase = pEncIn->busChromaU;
    asic->regs.inputCrBase = pEncIn->busChromaV;

    /* setup stabilization */
    if(pEncInst->preProcess.videoStab)
    {
        asic->regs.vsNextLumaBase = pEncIn->busLumaStab;
    }

    /* update any cropping/rotation/filling */
    EncPreProcess(&pEncInst->asic, &pEncInst->preProcess);

    /* Set video packet size table */
    if(pEncIn->pVpBuf != NULL && pEncIn->vpBufSize >= sizeof(*(pEncIn->pVpBuf)))
    {
        mb->vpSizes = (i32 *) pEncIn->pVpBuf;
        mb->vpSizeSpace = pEncIn->vpBufSize / sizeof(*(pEncIn->pVpBuf));
    }
    else
    {
        mb->vpSizes = NULL;
        mb->vpSizeSpace = 0;
    }

    if(pEncIn->vpBufSize < (sizeof(*(pEncIn->pVpBuf)) * (svh->gobs + 1)))
    {
        mb->vpSizes = NULL;
        mb->vpSizeSpace = 0;
    }

    /* Set final VOP parameters */
    vop->vopType = rc->vopTypeCur;
    vop->vopCoded = rc->vopCoded;
    vop->moduloTimeBase = timeCode->moduloTimeBase;
    vop->vopTimeInc = timeCode->vopTimeInc;
    svh->tempRef = timeCode->tempRef;
    svh->vopType = vop->vopType;
    svh->gobNum = 0;

    /* Check if HW resource is available */
    if(EWLReserveHw(pEncInst->asic.ewl) == EWL_ERROR)
    {
        APITRACE("MP4EncStrmEncode: ERROR HW unavailable");
        EncCopyTimeCode(timeCode, &timeCodeTmp);    /* restore time code */
        return ENC_HW_RESERVED;
    }

#ifdef INTERNAL_TEST
    /* Configure the encoder instance according to the test vector */
    EncConfigureTestBeforeFrame(pEncInst);
#endif

    /* Encode one VOP */
#ifdef MPEG4_HW_VLC_MODE_ENABLED
    if(asic->regs.rlcOutMode == 0)
    {
        asic->regs.outputStrmBase = pEncIn->outBufBusAddress;
        asic->regs.outputStrmSize = pEncIn->outBufSize;

        ret = EncCodeVop_V2(pEncInst, &stream);
    }
    else
#endif
    {
        ret = EncCodeVop(pEncInst, &stream);
    }

    if(ret != ENCODE_OK)
    {
        /* Error has occured and the frame is invalid */
        MP4EncRet to_user;

        switch (ret)
        {
        case ENCODE_TIMEOUT:
            APITRACE("MP4EncStrmEncode: ERROR HW timeout");
            to_user = ENC_HW_TIMEOUT;
            break;
        case ENCODE_DATA_ERROR:
            APITRACE("MP4EncStrmEncode: ERROR HW data corrupted");
            to_user = ENC_HW_DATA_ERROR;
            break;
        case ENCODE_HW_ERROR:
            APITRACE("MP4EncStrmEncode: ERROR HW failure");
            to_user = ENC_HW_BUS_ERROR;
            break;
        case ENCODE_HW_RESET:
            APITRACE("MP4EncStrmEncode: ERROR HW reset detected");
            to_user = ENC_HW_RESET;
            break;
        case ENCODE_SYSTEM_ERROR:
        default:
            /* System error has occured, encoding can't continue */
            pEncInst->encStatus = ENCSTAT_ERROR;
            APITRACE("MP4EncStrmEncode: ERROR Fatal system error");
            to_user = ENC_SYSTEM_ERROR;
        }

        return to_user;
    }

#ifdef TRACE_RECON
    EncDumpRecon(asic);
#endif

    pEncInst->frameCnt++;

#ifdef VIDEOSTAB_ENABLED
    /* Finalize video stabilization */
    if(pEncInst->preProcess.videoStab)
    {
        u32 no_motion;

        VSReadStabData(pEncInst->asic.regs.regMirror, &pEncInst->vsHwData);

        no_motion = VSAlgStabilize(&pEncInst->vsSwData, &pEncInst->vsHwData);
        if(no_motion)
        {
            VSAlgReset(&pEncInst->vsSwData);
        }

        /* update offset after stabilization */
        VSAlgGetResult(&pEncInst->vsSwData, &pEncInst->preProcess.horOffsetSrc,
                       &pEncInst->preProcess.verOffsetSrc);
    }
#endif

    /* After stream buffer overflow discard the coded vop */
    if(stream.overflow == ENCHW_YES)
    {
        /* Error has occured and the VOP is invalid. */
        EncCopyTimeCode(timeCode, &timeCodeTmp);
        APITRACE("MP4EncStrmEncode: ERROR Output buffer too small");
        return ENC_OUTPUT_BUFFER_OVERFLOW;
    }

    {
        /* Rate control action after vop */
        EncAfterVopRc(rc, (i32) asic->regs.rlcCount,
                      (i32) stream.byteCnt, (i32) asic->regs.qpSum);

        /* After vbv underflow discard the coded vop and go back old time, 
         * just like not coded vop */
        if(rc->videoBuffer.underflow == ENCHW_YES)
        {
            EncCopyTimeCode(timeCode, &timeCodeTmp);
            APITRACE("MP4EncStrmEncode: ENCHW_OK, VOP discarded by VBV");
            return ENC_VOP_READY;
        }
    }

    /* Use the reconstructed frame as the reference for the next frame */
    EncAsicRecycleInternalImage(&asic->regs);

    /* Store the stream size, vop type and time code in output structure */
    pEncOut->strmSize = (i32) stream.byteCnt;
    pEncOut->vopType = (MP4EncVopType) vop->vopType;
    GetTimeCode(timeCode, pEncOut);

    pEncInst->encStatus = ENCSTAT_START_VOP;

#ifdef MPEG4_HW_VLC_MODE_ENABLED
    if(mb->vpSizes != NULL)
    {
        MP4GetGobSizes(pEncInst, (u32 *) mb->vpSizes);
    }
#endif

    APITRACE("MP4EncStrmEncode: ENCHW_OK");
    return ENC_VOP_READY;
}

/*------------------------------------------------------------------------------
 Function name : MP4EncStrmEnd
 Description   : Ends a stream
 Return type   : MP4EncRet 
 Argument      : inst - encoder instance
 Argument      : pEncIn - user provided input parameters
                 pEncOut - place where output info is returned
------------------------------------------------------------------------------*/
MP4EncRet MP4EncStrmEnd(MP4EncInst inst, MP4EncIn * pEncIn, MP4EncOut * pEncOut)
{
    instance_s *pEncInst = (instance_s *) inst;
    stream_s stream;

    APITRACE("MP4EncStrmEnd#");

    /* Check for illegal inputs */
    if((pEncInst == NULL) || (pEncIn == NULL) || (pEncOut == NULL))
    {
        APITRACE("MP4EncStrmEnd: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncStrmEnd: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

    /* Check status, this also makes sure that the instance is valid */
    if((pEncInst->encStatus != ENCSTAT_START_STREAM) &&
       (pEncInst->encStatus != ENCSTAT_START_VOP) &&
       (pEncInst->encStatus != ENCSTAT_NEW_REF_IMG))
    {
        APITRACE("MP4EncStrmEnd: ERROR Invalid status");
        return ENC_INVALID_STATUS;
    }

    /* Check for invalid input values */
    if(pEncIn->pOutBuf == NULL)
    {
        APITRACE("MP4EncStrmEnd: ERROR Null output buffer");
        return ENC_INVALID_ARGUMENT;
    }

    /* Check for invalid input values */
    if(pEncIn->outBufSize < MP4ENCSTRMSTART_MIN_BUF)
    {
        APITRACE("MP4EncStrmEnd: ERROR Output buffer too small");
        return ENC_INVALID_ARGUMENT;
    }

    /* Set stream buffer and check the size */
    if(EncSetBuffer(&stream, (u8 *) pEncIn->pOutBuf, (u32) pEncIn->outBufSize)
       != ENCHW_OK)
    {
        APITRACE("MP4EncStrmEnd: ERROR Output buffer too small");
        return ENC_INVALID_ARGUMENT;
    }

    /* Write stream end header */
    EncVosEndHrd(&stream, &pEncInst->visualObjectSequence);

    /* Bytes generated */
    pEncOut->strmSize = (i32) stream.byteCnt;

    /* Status == INIT   Stream ended, next stream can be started */
    pEncInst->encStatus = ENCSTAT_INIT;

    APITRACE("MP4EncStrmEnd: ENCHW_OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : VSCheckSize
    Description     : 
    Return type     : i32 
    Argument        : u32 inputWidth
    Argument        : u32 inputHeight
    Argument        : u32 stabilizedWidth
    Argument        : u32 stabilizedHeight
------------------------------------------------------------------------------*/
i32 VSCheckSize(u32 inputWidth, u32 inputHeight, u32 stabilizedWidth,
                u32 stabilizedHeight)
{
    /* Input picture minimum dimensions */
    if((inputWidth < 104) || (inputHeight < 104))
        return 1;

    /* Stabilized picture minimum  values */
    if((stabilizedWidth < 96) || (stabilizedHeight < 96))
        return 1;

    /* Stabilized dimensions multiple of 4 */
    if(((stabilizedWidth & 3) != 0) || ((stabilizedHeight & 3) != 0))
        return 1;

    /* Edge >= 4 pixels */
    if((inputWidth < (stabilizedWidth + 8)) ||
       (inputHeight < (stabilizedHeight + 8)))
        return 1;

    return 0;
}

/*------------------------------------------------------------------------------
    Function name   : MP4EncSetPreProcessing
    Description     : Sets the preprocessing parameters
    Return type     : MP4EncRet 
    Argument        : inst - encoder instance in use
    Argument        : pPreProcCfg - user provided parameters
------------------------------------------------------------------------------*/
MP4EncRet MP4EncSetPreProcessing(MP4EncInst inst,
                                 const MP4EncPreProcessingCfg * pPreProcCfg)
{
    instance_s *pEncInst = (instance_s *) inst;
    preProcess_s pp_tmp;

    APITRACE("MP4EncSetPreProcessing#");

    /* Check for illegal inputs */
    if((inst == NULL) || (pPreProcCfg == NULL))
    {
        APITRACE("MP4EncSetPreProcessing: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncSetPreProcessing: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

    /* check HW limitations */
    {
        EWLHwConfig_t cfg = EWLReadAsicConfig();

        /* is video stabilization supported? */
        if(cfg.vsEnabled == EWL_HW_CONFIG_NOT_SUPPORTED &&
           pPreProcCfg->videoStabilization != 0)
        {
            APITRACE("MP4EncSetPreProcessing: ERROR Stabilization not supported");
            return ENC_INVALID_ARGUMENT;
        }
        if(cfg.rgbEnabled == EWL_HW_CONFIG_NOT_SUPPORTED &&
           pPreProcCfg->inputType > ENC_YUV422_INTERLEAVED_UYVY)
        {
            APITRACE("MP4EncSetPreProcessing: ERROR RGB input not supported");
            return ENC_INVALID_ARGUMENT;
        }
    }

    if(pPreProcCfg->origWidth > MP4ENC_MAX_PP_INPUT_WIDTH ||
       pPreProcCfg->origHeight > MP4ENC_MAX_PP_INPUT_HEIGHT)
    {
        APITRACE("MP4EncSetPreProcessing: ERROR Too big input image");
        return ENC_INVALID_ARGUMENT;
    }

    pp_tmp.lumHeightSrc = pPreProcCfg->origHeight;
    pp_tmp.lumWidthSrc = pPreProcCfg->origWidth;
    if(pPreProcCfg->videoStabilization == 0)
    {
        pp_tmp.horOffsetSrc = pPreProcCfg->xOffset;
        pp_tmp.verOffsetSrc = pPreProcCfg->yOffset;
    }
    else
    {
        pp_tmp.horOffsetSrc = pp_tmp.verOffsetSrc = 0;
    }

    pp_tmp.lumWidth = pEncInst->preProcess.lumWidth;
    pp_tmp.lumHeight = pEncInst->preProcess.lumHeight;
    pp_tmp.mbHeight = pEncInst->preProcess.mbHeight;

    pp_tmp.rotation = pPreProcCfg->rotation;
    pp_tmp.inputFormat = pPreProcCfg->inputType;

    pp_tmp.videoStab = pPreProcCfg->videoStabilization;

    /* Check for invalid values */
    if(EncPreProcessCheck(&pp_tmp) != ENCHW_OK)
    {
        APITRACE("MP4EncSetPreProcessing: ERROR Invalid cropping values");
        return ENC_INVALID_ARGUMENT;
    }

#ifdef VIDEOSTAB_ENABLED
    if(pp_tmp.videoStab != 0)
    {
        u32 width = pp_tmp.lumWidth;
        u32 height = pp_tmp.lumHeight;

        if(pp_tmp.rotation)
        {
            u32 tmp;

            tmp = width;
            width = height;
            height = tmp;
        }

        if(VSCheckSize(pp_tmp.lumWidthSrc, pp_tmp.lumHeightSrc, width, height)
           != 0)
        {
            APITRACE
                ("MP4EncSetPreProcessing: ERROR Invalid size for stabilization");
            return ENC_INVALID_ARGUMENT;
        }

        VSAlgInit(&pEncInst->vsSwData, pp_tmp.lumWidthSrc, pp_tmp.lumHeightSrc,
                  width, height);

        VSAlgGetResult(&pEncInst->vsSwData, &pp_tmp.horOffsetSrc,
                       &pp_tmp.verOffsetSrc);
    }
#endif

    pp_tmp.colorConversionType = pPreProcCfg->colorConversion.type;
    pp_tmp.colorConversionCoeffA = pPreProcCfg->colorConversion.coeffA;
    pp_tmp.colorConversionCoeffB = pPreProcCfg->colorConversion.coeffB;
    pp_tmp.colorConversionCoeffC = pPreProcCfg->colorConversion.coeffC;
    pp_tmp.colorConversionCoeffE = pPreProcCfg->colorConversion.coeffE;
    pp_tmp.colorConversionCoeffF = pPreProcCfg->colorConversion.coeffF;
    EncSetColorConversion(&pp_tmp, &pEncInst->asic);

    {
        preProcess_s *pp = &pEncInst->preProcess;

        if(EWLmemcpy(pp, &pp_tmp, sizeof(preProcess_s)) != pp)
        {
            APITRACE("MP4EncSetPreProcessing: memcpy failed");
            return ENC_SYSTEM_ERROR;
        }
    }

    APITRACE("MP4EncSetPreProcessing: OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : MP4EncGetPreProcessing
    Description     : Returns current preprocessing parameters
    Return type     : MP4EncRet 
    Argument        : inst - encoder instance
    Argument        : pPreProcCfg - place where the parameters are returned
------------------------------------------------------------------------------*/
MP4EncRet MP4EncGetPreProcessing(MP4EncInst inst,
                                 MP4EncPreProcessingCfg * pPreProcCfg)
{
    const instance_s *pEncInst = (instance_s *) inst;
    const preProcess_s *pPP;

    APITRACE("MP4EncGetPreProcessing#");

    /* Check for illegal inputs */
    if((inst == NULL) || (pPreProcCfg == NULL))
    {
        APITRACE("MP4EncGetPreProcessing: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncGetPreProcessing: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

    pPP = &pEncInst->preProcess;

    pPreProcCfg->origHeight = pPP->lumHeightSrc;
    pPreProcCfg->origWidth = pPP->lumWidthSrc;
    pPreProcCfg->xOffset = pPP->horOffsetSrc;
    pPreProcCfg->yOffset = pPP->verOffsetSrc;

    pPreProcCfg->rotation = (MP4EncPictureRotation) pPP->rotation;
    pPreProcCfg->inputType = (MP4EncPictureType) pPP->inputFormat;

    pPreProcCfg->videoStabilization = pPP->videoStab;

    pPreProcCfg->colorConversion.type =
                        (MP4EncColorConversionType) pPP->colorConversionType;
    pPreProcCfg->colorConversion.coeffA = pPP->colorConversionCoeffA;
    pPreProcCfg->colorConversion.coeffB = pPP->colorConversionCoeffB;
    pPreProcCfg->colorConversion.coeffC = pPP->colorConversionCoeffC;
    pPreProcCfg->colorConversion.coeffE = pPP->colorConversionCoeffE;
    pPreProcCfg->colorConversion.coeffF = pPP->colorConversionCoeffF;

    APITRACE("MP4EncGetPreProcessing: OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : MP4EncSetStreamInfo
    Description     : Sets advanced stream parameters
    Return type     : MP4EncRet 
    Argument        : inst - encoder instance
    Argument        : pStreamInfo - structure containg parameters to be set
------------------------------------------------------------------------------*/
MP4EncRet MP4EncSetStreamInfo(MP4EncInst inst,
                              const MP4EncStreamInfo * pStreamInfo)
{
    instance_s *pEncInst = (instance_s *) inst;
    vol_s *pVol;

    APITRACE("MP4EncSetStreamInfo#");

    /* Check for illegal inputs */
    if((inst == NULL) || (pStreamInfo == NULL))
    {
        APITRACE("MP4EncSetStreamInfo: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncSetStreamInfo: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

    if(pEncInst->encStatus != ENCSTAT_INIT)
    {
        APITRACE("MP4EncSetStreamInfo: ERROR Stream already started!");
        return ENC_INVALID_STATUS;
    }

    if(pStreamInfo->videoRange > 1 || pStreamInfo->pixelAspectRatioWidth > 255
       || pStreamInfo->pixelAspectRatioHeight > 255)
    {
        APITRACE("MP4EncSetStreamInfo: Input param out of range");
        return ENC_INVALID_ARGUMENT;
    }

    pVol = &pEncInst->videoObjectLayer;

    pEncInst->visualObject.videoSignalType.videoRange = pStreamInfo->videoRange;
    if(pStreamInfo->videoRange == 0)
    {
        pEncInst->visualObject.videoSignalType.videoSignalType = ENCHW_NO;
    }
    else
    {
        pEncInst->visualObject.videoSignalType.videoSignalType = ENCHW_YES;
    }

    if(pStreamInfo->pixelAspectRatioHeight == 0 ||
       pStreamInfo->pixelAspectRatioWidth == 0)
    {
        pVol->aspectRatioInfo = 1;  /* square PAR */
        pVol->parWidth = 1;
        pVol->parHeight = 1;
    }
    else
    {
        pVol->parWidth = pStreamInfo->pixelAspectRatioWidth;
        pVol->parHeight = pStreamInfo->pixelAspectRatioHeight;

        if(pVol->parWidth == pVol->parHeight)
        {
            pVol->aspectRatioInfo = 1;  /* square PAR */
        }
        else
        {
            pVol->aspectRatioInfo = 15; /* extended PAR */
        }
    }

    APITRACE("MP4EncSetStreamInfo: OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
    Function name   : MP4EncGetStreamInfo
    Description     : Returns advanced stream parameters
    Return type     : MP4EncRet 
    Argument        : inst - encoder instance
    Argument        : pStreamInfo - place where the info is returned
------------------------------------------------------------------------------*/
MP4EncRet MP4EncGetStreamInfo(MP4EncInst inst, MP4EncStreamInfo * pStreamInfo)
{
    const instance_s *pEncInst = (instance_s *) inst;

    APITRACE("MP4EncGetStreamInfo#");

    /* Check for illegal inputs */
    if((inst == NULL) || (pStreamInfo == NULL))
    {
        APITRACE("MP4EncGetStreamInfo: ERROR Null argument");
        return ENC_NULL_ARGUMENT;
    }

    /* Check for existing instance */
    if(pEncInst->inst != pEncInst)
    {
        APITRACE("MP4EncGetStreamInfo: ERROR Invalid instance");
        return ENC_INSTANCE_ERROR;
    }

    pStreamInfo->videoRange = pEncInst->visualObject.videoSignalType.videoRange;
    pStreamInfo->pixelAspectRatioWidth = pEncInst->videoObjectLayer.parWidth;
    pStreamInfo->pixelAspectRatioHeight = pEncInst->videoObjectLayer.parHeight;

    APITRACE("MP4EncGetStreamInfo: OK");
    return ENC_OK;
}

/*------------------------------------------------------------------------------
  Function name : GetTimeCode
  Description   : Get the time code of the previously encoded VOP and store
                    it in output structure.    
------------------------------------------------------------------------------*/
void GetTimeCode(const timeCode_s * timeCode, MP4EncOut * pEncOut)
{

    pEncOut->timeCode.hours = timeCode->timeCodeHours;
    pEncOut->timeCode.minutes = timeCode->timeCodeMinutes;
    pEncOut->timeCode.seconds = timeCode->timeCodeSecond;
    pEncOut->timeCode.timeIncr = timeCode->vopTimeInc;
    pEncOut->timeCode.timeRes = timeCode->vopTimeIncRes;
}

/*------------------------------------------------------------------------------

  Function name : Mpeg4GetGobSizes
  Description   : Writes each VP/GOB unit size to the user supplied buffer.
                  Takes sizes from the buffer written by HW.
                  List ends with a zero value.
------------------------------------------------------------------------------*/
void MP4GetGobSizes(const instance_s * pEncInst, u32 * pGobSizeBuf)
{
    const u32 *pTmp;
    i32 gobs = 0;

    ASSERT(pGobSizeBuf);

    if (pEncInst->shortVideoHeader.header == ENCHW_YES) {
        gobs = pEncInst->shortVideoHeader.gobs;
        pTmp = (u32 *) pEncInst->asic.sizeTbl.gob.virtualAddress;
    }
    else {
        gobs = pEncInst->asic.sizeTblSize/sizeof(u32); 
        pTmp = (u32 *) pEncInst->asic.sizeTbl.vp.virtualAddress;
    }

    /* whole frame can be in one VP/GOB */
    if (pTmp) do
    {
        *pGobSizeBuf++ = (*pTmp++) / 8;     /* Bits to bytes */
    }
    while((--gobs > 0) && (*pTmp != 0));

    *pGobSizeBuf = 0;   /* END of table */
}

/*------------------------------------------------------------------------------
    Function name : MP4EncSetTestId
    Description   : Sets the encoder configuration according to a test vector
    Return type   : MP4EncRet
    Argument      : inst - encoder instance
    Argument      : testId - test vector ID
------------------------------------------------------------------------------*/
MP4EncRet MP4EncSetTestId(MP4EncInst inst, u32 testId)
{
    instance_s *pEncInst = (instance_s *) inst;
    (void) pEncInst;
    (void) testId;

    APITRACE("MP4EncSetTestId#");

#ifdef INTERNAL_TEST
    pEncInst->testId = testId;
    
    APITRACE("MP4EncSetTestId# OK");
    return ENC_OK;
#else
    /* Software compiled without testing support, return error always */
    APITRACE("MP4EncSetTestId# ERROR, testing disabled at compile time");
    return ENC_ERROR;
#endif
}

