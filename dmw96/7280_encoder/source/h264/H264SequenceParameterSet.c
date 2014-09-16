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
--  Abstract : H264 Sequence Parameter Set
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: H264SequenceParameterSet.c,v $
--  $Date: 2008/06/12 10:57:53 $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "H264SequenceParameterSet.h"
#include "H264NalUnit.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define MAX_LEVEL_INDEX           (sizeof(H264LevelIdc)/sizeof(*H264LevelIdc))

/* max MV length is 64 in quarter pixel resolution */
#define LOG2_MAX_MV_LENGTH        6
#define EXTENDED_SAR            255


const u32 H264LevelIdc[15] =
    { 10, 11, 12, 13, 20, 21, 22, 30, 31, 32, 40, 41, 42, 50, 51 };

const u32 H264MaxCPBS[15] =
    { 210000, 600000, 1200000, 2400000, 2400000, 4800000, 4800000, 12000000,
    16800000, 24000000, 30000000, 75000000, 75000000, 162000000, 288000000
};

const u32 H264MaxFS[15] = { 99, 396, 396, 396, 396, 792, 1620, 1620,
    3600, 5120, 8192, 8192, 8192, 22080, 36864
};

/* sqrt(8*maxFrameSize) is maximum width and height of specific level */
const u32 H264SqrtMaxFS8[15] =
    { 28, 56, 56, 56, 56, 79, 113, 113, 169, 202, 256, 256, 256, 420, 543 };

const u32 H264MaxMBPS[15] =
    { 1485, 3000, 6000, 11880, 11880, 19800, 20250, 40500,
    108000, 216000, 245760, 245760, 491520, 589824, 983040
};

const u32 H264MaxBR[15] =
    { 76800, 230400, 460800, 921600, 2400000, 4800000, 4800000, 12000000,
    16800000, 24000000, 24000000, 60000000, 60000000, 162000000, 288000000
};

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

static void WriteVui(stream_s * strm, vui_t * vui, i32 numRefFrames);
static i32 GetAspectRatioIdc(i32 sarWidth, i32 sarHeight);
static void UpdateVuiPresence(sps_s * sps);

/*------------------------------------------------------------------------------

	H264SpsInit

------------------------------------------------------------------------------*/
void H264SeqParameterSetInit(sps_s * sps)
{
    sps->byteStream = ENCHW_YES;
    sps->profileIdc = 66;
    sps->constraintSet0 = ENCHW_YES;
    sps->constraintSet1 = ENCHW_YES;
    sps->constraintSet2 = ENCHW_YES;
    sps->constraintSet3 = ENCHW_NO;

    sps->levelIdc = 30;
    sps->seqParameterSetId = 0;
    sps->log2MaxFrameNumMinus4 = (16 - 4);
    sps->picOrderCntType = 2;

    sps->numRefFrames = 1;
    sps->gapsInFrameNumValueAllowed = ENCHW_NO;
    sps->picWidthInMbsMinus1 = 176 / 16 - 1;
    sps->picHeightInMapUnitsMinus1 = 144 / 16 - 1;
    sps->frameMbsOnly = ENCHW_YES;
    sps->direct8x8Inference = ENCHW_YES;
    sps->frameCropping = ENCHW_NO;
    sps->vuiParametersPresent = ENCHW_YES;
    sps->vui.bitStreamRestrictionFlag = 1;

    sps->vui.videoFullRange = 0;
    sps->vui.sarWidth = 0;
    sps->vui.sarHeight = 0;

    sps->frameCropLeftOffset = 0;
    sps->frameCropRightOffset = 0;
    sps->frameCropTopOffset = 0;
    sps->frameCropBottomOffset = 0;

    return;
}

