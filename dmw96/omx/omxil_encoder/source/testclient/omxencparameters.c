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
--  Description :  Encoder parameter functions
--
------------------------------------------------------------------------------*/

#include <OMX_Component.h>
#include <OMX_Video.h>
#include <OMX_Image.h>

#include <string.h>
#include <stdlib.h>

#include <util.h>

#include "omxencparameters.h"
#include "omxtestcommon.h"

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
    print_usage
 */
void print_usage(OMX_STRING swname)
{
    printf("usage: %s [options]\n"
           "\n"
           "  Available options:\n"
           "    -O, --outputFormat               Compression format; 'avc', 'mpeg4', 'h263' or 'jpeg'\n"
           "    -l, --inputFormat                Color format for output\n"
           "                                     0  yuv420packedplanar    1  yuv420packedsemiplanar\n"
           "                                     2  ycbycr                3  cbycry\n"
           "                                     4  rgb565                5  bgr565\n"
           "                                     6  rgb555                8  rgb444\n"
           "                                     10  yuv422planar\n\n"
           "    -o, --output                     File name of the output\n"
           "    -i, --input                      File name of the input\n"
           "    -e, --error-concealment          Use error concealment\n"
           "    -w, --lumWidthSrc                Width of source image\n"
           "    -h, --lumHeightSrc               Height of source image\n"
           "    -x, --height                     Height of output image\n"
           "    -y, --width                      Width of output image\n"
           "    -X, --horOffsetSrc               Output image horizontal offset. [0]\n"
           "    -Y, --verOffsetSrc               Output image vertical offset. [0]\n"
           "    -j, --inputRateNumer             Frame rate used for the input\n"
           "    -f, --outputRateNumer            Frame rate used for the output\n"
           "    -B, --bitsPerSecond              Bit-rate used for the output\n"
           "    -v, --input-variance-file        File containing variance numbers separated with newline\n"
           "    -a, --firstVop                   First vop of input file [0]\n"
           "    -b, --lastVop                    Last vop of input file  [EOF]\n"
           "    -s, --buffer-size                Size of allocated buffers (default provided by component) \n"
           "    -c, --buffer-count               count of buffers allocated for each port (30)\n"
           "\n"
           "    -r, --rotation                   Rotation value, angle in degrees\n"
           "    -Z, --videostab                  Use frame stabilization\n"
           "    -T, --write-frames               Write each frame additionally to a separate file\n"
           "    -D, --write-buffers              Write each buffer additionally to a separate\n"
           "                                     file (overrides -T)\n"
           "\n", swname);

    print_avc_usage();
    print_mpeg4_usage();
    print_h263_usage();
    print_jpeg_usage();

    printf("  Return value:\n"
           "    0 = OK; failures indicated as OMX error codes\n" "\n");
}

/*
    process_parameters
 */
