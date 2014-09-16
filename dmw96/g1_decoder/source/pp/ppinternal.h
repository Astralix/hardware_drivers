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
--  Description : PP API's internal functions.
--
------------------------------------------------------------------------------*/

#ifndef _PPAPI_INTERNAL_H_
#define _PPAPI_INTERNAL_H_

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "ppapi.h"
#include "basetype.h"
#include "regdrv.h"

/*------------------------------------------------------------------------------
    2. Internal Definitions
------------------------------------------------------------------------------*/

#define PP_X170_REGISTERS       (41)
#define PP_X170_REG_START       (240)

/* lint warning disabled: negative subscript of pointer used intentionally,
 * regBase points to start of pp regs, setReg uses start of decoder regs */

#define SetPpRegister(regBase,id,value)   /*lint -e(428)   */ \
    SetDecRegister(regBase-(PP_X170_REG_START>>2),(u32)id,value)
#define GetPpRegister(regBase,id)   /*lint -e(428)   */  \
    GetDecRegister(regBase-(PP_X170_REG_START>>2),(u32)id)

#define PP_OUT_MAX_WIDTH_D1     720
#define PP_OUT_MIN_WIDTH    16
#define PP_OUT_MAX_HEIGHT_D1    720
#define PP_OUT_MIN_HEIGHT   16

#define PP_OUT_D1   0
#define PP_OUT_XGA  1

#define PP_OUT_MAX_WIDTH_UPSCALED(d,vc1_p) ((vc1_p) == 0 ? 3*(d) : d)
#define PP_OUT_MAX_HEIGHT_UPSCALED(d,vc1_p ) ((vc1_p) == 0 ?  3*(d) - 2 : d)

/* Minimum downscaling factor, i.e. smaller than this not 
 * supported by HW */
#define PP_OUT_MIN_DOWNSCALING_FACTOR (70)

/* Max width of scanline in alphablending (if supported by HW) */
#define PP_IN_MAX_BLEND_SCANLINE    8191

/* Max out width height in JPEG in 292*16 */
#define PP_IN_MAX_WIDTH(pipeline)   ((pipeline) == 0 ? 2048U : 292U*16U)
#define PP_IN_MIN_WIDTH(pipeline)   ((pipeline) == 0 ? 16U : 48U)
#define PP_IN_MAX_HEIGHT(pipeline)  ((pipeline) == 0 ? 2048U : 292U*16U)
#define PP_IN_MIN_HEIGHT(pipeline)  ((pipeline) == 0 ? 16U : 16U)

/* Max out width height in JPEG in 511*16 */
#define PP_IN_MAX_WIDTH_EXT(pipeline,m)   ((pipeline) == 0 ? 4096U : (m)*16U)
#define PP_IN_MAX_HEIGHT_EXT(pipeline,m)  ((pipeline) == 0 ? 4096U : (m)*16U)

#define PP_IN_DIVISIBILITY(linked)       (linked == 0 ? 0xF : 0x7)

#define PP_MAX_FRM_BUFF_WIDTH       4096

#define PP_PIXEL_FORMAT_YUV420_MASK         0x020000U
#define PP_PIXEL_FORMAT_YUV422_MASK         0x010000U
#define PP_PIXEL_FORMAT_RGB_MASK            0x040000U
#define PP_PIXEL_FORMAT_RGB32_MASK          0x001000U

#define PP_ASIC_OUT_FORMAT_RGB          0
#define PP_ASIC_OUT_FORMAT_422          3
#define PP_ASIC_OUT_FORMAT_420          5

#define PP_ASIC_IN_FORMAT_422               0
#define PP_ASIC_IN_FORMAT_420_SEMIPLANAR    1
#define PP_ASIC_IN_FORMAT_420_PLANAR        2
#define PP_ASIC_IN_FORMAT_420_TILED         5
#define PP_ASIC_IN_FORMAT_440_SEMIPLANAR    6

/* extended formats */
#define PP_ASIC_IN_FORMAT_EXTENSION         7
#define PP_ASIC_IN_FORMAT_444_SEMIPLANAR    7
#define PP_ASIC_IN_FORMAT_411_SEMIPLANAR    8