/*------------------------------------------------------------------------------

	H264SeqParameterSet

------------------------------------------------------------------------------*/
void H264SeqParameterSet(stream_s * stream, sps_s * sps)
{
    /* Nal unit syntax */
    H264NalUnitHdr(stream, 1, SPSET, sps->byteStream);

    H264NalBits(stream, sps->profileIdc, 8);
    COMMENT("profile_idc");

    H264NalBits(stream, (i32) sps->constraintSet0, 1);
    COMMENT("constraint_set0_flag");

    H264NalBits(stream, (i32) sps->constraintSet1, 1);
    COMMENT("constraint_set1_flag");

    H264NalBits(stream, (i32) sps->constraintSet2, 1);
    COMMENT("constraint_set2_flag");

    H264NalBits(stream, (i32) sps->constraintSet3, 1);
    COMMENT("constraint_set3_flag");

    H264NalBits(stream, 0, 4);
    COMMENT("reserved_zero_4bits");

    H264NalBits(stream, sps->levelIdc, 8);
    COMMENT("level_idc");

    H264ExpGolombUnsigned(stream, sps->seqParameterSetId);
    COMMENT("seq_parameter_set_id");

    H264ExpGolombUnsigned(stream, sps->log2MaxFrameNumMinus4);
    COMMENT("log2_max_frame_num_minus4");

    H264ExpGolombUnsigned(stream, sps->picOrderCntType);
    COMMENT("pic_order_cnt_type");

    H264ExpGolombUnsigned(stream, sps->numRefFrames);
    COMMENT("num_ref_frames");

    H264NalBits(stream, (i32) sps->gapsInFrameNumValueAllowed, 1);
    COMMENT("gaps_in_frame_num_value_allowed_flag");

    H264ExpGolombUnsigned(stream, sps->picWidthInMbsMinus1);
    COMMENT("pic_width_in_mbs_minus1");

    H264ExpGolombUnsigned(stream, sps->picHeightInMapUnitsMinus1);
    COMMENT("pic_height_in_map_units_minus1");

    H264NalBits(stream, (i32) sps->frameMbsOnly, 1);
    COMMENT("frame_mbs_only_flag");

    H264NalBits(stream, (i32) sps->direct8x8Inference, 1);
    COMMENT("direct_8x8_inference_flag");

    H264NalBits(stream, (i32) sps->frameCropping, 1);
    COMMENT("frame_cropping_flag");

    /* Frame cropping parameters */
    if (sps->frameCropping)
    {
        H264ExpGolombUnsigned(stream, sps->frameCropLeftOffset);
        COMMENT("frame_crop_left_offset");
        H264ExpGolombUnsigned(stream, sps->frameCropRightOffset);
        COMMENT("frame_crop_right_offset");
        H264ExpGolombUnsigned(stream, sps->frameCropTopOffset);
        COMMENT("frame_crop_top_offset");
        H264ExpGolombUnsigned(stream, sps->frameCropBottomOffset);
        COMMENT("frame_crop_bottom_offset");
    }

    UpdateVuiPresence(sps);

    H264NalBits(stream, (i32) sps->vuiParametersPresent, 1);
    COMMENT("vui_parameters_present_flag");

    if(sps->vuiParametersPresent == ENCHW_YES)
        WriteVui(stream, &sps->vui, sps->numRefFrames);

    H264NalUnitTrailinBits(stream, sps->byteStream);
}

void UpdateVuiPresence(sps_s * sps)
{

    if(sps->vui.nalHrdParametersPresentFlag == 0 &&
       sps->vui.timeScale == 0 &&
       sps->vui.pictStructPresentFlag == 0 &&
       sps->vui.sarWidth == 0 && sps->vui.videoFullRange == 0 &&
       sps->vui.bitStreamRestrictionFlag == 0)
    {
        sps->vuiParametersPresent = ENCHW_NO;
    }
}