OMX_ERRORTYPE process_encoder_parameters(int argc, char **args,
                                         OMXENCODER_PARAMETERS * params)
{
    OMX_U32 i, fcount;
    OMX_STRING files[3] = { 0 };    // input, output, input-variance

    fcount = 0;
    params->lastvop = 100;

    i = 1;
    while(i < argc)
    {

        if(strcmp(args[i], "-i") == 0 || strcmp(args[i], "--input") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input file missing.\n");
            files[0] = args[i];
        }
        /* input compression format */
        else if(strcmp(args[i], "-O") == 0 ||
                strcmp(args[i], "--output-compression-format") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output format is missing.\n");
            params->jpeg_output =
                (strcasecmp(args[i], "jpeg") == 0) ? OMX_TRUE : OMX_FALSE;
        }
        else if(strcmp(args[i], "-o") == 0 || strcmp(args[i], "--output") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output file missing.\n");
            files[1] = args[i];
        }
        else if(strcmp(args[i], "-v") == 0 ||
                strcmp(args[i], "--input-variance-file") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input variance file missing.\n");
            files[2] = args[i];
        }
        else if(strcmp(args[i], "-a") == 0 ||
                strcmp(args[i], "--firstVop") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for first vop is missing.\n");
            params->firstvop = atoi(args[i]);
        }
        else if(strcmp(args[i], "-b") == 0 || strcmp(args[i], "--lastVop") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for last vop is missing.\n");
            params->lastvop = atoi(args[i]);
        }
        else if(strcmp(args[i], "-s") == 0 ||
                strcmp(args[i], "--buffer-size") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for buffer allocation size is missing.\n");
            params->buffer_size = atoi(args[i]);
        }
        else if(strcmp(args[i], "-S") == 0 ||
                strcmp(args[i], "--slice-height") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for slice height is missing.\n");
            params->sliced = OMX_TRUE;
        }
        else if(strcmp(args[i], "-T") == 0 ||
                strcmp(args[i], "--write-frames") == 0)
        {
            params->framed_output = OMX_TRUE;
        }
        else if(strcmp(args[i], "-D") == 0 ||
                strcmp(args[i], "--write-buffers") == 0)
        {
            params->sliced_output = OMX_TRUE;
        }
        else if(strcmp(args[i], "-c") == 0 ||
                strcmp(args[i], "--buffer-count") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
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
        else if(strcmp(args[i], "-Z") == 0 ||
                strcmp(args[i], "--videostab") == 0)
        {
            params->stabilization = OMX_TRUE;
        }
        else if(strcmp(args[i], "-x") == 0 || strcmp(args[i], "--width") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output crop width missing.\n");
            params->cropping = OMX_TRUE;
            params->cwidth = atoi(args[i]);
        }
        else if(strcmp(args[i], "-y") == 0 || strcmp(args[i], "--height") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output crop height missing.\n");
            params->cropping = OMX_TRUE;
            params->cheight = atoi(args[i]);
        }
        else if(strcmp(args[i], "-X") == 0 ||
                strcmp(args[i], "--horOffsetSrc") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for horizontal crop offset is missing.\n");
            params->cropping = OMX_TRUE;
            params->cleft = atoi(args[i]);
        }
        else if(strcmp(args[i], "-Y") == 0 ||
                strcmp(args[i], "--verOffsetSrc") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for vertical crop offset is missing.\n");
            params->cropping = OMX_TRUE;
            params->ctop = atoi(args[i]);
        }
        else
        {
            /* do nothing, paramter may be needed by subsequent parameter readers */
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

/*
 */
OMX_ERRORTYPE process_encoder_input_parameters(int argc, char **args,
                                               OMX_PARAM_PORTDEFINITIONTYPE *
                                               params)
{
    OMX_U32 i;

    params->format.video.nSliceHeight = 0;
    params->format.video.nFrameWidth = 0;

    i = 1;
    while(i < argc)
    {

        /* input color format */
        if(strcmp(args[i], "-l") == 0 || strcmp(args[i], "--inputFormat") == 0)
        {
            if(++i == argc)
            {
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                               "Parameter for input color format missing.\n");
                return OMX_ErrorBadParameter;
            }

            switch (atoi(args[i]))
            {
            case 0:
                params->format.video.eColorFormat =
                    OMX_COLOR_FormatYUV420PackedPlanar;
                break;
            case 1:
                params->format.video.eColorFormat =
                    OMX_COLOR_FormatYUV420PackedSemiPlanar;
                break;
            case 2:
                params->format.video.eColorFormat = OMX_COLOR_FormatYCbYCr;
                break;
            case 3:
                params->format.video.eColorFormat = OMX_COLOR_FormatCbYCrY;
                break;
            case 4:
                params->format.video.eColorFormat = OMX_COLOR_Format16bitRGB565;
                break;
            case 5:
                params->format.video.eColorFormat = OMX_COLOR_Format16bitBGR565;
                break;
            case 6:
                params->format.video.eColorFormat =
                    OMX_COLOR_Format16bitARGB1555;
                break;
            case 8:
                params->format.video.eColorFormat =
                    OMX_COLOR_Format16bitARGB4444;
                break;
            case 10:
                params->format.video.eColorFormat =
                    OMX_COLOR_FormatYUV422Planar;
                break;
            default:
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "Unknown color format.\n");
                return OMX_ErrorBadParameter;
            }
        }
        else if(strcmp(args[i], "-e") == 0 ||
                strcmp(args[i], "--error-concealment") == 0)
        {
            params->format.video.bFlagErrorConcealment = OMX_TRUE;
        }
        else if(strcmp(args[i], "-h") == 0 ||
                strcmp(args[i], "--lumHeightSrc") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input height missing.\n");
            params->format.video.nFrameHeight = atoi(args[i]);
        }
        else if(strcmp(args[i], "-w") == 0 ||
                strcmp(args[i], "--lumWidthSrc") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input width missing.\n");
            params->format.video.nStride = atoi(args[i]);
        }
        else if(strcmp(args[i], "-j") == 0 ||
                strcmp(args[i], "--inputRateNumer") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input frame rate is missing.\n");
            params->format.video.xFramerate = FLOAT_Q16(strtod(args[i], 0));
        }
        else if(strcmp(args[i], "-s") == 0 ||
                strcmp(args[i], "--buffer-size") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for buffer allocation size is missing.\n");
            params->nBufferSize = atoi(args[i]);
        }
        else if(strcmp(args[i], "-c") == 0 ||
                strcmp(args[i], "--buffer-count") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for buffer count is missing.\n");
            params->nBufferCountActual = atoi(args[i]);
        }
        else
        {
            /* do nothing, paramter may be needed by subsequent parameter readers */
        }

        ++i;
    }

    return OMX_ErrorNone;
}

/*
 */
OMX_ERRORTYPE process_encoder_jpeg_input_parameters(int argc, char **args,
                                                    OMX_PARAM_PORTDEFINITIONTYPE
                                                    * params)
{
    OMX_U32 i;

    params->format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
    params->format.image.nSliceHeight = 0;
    params->format.image.nFrameWidth = 0;

    i = 1;
    while(i < argc)
    {

        /* input color format */
        if(strcmp(args[i], "-l") == 0 || strcmp(args[i], "--inputFormat") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input color format missing.\n");
            switch (atoi(args[i]))
            {
            case 0:
                params->format.image.eColorFormat =
                    OMX_COLOR_FormatYUV420PackedPlanar;
                break;
            case 1:
                params->format.image.eColorFormat =
                    OMX_COLOR_FormatYUV420PackedSemiPlanar;
                break;
            case 2:
                params->format.image.eColorFormat = OMX_COLOR_FormatYCbYCr;
                break;
            case 3:
                params->format.image.eColorFormat = OMX_COLOR_FormatCbYCrY;
                break;
            case 4:
                params->format.image.eColorFormat = OMX_COLOR_Format16bitRGB565;
                break;
            case 5:
                params->format.image.eColorFormat = OMX_COLOR_Format16bitBGR565;
                break;
            case 6:
                params->format.image.eColorFormat =
                    OMX_COLOR_Format16bitARGB1555;
                break;
            case 8:
                params->format.image.eColorFormat =
                    OMX_COLOR_Format16bitARGB4444;
                break;
            case 10:
                params->format.image.eColorFormat =
                    OMX_COLOR_FormatYUV422Planar;
                break;
            default:
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "Unknown color format.\n");
                return OMX_ErrorBadParameter;
            }
        }
        else if(strcmp(args[i], "-e") == 0 ||
                strcmp(args[i], "--error-concealment") == 0)
        {
            params->format.image.bFlagErrorConcealment = OMX_TRUE;
        }
        else if(strcmp(args[i], "-h") == 0 ||
                strcmp(args[i], "--lumHeightSrc") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input height missing.\n");
            params->format.image.nFrameHeight = atoi(args[i]);
        }
        else if(strcmp(args[i], "-w") == 0 ||
                strcmp(args[i], "--lumWidthSrc") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input width missing.\n");
            params->format.image.nStride = atoi(args[i]);
        }
        else if(strcmp(args[i], "-s") == 0 ||
                strcmp(args[i], "--buffer-size") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for buffer allocation size is missing.\n");
            params->nBufferSize = atoi(args[i]);
        }
        else if(strcmp(args[i], "-S") == 0 ||
                strcmp(args[i], "--slice-height") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for slice height is missing.\n");
            params->format.video.nSliceHeight = atoi(args[i]);
        }
        else if(strcmp(args[i], "-c") == 0 ||
                strcmp(args[i], "--buffer-count") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for buffer count is missing.\n");
            params->nBufferCountActual = atoi(args[i]);
        }
        else
        {
            /* do nothing, paramter may be needed by subsequent parameter readers */
        }

        ++i;
    }

    return OMX_ErrorNone;
}

/*
 */
OMX_ERRORTYPE process_encoder_output_parameters(int argc, char **args,
                                                OMX_PARAM_PORTDEFINITIONTYPE *
                                                params)
{
    OMX_U32 i;

    params->format.video.nSliceHeight = 0;
    params->format.video.nStride = 0;

    i = 1;
    while(i < argc)
    {

        /* input compression format */
        if(strcmp(args[i], "-O") == 0 ||
           strcmp(args[i], "--output-compression-format") == 0)
        {
            if(++i == argc)
            {
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                               "Parameter for output compression format missing.\n");
                return OMX_ErrorBadParameter;
            }

            if(strcasecmp(args[i], "avc") == 0)
            {
                params->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
            }
            else if(strcasecmp(args[i], "mpeg4") == 0)
            {
                params->format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
            }
            else if(strcasecmp(args[i], "h263") == 0)
                params->format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
            else if(strcasecmp(args[i], "wmv") == 0)
                params->format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
            else if(strcmp(args[i], "jpeg") == 0)
                params->format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
            else
            {
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                               "Unknown compression format.\n");
                return OMX_ErrorBadParameter;
            }
        }
        else if(strcmp(args[i], "-B") == 0 ||
                strcmp(args[i], "--bitsPerSecond") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for bit-rate missing.\n");
            params->format.video.nBitrate = atoi(args[i]);
        }
        else if(strcmp(args[i], "-h") == 0 ||
                strcmp(args[i], "--lumHeightSrc") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input height missing.\n");
            params->format.video.nFrameHeight = atoi(args[i]);
        }
        else if(strcmp(args[i], "-w") == 0 ||
                strcmp(args[i], "--lumWidthSrc") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input width missing.\n");
            params->format.video.nFrameWidth = atoi(args[i]);
        }
        else if(strcmp(args[i], "-f") == 0 ||
                strcmp(args[i], "--outputRateNumer") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for output frame rate is missing.\n");
            params->format.video.xFramerate = FLOAT_Q16(strtod(args[i], 0));
        }
        else if(strcmp(args[i], "-s") == 0 ||
                strcmp(args[i], "--buffer-size") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for buffer allocation size is missing.\n");
            params->nBufferSize = atoi(args[i]);
        }
        else if(strcmp(args[i], "-c") == 0 ||
                strcmp(args[i], "--buffer-count") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for buffer count is missing.\n");
            params->nBufferCountActual = atoi(args[i]);
        }
        else
        {
            /* do nothing, paramter may be needed by subsequent parameter readers */
        }
        ++i;
    }

    return OMX_ErrorNone;
}

