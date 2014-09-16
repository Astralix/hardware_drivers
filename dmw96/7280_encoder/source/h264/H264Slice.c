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
--  $RCSfile: H264Slice.c,v $
--  $Date: 2010/04/19 09:19:03 $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "H264Slice.h"

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

	H264SliceInit

------------------------------------------------------------------------------*/
void H264SliceInit(slice_s * slice)
{
    slice->byteStream = ENCHW_YES;
    slice->nalUnitType = IDR;
    slice->sliceType = ISLICE;
    slice->picParameterSetId = 0;
    slice->frameNum = 0;
    slice->frameNumBits = 16;
    slice->idrPicId = 0;
    slice->nalRefIdc = 1;
    slice->disableDeblocking = 0;
    slice->filterOffsetA = 0;
    slice->filterOffsetB = 0;
    slice->sliceSize = 0;
    slice->sliceQpDelta = 0;
}

/*------------------------------------------------------------------------------

        H264SliceHdr

------------------------------------------------------------------------------*/
void H264SliceHdr(stream_s *stream, slice_s *slice)
{
        /* Nal unit sytax */
        if(slice->byteStream == ENCHW_YES)
        {
            H264PutBits(stream, 0, 8);
            COMMENT("BYTE STREAM: leadin_zero_8bits");

            H264PutBits(stream, 0, 8);
            COMMENT("BYTE STREAM: Start_code_prefix");

            H264PutBits(stream, 0, 8);
            COMMENT("BYTE STREAM: Start_code_prefix");

            H264PutBits(stream, 1, 8);
            COMMENT("BYTE STREAM: Start_code_prefix");
        }

        H264PutBits(stream, 0, 1);
        COMMENT("forbidden_zero_bit");

        H264PutBits(stream, slice->nalRefIdc, 2);
        COMMENT("nal_ref_idc");

        H264PutBits(stream, (i32) slice->nalUnitType, 5);
        COMMENT("nal_unit_type");

        stream->zeroBytes = 0; /* we start new counter for zero bytes */

        H264ExpGolombUnsigned(stream, 0);
        COMMENT("first_mb_in_slice");

        H264ExpGolombUnsigned(stream, (i32)slice->sliceType/* + 5*/);
        COMMENT("slice_type");

        H264ExpGolombUnsigned(stream, (i32)slice->picParameterSetId);
        COMMENT("pic_parameter_set_id");

        H264PutBits(stream, slice->frameNum, slice->frameNumBits);
        COMMENT("frame_num");

        /* if( !frame_mbs_only_flag ) etc.. not implemented */

        if (slice->nalUnitType == IDR) {
                H264ExpGolombUnsigned(stream, slice->idrPicId);
                COMMENT("idr_pic_id");
        }

        /* if( pic_order_cnt_type = = 0 )  etc.. not implemented */

        /* if( pic_order_cnt_type = = 1 && etc... not implemented */

        /* if( redundant_pic_cnt_present_flag ) etc... not implemented */

        /* if( slice_type = = B ) etc.. not implemented */
        if (slice->sliceType == PSLICE) {
                H264PutBits(stream, 0, 1);
                COMMENT("num_ref_idx_active_override_flag");
        }

        /* ref_pic_list_reordering( ) not ready */
        if (slice->sliceType == PSLICE) {
                H264PutBits(stream, 0, 1);
                COMMENT("ref_pic_list_reordering_flag_l0");
        }

        /* if( ( weighted_pred_flag && ( not implemented */

        if (slice->nalRefIdc != 0) {
                if (slice->nalUnitType == IDR) {
                        H264PutBits(stream, 0, 1);
                        COMMENT("no_output_of_prior_pics_flag");
                        H264PutBits(stream, 0, 1);
                        COMMENT("long_term_reference_flag");
                } else {
                        H264PutBits(stream, 0, 1);
                        COMMENT("adaptive_ref_pic_marking_mode_flag");
                }
        }

        H264ExpGolombSigned(stream, slice->sliceQpDelta);
        COMMENT("slice_qp_delta");

        /* if( slice_type = = SP || slice_type == SI etc... not implemented */

        /* Filter control always present
         * if(slice->deblockingFilterControlPresent == ENCHW_YES) */
        {
                H264ExpGolombUnsigned(stream, slice->disableDeblocking);
                COMMENT("disable_deblocking_filter_idc");
                if (slice->disableDeblocking != 1) {
                        H264ExpGolombSigned(stream, slice->filterOffsetA>>1);
                        COMMENT("slice_alpha_c0_offset_div2");
                        H264ExpGolombSigned(stream, slice->filterOffsetB>>1);
                        COMMENT("slice_beta_offset_div2");
                }
        }

        /* if( num_slice_groups_minus1 > 0 && etc.. not implemented */
}