/*------------------------------------------------------------------------------

    Function: WriteVui

        Functional description:
          Write VUI params into the stream

        Inputs:
          vui_t *vui            pointer to VUI params structure
          u32 numRefFrames      number of reference frames, used as
                                max_dec_frame_buffering

        Outputs:
          stream_s *            pointer to stream data

------------------------------------------------------------------------------*/
static void WriteVui(stream_s * strm, vui_t * vui, i32 numRefFrames)
{

/* Variables */

    i32 sarIdc;

/* Code */

    ASSERT(strm);
    ASSERT(vui);

    sarIdc = GetAspectRatioIdc(vui->sarWidth, vui->sarHeight);

    if(sarIdc == 0) /* unspecified sample aspect ratio -> not present */
    {
        H264NalBits(strm, 0, 1);
        COMMENT("aspect_ratio_info_present_flag");
    }
    else
    {
        H264NalBits(strm, 1, 1);
        COMMENT("aspect_ratio_info_present_flag");
        H264NalBits(strm, sarIdc, 8);
        COMMENT("aspect_ratio_idc");
        if(sarIdc == EXTENDED_SAR)
        {
            H264NalBits(strm, vui->sarWidth, 16);
            COMMENT("sar_width");
            H264NalBits(strm, vui->sarHeight, 16);
            COMMENT("sar_height");
        }
    }

    H264NalBits(strm, 0, 1);
    COMMENT("overscan_info_present_flag");

    if(vui->videoFullRange != 0)
    {
        H264NalBits(strm, 1, 1);
        COMMENT("video_signal_type_present_flag");
        H264NalBits(strm, 5, 3);
        COMMENT("unspecified video_format");
        H264NalBits(strm, 1, 1);
        COMMENT("video_full_range_flag");
        H264NalBits(strm, 0, 1);
        COMMENT("colour_description_present_flag");
    }
    else
    {
        H264NalBits(strm, 0, 1);
        COMMENT("video_signal_type_present_flag");
    }

    H264NalBits(strm, 0, 1);
    COMMENT("chroma_loc_info_present_flag");

    if(vui->timeScale != 0)
    {
        H264NalBits(strm, 1, 1);
        COMMENT("timing_info_present_flag");
        H264NalBits(strm, vui->numUnitsInTick >> 16, 16);
        COMMENT("num_units_in_tick msb");
        H264NalBits(strm, vui->numUnitsInTick & 0xFFFF, 16);
        COMMENT("num_units_in_tick lsb");
        H264NalBits(strm, vui->timeScale >> 16, 16);
        COMMENT("time_scale msb");
        H264NalBits(strm, vui->timeScale & 0xFFFF, 16);
        COMMENT("time_scale lsb");
        H264NalBits(strm, 0, 1);
        COMMENT("fixed_frame_rate_flag");
    }
    else
    {
        H264NalBits(strm, 0, 1);
        COMMENT("timing_info_present_flag");
    }

    H264NalBits(strm, (i32) vui->nalHrdParametersPresentFlag, 1);
    COMMENT("nal_hrd_parameters_present_flag");

    if(vui->nalHrdParametersPresentFlag == ENCHW_YES)
    {
        H264ExpGolombUnsigned(strm, 0);
        COMMENT("cpb_cnt_minus1");
        
        {
            u32 bit_rate_scale = 1;
            u32 cpb_size_scale = 1;
            u32 tmp, i = 0;

            tmp = vui->cpbSize;
            while (4095 < (tmp >> (4 + i++)));
            cpb_size_scale = i;

            i = 0;
            tmp = vui->bitRate;
            while (4095 < (tmp >> (6 + i++)));
            bit_rate_scale = i;
            
            H264NalBits(strm, bit_rate_scale, 4);
            COMMENT("bit_rate_scale");

            H264NalBits(strm, cpb_size_scale, 4);
            COMMENT("cpb_size_scale");

            tmp = vui->bitRate >> (6 + bit_rate_scale);
            H264ExpGolombUnsigned(strm, tmp - 1);
            vui->bitRate = tmp << (6 + bit_rate_scale);
            COMMENT("bit_rate_value_minus1");

            tmp = vui->cpbSize >> (4 + cpb_size_scale);
            H264ExpGolombUnsigned(strm, tmp - 1);
            vui->cpbSize = tmp << (4 + cpb_size_scale);
            COMMENT("cpb_size_value_minus1");
        }

        H264NalBits(strm, 0, 1);
        COMMENT("cbr_flag");

        H264NalBits(strm, vui->initialCpbRemovalDelayLength - 1, 5);
        COMMENT("initial_cpb_removal_delay_length_minus1");
        H264NalBits(strm, vui->cpbRemovalDelayLength - 1, 5);
        COMMENT("cpb_removal_delay_length_minus1");
        H264NalBits(strm, vui->dpbOutputDelayLength - 1, 5);
        COMMENT("dpb_output_delay_length_minus1");
        H264NalBits(strm, vui->timeOffsetLength, 5);
        COMMENT("time_offset_length");
    }

    H264NalBits(strm, 0, 1);
    COMMENT("vcl_hrd_parameters_present_flag");

    if(vui->nalHrdParametersPresentFlag == ENCHW_YES)
    {
        H264NalBits(strm, 0, 1);
        COMMENT("low_delay_hrd_flag");
    }

    H264NalBits(strm, (i32) vui->pictStructPresentFlag, 1);
    COMMENT("pic_struct_present_flag");

    H264NalBits(strm, (i32) vui->bitStreamRestrictionFlag, 1);
    COMMENT("bit_stream_restriction_flag");

    if(vui->bitStreamRestrictionFlag == ENCHW_YES)
    {
        H264NalBits(strm, 1, 1);
        COMMENT("motion_vectors_over_pic_boundaries");

        H264ExpGolombUnsigned(strm, 0);
        COMMENT("max_bytes_per_pic_denom");

        H264ExpGolombUnsigned(strm, 0);
        COMMENT("max_bits_per_mb_denom");

        H264ExpGolombUnsigned(strm, LOG2_MAX_MV_LENGTH);
        COMMENT("log2_mv_length_horizontal");

        H264ExpGolombUnsigned(strm, LOG2_MAX_MV_LENGTH);
        COMMENT("log2_mv_length_vertical");

        H264ExpGolombUnsigned(strm, 0);
        COMMENT("num_reorder_frames");

        H264ExpGolombUnsigned(strm, numRefFrames);
        COMMENT("max_dec_frame_buffering");
    }

}

