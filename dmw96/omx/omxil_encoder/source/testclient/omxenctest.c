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
--  Description :  Encoder test client
-- 
------------------------------------------------------------------------------*/

/* OMX includes */
#include <OMX_Core.h>
#include <OMX_Types.h>

/* std includes */
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/* common */
#include <util.h>

/* test client */
#include "omxtestcommon.h"
#include "omxencparameters.h"

OMXENCODER_PARAMETERS parameters;

OMX_VIDEO_PARAM_MPEG4TYPE mpeg4_parameters;

OMX_VIDEO_PARAM_AVCTYPE avc_parameters;

OMX_VIDEO_PARAM_H263TYPE h263_parameters;

static int arg_count;

static char **arguments;

/* forward declarations */

OMX_U32 omxclient_next_vop(OMX_U32 inputRateNumer, OMX_U32 inputRateDenom,
                           OMX_U32 outputRateNumer, OMX_U32 outputRateDenom,
                           OMX_U32 frameCnt, OMX_U32 firstVop);

/*
    Macro that is used specifically to check parameter values
    in parameter checking loop.
 */
#define OMXENCODER_CHECK_NEXT_VALUE(i, args, argc, msg) \
    if(++(i) == argc || (*((args)[i])) == '-')  { \
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, msg); \
        return OMX_ErrorBadParameter; \
    }

/*
    omx_decoder_port_initialize
*/
OMX_ERRORTYPE omx_encoder_port_initialize(OMXCLIENT * appdata,
                                          OMX_PARAM_PORTDEFINITIONTYPE * p)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;

    switch (p->eDir)
    {
        /* CASE IN: initialize input port */
    case OMX_DirInput:
        OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                       "Using port at index %i as input port\n", p->nPortIndex);

        p->nBufferCountActual = parameters.buffer_count;
        error = process_encoder_input_parameters(arg_count, arguments, p);
        break;

        /* CASE OUT: initialize output port */
    case OMX_DirOutput:

        OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                       "Using port at index %i as output port\n",
                       p->nPortIndex);

        p->nBufferCountActual = parameters.buffer_count;
        error = process_encoder_output_parameters(arg_count, arguments, p);

        if(error != OMX_ErrorNone)
            break;

        parameters.output_compression = p->format.video.eCompressionFormat;
        switch (parameters.output_compression)
        {
        case OMX_VIDEO_CodingAVC:
            omxclient_struct_init(&avc_parameters, OMX_VIDEO_PARAM_AVCTYPE);
            avc_parameters.nPortIndex = 1;

            error =
                process_avc_parameters(arg_count, arguments, &avc_parameters);
            break;

        case OMX_VIDEO_CodingMPEG4:
            omxclient_struct_init(&mpeg4_parameters, OMX_VIDEO_PARAM_MPEG4TYPE);
            mpeg4_parameters.nPortIndex = 1;

            error =
                process_mpeg4_parameters(arg_count, arguments,
                                         &mpeg4_parameters);
            break;

        case OMX_VIDEO_CodingH263:
            omxclient_struct_init(&h263_parameters, OMX_VIDEO_PARAM_H263TYPE);
            h263_parameters.nPortIndex = 1;

            error =
                process_h263_parameters(arg_count, arguments, &h263_parameters);
            break;

        default:
            return OMX_ErrorBadParameter;
        }

        break;

    case OMX_DirMax:
    default:
        break;
    }

    return error;
}

OMX_ERRORTYPE omx_encoder_jpeg_port_initialize(OMXCLIENT * appdata,
                                               OMX_PARAM_PORTDEFINITIONTYPE * p)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;

    switch (p->eDir)
    {
        /* CASE IN: initialize input port */
    case OMX_DirInput:
        OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                       "Using port at index %i as input port\n", p->nPortIndex);

        p->nBufferCountActual = parameters.buffer_count;
        error = process_encoder_jpeg_input_parameters(arg_count, arguments, p);
        break;

        /* CASE OUT: initialize output port */
    case OMX_DirOutput:

        OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                       "Using port at index %i as output port\n",
                       p->nPortIndex);

        p->nBufferCountActual = parameters.buffer_count;
        error = process_encoder_jpeg_output_parameters(arg_count, arguments, p);
        break;

    case OMX_DirMax:
    default:
        break;
    }
    return error;
}