/*
 */
OMX_ERRORTYPE process_encoder_jpeg_output_parameters(int argc, char **args,
                                                     OMX_PARAM_PORTDEFINITIONTYPE
                                                     * params)
{
    OMX_U32 i;

    params->format.image.nSliceHeight = 0;
    params->format.image.nStride = 0;

    params->format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
    params->format.image.eColorFormat = OMX_COLOR_FormatUnused;

    i = 1;
    while(i < argc)
    {

        if(strcmp(args[i], "-h") == 0 || strcmp(args[i], "--lumHeightSrc") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input height missing.\n");
            params->format.video.nFrameHeight = atoi(args[i]);
        }
        else if(strcmp(args[i], "-w") == 0 ||
                strcmp(args[i], "--lumWidthSrc") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for input width missing.\n");
            params->format.video.nFrameWidth = atoi(args[i]);
        }
        else if(strcmp(args[i], "-s") == 0 ||
                strcmp(args[i], "--buffer-size") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for buffer allocation size is missing.\n");
            params->nBufferSize = atoi(args[i]);
        }
        else if(strcmp(args[i], "-S") == 0 ||
                strcmp(args[i], "--slice-height") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for slice height is missing.\n");
            params->format.video.nSliceHeight = atoi(args[i]);
        }
        else if(strcmp(args[i], "-c") == 0 ||
                strcmp(args[i], "--buffer-count") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for buffer count is missing.\n");
            params->nBufferCountActual = atoi(args[i]);
        }
        else
        {
            /* do nothing, paramter may be needed by subsequent parameter readers */
        }
        ++i;
    }

    return OMX_ErrorNone;
}