/*------------------------------------------------------------------------------

    Function: GetAspectRatioIdc

        Functional description:

        Inputs:
          u32 sarWidth      sample aspect ratio width
          u32 sarHeight     sample aspect ratio height

        Outputs:

        Returns:
          u32   acpectRatioIdc

------------------------------------------------------------------------------*/
static i32 GetAspectRatioIdc(i32 sarWidth, i32 sarHeight)
{

    i32 aspectRatioIdc;

    if(sarWidth == 0 || sarHeight == 0) /* unspecified */
        aspectRatioIdc = 0;
    else if(sarWidth == sarHeight)  /* square, 1:1 */
        aspectRatioIdc = 1;
    else if(sarHeight == 11)
    {
        if(sarWidth == 12)  /* 12:11 */
            aspectRatioIdc = 2;
        else if(sarWidth == 10) /* 10:11 */
            aspectRatioIdc = 3;
        else if(sarWidth == 16) /* 16:11 */
            aspectRatioIdc = 4;
        else if(sarWidth == 24) /* 24:11 */
            aspectRatioIdc = 6;
        else if(sarWidth == 20) /* 20:11 */
            aspectRatioIdc = 7;
        else if(sarWidth == 32) /* 32:11 */
            aspectRatioIdc = 8;
        else if(sarWidth == 18) /* 18:11 */
            aspectRatioIdc = 10;
        else if(sarWidth == 15) /* 15:11 */
            aspectRatioIdc = 11;
        else    /* Extended_SAR */
            aspectRatioIdc = EXTENDED_SAR;
    }
    else if(sarHeight == 33)
    {
        if(sarWidth == 40)  /* 40:33 */
            aspectRatioIdc = 5;
        else if(sarWidth == 80) /* 80:33 */
            aspectRatioIdc = 9;
        else if(sarWidth == 64) /* 64:33 */
            aspectRatioIdc = 12;
        else    /* Extended_SAR */
            aspectRatioIdc = EXTENDED_SAR;
    }
    else if(sarWidth == 160 && sarHeight == 99) /* 160:99 */
        aspectRatioIdc = 13;
    else    /* Extended_SAR */
        aspectRatioIdc = EXTENDED_SAR;

    return (aspectRatioIdc);

}

