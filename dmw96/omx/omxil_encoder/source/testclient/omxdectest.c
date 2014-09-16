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
--  Description :  Decoder test client
-- 
------------------------------------------------------------------------------*/

/* OMX includes */
#include <OMX_Component.h>
#include <OMX_Core.h>
#include <OMX_Types.h>

/* std includes */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* test client */
#include "omxtestcommon.h"

/*
    struct for transferring parameter definitions
    between functions
 */
typedef struct OMXDECODER_PARAMETERS
{
    OMX_STRING infile;
    OMX_STRING outfile;
    OMX_STRING varfile;

    OMX_U32 buffer_count;
    OMX_U32 buffer_size;

    OMX_BOOL jpeg_input;
    OMX_BOOL rcv_input;
    OMX_BOOL splitted_output;

    int rotation;
    OMX_MIRRORTYPE mirror;

} OMXDECODER_PARAMETERS;

/* parameters instance */
OMXDECODER_PARAMETERS parameters;

int arg_count;

char **arguments;

/*
    print usage
 */
void print_usage(OMX_STRING swname)
{
    OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                   "usage: %s [options] <input-file> <output-file>\n"
                   "\n"
                   "  Available options:\n"
                   "    -i, --input-file                 Input file\n"
                   "    -o, --output-file                Output file\n"
                   "    -I, --input-compression-format   Compression format; 'avc', 'mpeg4', 'h263', 'wmv' or 'jpeg'\n"
                   "    -O, --output-color-format        Color format for output; 'YUV420Planar', 'YUV420SemiPlanar',\n"
                   "                                       'YCbCr', '16bitARGB4444', '16bitARGB1555', '16bitRGB565',\n"
                   "                                       '16bitBGR565', '32bitBGRA8888', '32bitARGB8888'\n"
                   "    -e, --error-concealment          Use error concealment\n"
                   "    -x, --output-width               Width of output image\n"
                   "    -y, --output-height              Height of output image\n"
                   "    -s, --buffer-size                Size of allocated buffers (640*480*3/2) \n"
                   "    -c, --buffer-count               count of buffers allocated for each port (30)\n"
                   "    -v, --buffer-variance-file       Name of variance file\n"
                   "\n"
                   "    -r, --rotation                   Rotation value, angle in degrees\n"
                   "    -m, --mirror                     Mirroring, 1=horizontal, 2=vertical, 3=both\n"
                   "    -D, --write-buffers              Write each buffer additionally to a separate file\n"
                   "\n"
                   "  Return value:\n"
                   "    0  OK\n"
                   "    -  Failures indicated as OMX error codes\n"
                   "\n", swname);
}

/*
    Macro that is used specifically to check parameter values
    in parameter checking loop.
 */
#define OMXDECODER_CHECK_NEXT_VALUE(i, args, argc, msg) \
    if(++(i) == argc || (*((args)[i])) == '-')  { \
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, msg); \
        return OMX_ErrorBadParameter; \
    }

OMX_ERRORTYPE process_parameters(int argc, char **args,
                                 OMXDECODER_PARAMETERS * params)
{
    OMX_U32 i, fcount;
    OMX_STRING files[3] = { 0 };    // input, output, input-variance

    fcount = 0;

    params->infile = 0;
    params->outfile = 0;

    params->rotation = 0;
    params->mirror = OMX_MirrorMax;

    i = 1;
    while(i < argc)
    {

        if(strcmp(args[i], "-v") == 0 ||
           strcmp(args[i], "--input-variance-file") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input variance file missing.\n");
            files[2] = args[i];
        }
        else if(strcmp(args[i], "-i") == 0 ||
                strcmp(args[i], "--input-file") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input file is missing.\n");
            files[0] = args[i];
        }
        else if(strcmp(args[i], "-o") == 0 ||
                strcmp(args[i], "--output-file") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output file is missing.\n");
            files[1] = args[i];
        }
        if(strcmp(args[i], "-I") == 0 ||
           strcmp(args[i], "--input-compression-format") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input compression format missing.\n");
            if(strcmp(args[i], "jpeg") == 0)
            {
                parameters.jpeg_input = OMX_TRUE;
            }
        }
        else if(strcmp(args[i], "-s") == 0 ||
                strcmp(args[i], "--buffer-size") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for buffer size missing.\n");
            params->buffer_size = atoi(args[i]);
        }
        else if(strcmp(args[i], "-c") == 0 ||
                strcmp(args[i], "--buffer-count") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for buffer count is missing.\n");
            params->buffer_count = atoi(args[i]);
        }
        else if(strcmp(args[i], "-r") == 0 ||
                strcmp(args[i], "--rotation") == 0)
        {
            if(++i == argc)
            {
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                               "Parameter for rotation is missing.\n");
                return OMX_ErrorBadParameter;
            }
            params->rotation = atoi(args[i]);
        }
        else if(strcmp(args[i], "-m") == 0 || strcmp(args[i], "--mirror") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for mirroring is missing.\n");
            switch (atoi(args[i]))
            {
            case 1:
                params->mirror = OMX_MirrorHorizontal;
                break;
            case 2:
                params->mirror = OMX_MirrorVertical;
                break;
            case 3:
                params->mirror = OMX_MirrorBoth;
                break;
            default:
                params->mirror = OMX_MirrorNone;
                break;
            }
        }
        else if(strcmp(args[i], "-D") == 0 ||
                strcmp(args[i], "--write-buffers") == 0)
        {
            params->splitted_output = OMX_TRUE;
        }
        else
        {
            // do nothing
        }
        ++i;
    }

    if(files[0] == 0)
    {
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "No input file.\n");
        return OMX_ErrorBadParameter;
    }

    if(files[1] == 0)
    {
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "No output file.\n");
        return OMX_ErrorBadParameter;
    }

    params->infile = files[0];
    params->outfile = files[1];
    params->varfile = files[2];

    return OMX_ErrorNone;
}