/*
*/
void print_mpeg4_usage()
{
    printf("  MPEG4 parameters:\n"
           "    -p, --profile                    Sum of following:\n"
           "                                      0x0001    Simple Profile, Levels 1-3 (def)\n"
           "                                      0x0002    Simple Scalable Profile, Levels 1-2\n"
           "                                      0x0004    Core Profile, Levels 1-2\n"
           "                                      0x0008    Main Profile, Levels 2-4\n"
           "                                      0x0010    N-bit Profile, Level 2\n"
           "                                      0x0020    Scalable Texture Profile, Level 1\n"
           "                                      0x0040    Simple Face Animation Profile, Levels 1-2\n"
           "                                      0x0080    Simple Face and Body Animation (FBA) Profile,\n"
           "                                                Levels 1-2\n"
           "                                      0x0100    Basic Animated Texture Profile, Levels 1-2\n"
           "                                      0x0200    Hybrid Profile, Levels 1-2\n"
           "                                      0x0400    Advanced Real Time Simple Profiles, Levels 1-4\n"
           "                                      0x0800    Core Scalable Profile, Levels 1-3\n"
           "                                      0x1000    Advanced Coding Efficiency Profile, Levels 1-4\n"
           "                                      0x2000    Advanced Core Profile, Levels 1-2\n"
           "                                      0x4000    Advanced Scalable Texture, Levels 2-3\n"
           "\n"
           "    -L, --level                      Sum of following:\n"
           "                                      0x01  Level 0      0x10  Level 3\n"
           "                                      0x02  Level 0b     0x20  Level 4\n"
           "                                      0x04  Level 1      0x40  Level 4a\n"
           "                                      0x08  Level 2      0x80  Level 5 (def)\n"
           "\n"
           "    -U, --picture-types              Picture types allowed in the bitstream\n"
           "                                     i, p, b, s\n"
           "    -G, --svh-mode                   Enable Short Video Header mode\n"
           "    -g, --gov                        Enable GOV\n"
           "    -n, --npframes                   Number of P frames between each I frame\n"
           "    -M, --max-packet-size            Maximum size of packet in bytes.\n"
           "    -V, --time-inc-resolution        VOP time increment resolution for MPEG4.\n"
           "                                     Interpreted as described in MPEG4 standard.\n"
           "    -E, --hec                        Specifies the number of consecutive video packet\n"
           "    -R, --rvlc                       Use variable length coding\n"
           "    -C, --controll-rate              disable, variable, constant, variable-skipframes,\n"
           "                                     constant-skipframes\n"
           "    -q, --qpp                        QP value to use for P frames\n"
           "\n");

}