/*------------------------------------------------------------------------------

    Function: H264CheckLevel

        Functional description:
          Check whether levelIdc can accommodate the stream based on bit and
          frame rates set by the application

        Inputs:
          seqParamSet_t *       pointer to SPS data structure
          u32 bitRate           bit rate in bits per second
          u32 frameRateNum      numerator of the frame rate
          u32 frameRateDenom    denominator of the frame rate

        Outputs:
          seqParamSet_t *       pointer to SPS data structure

        Returns:
          ENCHW_OK for success
          ENCHW_NOK for invalid params

------------------------------------------------------------------------------*/
bool_e H264CheckLevel(sps_s * sps, i32 bitRate, i32 frameRateNum,
                      i32 frameRateDenom)
{

/* Variables */

    i32 tmp, i;

/* Code */

    ASSERT(sps);

    if(bitRate <= 0 || frameRateNum <= 0 || frameRateDenom <= 0)
        return (ENCHW_NOK);

    i = sps->levelIdx;

    tmp = (sps->picWidthInMbsMinus1 + 1) * (sps->picHeightInMapUnitsMinus1 + 1);

    if((u32) tmp > H264MaxFS[i] ||
       (u32) sps->picWidthInMbsMinus1 >= H264SqrtMaxFS8[i] ||
       (u32) sps->picHeightInMapUnitsMinus1 >= H264SqrtMaxFS8[i])
        return (ENCHW_NOK);

    tmp = frameRateNum * tmp / frameRateDenom;

    if(H264MaxMBPS[sps->levelIdx] < (u32) tmp)
        return (ENCHW_NOK);

    return (ENCHW_OK);

}

/*------------------------------------------------------------------------------

    Function: H264GetLevelIndex

        Functional description:
            function determines index to level tables
            level argument. If invalid level -> return INVALID_LEVEL

        Inputs:
            u32 levelIdc

        Outputs:

        Returns:
            u32 index
            INVALID_LEVEL

------------------------------------------------------------------------------*/

u32 H264GetLevelIndex(u32 levelIdc)
{

    u32 i;

    i = 0;
    while(H264LevelIdc[i] != levelIdc)
    {
        i++;
        if(i >= MAX_LEVEL_INDEX)
            return INVALID_LEVEL;
    }

    return (i);

}

/*------------------------------------------------------------------------------

    Function: H264SpsSetVui

        Functional description:
          Set VUI parameters in the SPS structure

        Inputs:
          u32 timeScale
          u32 numUnitsInTick
          bool zeroReorderFrames

        Outputs:
          seqParamSet_t *       pointer to SPS structure

------------------------------------------------------------------------------*/
void H264SpsSetVuiTimigInfo(sps_s * sps, u32 timeScale, u32 numUnitsInTick)
{
    if(timeScale)
        sps->vuiParametersPresent = ENCHW_YES;

    sps->vui.timeScale = timeScale; /* used as timing_info_present_flag */
    sps->vui.numUnitsInTick = numUnitsInTick;
}

void H264SpsSetVuiVideoInfo(sps_s * sps, u32 videoFullRange)
{
    if(videoFullRange)
        sps->vuiParametersPresent = ENCHW_YES;

    sps->vui.videoFullRange = videoFullRange;   /* used as video_signal_type_present_flag */
}

