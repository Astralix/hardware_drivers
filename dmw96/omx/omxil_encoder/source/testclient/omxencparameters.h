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
--  Description : OMX Encoder tester parameter types.
-- 
------------------------------------------------------------------------------*/

#ifndef OMXENCPARAMETERS_H_
#define OMXENCPARAMETERS_H_

/*
    struct for transferring parameter definitions
    between functions
 */
typedef struct OMXENCODER_PARAMETERS
{
    OMX_STRING infile;
    OMX_STRING outfile;
    OMX_STRING varfile;

    OMX_U32 firstvop;
    OMX_U32 lastvop;

    OMX_U32 buffer_count;
    OMX_U32 buffer_size;

    OMX_BOOL jpeg_output;
    OMX_BOOL sliced;

    OMX_VIDEO_CODINGTYPE output_compression;

    int rotation;
    OMX_BOOL stabilization;

    OMX_BOOL framed_output;
    OMX_BOOL sliced_output;

    OMX_BOOL cropping;
    OMX_U32 ctop, cleft, cwidth, cheight;

} OMXENCODER_PARAMETERS;

#ifdef __CPLUSPLUS
extern "C"
{
#endif                       /* __CPLUSPLUS */

/* common */

    void print_usage(OMX_STRING swname);

    OMX_ERRORTYPE process_encoder_parameters(int argc, char **args,
                                             OMXENCODER_PARAMETERS * params);

    OMX_ERRORTYPE process_encoder_input_parameters(int argc, char **args,
                                                   OMX_PARAM_PORTDEFINITIONTYPE
                                                   * params);

    OMX_ERRORTYPE process_encoder_output_parameters(int argc, char **args,
                                                    OMX_PARAM_PORTDEFINITIONTYPE
                                                    * params);

    OMX_ERRORTYPE process_parameters_quantization(int argc, char **args,
                                                  OMX_VIDEO_PARAM_QUANTIZATIONTYPE
                                                  * quantization);

    OMX_ERRORTYPE process_parameters_bitrate(int argc, char **args,
                                             OMX_VIDEO_PARAM_BITRATETYPE *
                                             bitrate);

/* MPEG4 */

    void print_mpeg4_usage();

    OMX_ERRORTYPE process_mpeg4_parameters(int argc, char **args,
                                           OMX_VIDEO_PARAM_MPEG4TYPE *
                                           mpeg4parameters);

/* AVC */

    void print_avc_usage();

    OMX_ERRORTYPE process_avc_parameters_deblocking(int argc, char **args,
                                                    OMX_PARAM_DEBLOCKINGTYPE *
                                                    deblocking);

    OMX_ERRORTYPE process_avc_parameters(int argc, char **args,
                                         OMX_VIDEO_PARAM_AVCTYPE * parameters);

/* H263 */

    void print_h263_usage();

    OMX_ERRORTYPE process_h263_parameters(int argc, char **args,
                                          OMX_VIDEO_PARAM_H263TYPE *
                                          parameters);

/* JPEG */

    void print_jpeg_usage();

    OMX_ERRORTYPE process_encoder_jpeg_input_parameters(int argc, char **args,
                                                        OMX_PARAM_PORTDEFINITIONTYPE
                                                        * params);

    OMX_ERRORTYPE process_encoder_jpeg_output_parameters(int argc, char **args,
                                                         OMX_PARAM_PORTDEFINITIONTYPE
                                                         * params);

    OMX_ERRORTYPE process_parameters_image_qfactor(int argc, char **args,
                                                   OMX_IMAGE_PARAM_QFACTORTYPE *
                                                   quantization);

#ifdef __CPLUSPLUS
}
#endif                       /* __CPLUSPLUS */

#endif                       /*OMXENCPARAMETERS_H_ */