/*
*/
void print_avc_usage()
{
    printf("  AVC parameters:\n"
           "    -p, --profile                    Sum of following: \n"
           "                                      0x01    baseline (def)  0x10    high10\n"
           "                                      0x02    main            0x20    high422\n"
           "                                      0x04    extended        0x40    high444\n"
           "                                      0x08    high\n"
           "\n"
           "    -L, --level                      Sum of following: \n"
           "                                      0x0001   level1         0x0100    level3\n"
           "                                      0x0002   level1b        0x0200    level31 (def)\n"
           "                                      0x0004   level11        0x0400    level32\n"
           "                                      0x0008   level12        0x0800    level4\n"
           "                                      0x0010   level13        0x1000    level41\n"
           "                                      0x0020   level2         0x2000    level42\n"
           "                                      0x0040   level21        0x4000    level5\n"
           "                                      0x0080   level22        0x8000    level51\n"
           "\n"
           "    -C, --controll-rate              disable, variable, constant, variable-skipframes,\n"
           "                                     constant-skipframes\n"
           "    -n, --npframes                   Number of P frames between each I frame\n"
           "    -d, --deblocking                 Set deblocking\n"
           "    -q, --qpp                        QP value to use for P frames\n"
           "\n");

}

/*
*/
void print_h263_usage()
{
    printf("  H263 parameters:\n"
           "    -p, --profile                    Sum of following: \n"
           "                                (def) Baseline            0x01   HighCompression     0x20\n"
           "                                      H320Coding          0x02   Internet            0x40\n"
           "                                      BackwardCompatible  0x04   Interlace           0x80\n"
           "                                      ISWV2               0x08   HighLatency         0x100\n"
           "                                      ISWV3               0x10\n"
           "\n"
           "    -L, --level                      Sum of following: \n"
           "                                      Level10    0x01    Level45    0x10\n"
           "                                      Level20    0x02    Level50    0x20\n"
           "                                      Level30    0x04    Level60    0x40\n"
           "                                      Level40    0x08    Level70    0x80 (def)\n"
           "\n"
           "    -U, --picture-types              Picture types allowed in the bitstream\n"
           "                                     i, p, b, I (si), P (sp)\n"
           "    -P, --plus-type                  Use of PLUSPTYPE is allowed\n"
           "    -C, --controll-rate              disable, variable, constant, variable-skipframes,\n"
           "                                     constant-skipframes\n"
           "    -q, --qpp                        QP value to use for P frames\n"
           "\n");

}

