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
--  Abstract  : 
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: EncJpegInit.c,v $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents

    1. Include headers
    2. External compiler flags
    3. Module defines
    4. Local function prototypes
    5. Functions

------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "EncJpegInit.h"

#include "EncJpegCommon.h"
#include "ewl.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    JpegInit

------------------------------------------------------------------------------*/
JpegEncRet JpegInit(JpegEncCfg * pEncCfg, jpegInstance_s ** instAddr)
{
    jpegInstance_s *inst = NULL;
    const void *ewl = NULL;

    JpegEncRet ret = JPEGENC_OK;
    EWLInitParam_t param;

    ASSERT(pEncCfg);
    ASSERT(instAddr);

    *instAddr = NULL;

    param.clientType = EWL_CLIENT_TYPE_JPEG_ENC;

    /* Init EWL */
    if((ewl = EWLInit(&param)) == NULL)
        return JPEGENC_EWL_ERROR;

    /* Encoder instance */
    inst = (jpegInstance_s *) EWLcalloc(1, sizeof(jpegInstance_s));

    if(inst == NULL)
    {
        ret = JPEGENC_MEMORY_ERROR;
        goto err;
    }

    /* Default values */
    EncJpegInit(&inst->jpeg);

    /* Set parameters depending on user config */
    /* Choose quantization tables */
    inst->jpeg.qTable.pQlumi = QuantLuminance[pEncCfg->qLevel];
    inst->jpeg.qTable.pQchromi = QuantChrominance[pEncCfg->qLevel];

    /* Comment header data */
    if(pEncCfg->comLength > 0 && pEncCfg->pCom != NULL)
    {
        inst->jpeg.com.comLen = pEncCfg->comLength;
        inst->jpeg.com.pComment = pEncCfg->pCom;
        inst->jpeg.com.comEnable = 1;
    }

    /* Units type */
    if(pEncCfg->unitsType == JPEGENC_NO_UNITS)
    {
        inst->jpeg.appn.units = ENC_NO_UNITS;
        inst->jpeg.appn.Xdensity = 1;
        inst->jpeg.appn.Ydensity = 1;
    }
    else
    {
        inst->jpeg.appn.units = pEncCfg->unitsType;
        inst->jpeg.appn.Xdensity = pEncCfg->xDensity;
        inst->jpeg.appn.Ydensity = pEncCfg->yDensity;
    }

    /* Marker type */
    if(pEncCfg->markerType == JPEGENC_SINGLE_MARKER)
    {
        inst->jpeg.markerType = ENC_SINGLE_MARKER;
    }
    else
    {
        inst->jpeg.markerType = pEncCfg->markerType;
    }

    /* Rotation type */
    if(pEncCfg->rotation == JPEGENC_ROTATE_90R)
    {
        inst->jpeg.rotation = ROTATE_90R;
    }
    else if(pEncCfg->rotation == JPEGENC_ROTATE_90L)
    {
        inst->jpeg.rotation = ROTATE_90L;
    }
    else
    {
        inst->jpeg.rotation = ROTATE_0;
    }

    /* Thumbnail mode */
    if(pEncCfg->thumbnail)
        inst->jpeg.appn.thumbEnable = 1;
    else
        inst->jpeg.appn.thumbEnable = 0;

    /* Copy quantization tables to ASIC internal memories */
    EncAsicSetQuantTable(&inst->asic, inst->jpeg.qTable.pQlumi,
                         inst->jpeg.qTable.pQchromi);

    /* Initialize ASIC */
    inst->asic.ewl = ewl;
    (void) EncAsicControllerInit(&inst->asic);

    *instAddr = inst;

    return ret;

  err:
    if(inst != NULL)
        EWLfree(inst);
    if(ewl != NULL)
        (void) EWLRelease(ewl);

    return ret;
}

/*------------------------------------------------------------------------------

    JpegShutdown

    Function frees the encoder instance.

    Input   instance_s *    Pointer to the encoder instance to be freed.
                            After this the pointer is no longer valid.

------------------------------------------------------------------------------*/
void JpegShutdown(jpegInstance_s * data)
{
    const void *ewl;

    ASSERT(data);

    ewl = data->asic.ewl;

    EncAsicMemFree_V2(&data->asic);

    EWLfree(data);

    (void) EWLRelease(ewl);
}