void H264SpsSetVuiAspectRatio(sps_s * sps, u32 sampleAspectRatioWidth,
                              u32 sampleAspectRatioHeight)
{
    ASSERT(sampleAspectRatioWidth < (1 << 16));
    ASSERT(sampleAspectRatioHeight < (1 << 16));

    if(sampleAspectRatioWidth)
        sps->vuiParametersPresent = ENCHW_YES;

    sps->vui.sarWidth = sampleAspectRatioWidth; /* used as aspect_ratio_info_present_flag */
    sps->vui.sarHeight = sampleAspectRatioHeight;
}

/*------------------------------------------------------------------------------

    Function: H264SpsSetVuiHrd

        Functional description:
          Set VUI HRD parameters in the SPS structure

        Inputs:
          seqParamSet_t *       pointer to SPS structure

        Outputs:
          seqParamSet_t *       pointer to SPS structure

------------------------------------------------------------------------------*/

void H264SpsSetVuiHrd(sps_s * sps, u32 present)
{
    ASSERT(sps);

    sps->vui.nalHrdParametersPresentFlag = present;

    if(present)
        sps->vuiParametersPresent = ENCHW_YES;
    else
    {
        return;
    }

    ASSERT(sps->vui.timeScale && sps->vui.numUnitsInTick);  /* set these first */

    sps->vui.initialCpbRemovalDelayLength = 24;
    sps->vui.cpbRemovalDelayLength = 24;
    sps->vui.dpbOutputDelayLength = 24;

    {
        u32 n = 1;

        while(sps->vui.numUnitsInTick > (1U << n))
        {
            n++;
        }
        sps->vui.timeOffsetLength = n;

    }
}

/*------------------------------------------------------------------------------

    Function: H264SpsSetVuiHrdBitRate

        Functional description:
          Set VUI HRD bit rate in the SPS structure

        Inputs:
          seqParamSet_t *       pointer to SPS structure
          u32 bitRate

        Outputs:
          seqParamSet_t *       pointer to SPS structure

------------------------------------------------------------------------------*/

void H264SpsSetVuiHrdBitRate(sps_s * sps, u32 bitRate)
{
    ASSERT(sps);

    sps->vui.bitRate = bitRate;
}

void H264SpsSetVuiHrdCpbSize(sps_s * sps, u32 cpbSize)
{
    ASSERT(sps);

    sps->vui.cpbSize = cpbSize;
}

u32 H264SpsGetVuiHrdBitRate(sps_s * sps)
{
    ASSERT(sps);

    return sps->vui.bitRate;
}

u32 H264SpsGetVuiHrdCpbSize(sps_s * sps)
{
    ASSERT(sps);

    return sps->vui.cpbSize;
}

/*------------------------------------------------------------------------------
    Function name   : H264EndOfSequence
    Description     : 
    Return type     : void 
    Argument        : stream_s *stream
    Argument        : sps_s *sps
------------------------------------------------------------------------------*/
void H264EndOfSequence(stream_s * stream, sps_s * sps)
{
    H264NalUnitHdr(stream, 0, ENDOFSEQUENCE, sps->byteStream);
}

/*------------------------------------------------------------------------------
    Function name   : H264EndOfStream
    Description     : 
    Return type     : void 
    Argument        : stream_s *stream
    Argument        : sps_s *sps
------------------------------------------------------------------------------*/
void H264EndOfStream(stream_s * stream, sps_s * sps)
{
    H264NalUnitHdr(stream, 0, ENDOFSTREAM, sps->byteStream);
}

/*------------------------------------------------------------------------------
    Function name   : H264SpsSetVuiPictStructPresentFlag
    Description     : Signal presence of pic_struct in picture timing SEI
    Return type     : void 
    Argument        : sps_s * sps
    Argument        : u32 flag
------------------------------------------------------------------------------*/
void H264SpsSetVuiPictStructPresentFlag(sps_s * sps, u32 flag)
{
    sps->vui.pictStructPresentFlag = flag;
}