/*
*/
void print_jpeg_usage()
{
    printf("  JPEG parameters:\n"
           "    -q, --qLevel                     JPEG Q factor value in the range of 1-100\n"
           "    -S, --slice-height               Height of slice\n" "\n");

}

/*
*/
OMX_ERRORTYPE process_mpeg4_parameters(int argc, char **args,
                                       OMX_VIDEO_PARAM_MPEG4TYPE *
                                       mpeg4parameters)
{
    int i, j;

    mpeg4parameters->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
    mpeg4parameters->eLevel = OMX_VIDEO_MPEG4Level5;

    i = 0;
    while(++i < argc)
    {

        if(strcmp(args[i], "-p") == 0 || strcmp(args[i], "--profile") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for profile is missing.\n");
            mpeg4parameters->eProfile = strtol(args[i], 0, 16);
        }
        else if(strcmp(args[i], "-L") == 0 || strcmp(args[i], "--level") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for level is missing.\n");
            mpeg4parameters->eLevel = strtol(args[i], 0, 16);
        }
        else if(strcmp(args[i], "-G") == 0 ||
                strcmp(args[i], "--svh-mode") == 0)
        {
            mpeg4parameters->bSVH = OMX_TRUE;
        }
        else if(strcmp(args[i], "-g") == 0 || strcmp(args[i], "--gov") == 0)
        {
            mpeg4parameters->bGov = OMX_TRUE;
        }
        else if(strcmp(args[i], "-n") == 0 ||
                strcmp(args[i], "--npframes") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for number of P frames is missing.\n");
            mpeg4parameters->nPFrames = atoi(args[i]);
        }
        else if(strcmp(args[i], "-R") == 0 || strcmp(args[i], "--rvlc") == 0)
        {
            mpeg4parameters->bReversibleVLC = OMX_TRUE;
        }
        else if(strcmp(args[i], "-A") == 0 ||
                strcmp(args[i], "--ac-prediction") == 0)
        {
            mpeg4parameters->bACPred = OMX_TRUE;
        }
        else if(strcmp(args[i], "-M") == 0 ||
                strcmp(args[i], "--max-packet-size") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for max packet size is missing.\n");
            mpeg4parameters->nMaxPacketSize = atoi(args[i]);
        }
        else if(strcmp(args[i], "-V") == 0 ||
                strcmp(args[i], "--time-inc-resolution") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for VOP time increment resolution is missing.\n");
            mpeg4parameters->nTimeIncRes = atoi(args[i]);
        }
        else if(strcmp(args[i], "-U") == 0 ||
                strcmp(args[i], "--picture-types") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for allowed picture types is missing.\n");

            j = strlen(args[i]);
            do
            {
                switch (args[i][--j])
                {
                case 'I':
                case 'i':
                    mpeg4parameters->nAllowedPictureTypes |=
                        OMX_VIDEO_PictureTypeI;
                    break;
                case 'P':
                case 'p':
                    mpeg4parameters->nAllowedPictureTypes |=
                        OMX_VIDEO_PictureTypeP;
                    break;
                case 'B':
                case 'b':
                    mpeg4parameters->nAllowedPictureTypes |=
                        OMX_VIDEO_PictureTypeB;
                    break;
                case 'S':
                case 's':
                    mpeg4parameters->nAllowedPictureTypes |=
                        OMX_VIDEO_PictureTypeS;
                    break;
                default:
                    return OMX_ErrorBadParameter;
                }
            }
            while(j);
        }
        else
        {
            /* Do nothing, puspose is to traverse all options and to find known ones */
        }
    }

    return OMX_ErrorNone;
}