/* inputs supported just in JPEG pipeline */
#define PP_ASIC_IN_FORMAT_400               3
#define PP_ASIC_IN_FORMAT_422_SEMIPLANAR    4

/* Width multiple of: */
#define WIDTH_MULTIPLE 8
#define HEIGHT_MULTIPLE 2

#define TOFIX(d, q) ((u32)( (d) * (u32)(1<<(q)) ))
#define FDIVI(a, b) ((a)/(b))

#define PP_STATUS_IDLE      0
#define PP_STATUS_RUNNING   1

/* it is allowed to allocate full register amount for the shadow regs:
 * but they are not used. so here the constant boolean is OK */
/*lint -e(506) */

 /* PPBufferData multibuffer: buffer config data
  * ID and input source pic addr. With this info we can later rerun pictures */
typedef struct BufferData_
{
    u32 inputBusLuma;
    u32 inputBusChroma;
    u32 setupID;
} PPBufferData;

typedef struct PPContainer_
{
    u32 ppRegs[PP_X170_REGISTERS];

    PPConfig ppCfg;
    PPConfig prevCfg;
    PPOutputBuffers combinedModeBuffers;
    PPBufferData bufferData[PP_MAX_MULTIBUFFER];
    u32 displayIndex;
    u32 currentSetupID; /* The setup ID given to the previous decoded pic */
    u32 prevOutSetupID; /* The setup ID of previous output picture */

    PPResult PPCombinedRet; /* pp stores here combined mode errors */

    u32 status;
    u32 pipeline;
    u32 multiBuffer;
    const void *dwl;
    const void *decInst;
    u32 decType;

    i32 frmBufferLumaOrRgbOffset;
    i32 frmBufferChromaOffset;
    u32 outFormat;
    u32 outStartCh;
    u32 outCrFirst;
    u32 outTiled4x4;
    u32 inFormat;
    u32 inStartCh;
    u32 inCrFirst;
    u32 rgbDepth;
    u32 inWidth;
    u32 inHeight;
    u32 altRegs;
    u32 maxOutWidth;
    u32 maxOutHeight;
    u32 blendEna;
    u32 deintEna;
    u32 ditherEna;
    u32 scalingEna;
    u32 tiledEna;
    u32 pixAccSupport;
    u32 blendCropSupport;
    u32 fastScalingSupport;
    u32 fastVerticalDownscale;
    u32 fastHorizontalDownscale;

    /* for TB control over scaling shortcut features */
    u32 fastVerticalDownscaleDisable;
    u32 fastHorizontalDownscaleDisable;

    u32 cVnorm;
    u32 cVfast;

    u32 cHnorm;
    u32 cHfast;
    u32 cHfast4x;
    u32 cHworkaroundFlag;
    u32 fastScaleMode;

    u32 hwId;
    u32 hwEndianVer;
    u32 tiledModeSupport;
    u32 jpeg16kSupport;
} PPContainer;

/*------------------------------------------------------------------------------
    3. Prototypes of Decoder API internal functions
------------------------------------------------------------------------------*/

void PPInitDataStructures(PPContainer * ppC);
void PPInitHW(PPContainer * ppC);
void PPSetupHW(PPContainer * ppC);
i32 PPCheckConfig(PPContainer * ppC, PPConfig * ppCfg,
                  u32 decLinked, u32 decType);
void PPSetupScaling(PPContainer * ppC, const PPOutImage * ppOutImg);

PPResult PPRun(PPContainer * ppC);
void PPRefreshRegs(PPContainer * ppC);
void PPFlushRegs(PPContainer * ppC);

u32 PPGetStatus(const PPContainer * ppC);
void PPSetStatus(PPContainer * ppC, u32 status);

i32 PPSelectOutputSize(PPContainer * ppC);
PPResult WaitForPp(PPContainer * ppC);
u32 PPCheckSetupChanges(PPConfig * prevCfg, PPConfig * newCfg);
void PPSetConfigIdData(PPContainer * ppC);

#endif