OMX_ERRORTYPE initialize_avc_output(OMXCLIENT * client, int argc, char **args)
{
    OMX_ERRORTYPE omxError;

    OMX_VIDEO_PARAM_BITRATETYPE bitrate;

    OMX_PARAM_DEBLOCKINGTYPE deblocking;

    OMX_VIDEO_PARAM_AVCTYPE parameters;

    OMX_VIDEO_PARAM_QUANTIZATIONTYPE quantization;

    /* set common AVC parameters */
    omxclient_struct_init(&parameters, OMX_VIDEO_PARAM_AVCTYPE);
    parameters.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (client->component, OMX_IndexParamVideoAvc,
                               &parameters), omxError);
    OMXCLIENT_RETURN_ON_ERROR(process_avc_parameters(argc, args, &parameters),
                              omxError);

    OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter
                              (client->component, OMX_IndexParamVideoAvc,
                               &parameters), omxError);

    /* set bitrate */
    omxclient_struct_init(&bitrate, OMX_VIDEO_PARAM_BITRATETYPE);
    bitrate.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (client->component, OMX_IndexParamVideoBitrate,
                               &bitrate), omxError);
    OMXCLIENT_RETURN_ON_ERROR(process_parameters_bitrate(argc, args, &bitrate),
                              omxError);

    OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter
                              (client->component, OMX_IndexParamVideoBitrate,
                               &bitrate), omxError);

    /* set deblocking */
    omxclient_struct_init(&deblocking, OMX_PARAM_DEBLOCKINGTYPE);
    deblocking.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (client->component,
                               OMX_IndexParamCommonDeblocking, &deblocking),
                              omxError);
    OMXCLIENT_RETURN_ON_ERROR(process_avc_parameters_deblocking
                              (argc, args, &deblocking), omxError);

    deblocking.nPortIndex = 1;
    OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter
                              (client->component,
                               OMX_IndexParamCommonDeblocking, &deblocking),
                              omxError);

    /* set quantization */
    omxclient_struct_init(&quantization, OMX_VIDEO_PARAM_QUANTIZATIONTYPE);
    quantization.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (client->component,
                               OMX_IndexParamVideoQuantization, &quantization),
                              omxError);

    OMXCLIENT_RETURN_ON_ERROR(process_parameters_quantization
                              (argc, args, &quantization), omxError);

    OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter
                              (client->component,
                               OMX_IndexParamVideoQuantization, &quantization),
                              omxError);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE initialize_mpeg4_output(OMXCLIENT * client, int argc, char **args)
{
    OMX_ERRORTYPE omxError;

    OMX_VIDEO_PARAM_MPEG4TYPE parameters;

    OMX_VIDEO_PARAM_QUANTIZATIONTYPE quantization;

    OMX_VIDEO_PARAM_BITRATETYPE bitrate;

    /* set bitrate */
    omxclient_struct_init(&parameters, OMX_VIDEO_PARAM_MPEG4TYPE);
    parameters.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (client->component, OMX_IndexParamVideoMpeg4,
                               &parameters), omxError);
    OMXCLIENT_RETURN_ON_ERROR(process_mpeg4_parameters(argc, args, &parameters),
                              omxError);

    OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter
                              (client->component, OMX_IndexParamVideoMpeg4,
                               &parameters), omxError);

    /* set quantization */
    omxclient_struct_init(&quantization, OMX_VIDEO_PARAM_QUANTIZATIONTYPE);
    quantization.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (client->component,
                               OMX_IndexParamVideoQuantization, &quantization),
                              omxError);

    OMXCLIENT_RETURN_ON_ERROR(process_parameters_quantization
                              (argc, args, &quantization), omxError);

    OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter
                              (client->component,
                               OMX_IndexParamVideoQuantization, &quantization),
                              omxError);

    /* set bitrate */
    omxclient_struct_init(&bitrate, OMX_VIDEO_PARAM_BITRATETYPE);
    bitrate.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (client->component, OMX_IndexParamVideoBitrate,
                               &bitrate), omxError);
    OMXCLIENT_RETURN_ON_ERROR(process_parameters_bitrate(argc, args, &bitrate),
                              omxError);

    OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter
                              (client->component, OMX_IndexParamVideoBitrate,
                               &bitrate), omxError);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE initialize_h263_output(OMXCLIENT * client, int argc, char **args)
{
    OMX_ERRORTYPE omxError;

    OMX_VIDEO_PARAM_H263TYPE parameters;

    OMX_VIDEO_PARAM_BITRATETYPE bitrate;

    OMX_VIDEO_PARAM_QUANTIZATIONTYPE quantization;

    /* set parameters */
    omxclient_struct_init(&parameters, OMX_VIDEO_PARAM_H263TYPE);
    parameters.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (client->component, OMX_IndexParamVideoH263,
                               &parameters), omxError);
    OMXCLIENT_RETURN_ON_ERROR(process_h263_parameters(argc, args, &parameters),
                              omxError);

    OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter
                              (client->component, OMX_IndexParamVideoH263,
                               &parameters), omxError);

    /* set bitrate */
    omxclient_struct_init(&bitrate, OMX_VIDEO_PARAM_BITRATETYPE);
    bitrate.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (client->component, OMX_IndexParamVideoBitrate,
                               &bitrate), omxError);
    OMXCLIENT_RETURN_ON_ERROR(process_parameters_bitrate(argc, args, &bitrate),
                              omxError);

    OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter
                              (client->component, OMX_IndexParamVideoBitrate,
                               &bitrate), omxError);

    /* set quantization */
    omxclient_struct_init(&quantization, OMX_VIDEO_PARAM_QUANTIZATIONTYPE);
    quantization.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (client->component,
                               OMX_IndexParamVideoQuantization, &quantization),
                              omxError);

    OMXCLIENT_RETURN_ON_ERROR(process_parameters_quantization
                              (argc, args, &quantization), omxError);

    OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter
                              (client->component,
                               OMX_IndexParamVideoQuantization, &quantization),
                              omxError);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE initialize_jpeg_output(OMXCLIENT * client, int argc, char **args)
{
    OMX_ERRORTYPE omxError;

    OMX_IMAGE_PARAM_QFACTORTYPE qfactor;

    printf("%s\n", __FUNCTION__);

    /* set parameters */
    omxclient_struct_init(&qfactor, OMX_IMAGE_PARAM_QFACTORTYPE);
    qfactor.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (client->component, OMX_IndexParamQFactor,
                               &qfactor), omxError);
    OMXCLIENT_RETURN_ON_ERROR(process_parameters_image_qfactor
                              (argc, args, &qfactor), omxError);

    printf("QValue: %d\n", (int) qfactor.nQFactor);
    qfactor.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter
                              (client->component, OMX_IndexParamQFactor,
                               &qfactor), omxError);

    return OMX_ErrorNone;
}