/*
*/
OMX_ERRORTYPE process_avc_parameters(int argc, char **args,
                                     OMX_VIDEO_PARAM_AVCTYPE * parameters)
{
    int i;

    parameters->eProfile = OMX_VIDEO_AVCProfileBaseline;
    parameters->eLevel = OMX_VIDEO_AVCLevel3;

    i = 0;
    while(++i < argc)
    {

        if(strcmp(args[i], "-p") == 0 || strcmp(args[i], "--profile") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for profile is missing.\n");
            parameters->eProfile = strtol(args[i], 0, 16);
        }
        else if(strcmp(args[i], "-L") == 0 || strcmp(args[i], "--level") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for level is missing.\n");

            switch (atoi(args[i]))
            {
                /*
                 * H264ENC_BASELINE_LEVEL_1 = 10,
                 * H264ENC_BASELINE_LEVEL_1_b = 99,
                 * H264ENC_BASELINE_LEVEL_1_1 = 11,
                 * H264ENC_BASELINE_LEVEL_1_2 = 12,
                 * H264ENC_BASELINE_LEVEL_1_3 = 13,
                 * H264ENC_BASELINE_LEVEL_2 = 20,
                 * H264ENC_BASELINE_LEVEL_2_1 = 21,
                 * H264ENC_BASELINE_LEVEL_2_2 = 22,
                 * H264ENC_BASELINE_LEVEL_3 = 30,
                 * H264ENC_BASELINE_LEVEL_3_1 = 31,
                 * H264ENC_BASELINE_LEVEL_3_2 = 32
                 */
            case 10:
                parameters->eLevel = OMX_VIDEO_AVCLevel1;
                break;

            case 99:
                parameters->eLevel = OMX_VIDEO_AVCLevel1b;
                break;

            case 11:
                parameters->eLevel = OMX_VIDEO_AVCLevel11;
                break;

            case 12:
                parameters->eLevel = OMX_VIDEO_AVCLevel12;
                break;

            case 13:
                parameters->eLevel = OMX_VIDEO_AVCLevel13;
                break;

            case 20:
                parameters->eLevel = OMX_VIDEO_AVCLevel2;
                break;

            case 21:
                parameters->eLevel = OMX_VIDEO_AVCLevel21;
                break;

            case 22:
                parameters->eLevel = OMX_VIDEO_AVCLevel22;
                break;

            case 30:
                parameters->eLevel = OMX_VIDEO_AVCLevel3;
                break;

            case 31:
                parameters->eLevel = OMX_VIDEO_AVCLevel31;
                break;

            case 32:
                parameters->eLevel = OMX_VIDEO_AVCLevel32;
                break;

            default:
                parameters->eLevel = OMX_VIDEO_AVCLevel3;
                break;
            }
        }
        else if(strcmp(args[i], "-n") == 0 ||
                strcmp(args[i], "--npframes") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for number of P frames is missing.\n");
            parameters->nPFrames = atoi(args[i]);
        }
        else
        {
            /* Do nothing, puspose is to traverse all options and to find known ones */
        }
    }

    return OMX_ErrorNone;
}

/*
*/
OMX_ERRORTYPE process_parameters_bitrate(int argc, char **args,
                                         OMX_VIDEO_PARAM_BITRATETYPE * bitrate)
{
    int i;

    bitrate->eControlRate = OMX_Video_ControlRateDisable;

    i = 0;
    while(++i < argc)
    {

        if(strcmp(args[i], "-C") == 0 || strcmp(args[i], "--control-rate") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for control rate is missing.\n");

            if(strcasecmp(args[i], "disable") == 0)
                bitrate->eControlRate = OMX_Video_ControlRateDisable;
            else if(strcasecmp(args[i], "variable") == 0)
                bitrate->eControlRate = OMX_Video_ControlRateVariable;
            else if(strcasecmp(args[i], "constant") == 0)
                bitrate->eControlRate = OMX_Video_ControlRateConstant;
            else if(strcasecmp(args[i], "variable-skipframes") == 0)
                bitrate->eControlRate = OMX_Video_ControlRateVariableSkipFrames;
            else if(strcasecmp(args[i], "constant-skipframes") == 0)
                bitrate->eControlRate = OMX_Video_ControlRateConstantSkipFrames;
            else
            {
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                               "Invalid control rate type\n");
                return OMX_ErrorBadParameter;
            }
        }
        /* duplicate for common parameters */
        if(strcmp(args[i], "-B") == 0 ||
           strcmp(args[i], "--bitsPerSecond") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for bit rate is missing.\n");
            bitrate->nTargetBitrate = atoi(args[i]);
        }
        else
        {
            /* Do nothing, puspose is to traverse all options and to find known ones */
        }
    }
    return OMX_ErrorNone;
}