OMX_ERRORTYPE process_output_parameters(int argc, char **args,
                                        OMX_PARAM_PORTDEFINITIONTYPE * params)
{
    OMX_U32 i;

    i = 1;
    while(i < argc)
    {
        /* output color format */
        if(strcmp(args[i], "-O") == 0 ||
           strcmp(args[i], "--output-color-format") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output color format missing.\n");

            if(strcasecmp(args[i], "YUV420Planar") == 0)
                params->format.video.eColorFormat =
                    OMX_COLOR_FormatYUV420PackedPlanar;
            else if(strcasecmp(args[i], "YUV420SemiPlanar") == 0)
                params->format.video.eColorFormat =
                    OMX_COLOR_FormatYUV420PackedSemiPlanar;
            else if(strcasecmp(args[i], "YCbCr") == 0)
                params->format.video.eColorFormat = OMX_COLOR_FormatYCbYCr;
            else if(strcasecmp(args[i], "16bitARGB4444") == 0)
                params->format.video.eColorFormat =
                    OMX_COLOR_Format16bitARGB4444;
            else if(strcasecmp(args[i], "16bitARGB1555") == 0)
                params->format.video.eColorFormat =
                    OMX_COLOR_Format16bitARGB1555;
            else if(strcasecmp(args[i], "16bitRGB565") == 0)
                params->format.video.eColorFormat = OMX_COLOR_Format16bitRGB565;
            else if(strcasecmp(args[i], "16bitBGR565") == 0)
                params->format.video.eColorFormat = OMX_COLOR_Format16bitBGR565;
            else if(strcasecmp(args[i], "32bitBGRA8888") == 0)
                params->format.video.eColorFormat =
                    OMX_COLOR_Format32bitBGRA8888;
            else if(strcasecmp(args[i], "32bitARGB8888") == 0)
                params->format.video.eColorFormat =
                    OMX_COLOR_Format32bitARGB8888;
            else
            {
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                               "Unknown output color format.\n");
                return OMX_ErrorBadParameter;
            }
        }
        else if(strcmp(args[i], "-x") == 0 ||
                strcmp(args[i], "--output-width") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output width is missing.\n");
            params->format.video.nFrameWidth = atoi(args[i]);
        }
        else if(strcmp(args[i], "-y") == 0 ||
                strcmp(args[i], "--output-height") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output height is missing.\n");
            params->format.video.nFrameHeight = atoi(args[i]);
        }
        else
        {
            /* do nothing */
        }
        ++i;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE process_jpeg_output_parameters(int argc, char **args,
                                             OMX_PARAM_PORTDEFINITIONTYPE *
                                             params)
{
    OMX_U32 i;

    i = 1;
    while(i < argc)
    {
        /* output color format */
        if(strcmp(args[i], "-O") == 0 ||
           strcmp(args[i], "--output-color-format") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output color format missing.\n");

            if(strcasecmp(args[i], "YUV420Planar") == 0)
                params->format.image.eColorFormat =
                    OMX_COLOR_FormatYUV420PackedPlanar;
            else if(strcasecmp(args[i], "YUV420SemiPlanar") == 0)
                params->format.image.eColorFormat =
                    OMX_COLOR_FormatYUV420PackedSemiPlanar;
            else if(strcasecmp(args[i], "YCbCr") == 0)
                params->format.image.eColorFormat = OMX_COLOR_FormatYCbYCr;
            else if(strcasecmp(args[i], "16bitARGB4444") == 0)
                params->format.image.eColorFormat =
                    OMX_COLOR_Format16bitARGB4444;
            else if(strcasecmp(args[i], "16bitARGB1555") == 0)
                params->format.image.eColorFormat =
                    OMX_COLOR_Format16bitARGB1555;
            else if(strcasecmp(args[i], "16bitRGB565") == 0)
                params->format.image.eColorFormat = OMX_COLOR_Format16bitRGB565;
            else if(strcasecmp(args[i], "16bitBGR565") == 0)
                params->format.image.eColorFormat = OMX_COLOR_Format16bitBGR565;
            else if(strcasecmp(args[i], "32bitBGRA8888") == 0)
                params->format.image.eColorFormat =
                    OMX_COLOR_Format32bitBGRA8888;
            else if(strcasecmp(args[i], "32bitARGB8888") == 0)
                params->format.image.eColorFormat =
                    OMX_COLOR_Format32bitARGB8888;
            else
            {
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                               "Unknown output color format.\n");
                return OMX_ErrorBadParameter;
            }
        }
        else if(strcmp(args[i], "-x") == 0 ||
                strcmp(args[i], "--output-width") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output width is missing.\n");
            params->format.image.nFrameWidth = atoi(args[i]);
        }
        else if(strcmp(args[i], "-y") == 0 ||
                strcmp(args[i], "--output-height") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output height is missing.\n");
            params->format.image.nFrameHeight = atoi(args[i]);
        }
        else
        {
            /* do nothing */
        }
        ++i;
    }

    return OMX_ErrorNone;
}