#include <unistd.h>

/*
    main
 */
int main(int argc, char **args)
{
    OMXCLIENT client;

    OMX_ERRORTYPE omxError;

    OMX_U32 *variance_array;

    OMX_U32 variance_array_len;

    arg_count = argc;
    arguments = args;

    memset(&parameters, 0, sizeof(OMXENCODER_PARAMETERS));

    parameters.buffer_size = 0;
    parameters.buffer_count = 3;

    parameters.stabilization = OMX_FALSE;

    omxError = process_encoder_parameters(argc, args, &parameters);
    if(omxError != OMX_ErrorNone)
    {
        print_usage(args[0]);
        return omxError;
    }

    variance_array_len = 0;
    variance_array = 0;

    if(parameters.varfile &&
       (omxError =
        omxclient_read_variance_file(&variance_array, &variance_array_len,
                                     parameters.varfile)) != OMX_ErrorNone)
    {
        return omxError;
    }

    omxError = OMX_Init();
    if(omxError == OMX_ErrorNone)
    {
        if(parameters.jpeg_output)
        {
            omxError =
                omxclient_component_create(&client,
                                           "OMX.hantro.7280.image.encoder",
                                           "image_encoder.jpeg",
                                           parameters.buffer_count);
        }
        else
        {
            omxError =
                omxclient_component_create(&client,
                                           "OMX.hantro.7280.video.encoder",
                                           "video_encoder.mpeg4",
                                           parameters.buffer_count);
        }

        client.store_frames = parameters.framed_output;
        client.store_buffers = parameters.sliced_output;
        client.output_name = parameters.outfile;

        if(omxError == OMX_ErrorNone)
        {
            omxError = omxclient_check_component_version(client.component);
        }
        else
        {
            OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                           "Component creation failed: '%s'\n",
                           OMX_OSAL_TraceErrorStr(omxError));
        }

        if(omxError == OMX_ErrorNone)
        {
            if(parameters.jpeg_output == OMX_FALSE)
            {
                omxError =
                    omxclient_component_initialize(&client,
                                                   &omx_encoder_port_initialize);

                switch (parameters.output_compression)
                {

                case OMX_VIDEO_CodingAVC:
                    omxError = initialize_avc_output(&client, argc, args);
                    break;

                case OMX_VIDEO_CodingMPEG4:
                    omxError = initialize_mpeg4_output(&client, argc, args);
                    break;

                case OMX_VIDEO_CodingH263:
                    omxError = initialize_h263_output(&client, argc, args);
                    break;

                default:
                    {
                        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                                       "Format is unsupported\n");
                        return OMX_ErrorNone;
                    }
                }
            }
            else
            {
                omxError =
                    omxclient_component_initialize_jpeg(&client,
                                                        &omx_encoder_jpeg_port_initialize);
                if(omxError == OMX_ErrorNone)
                {
                    initialize_jpeg_output(&client, argc, args);
                }
            }

            /* set rotation */
            if(omxError == OMX_ErrorNone && parameters.rotation != 0)
            {

                OMX_CONFIG_ROTATIONTYPE rotation;

                omxclient_struct_init(&rotation, OMX_CONFIG_ROTATIONTYPE);

                rotation.nPortIndex = 0;
                rotation.nRotation = parameters.rotation;

                if((omxError =
                    OMX_SetConfig(client.component, OMX_IndexConfigCommonRotate,
                                  &rotation)) != OMX_ErrorNone)
                {

                    OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                                   "Rotation could not be set: %s\n",
                                   OMX_OSAL_TraceErrorStr(omxError));
                    return omxError;
                }
            }

            /* set stabilization */
            if(omxError == OMX_ErrorNone && !parameters.jpeg_output &&
               parameters.stabilization)
            {

                OMX_CONFIG_FRAMESTABTYPE stab;

                omxclient_struct_init(&stab, OMX_CONFIG_FRAMESTABTYPE);

                stab.nPortIndex = 0;
                stab.bStab = parameters.stabilization;

                if((omxError =
                    OMX_SetConfig(client.component,
                                  OMX_IndexConfigCommonFrameStabilisation,
                                  &stab)) != OMX_ErrorNone)
                {

                    OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                                   "Stabilization could not be enabled: %s\n",
                                   OMX_OSAL_TraceErrorStr(omxError));
                    return omxError;
                }
            }

            /* set cropping */
            if(omxError == OMX_ErrorNone && parameters.cropping)
            {

                OMX_CONFIG_RECTTYPE rect;

                omxclient_struct_init(&rect, OMX_CONFIG_RECTTYPE);

                rect.nPortIndex = 0;

                rect.nLeft = parameters.cleft;
                rect.nTop = parameters.ctop;
                rect.nHeight = parameters.cheight;
                rect.nWidth = parameters.cwidth;

                if((omxError =
                    OMX_SetConfig(client.component,
                                  OMX_IndexConfigCommonInputCrop,
                                  &rect)) != OMX_ErrorNone)
                {

                    OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                                   "Cropping could not be enabled: %s\n",
                                   OMX_OSAL_TraceErrorStr(omxError));

                    return omxError;
                }
            }

            if(omxError == OMX_ErrorNone)
            {
                omxError = omxlclient_initialize_buffers_fixed(&client,
                                                               parameters.
                                                               buffer_size,
                                                               parameters.
                                                               buffer_count);

                if(omxError != OMX_ErrorNone)
                {
                    /* raw free, because state was not changed to idle */
                    omxclient_component_free_buffers(&client);
                }

            }

            if(omxError == OMX_ErrorNone)
            {
                if(omxError == OMX_ErrorNone)
                {
                    if(parameters.jpeg_output)
                    {
                        /* execute conversion as sliced */
                        omxError =
                            omxclient_execute_yuv_sliced(&client,
                                                         parameters.infile,
                                                         parameters.outfile,
                                                         parameters.firstvop,
                                                         parameters.lastvop);
                    }
                    else
                    {
                        omxError =
                            omxclient_execute_yuv_range(&client,
                                                        parameters.infile,
                                                        variance_array,
                                                        variance_array_len,
                                                        parameters.outfile,
                                                        parameters.firstvop,
                                                        parameters.lastvop);
                    }
                }

                if(omxError != OMX_ErrorNone)
                {
                    OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                                   "Video processing failed: '%s'\n",
                                   OMX_OSAL_TraceErrorStr(omxError));
                }
            }
            else
            {
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                               "Component video initialization failed: '%s'\n",
                               OMX_OSAL_TraceErrorStr(omxError));
            }

            /* destroy the component since it was succesfully created */
            omxError = omxclient_component_destroy(&client);
            if(omxError != OMX_ErrorNone)
            {
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                               "Component destroy failed: '%s'\n",
                               OMX_OSAL_TraceErrorStr(omxError));
            }
        }

        OMX_Deinit();
    }
    else
    {
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                       "OMX initialization failed: '%s'\n",
                       OMX_OSAL_TraceErrorStr(omxError));

        /* FAIL: OMX initialization failed, reason */
    }

    return omxError;
}