/*
*/
OMX_ERRORTYPE process_avc_parameters_deblocking(int argc, char **args,
                                                OMX_PARAM_DEBLOCKINGTYPE *
                                                deblocking)
{
    int i;

    i = 0;
    while(++i < argc)
    {

        if(strcmp(args[i], "-d") == 0 || strcmp(args[i], "--deblocking") == 0)
        {
            deblocking->bDeblocking = OMX_TRUE;
            break;
        }
        else
        {
            /* Do nothing, puspose is to traverse all options and to find known ones */
        }
    }

    return OMX_ErrorNone;
}

/*
*/
OMX_ERRORTYPE process_parameters_quantization(int argc, char **args,
                                              OMX_VIDEO_PARAM_QUANTIZATIONTYPE *
                                              quantization)
{
    int i;

    i = 0;
    while(++i < argc)
    {

        if(strcmp(args[i], "-q") == 0 || strcmp(args[i], "--qpp") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for QPP is missing.\n");
            quantization->nQpI = atoi(args[i]);
            break;
        }
        else
        {
            /* Do nothing, puspose is to traverse all options and to find known ones */
        }
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE process_parameters_image_qfactor(int argc, char **args,
                                               OMX_IMAGE_PARAM_QFACTORTYPE *
                                               quantization)
{
    int i;

    i = 0;
    while(++i < argc)
    {

        if(strcmp(args[i], "-q") == 0 || strcmp(args[i], "--qfactor") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for QFactor is missing.\n");
            quantization->nQFactor = atoi(args[i]);

            break;
        }
        else
        {
            /* Do nothing, purpose is to traverse all options and to find known ones */
        }
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE process_h263_parameters(int argc, char **args,
                                      OMX_VIDEO_PARAM_H263TYPE * parameters)
{
    int i, j;

    parameters->eProfile = OMX_VIDEO_H263ProfileBaseline;
    parameters->eLevel = OMX_VIDEO_H263Level70;

    i = 0;
    while(++i < argc)
    {

        if(strcmp(args[i], "-p") == 0 || strcmp(args[i], "--profile") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for profile is missing.\n");
            parameters->eProfile = strtol(args[i], 0, 16);
        }
        else if(strcmp(args[i], "-L") == 0 || strcmp(args[i], "--level") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for level is missing.\n");
            parameters->eLevel = strtol(args[i], 0, 16);
        }
        else if(strcmp(args[i], "-P") == 0 ||
                strcmp(args[i], "--plus-type") == 0)
        {
            parameters->bPLUSPTYPEAllowed = OMX_TRUE;
        }
        else if(strcmp(args[i], "-U") == 0 ||
                strcmp(args[i], "--picture-types") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for allowed picture types is missing.\n");

            j = strlen(args[i]);
            do
            {
                switch (args[i][--j])
                {
                case 'i':
                    parameters->nAllowedPictureTypes |= OMX_VIDEO_PictureTypeI;
                    break;
                case 'p':
                    parameters->nAllowedPictureTypes |= OMX_VIDEO_PictureTypeP;
                    break;
                case 'b':
                    parameters->nAllowedPictureTypes |= OMX_VIDEO_PictureTypeB;
                    break;
                case 'I':
                    parameters->nAllowedPictureTypes |= OMX_VIDEO_PictureTypeSI;
                    break;
                case 'P':
                    parameters->nAllowedPictureTypes |= OMX_VIDEO_PictureTypeSP;
                    break;
                default:
                    return OMX_ErrorBadParameter;
                }
            }
            while(j);
        }
        else if(strcmp(args[i], "-n") == 0 ||
                strcmp(args[i], "--npframes") == 0)
        {
            OMXENCODER_CHECK_NEXT_VALUE(i, args, argc,
                                        "Parameter for number of P frames is missing.\n");
            parameters->nPFrames = atoi(args[i]);
        }
        else
        {
            /* Do nothing, puspose is to traverse all options and to find known ones */
        }
    }

    return OMX_ErrorNone;
}