/*
*/
OMX_ERRORTYPE process_input_parameters(int argc, char **args,
                                       OMX_PARAM_PORTDEFINITIONTYPE * params)
{
    OMX_U32 i;               // input, output, input-variance

    i = 1;
    while(i < argc)
    {

        /* input compression format */
        if(strcmp(args[i], "-I") == 0 ||
           strcmp(args[i], "--input-compression-format") == 0)
        {
            OMXDECODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input compression format missing.\n");

            if(strcasecmp(args[i], "avc") == 0)
                params->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
            else if(strcasecmp(args[i], "mpeg4") == 0)
                params->format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
            else if(strcasecmp(args[i], "h263") == 0)
                params->format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
            else if(strcasecmp(args[i], "wmv") == 0)
            {
                params->format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
                parameters.rcv_input = OMX_TRUE;
            }
            else if(strcmp(args[i], "jpeg") == 0)
            {
                parameters.jpeg_input = OMX_TRUE;
                params->format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
            }
            else
            {
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                               "Unknown input compression format.\n");
                return OMX_ErrorBadParameter;
            }
        }
        else if(strcmp(args[i], "-e") == 0 ||
                strcmp(args[i], "--error-concealment") == 0)
        {
            params->format.video.bFlagErrorConcealment = OMX_TRUE;
        }
        else
        {
            /* do nothing */
        }
        ++i;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE process_jpeg_input_parameters(int argc, char **args,
                                            OMX_PARAM_PORTDEFINITIONTYPE *
                                            params)
{
    OMX_U32 i;               // input, output, input-variance

    params->format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;

    i = 1;
    while(i < argc)
    {

        if(strcmp(args[i], "-e") == 0 ||
           strcmp(args[i], "--error-concealment") == 0)
        {
            params->format.video.bFlagErrorConcealment = OMX_TRUE;
            break;
        }
        ++i;
    }

    return OMX_ErrorNone;
}

/*
    omx_decoder_port_initialize
 */
OMX_ERRORTYPE omx_decoder_port_initialize(OMXCLIENT * appdata,
                                          OMX_PARAM_PORTDEFINITIONTYPE * p)
{
    OMX_ERRORTYPE omxError;

    omxError = OMX_ErrorNone;
    switch (p->eDir)
    {
        /* CASE IN: initialize input port */
    case OMX_DirInput:

        OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                       "Using port at index %i as input port\n", p->nPortIndex);

        if(parameters.jpeg_input)
        {
            OMXCLIENT_RETURN_ON_ERROR(process_jpeg_input_parameters
                                      (arg_count, arguments, p), omxError);
        }
        else
        {
            OMXCLIENT_RETURN_ON_ERROR(process_input_parameters
                                      (arg_count, arguments, p), omxError);
        }

        p->nBufferCountActual = parameters.buffer_count;

        /*if(parameters.buffer_size) {
         * p->nBufferSize = parameters.buffer_size;
         * } */

        break;

        /* CASE OUT: initialize output port */
    case OMX_DirOutput:

        OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                       "Using port at index %i as output port\n",
                       p->nPortIndex);

        if(parameters.jpeg_input)
        {
            OMXCLIENT_RETURN_ON_ERROR(process_jpeg_output_parameters
                                      (arg_count, arguments, p), omxError);
        }
        else
        {
            OMXCLIENT_RETURN_ON_ERROR(process_output_parameters
                                      (arg_count, arguments, p), omxError);
        }

        p->nBufferCountActual = parameters.buffer_count;

        /*if(parameters.buffer_size) {
         * p->nBufferSize = parameters.buffer_size;
         * } */
        break;

    case OMX_DirMax:
    default:
        break;
    }

    return omxError;
}

/*
    main
 */
int main(int argc, char **args)
{
    OMXCLIENT client;

    OMX_ERRORTYPE omxError;

    OMX_U32 *variance_array;

    OMX_U32 variance_array_len;

    memset(&parameters, 0, sizeof(OMXDECODER_PARAMETERS));

    arg_count = argc;
    arguments = args;

    parameters.buffer_size = 0; //640 * 480 * 3 / 2;
    parameters.buffer_count = 3;

    omxError = process_parameters(argc, args, &parameters);
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

        if(parameters.jpeg_input)
        {
            omxError =
                omxclient_component_create(&client,
                                           "OMX.hantro.7190.image.decoder",
                                           "image_decoder.jpeg",
                                           parameters.buffer_count);
        }
        else
        {
            omxError =
                omxclient_component_create(&client,
                                           "OMX.hantro.7190.video.decoder",
                                           "video_decoder.h263",
                                           parameters.buffer_count);
        }

        client.store_buffers = parameters.splitted_output;
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
            if(parameters.jpeg_input)
                omxError =
                    omxclient_component_initialize_jpeg(&client,
                                                        &omx_decoder_port_initialize);
            else
                omxError =
                    omxclient_component_initialize(&client,
                                                   &omx_decoder_port_initialize);

            if(omxError == OMX_ErrorNone)
            {

                omxError =
                    omxlclient_initialize_buffers_fixed(&client,
                                                        parameters.buffer_size,
                                                        parameters.
                                                        buffer_count);

                /* re-allocation of buffers after this is not properly defined, can't rely on port
                 * configuration */

                /* omxError = omxlclient_initialize_buffers(&client); */
            }

            /* set rotation */
            if(parameters.rotation != 0)
            {

                OMX_CONFIG_ROTATIONTYPE rotation;

                omxclient_struct_init(&rotation, OMX_CONFIG_ROTATIONTYPE);

                rotation.nPortIndex = 1;
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

            /* set mirroring */
            if(parameters.mirror != OMX_MirrorMax)
            {

                OMX_CONFIG_MIRRORTYPE mirror;

                omxclient_struct_init(&mirror, OMX_CONFIG_MIRRORTYPE);

                mirror.nPortIndex = 1;
                mirror.eMirror = parameters.mirror;

                if((omxError =
                    OMX_SetConfig(client.component, OMX_IndexConfigCommonMirror,
                                  &mirror)) != OMX_ErrorNone)
                {

                    OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                                   "Mirroring could not be set: %s\n",
                                   OMX_OSAL_TraceErrorStr(omxError));
                    return omxError;
                }
            }

            if(omxError == OMX_ErrorNone)
            {
                /* disable post-processor port */
                omxError =
                    OMX_SendCommand(client.component, OMX_CommandPortDisable, 2,
                                    NULL);
                if(omxError != OMX_ErrorNone)
                {
                    return omxError;
                }

                /* execute conversion */

                if(parameters.rcv_input)
                    omxError =
                        omxclient_execute_rcv(&client, parameters.infile,
                                              parameters.outfile);
                else
                    omxError =
                        omxclient_execute(&client, parameters.infile,
                                          variance_array, variance_array_len,
                                          parameters.outfile);

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
