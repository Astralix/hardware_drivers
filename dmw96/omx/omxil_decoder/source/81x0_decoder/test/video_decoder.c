/*-----------------------------------------------------------------------------
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
--  Description :  Video/Image decoder test client.
--
------------------------------------------------------------------------------*/

#define _GNU_SOURCE

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <OMX_Core.h>
#include <OMX_Video.h>
#include <OMX_Types.h>
#include <OMX_Component.h>

#include <OSAL.h>
#include <util.h>

#include "basetype.h"
#include "rm_parse.h"
#include "rv_depack.h"
#include "rv_decode.h"
#include "rvdecapi.h"
#include "rv_ff_read.h"
#include "ivf.h"

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

#define DEFAULT_BUFFER_COUNT       4
#define DEFAULT_BUFFER_SIZE_INPUT  262144 * 5
#define DEFAULT_BUFFER_SIZE_OUTPUT 1920 * 1088 * 4
#define STREAMBUFFER_BLOCKSIZE 0xfffff
#define RM2YUV_INITIAL_READ_SIZE	16

#define CODEC_VIDEO 0
#define CODEC_IMAGE 1

// for testing on PC only. does not work on HW
//#define CLIENT_ALLOC

#define INIT_OMX_TYPE(f) \
    memset(&f, 0, sizeof(f)); \
  (f).nSize = sizeof(f);      \
  (f).nVersion.s.nVersionMajor = 1; \
  (f).nVersion.s.nVersionMinor = 1


  // actual video properties when it comes out from the decoder
typedef struct VIDEOPROPS
{
    int framecount;
    int displaywidth;
    int displayheight;
    int stride;
    int sliceheight;
} VIDEOPROPS;

  // test case parameters
  // use 0 for "don't care"
typedef struct TESTARGS
{
    char* input;                // input file to be decoded
    char* reference_file;       // reference file (if there is a single reference. in case of multiple ref files this is NULL)
    char* reference_dir;        // reference file directory
    char* blend_input;          // blend input data for mask1 if any
    char* test_id;              // testcase ID
    int input_format;           // input video color format in case of raw input
    int width;                  // output frame width (scale X)
    int height;                 // output frame height (scale Y)
    int rotation;               // output rotation
    int mirroring;              // output mirroring (exlusive with rotation) 1 for vertical 2 for horizontal
    int output_format;          // output color format
    int crop_x;                 // input video frame cropping X start offset
    int crop_y;                 // input video frame cropping Y start offset
    int crop_width;             // cropping width
    int crop_height;            // cropping height
    int input_width;            // input video width in case of raw input
    int input_height;           // input video height in case of raw input
    int domain;                 // 0 for video, 1 for image
    int fail;                   // should this testcase fail to decode
    int no_compare;             // only decode, do not compare
    int coding_type;
    int buffer_size_in;
    int buffer_size_out;
    int limit;                  // frame limit.
    int deinterlace;
    int dithering;
    int deblocking;
    struct rgb
    {
        int alpha;
        int contrast;
        int brightness;
        int saturation;
    } rgb;

    struct mask
    {
        int originX;
        int originY;
        int width;
        int height;
    } mask1, mask2;

} TESTARGS;

typedef struct HEADERLIST
{
   OMX_BUFFERHEADERTYPE* hdrs[DEFAULT_BUFFER_COUNT];
   OMX_U32 readpos;
   OMX_U32 writepos;
} HEADERLIST;



OMX_HANDLETYPE queue_mutex;
OMX_BOOL       EOS;
OMX_BOOL       ERROR;
OMX_BOOL       PORT_SET;
OMX_BOOL       DO_DECODE;
OMX_BOOL       VERBOSE_OUTPUT;

HEADERLIST src_queue;
HEADERLIST dst_queue;

HEADERLIST input_queue;
HEADERLIST output_queue;
HEADERLIST input_ret_queue;
HEADERLIST output_ret_queue;

OMX_BUFFERHEADERTYPE* alpha_mask_buffer;

FILE* vid_out;
VIDEOPROPS* vid_props;

/* divx3 */
int divx3_width = 0;
int divx3_height = 0;
OMX_U32 offset;
int start_custom_1_3;

u32 traceUsedStream;
u32 previousUsed;

/* real media */
pthread_t threads[1];
OMX_VIDEO_PARAM_RVTYPE rv;
thread_data buffering_data;
pthread_mutex_t buff_mx;
pthread_cond_t fillbuffer;
int empty_buffer_avail = 1;
int headers_ready = 0;
int stream_end = 0;
OMX_BOOL rawRVFile = OMX_FALSE;
OMX_BOOL rawRV8 = OMX_FALSE;

/* real parameters from fileformat */
extern    rv_frame                   inputFrame;
extern    rv_backend_in_params       inputParams;
extern    RvSliceInfo slcInfo[128];
extern    u32 maxCodedWidth;
extern    u32 maxCodedHeight;
extern    u32 bIsRV8;
extern    u32 pctszSize;


void startRealFileReader(FILE * pFile, OMX_U32** pBuffer, OMX_U32* pAllocLen, OMX_BOOL* eof);

/*
 * VP6 stream file handling macros
 * */


#define leRushort( P, V) { \
    register ushort v = (uchar) *P++; \
    (V) = v + ( (ushort) (uchar) *P++ << 8); \
}
#define leRushortF( F, V) { \
    char b[2], *p = b; \
    fread( (void *) b, 1, 2, F); \
    leRushort( p, V); \
}

#define leRulong( P, V) { \
    register ulong v = (uchar) *P++; \
    v += (ulong) (uchar) *P++ << 8; \
    v += (ulong) (uchar) *P++ << 16; \
    (V) = v + ( (ulong) (uchar) *P++ << 24); \
}
#define leRulongF( F, V) { \
    char b[4] = {0,0,0,0}, *p = b; \
    V = 0; \
    fread( (void *) b, 1, 4, F); \
    leRulong( p, V); \
}

#if 0
#   define leWchar( P, V)  { * ( (char *) P)++ = (char) (V);}
#else
#   define leWchar( P, V)  { *P++ = (char) (V);}
#endif

#define leWshort( P, V)  { \
    register short v = (V); \
    leWchar( P, v) \
    leWchar( P, v >> 8) \
}
#define leWshortF( F, V) { \
    char b[2], *p = b; \
    leWshort( p, V); \
    fwrite( (void *) b, 1, 2, F); \
}

#define leWlong( P, V)  { \
    register long v = (V); \
    leWchar( P, v) \
    leWchar( P, v >> 8) \
    leWchar( P, v >> 16) \
    leWchar( P, v >> 24) \
}
#define leWlongF( F, V)  { \
    char b[4], *p = b; \
    leWlong( p, V); \
    fwrite( (void *) b, 1, 4, F); \
}
/*
 * VP6 stream file handling macros end
 * */

void init_list(HEADERLIST* list)
{
    memset(list, 0, sizeof(HEADERLIST));
}

void push_header(HEADERLIST* list, OMX_BUFFERHEADERTYPE* header)
{
    assert(list->writepos < DEFAULT_BUFFER_COUNT);
    list->hdrs[list->writepos++] = header;
}

void get_header(HEADERLIST* list, OMX_BUFFERHEADERTYPE** header)
{
    if (list->readpos == list->writepos)
    {
        *header = NULL;
        return;
    }
    *header = list->hdrs[list->readpos++];
}

void copy_list(HEADERLIST* dst, HEADERLIST* src)
{
    memcpy(dst, src, sizeof(HEADERLIST));
}

OMX_ERRORTYPE fill_buffer_done(OMX_HANDLETYPE comp, OMX_PTR appdata, OMX_BUFFERHEADERTYPE* buffer)
{
    if (EOS == OMX_TRUE || DO_DECODE == OMX_FALSE)
        return OMX_ErrorNone; // this is a buffer that is returned on transition from executing to idle. do not queue it up anymore

    if (VERBOSE_OUTPUT)
    {
        printf("\rFill buffer done: nFilledLen:%d nOffset:%d frame:%d timestamp:%lld\n",
               (int)buffer->nFilledLen,
               (int)buffer->nOffset,
               vid_props->framecount,
                buffer->nTimeStamp);
        fflush(stdout);
    }

    if (vid_out)
    {
        fwrite(buffer->pBuffer, 1, buffer->nFilledLen, vid_out);
    }
    else
    {
        FILE* strm = NULL;
        char framename[255];
        sprintf(framename, "frame%d.yuv", vid_props->framecount);
        strm = fopen(framename, "wb");
        if (strm)
        {
            fwrite(buffer->pBuffer, 1, buffer->nFilledLen, strm);
            fclose(strm);
        }
    }
    vid_props->framecount++;

    if (buffer->nFlags & OMX_BUFFERFLAG_EOS)
    {
        if (VERBOSE_OUTPUT)
            printf("\tOMX_BUFFERFLAG_EOS\n");

        return OMX_ErrorNone;
    }

    buffer->nFilledLen = 0;
    buffer->nOffset    = 0;
    buffer->nTimeStamp = 0;
    ((OMX_COMPONENTTYPE*)comp)->FillThisBuffer(comp, buffer);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE empty_buffer_done(OMX_HANDLETYPE comp, OMX_PTR appdata, OMX_BUFFERHEADERTYPE* buffer)
{
    if (buffer->nInputPortIndex == 2)
        return OMX_ErrorNone; // do not queue the alpha blend buffer in the "normal" input buffer queue

    OSAL_MutexLock(queue_mutex);
    {
        if (input_ret_queue.writepos >= DEFAULT_BUFFER_COUNT)
            printf("No space in return queue\n");
        else
            push_header(&input_ret_queue, buffer);
    }
    OSAL_MutexUnlock(queue_mutex);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE comp_event(OMX_HANDLETYPE comp, OMX_PTR appdata, OMX_EVENTTYPE event, OMX_U32 data1, OMX_U32 data2, OMX_PTR eventdata)
{
    assert(comp);

    if (VERBOSE_OUTPUT)
    {
        printf("Got component event: event:%s data1:%u data2:%u\n",
               HantroOmx_str_omx_event(event),
               (unsigned)data1,
               (unsigned)data2);
    }
    OMX_ERRORTYPE err = OMX_ErrorNone;
    switch (event)
    {
        case OMX_EventPortSettingsChanged:
            {

                PORT_SET = OMX_TRUE;
                OMX_PARAM_PORTDEFINITIONTYPE def;
                INIT_OMX_TYPE(def);
                def.nPortIndex = 1; // output
                err = ((OMX_COMPONENTTYPE*)comp)->GetParameter(comp, OMX_IndexParamPortDefinition, &def);
                assert(err == OMX_ErrorNone);
                if (VERBOSE_OUTPUT)
                {
                    printf("Output port settings:\n"
                           "  nBufferSize:%d\n"
                           "  nFrameWidth:%d\n"
                           "  nFrameHeight:%d\n"
                           "  nStride:%d\n"
                           "  nSliceHeight:%d\n"
                           "  colorformat:%s\n",
                           (int)def.nBufferSize,
                           (int)def.format.video.nFrameWidth,
                           (int)def.format.video.nFrameHeight,
                           (int)def.format.video.nStride,
                           (int)def.format.video.nSliceHeight,
                           HantroOmx_str_omx_color(def.format.video.eColorFormat));
                }

                vid_props->displaywidth  = def.format.video.nFrameWidth;
                vid_props->displayheight = def.format.video.nFrameHeight;
                vid_props->stride        = def.format.video.nStride;
                vid_props->sliceheight   = def.format.video.nSliceHeight;
            }
            break;

        case OMX_EventCmdComplete:
            {
                OMX_STATETYPE state;
                ((OMX_COMPONENTTYPE*)comp)->GetState(comp, &state);
                if (VERBOSE_OUTPUT)
                    printf("\tState: %s\n", HantroOmx_str_omx_state(state));
            }
            break;
        case OMX_EventBufferFlag:
            {
                EOS = OMX_TRUE;
            }
            break;

        case OMX_EventError:
            {
                printf("\tError: %s\n", HantroOmx_str_omx_err((OMX_ERRORTYPE)data1));
                ERROR = OMX_TRUE;
            }
            break;

        default: break;

    }
    return OMX_ErrorNone;
}

OMX_COMPONENTTYPE* create_decoder(int codec)
{
    OMX_ERRORTYPE err = OMX_ErrorBadParameter;
    OMX_COMPONENTTYPE* comp = (OMX_COMPONENTTYPE*)malloc(sizeof(OMX_COMPONENTTYPE));
    if (!comp)
    {
        err = OMX_ErrorInsufficientResources;
        goto FAIL;
    }
    memset(comp, 0, sizeof(OMX_COMPONENTTYPE));

    // somewhat hackish method for creating the components...
    OMX_ERRORTYPE HantroHwDecOmx_video_constructor(OMX_COMPONENTTYPE*, OMX_STRING);
#ifndef VIDEO_ONLY
    OMX_ERRORTYPE HantroHwDecOmx_image_constructor(OMX_COMPONENTTYPE*, OMX_STRING);
#endif
    if (codec == CODEC_IMAGE)
    {
#ifndef VIDEO_ONLY
        err = HantroHwDecOmx_image_constructor(comp, "OMX.hantro.G1.image.decoder");
#endif
        if (err != OMX_ErrorNone)
            goto FAIL;
    }
    else
    {
        err = HantroHwDecOmx_video_constructor(comp, "OMX.hantro.G1.video.decoder");
        if (err != OMX_ErrorNone)
            goto FAIL;
    }

    OMX_CALLBACKTYPE cb;
    cb.EventHandler    = comp_event;
    cb.EmptyBufferDone = empty_buffer_done;
    cb.FillBufferDone  = fill_buffer_done;

    err = comp->SetCallbacks(comp, &cb, NULL);
    if (err != OMX_ErrorNone)
        goto FAIL;

    return comp;
 FAIL:
    if (comp)
        free(comp);
    printf("error: %s\n", HantroOmx_str_omx_err(err));
    return NULL;
}

OMX_ERRORTYPE setup_component(OMX_COMPONENTTYPE* comp, const TESTARGS* args)
{
    assert(args);
    assert(comp);

    OMX_ERRORTYPE err;
    // get the expected buffer sizes
    OMX_PARAM_PORTDEFINITIONTYPE src;
    OMX_PARAM_PORTDEFINITIONTYPE dst;
    OMX_PARAM_PORTDEFINITIONTYPE pp;
    INIT_OMX_TYPE(src);
    INIT_OMX_TYPE(dst);
    INIT_OMX_TYPE(pp);
    src.nPortIndex = 0; // input port
    dst.nPortIndex = 1; // output port
    pp.nPortIndex  = 2; // post-processor input

    comp->GetParameter(comp, OMX_IndexParamPortDefinition, &src);
    comp->GetParameter(comp, OMX_IndexParamPortDefinition, &dst);
    comp->GetParameter(comp, OMX_IndexParamPortDefinition, &pp);

    // set buffers and input video format
    src.nBufferCountActual = DEFAULT_BUFFER_COUNT;
    if (args->domain == CODEC_IMAGE)
    {
        src.format.image.eCompressionFormat = args->coding_type;
        src.format.image.eColorFormat       = args->input_format;
        src.format.image.nFrameWidth        = args->input_width;
        src.format.image.nFrameHeight       = args->input_height;
    }
    else
    {
        src.format.video.eCompressionFormat = args->coding_type;
        src.format.video.eColorFormat       = args->input_format;
        src.format.video.nFrameWidth        = args->input_width;
        src.format.video.nFrameHeight       = args->input_height;
    }
    err = comp->SetParameter(comp, OMX_IndexParamPortDefinition, &src);
    if (err != OMX_ErrorNone)
        goto FAIL;

    dst.nBufferCountActual = DEFAULT_BUFFER_COUNT;
    if (args->output_format)
    {
        // setup color conversion
        if (args->domain == CODEC_IMAGE)
            dst.format.image.eColorFormat = args->output_format;
        else
            dst.format.video.eColorFormat = args->output_format;
    }
    if (args->width && args->height)
    {
        // setup scaling
        if (args->domain==CODEC_IMAGE)
        {
            dst.format.image.nFrameWidth  = args->width;
            dst.format.image.nFrameHeight = args->height;
        }
        else
        {
            dst.format.video.nFrameWidth  = args->width;
            dst.format.video.nFrameHeight = args->height;
        }
    }
    err = comp->SetParameter(comp, OMX_IndexParamPortDefinition, &dst);
    if (err != OMX_ErrorNone)
        goto FAIL;

    pp.format.image.nFrameWidth  = args->mask1.width;
    pp.format.image.nFrameHeight = args->mask1.height;
    err = comp->SetParameter(comp, OMX_IndexParamPortDefinition, &pp);

    if (err != OMX_ErrorNone)
        goto FAIL;

    if (args->rotation)
    {
        // setup rotation
        OMX_CONFIG_ROTATIONTYPE rot;
        INIT_OMX_TYPE(rot);
        rot.nRotation = args->rotation;
        err = comp->SetConfig(comp, OMX_IndexConfigCommonRotate, &rot);
        if (err != OMX_ErrorNone)
            goto FAIL;
    }
    if (args->mirroring)
    {
        OMX_CONFIG_MIRRORTYPE mir;
        INIT_OMX_TYPE(mir);
        if (args->mirroring == 1)
            mir.eMirror = OMX_MirrorVertical;
        if (args->mirroring == 2)
            mir.eMirror = OMX_MirrorHorizontal;

        err = comp->SetConfig(comp, OMX_IndexConfigCommonMirror, &mir);
        if (err != OMX_ErrorNone)
            goto FAIL;
    }
    if (args->dithering)
    {
        OMX_CONFIG_DITHERTYPE dither;
        INIT_OMX_TYPE(dither);
        dither.eDither = OMX_DitherOther;
        err = comp->SetConfig(comp, OMX_IndexConfigCommonDithering, &dither);
        if(err != OMX_ErrorNone)
           goto FAIL;
    }
    if (args->deblocking)
    {
       OMX_PARAM_DEBLOCKINGTYPE deblock;
       INIT_OMX_TYPE(deblock);
       deblock.bDeblocking = 1;
       err = comp->SetParameter(comp, OMX_IndexParamCommonDeblocking, &deblock);
       if(err != OMX_ErrorNone)
           goto FAIL;
    }

    if (args->crop_width && args->crop_height)
    {
        // setup cropping
        OMX_CONFIG_RECTTYPE rect;
        INIT_OMX_TYPE(rect);
        rect.nLeft   = args->crop_x;
        rect.nTop    = args->crop_y;
        rect.nWidth  = args->crop_width;
        rect.nHeight = args->crop_height;
        err = comp->SetConfig(comp, OMX_IndexConfigCommonInputCrop, &rect);
        if (err != OMX_ErrorNone)
            goto FAIL;
    }
    if (args->rgb.alpha)
    {
        // setup fixed alpha value for RGB conversion
        OMX_CONFIG_PLANEBLENDTYPE blend;
        INIT_OMX_TYPE(blend);
        blend.nAlpha = args->rgb.alpha;
        err = comp->SetConfig(comp, OMX_IndexConfigCommonPlaneBlend, &blend);
        if (err != OMX_ErrorNone)
            goto FAIL;
    }
    if (args->rgb.contrast)
    {
        // setup contrast for RGB conversion
        OMX_CONFIG_CONTRASTTYPE ctr;
        INIT_OMX_TYPE(ctr);
        ctr.nContrast = args->rgb.contrast;
        err = comp->SetConfig(comp, OMX_IndexConfigCommonContrast, &ctr);
        if (err != OMX_ErrorNone)
            goto FAIL;
    }

    if (args->rgb.brightness)
    {
        // setup brightness for RGB conversion
        OMX_CONFIG_BRIGHTNESSTYPE brig;
        INIT_OMX_TYPE(brig);
        brig.nBrightness = args->rgb.brightness;
        err = comp->SetConfig(comp, OMX_IndexConfigCommonBrightness, &brig);
        if (err != OMX_ErrorNone)
            goto FAIL;
    }

    if (args->rgb.saturation)
    {
        // setup saturation for RGB conversion
        OMX_CONFIG_SATURATIONTYPE satur;
        INIT_OMX_TYPE(satur);
        satur.nSaturation = args->rgb.saturation;
        err = comp->SetConfig(comp, OMX_IndexConfigCommonSaturation, &satur);
        if (err != OMX_ErrorNone)
            goto FAIL;
    }
    if (args->mask1.width && args->mask1.height)
    {
        // setup blending mask1
        pp.format.image.nFrameWidth  = args->mask1.width;
        pp.format.image.nFrameHeight = args->mask1.height;
        err = comp->SetParameter(comp, OMX_IndexParamPortDefinition, &pp);
        if (err != OMX_ErrorNone)
            goto FAIL;

        OMX_CONFIG_POINTTYPE point;
        INIT_OMX_TYPE(point);
        point.nX = args->mask1.originX;
        point.nY = args->mask1.originY;
        err = comp->SetConfig(comp, OMX_IndexConfigCommonOutputPosition, &point);
        if (err != OMX_ErrorNone)
            goto FAIL;
    }

    if (args->mask2.width && args->mask2.height)
    {
        // setup non-blending (mask2)
        OMX_CONFIG_RECTTYPE rect;
        INIT_OMX_TYPE(rect);
        rect.nLeft   = args->mask2.originX;
        rect.nTop    = args->mask2.originY;
        rect.nWidth  = args->mask2.width;
        rect.nHeight = args->mask2.height;
        err = comp->SetConfig(comp, OMX_IndexConfigCommonExclusionRect, &rect);
        if (err != OMX_ErrorNone)
            goto FAIL;

    }

    if (args->coding_type == OMX_VIDEO_CodingRV)
    {
        INIT_OMX_TYPE(rv);

        if (rawRVFile == OMX_TRUE)
        {
            rv.nMaxEncodeFrameSize = 0;
            rv.nPaddedWidth = 0;
            rv.nPaddedHeight = 0;
        }
        else
        {
            rv.nMaxEncodeFrameSize = pctszSize;
            rv.nPaddedWidth = maxCodedWidth;
            rv.nPaddedHeight = maxCodedHeight;
        }
        if (bIsRV8 || rawRV8 == OMX_TRUE)
            rv.eFormat = OMX_VIDEO_RVFormat8;
        else
            rv.eFormat = OMX_VIDEO_RVFormat9;

        err = comp->SetParameter(comp, OMX_IndexParamVideoRv, &rv);
        if (err != OMX_ErrorNone)
            goto FAIL;
    }

    // create the buffers
    err = comp->SendCommand(comp, OMX_CommandStateSet, OMX_StateIdle, NULL);
    if (err != OMX_ErrorNone)
        goto FAIL;

    // create the post-proc input buffer
    alpha_mask_buffer = NULL;
    int alpha_blend_buffer_size = args->mask1.width * args->mask1.height * 4; // 32bit ARGB
    if (alpha_blend_buffer_size == 0)
        alpha_blend_buffer_size = 4;
    err = comp->AllocateBuffer(comp, &alpha_mask_buffer, 2, NULL, alpha_blend_buffer_size);
    if (err != OMX_ErrorNone)
        goto FAIL;

    assert(alpha_mask_buffer);
    assert(args->buffer_size_in);
    assert(args->buffer_size_out);

    int i=0;
    for (i=0; i<DEFAULT_BUFFER_COUNT; ++i)
    {
        OMX_BUFFERHEADERTYPE* header = NULL;
#ifdef CLIENT_ALLOC
        void* mem = malloc(args->buffer_size_in);
        assert(mem);
        err = comp->UseBuffer(comp, &header, 0, NULL, args->buffer_size_in, mem);
#else
        err = comp->AllocateBuffer(comp, &header, 0, NULL, args->buffer_size_in);
#endif
        if (err != OMX_ErrorNone)
            goto FAIL;
        assert(header);
        push_header(&src_queue, header);
        push_header(&input_queue, header);
    }
    for (i=0; i<DEFAULT_BUFFER_COUNT; ++i)
    {
        OMX_BUFFERHEADERTYPE* header = NULL;
#ifdef CLIENT_ALLOC
        void* mem = malloc(args->buffer_size_out);
        assert(mem);
        err = comp->UseBuffer(comp, &header, 1, NULL, args->buffer_size_out, mem);
#else
        err = comp->AllocateBuffer(comp, &header, 1, NULL, args->buffer_size_out);
#endif
        if (err != OMX_ErrorNone)
            goto FAIL;
        assert(header);
        push_header(&dst_queue, header);
        push_header(&output_queue, header);
    }

    // should have transitioned to idle state now

    OMX_STATETYPE state = OMX_StateLoaded;
    while (state != OMX_StateIdle && state != OMX_StateInvalid)
    {
        err = comp->GetState(comp, &state);
        if (err != OMX_ErrorNone)
            goto FAIL;
    }
    if (state == OMX_StateInvalid)
        goto FAIL;

    err = comp->SendCommand(comp, OMX_CommandStateSet, OMX_StateExecuting, NULL);
    if (err != OMX_ErrorNone)
        goto FAIL;

    while (state != OMX_StateExecuting && state != OMX_StateInvalid)
    {
        err = comp->GetState(comp, &state);
        if (err != OMX_ErrorNone)
            goto FAIL;
    }
    if (state == OMX_StateInvalid)
        goto FAIL;

    return OMX_ErrorNone;
 FAIL:
    // todo: should deallocate buffers and stuff
    printf("error: %s\n", HantroOmx_str_omx_err(err));
    return err;
}


void destroy_decoder(OMX_COMPONENTTYPE* comp)
{
    OMX_STATETYPE state = OMX_StateExecuting;
    comp->GetState(comp, &state);
    if (state == OMX_StateExecuting)
    {
        comp->SendCommand(comp, OMX_CommandStateSet, OMX_StateIdle, NULL);
        while (state != OMX_StateIdle)
            comp->GetState(comp, &state);
    }
    if (state == OMX_StateIdle)
        comp->SendCommand(comp, OMX_CommandStateSet, OMX_StateLoaded, NULL);

    assert(alpha_mask_buffer);
    comp->FreeBuffer(comp, 2, alpha_mask_buffer);

    int i;
    for (i=0; i<DEFAULT_BUFFER_COUNT; ++i)
    {
        OMX_BUFFERHEADERTYPE* src_hdr = NULL;
        OMX_BUFFERHEADERTYPE* dst_hdr = NULL;
        get_header(&src_queue, &src_hdr);
        get_header(&dst_queue, &dst_hdr);
#ifdef CLIENT_ALLOC
        if (src_hdr->pBuffer) free(src_hdr->pBuffer);
        if (dst_hdr->pBuffer) free(dst_hdr->pBuffer);
#endif
        comp->FreeBuffer(comp, 1, dst_hdr);
        comp->FreeBuffer(comp, 0, src_hdr);
    }

    while (state != OMX_StateLoaded && state != OMX_StateInvalid)
        comp->GetState(comp, &state);

    comp->ComponentDeInit(comp);
    free(comp);
}


typedef int (*read_func)(FILE*, char*, int, void*, OMX_BOOL*);

int read_any_file(FILE* strm, char* buffer, int bufflen, void* state, OMX_BOOL* eof)
{
    int ret = fread(buffer, 1, bufflen, strm);

    *eof = feof(strm);
    return ret;
}

int read_mpeg4_file(FILE* strm, char* buffer, int bufflen, void* state, OMX_BOOL* eof)
{
    u32 idx = 0, VopStart = 0;
    u8 *tempBuffer;
    u32 StartCode = 0;
    fpos_t strmPos;

    tempBuffer = (u8*) buffer;

    if(fgetpos(strm, &strmPos))
    {
        printf("FILE POSITION GET ERROR\n");
        return 0;
    }

    while(!VopStart)
    {
        fread(tempBuffer+idx, sizeof(u8), 1, strm);

        if(feof(strm))
        {

            fprintf(stdout, "TB: End of stream\n");
            *eof = 1;
            idx += 4;
            break;
        }

        if(idx > 7)
        {
            if(((tempBuffer[idx - 3] == 0x00) &&
                        (tempBuffer[idx - 2] == 0x00) &&
                        (((tempBuffer[idx - 1] == 0x01) &&
                          (tempBuffer[idx] == 0xB6)))))
            {
                VopStart = 1;
                StartCode = ((tempBuffer[idx] << 24) |
                                (tempBuffer[idx - 1] << 16) |
                                (tempBuffer[idx - 2] << 8) |
                                tempBuffer[idx - 3]);
            }
        }
        idx++;
    }

    idx -= 4; // Next VOP start code was also read so we need to decrease 4
    traceUsedStream = previousUsed;
    previousUsed += idx;

    if(fsetpos(strm, &strmPos))
    {
        printf("FILE POSITION SET ERROR\n");
        return 0;
    }
    /* Read the rewind stream */
    fread(buffer, sizeof(u8), idx, strm);
    if(feof(strm))
    {
        printf("TRYING TO READ STREAM BEYOND STREAM END\n");
        *eof = feof(strm);
        return 0;
    }
    if(ferror(strm))
    {
        printf("FILE ERROR\n");
        return 0;
    }
    if (VERBOSE_OUTPUT)
        printf("READ DECODE UNIT %d\n", idx);

    return (idx);
}

int read_sorenson_file(FILE* strm, char* buffer, int bufflen, void* state, OMX_BOOL* eof)
{
    u32 idx = 0, VopStart = 0;
    u8 *tempBuffer;
    u32 StartCode = 0;
    fpos_t strmPos;

    tempBuffer = (u8*) buffer;

    if(fgetpos(strm, &strmPos))
    {
        printf("FILE POSITION GET ERROR\n");
        return 0;
    }

    while(!VopStart)
    {
        fread(tempBuffer+idx, sizeof(u8), 1, strm);

        if(feof(strm))
        {

            fprintf(stdout, "TB: End of stream\n");
            *eof = 1;
            idx += 4;
            break;
        }

        if(idx > 7)
        {
            if((tempBuffer[idx - 3] == 0x00) &&
                (tempBuffer[idx - 2] == 0x00) &&
                ((tempBuffer[idx - 1] & 0xF8) == 0x80))
            {
                VopStart = 1;
                StartCode = ((tempBuffer[idx] << 24) |
                                (tempBuffer[idx - 1] << 16) |
                                (tempBuffer[idx - 2] << 8) |
                                tempBuffer[idx - 3]);
            }
        }
        idx++;
    }

    idx -= 4; // Next VOP start code was also read so we need to decrease 4
    traceUsedStream = previousUsed;
    previousUsed += idx;

    if(fsetpos(strm, &strmPos))
    {
        printf("FILE POSITION SET ERROR\n");
        return 0;
    }
    /* Read the rewind stream */
    fread(buffer, sizeof(u8), idx, strm);
    if(feof(strm))
    {
        printf("TRYING TO READ STREAM BEYOND STREAM END\n");
        *eof = feof(strm);
        return 0;
    }
    if(ferror(strm))
    {
        printf("FILE ERROR\n");
        return 0;
    }
    if (VERBOSE_OUTPUT)
        printf("READ DECODE UNIT %d\n", idx);

    return (idx);
}

int read_h264_file(FILE* strm, char* buffer, int bufflen, void* state, OMX_BOOL* eof)
{
    u32 index = 0;
    u32 zeroCount;
    u8 byte;
    //u8 nextPacket = 0;
    i32 ret = 0;
    u8 firstRead = 1;
    fpos_t strmPos;
    //static u8 *stream = NULL;

    if(fgetpos(strm, &strmPos))
    {
        printf("FILE POSITION GET ERROR\n");
        return 0;
    }

    /* test end of stream */
    ret = fread(&byte, sizeof(byte), 1, strm);
    if(ferror(strm))
    {
        printf("STREAM READ ERROR\n");
        return 0;
    }
    if(feof(strm))
    {
        printf("END OF STREAM\n");
        *eof = feof(strm);
        return 0;
    }

    /* leading zeros of first NAL unit */
    do
    {
        index++;
        /* the byte is already read to test the end of stream */
        if(!firstRead)
        {
            ret = fread(&byte, sizeof(byte), 1, strm);
            if(ferror(strm))
            {
                printf("STREAM READ ERROR\n");
                return 0;
            }
        }
        else
        {
            firstRead = 0;
        }
    }
    while(byte != 1 && !feof(strm));

    /* invalid start code prefix */
    if(feof(strm) || index < 3)
    {
        printf("INVALID BYTE STREAM\n");
        return 0;
    }

    zeroCount = 0;

    /* Search stream for next start code prefix */
    /*lint -e(716) while(1) used consciously */
    while(1)
    {
        /*byte = stream[index++]; */
        index++;
        ret = fread(&byte, sizeof(byte), 1, strm);
        if(ferror(strm))
        {
            printf("FILE ERROR\n");
            return 0;
        }
        if(!byte)
            zeroCount++;

        if((byte == 0x01) && (zeroCount >= 2))
        {
            /* Start code prefix has two zeros
             * Third zero is assumed to be leading zero of next packet
             * Fourth and more zeros are assumed to be trailing zeros of this
             * packet */
            if(zeroCount > 3)
            {
                index -= 4;
                zeroCount -= 3;
            }
            else
            {
                index -= zeroCount + 1;
                zeroCount = 0;
            }
            break;
        }
        else if(byte)
            zeroCount = 0;

        if(feof(strm))
        {
            --index;
            break;
        }

    }

    /* Store pointer to the beginning of the packet */
    if(fsetpos(strm, &strmPos))
    {
        printf("FILE POSITION SET ERROR\n");
        return 0;
    }

    /* Read the rewind stream */
    fread(buffer, sizeof(byte), index, strm);
    if(feof(strm))
    {
        printf("TRYING TO READ STREAM BEYOND STREAM END\n");
        *eof = feof(strm);
        return 0;
    }
    if(ferror(strm))
    {
        printf("FILE ERROR\n");
        return 0;
    }
    if (VERBOSE_OUTPUT)
        printf("READ DECODE UNIT %d\n", index);

    return index;
}

int read_vp6_file(FILE* strm, char* buffer, int bufflen, void* state, OMX_BOOL* eof)
{
    ulong frameSize = 0;

    leRulongF(strm, frameSize);

    if( frameSize > bufflen)
    {
        /* too big a frame */
        return -1;
    }

    fread(buffer, 1, frameSize, strm);

    *eof = feof(strm);
    return (int) frameSize;

}

int read_custom_1_3_file(FILE* strm, char* buffer, int bufflen, void* state, OMX_BOOL* eof)
{
    OMX_U8  sizeTmp[4];
    OMX_U32 size, tmp;
    OMX_U32 *tmpBuffer;

    tmpBuffer = (OMX_U32*) buffer;
    switch (start_custom_1_3)
    {
        case 0:
        {
            /* set divx3 stream width and height */
            tmpBuffer[0] = divx3_width;
            tmpBuffer[1] = divx3_height;
            start_custom_1_3 = 1;
            return 8;
        }
        break;
        default:
        {
            /* skip "00dc" from frame beginning (may signal video chunk start code) */
            for(;;)
            {
                fseek( strm, offset, SEEK_SET );
                fread( &sizeTmp, sizeof(OMX_U8), 4, strm );
                if( (sizeTmp[0] == '0' &&
                    sizeTmp[1] == '0' &&
                    sizeTmp[2] == 'd' &&
                    sizeTmp[3] == 'c')  ||
                    ( sizeTmp[0] == 0x0 &&
                      sizeTmp[1] == 0x0 &&
                      sizeTmp[2] == 0x0 &&
                      sizeTmp[3] == 0x0 ) )
                {
                    offset += 4;
                    continue;
                }
                break;
            }

            size = (sizeTmp[0]) +
                   (sizeTmp[1] << 8) +
                   (sizeTmp[2] << 16) +
                   (sizeTmp[3] << 24);

            if( size == -1 )
            {
                *eof = 1;
                offset = 0;
                return 0;
            }

            tmp = fread( buffer, sizeof(OMX_U8), size, strm );
            if( size != tmp )
            {
                *eof = 1;
                offset = 0;
                return 0;
            }

            offset += size + 4;
            *eof = 0;
            return size;
        }
        break;
    }
    assert(0);
    return 0; // make gcc happy
}

enum
{
    FF_VP7,
    FF_VP8,
    FF_NONE
};

union {
    IVF_FRAME_HEADER ivf;
    u8 p[12];
    } fh;

int formatCheck;

int read_vp8_file(FILE* strm, char* buffer, int bufflen, void* state, OMX_BOOL* eof)
{
    u32 ret = 0;
    u32 frameSize = 0;
    IVF_HEADER ivf;

    if (formatCheck == 0)
    {
        /* check format */
        char id[5] = "DKIF";
        char string[5];

        formatCheck = 1;

        if (fread(string, 1, 5, strm) == 5)
        {
            if (strncmp(id, string, 5))
            {
                printf("ERROR: NOT VP8!\n");
                return 0;
            }
        }
        rewind(strm);

        /* Read VP8 IVF file header */
        ret = fread( &ivf, sizeof(char), sizeof(IVF_HEADER), strm );
        if( ret == 0 )
        {
            printf("Read IVF file header failed\n");
            return 0;
        }
    }

    /* Read frame header */
    ret = fread( &fh, sizeof(char), sizeof(IVF_FRAME_HEADER), strm );
    if( ret == 0 )
    {
        if(feof(strm))
        {
            *eof = 1;
            return 0;
        }
        printf("Read frame header failed\n");
        return 0;
    }

    frameSize =
         fh.p[0] +
        (fh.p[1] << 8) +
        (fh.p[2] << 16) +
        (fh.p[3] << 24);

    if(feof(strm))
    {
        printf("EOF\n");
        *eof = 1;
        return 0;
    }

    if(frameSize > bufflen)
    {
        fprintf(stderr, "VP8 frame size %d > buffer size %d\n",
            frameSize, bufflen );
        return 0;
    }

    fread( buffer, sizeof(u8), frameSize, strm );
    return frameSize;
}

int read_webp_file(FILE* strm, char* buffer, int bufflen, void* state, OMX_BOOL* eof)
{
    char signature[] = "WEBP";
    char format[] = "VP8 ";
    u8 tmp[4];
    u32 frameSize = 0;
    
    fseek(strm, 8, SEEK_CUR);
    fread(tmp, sizeof(u8), 4, strm);
    
    if (strncmp(signature, (char*)tmp, 4))
        return 0;
    
    fread(tmp, sizeof(u8), 4, strm);
    
    if (strncmp(format, (char*)tmp, 4))
        return 0;
    
    fread(tmp, sizeof(u8), 4, strm);
    
    frameSize = 
         tmp[0] +
        (tmp[1] << 8) +
        (tmp[2] << 16) +
        (tmp[3] << 24);
    
    if(frameSize > bufflen)
    {
        fprintf(stderr, "WebP frame size %d > buffer size %d\n",
            frameSize, bufflen );
        return 0;
    }

    fread( buffer, sizeof(u8), frameSize, strm );
    
    *eof = 1;
    
    printf("WEBP frame size %d\n", frameSize);
    return frameSize;
}

i32 strmRew = 0;
int read_avs_file(FILE* strm, char* buffer, int bufflen, void* state, OMX_BOOL* eof)
{
#define FBLOCK 1024

    u32 idx = 0, VopStart = 0;
    u8 *tempBuffer;
    u32 buffBytes = 0;
    u32 StartCode;

    tempBuffer = (u8*) buffer;

    while(!VopStart)
    {
        if(buffBytes == 0 && feof(strm))
        {

            fprintf(stdout, "TB: End of stream\n");
            *eof = 1;
            if (VERBOSE_OUTPUT)
                printf("READ DECODE UNIT %d\n", idx);
            return idx;
        }

        if (buffBytes == 0)
        {
            buffBytes = fread(tempBuffer+idx, sizeof(u8), FBLOCK, strm);
        }

        if(idx >= 4)
        {
            if(( (tempBuffer[idx - 3] == 0x00) &&
                 (tempBuffer[idx - 2] == 0x00) &&
                (( (tempBuffer[idx - 1] == 0x01) &&
                    (tempBuffer[idx] == 0xB3 ||
                    tempBuffer[idx] == 0xB6)))))
            {
                VopStart = 1;
                StartCode = ( (tempBuffer[idx] << 24) |
                              (tempBuffer[idx - 1] << 16) |
                              (tempBuffer[idx - 2] << 8) |
                               tempBuffer[idx - 3]);
            }
        }
        if(idx >= STREAMBUFFER_BLOCKSIZE)
        {
            fprintf(stdout, "idx = %d,lenght = %d \n", idx, STREAMBUFFER_BLOCKSIZE);
            fprintf(stdout, "TB: Out Of Stream Buffer\n");
            *eof = 1;
            return 0;
        }
        if(idx > strmRew + 128)
        {
            idx -= strmRew;
        }
        idx++;
        buffBytes--;
    }
    idx -= 4; // Next VOP start code was also read so we need to decrease 4
    buffBytes += 4;
    traceUsedStream = previousUsed;
    previousUsed += idx;

    if (buffBytes)
    {
        fseek(strm, -buffBytes, SEEK_CUR);
    }
    if (VERBOSE_OUTPUT)
        printf("READ DECODE UNIT %d\n", idx);

#undef FBLOCK
    return (idx);
}

int start_rv = 0;
static OMX_U32 rv_offset = 0;
int read_rv_file(FILE* strm, char* buffer, int bufflen, void* state, OMX_BOOL* eof)
{

    OMX_U32 filledLenght = 0;

    if(rawRVFile == OMX_TRUE)
    {
        u32 idx = 0, VopStart = 0;
        u8 *tempBuffer;
        //u32 StartCode = 0;
        fpos_t strmPos;

        tempBuffer = (u8*) buffer;

        if(fgetpos(strm, &strmPos))
        {
            printf("FILE POSITION GET ERROR\n");
            return 0;
        }

        while(!VopStart)
        {
            fread(tempBuffer+idx, sizeof(u8), 1, strm);

            if(feof(strm))
            {

                fprintf(stdout, "TB: End of stream, idx %d\n", idx);
                *eof = 1;
                idx += 8;
                if (rawRV8 != OMX_TRUE)
                    idx++;
                break;
            }

            if(idx > 8)
            {
                if (rawRV8 == OMX_TRUE)
                {
                    if (tempBuffer[idx - 7] == 0 && tempBuffer[idx - 6] == 0 &&
                        tempBuffer[idx - 5] == 1 && !(tempBuffer[idx - 4]&170) &&
                        !(tempBuffer[idx - 3]&170) && !(tempBuffer[idx - 2]&170) &&
                        (tempBuffer[idx - 1]&170) == 2)
                    {
                        VopStart = 1;
                    }
                }
                else
                {
                    if (tempBuffer[idx - 8] == 85  && tempBuffer[idx - 7] == 85 &&
                        tempBuffer[idx - 6] == 85  && tempBuffer[idx - 5] == 85 &&
                        !(tempBuffer[idx - 4]&170) && !(tempBuffer[idx - 3]&170) &&
                        !(tempBuffer[idx - 2]&170) && (tempBuffer[idx - 1]&170) == 2)
                    {
                        VopStart = 1;
                    }
                }
            }
            idx++;
        }

        idx -= 8; // Next VOP start code was also read so we need to decrease 8

        if (rawRV8 != OMX_TRUE)
            idx--;

        traceUsedStream = previousUsed;
        previousUsed += idx;

        if(fsetpos(strm, &strmPos))
        {
            printf("FILE POSITION SET ERROR\n");
            return 0;
        }
        /* Read the rewind stream */
        fread(buffer, sizeof(u8), idx, strm);
        if(feof(strm))
        {
            printf("TRYING TO READ STREAM BEYOND STREAM END\n");
            *eof = feof(strm);
            return 0;
        }
        if(ferror(strm))
        {
            printf("FILE ERROR\n");
            return 0;
        }
        if (VERBOSE_OUTPUT)
            printf("READ DECODE UNIT %d\n", idx);

        return (idx);
    }
    else
    {
        pthread_mutex_lock(&buff_mx);

        if(empty_buffer_avail && !stream_end)
            pthread_cond_wait(&fillbuffer, &buff_mx);

        pthread_mutex_unlock(&buff_mx);

        if(stream_end == 1)
        {
            *eof = OMX_TRUE;
            filledLenght = 0;
        }
        else
        {
            OMX_OTHER_EXTRADATATYPE * segDataBuffer;
            memcpy(buffer, inputFrame.pData, inputParams.dataLength);

            segDataBuffer = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U32)buffer + inputParams.dataLength +3)& ~3);

            segDataBuffer->nDataSize = 8 * (inputParams.numDataSegments+1);

            memcpy(segDataBuffer->data, slcInfo, segDataBuffer->nDataSize);

            memset(slcInfo,0,sizeof( RvDecSliceInfo)*128);

            filledLenght = inputParams.dataLength;

            pthread_mutex_lock(&buff_mx);

            empty_buffer_avail++;
            /* start for file format */
            pthread_cond_signal(&fillbuffer);

            pthread_mutex_unlock(&buff_mx);

        }
    }

    return filledLenght; //decInput.dataLen;

}

typedef struct RCVSTATE
{
   int rcV1;
   int advanced;
   int filesize;
} RCVSTATE;

int read_rcv_file(FILE* strm, char* buffer, int bufflen, void* state, OMX_BOOL* eof)
{
    RCVSTATE* rcv = (RCVSTATE*)state;
    switch (ftell(strm))
    {
        case 0:
        {
            fseek(strm, 0, SEEK_END);
            rcv->filesize = ftell(strm);
            fseek(strm, 0, SEEK_SET);
            char buff[4];
            fread(buff, 1, 4, strm);
            rcv->advanced = buff[2];
            rcv->rcV1     = !(buff[3] & 0x40);
            fseek(strm, 0, SEEK_SET);
            if (rcv->advanced)
                return fread(buffer, 1, bufflen, strm);

            if (rcv->rcV1)
                return fread(buffer, 1, 20, strm);
            else
                return fread(buffer, 1, 36, strm);
        }
        break;
        default:
        {
            if (rcv->advanced)
            {
                int ret = fread(buffer, 1, bufflen, strm);
                *eof = feof(strm);
                return ret;
            }

            int framesize = 0;
            int ret = 0;
            unsigned char buff[8];
            if (rcv->rcV1)
                fread(buff, 1, 4, strm);
            else
                fread(buff, 1, 8, strm);

            framesize |= buff[0];
            framesize |= buff[1] << 8;
            framesize |= buff[2] << 16;

            assert(framesize > 0);
            if (bufflen < framesize)
            {
                printf("data buffer is too small for vc-1 frame\n");
                printf("buffer: %d framesize: %d\n", bufflen, framesize);
                return -1;
            }
            ret = fread(buffer, 1, framesize, strm);
            *eof = ftell(strm) == rcv->filesize;
            return ret;
        }
        break;
    }
    assert(0);
    return 0; // make gcc happy
}

int test_rv_file_read(const char* input_file, const char* yuv_file,  void* readstate)
{
    FILE* video = NULL;
    FILE* yuv   = NULL;
    int ret = 0;

    video = fopen(input_file, "rb");
    if (video==NULL)
    {
        printf("failed to open input file:%s\n", input_file);
        ret = -1;
    }
    if (yuv_file)
    {
        yuv = fopen(yuv_file, "wb");
        if (yuv==NULL)
        {
            printf("failed to open output file\n");
            ret = -1;
        }
    }

    OMX_BOOL eof                 = OMX_FALSE;
    int bufflen                  = 300000;

    void *buffer                 = malloc(bufflen);
    memset(buffer,0,bufflen);

    while (eof == OMX_FALSE)
    {
        int size = read_rv_file(video, buffer, bufflen, readstate, &eof);

        if (size != 0)
        {
            fwrite(buffer,1,size, yuv);
        }
    }
    printf("close files\n");
    if (video) fclose(video);

    if (yuv)
    {
        fclose(yuv);
    }
    printf("step3\n");
    return ret;
}

int decode_file(const char* input_file, const char* yuv_file,  VIDEOPROPS* props, const TESTARGS* args, read_func rf, void* readstate)
{

    init_list(&src_queue);
    init_list(&dst_queue);
    init_list(&input_queue);
    init_list(&output_queue);
    init_list(&input_ret_queue);
    init_list(&output_ret_queue);
    memset(props, 0, sizeof(VIDEOPROPS));
    vid_out = NULL;
    vid_props = props;
    int ret = 0;
    OMX_U32** ppStreamBuffer = NULL;
    OMX_U32* pAllocLen = NULL;
    OMX_BOOL eof = OMX_FALSE;
    void *thread_ret;

    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_COMPONENTTYPE* dec = NULL;
    FILE* video = NULL;
    FILE* yuv   = NULL;
    FILE* mask  = NULL;
    
    OMX_VIDEO_VP8REFERENCEFRAMEINFOTYPE Vp8RefType;
    INIT_OMX_TYPE(Vp8RefType);
    Vp8RefType.nPortIndex = 1;

    BYTE ucBuf[RM2YUV_INITIAL_READ_SIZE];
    OMX_S32 lBytesRead = 0;

    video = fopen(input_file, "rb");
    if (video==NULL)
    {
        printf("failed to open input file:%s\n", input_file);
        goto FAIL;
    }
    if (yuv_file)
    {
        yuv = fopen(yuv_file, "wb");
        if (yuv==NULL)
        {
            printf("failed to open output file\n");
            goto FAIL;
        }
    }

    if (args->coding_type == OMX_VIDEO_CodingRV)
    {
        /* Real fileformat thread is started here */
        /* Start Real file format thread */

        /* Read the first few bytes of the file */
        lBytesRead = (OMX_S32) fread((void*) ucBuf, 1, RM2YUV_INITIAL_READ_SIZE, video);
        if (lBytesRead != RM2YUV_INITIAL_READ_SIZE)
        {
            printf("Could not read %d bytes at the beginning of %s\n",
            RM2YUV_INITIAL_READ_SIZE, input_file);
            goto FAIL;
        }
        /* Seek back to the beginning */
        fseek(video, 0, SEEK_SET);

        if (!rm_parser_is_rm_file(ucBuf, RM2YUV_INITIAL_READ_SIZE))
        {
            if(ucBuf[0] == 0 && ucBuf[1] == 0 && ucBuf[2] == 1)
                rawRV8 = OMX_TRUE;
            else
                rawRV8 = OMX_FALSE;

            rawRVFile = OMX_TRUE;

            printf("Raw RV file, RV8 %d\n", rawRV8);
        }
        else
        {

            pthread_mutex_init(&buff_mx, NULL);
            pthread_cond_init(&fillbuffer, NULL);

            startRealFileReader(video, ppStreamBuffer, pAllocLen, &eof);

            pthread_mutex_lock(&buff_mx);

            if(!headers_ready)
            {
                /* wait for signal */
               pthread_cond_wait(&fillbuffer, &buff_mx);
            }
            /* unlock mutex */
            pthread_mutex_unlock(&buff_mx);
        }

    }

    dec = create_decoder(args->domain);
    if (!dec)
        goto FAIL;

    if (setup_component(dec, args) != OMX_ErrorNone)
        goto FAIL;

    vid_out = yuv;

    if (args->blend_input)
    {
        // send blend input to the decoder
        assert(alpha_mask_buffer);
        mask = fopen(args->blend_input, "rb");
        if (mask == NULL)
        {
            printf("failed to open mask file: %s\n", args->blend_input);
            goto FAIL;
        }
        // read the whole thing into the buffer
        fseek(mask, 0, SEEK_END);
        int size = ftell(mask);
        fseek(mask, 0, SEEK_SET);
        if (size > alpha_mask_buffer->nAllocLen)
        {
            printf("alpha mask data size greater than buffer size. mask ignored\n");
        }
        else
        {
            fread(alpha_mask_buffer->pBuffer, 1, size, mask);
            alpha_mask_buffer->nInputPortIndex = 2;
            alpha_mask_buffer->nOffset         = 0;
            alpha_mask_buffer->nFilledLen      = size;
            err = dec->EmptyThisBuffer(dec, alpha_mask_buffer);
            if (err != OMX_ErrorNone)
                goto FAIL;
        }
    }


    OMX_BUFFERHEADERTYPE* output = NULL;
    do
    {
        get_header(&output_queue, &output);
        if (output)
        {
            output->nOutputPortIndex = 1;
            err = dec->FillThisBuffer(dec, output);
            if (err != OMX_ErrorNone)
                goto FAIL;
        }
    }
    while (output);

    EOS          = OMX_FALSE;
    ERROR        = OMX_FALSE;
    DO_DECODE    = OMX_TRUE;
    int over_the_limit = 0;
    OMX_S64 timeStamp = 0;

    while (eof == OMX_FALSE && ERROR == OMX_FALSE)
    {
        OMX_BUFFERHEADERTYPE* input  = NULL;
        get_header(&input_queue, &input);
        if (input==NULL)
        {
            OSAL_MutexLock(queue_mutex);
            copy_list(&input_queue, &input_ret_queue);
            init_list(&input_ret_queue);
            OSAL_MutexUnlock(queue_mutex);
            get_header(&input_queue, &input);
            if (input==NULL)
            {
                usleep(1000);
                continue;
            }
        }
        assert(input);
        input->nInputPortIndex   = 0;

        int ret = rf(video, (char*)input->pBuffer, input->nAllocLen, readstate, &eof);
        if (ret < 0)
            goto FAIL;

        //over_the_limit = 0;
        if (args->limit && props->framecount >= args->limit)
        {
            printf("frame limit reached\n");
            over_the_limit = 1;
        }

        input->nOffset     = 0;
        input->nFilledLen  = ret;
        input->nTimeStamp  = timeStamp;
        timeStamp += 10;

        //eof = feof(video) != 0;
        if (eof)
            input->nFlags |= OMX_BUFFERFLAG_EOS;

        if (args->coding_type == OMX_VIDEO_CodingRV && rawRVFile == OMX_FALSE)
        {
            input->nFlags |= OMX_BUFFERFLAG_EXTRADATA;
        }

        err = dec->EmptyThisBuffer(dec, input);
        if (err != OMX_ErrorNone)
            goto FAIL;
        usleep(0);
        if (args->coding_type == OMX_VIDEO_CodingVP8)
        {
            dec->GetConfig(dec, OMX_IndexConfigVideoVp8ReferenceFrameType, &Vp8RefType);
            if (VERBOSE_OUTPUT)
                printf("Fill buffer: isIntra %d isGoldenOrAlternate %d\n", Vp8RefType.bIsIntraFrame, Vp8RefType.bIsGoldenOrAlternateFrame);
        }
        if (over_the_limit)
            break;
    }

        empty_buffer_avail = 1;
        headers_ready = 0;
        stream_end = 0;

    if (!over_the_limit)
    {
        // get stream end event
        while (EOS == OMX_FALSE && ERROR == OMX_FALSE)
           usleep(1000);
    }
    ret = !ERROR;

 FAIL:
    DO_DECODE = OMX_FALSE;
    if (video) fclose(video);
    if (yuv)   fclose(yuv);
    if (mask)  fclose(mask);
    if (dec)   destroy_decoder(dec);
    if(threads[0]) // join real file format reader thread
        pthread_join(threads[0], &thread_ret);
    return ret;
}



#define CMP_BUFF_SIZE 10*1024

int compare_output(const char* reference, const char* temp)
{
    printf("comparing files\n");
    printf("temporary file:%s\nreference file:%s\n", temp, reference);
    int success    = 0;
    char* buff_tmp = NULL;
    char* buff_ref = NULL;
    FILE* file_tmp = fopen(temp, "rb");
    FILE* file_ref = fopen(reference, "rb");
    if (!file_tmp)
    {
        printf("failed to open temp file\n");
        goto FAIL;
    }
    if (!file_ref)
    {
        printf("failed to open reference file\n");
        goto FAIL;
    }
    fseek(file_tmp, 0, SEEK_END);
    fseek(file_ref, 0, SEEK_END);
    int tmp_size = ftell(file_tmp);
    int ref_size = ftell(file_ref);
    int min_size = tmp_size;
    int wrong_size = 0;
    if (tmp_size != ref_size)
    {
        min_size = tmp_size < ref_size ? tmp_size : ref_size;
        printf("file sizes do not match: temp: %d reference: %d bytes\n", tmp_size, ref_size);
        printf("comparing first %d bytes\n", min_size);
    if(tmp_size == 0)
    {
        wrong_size = 1;
    }
    }
    if(wrong_size)
    {
        success = 0;
    }
    else
    {
    fseek(file_tmp, 0, SEEK_SET);
    fseek(file_ref, 0, SEEK_SET);

    buff_tmp = (char*)malloc(CMP_BUFF_SIZE);
    buff_ref = (char*)malloc(CMP_BUFF_SIZE);

    int pos = 0;
    int min = 0;
    int rem = 0;

    success = 1;
    while (pos < min_size)
    {
        rem = min_size - pos;
            min = rem < CMP_BUFF_SIZE ? rem : CMP_BUFF_SIZE;
            fread(buff_tmp, min, 1, file_tmp);
            fread(buff_ref, min, 1, file_ref);

            if (memcmp(buff_tmp, buff_ref, min))
            {
                success = 0;
            }
                pos += min;
    }
    }

    printf("%s file cmp done, %s\n", reference, success ? "SUCCESS!" : "FAIL");

FAIL:
    if (file_tmp) fclose(file_tmp);
    if (file_ref) fclose(file_ref);
    if (buff_tmp) free(buff_tmp);
    if (buff_ref) free(buff_ref);
    return success;
}

typedef struct SCRIPVAR {
    char* name;
    char* value;
} SCRIPTVAR;

int map_format_string(const char* format)
{
    char name[255];
    memset(name, 0, sizeof(name));
    if (strncmp(format, "OMX_COLOR_Format", 16)!=0)
        strcat(name, "OMX_COLOR_Format");
    strcat(name, format);

    int i=OMX_COLOR_FormatUnused;
    for (; i<=OMX_COLOR_Format24BitABGR6666; ++i)
    {
        if (strcmp(name, HantroOmx_str_omx_color(i))==0)
            return i;
    }
    return -1;
}

const char* format_to_file_ext(int format)
{
    switch (format)
    {
        case OMX_COLOR_FormatUnused:
        case OMX_COLOR_FormatL8:
            return "yuv";
        case OMX_COLOR_Format16bitRGB565:
        case OMX_COLOR_Format16bitBGR565:
        case OMX_COLOR_Format16bitARGB1555:
        case OMX_COLOR_Format16bitARGB4444:
            return "rgb16";
        case OMX_COLOR_Format32bitARGB8888:
        case OMX_COLOR_Format32bitBGRA8888:
            return "rgb32";
        case OMX_COLOR_FormatYCbYCr:
            return "yuyv";
        case OMX_COLOR_FormatYCrYCb:
            return "yuyv";
        case OMX_COLOR_FormatCbYCrY:
            return "yuyv";
        case OMX_COLOR_FormatCrYCbY:
            return "yuyv";
        default:assert(!"unknown color format");
    }
    return NULL;
}


#define ASSURE_NEXT_ARG(i, argc)                \
  if (!((i)+1 < (argc)))                        \
      return 0

int parse_argument_vector(const char** argv, int argc, TESTARGS* args)
{
    int i;
    for(i=0; i<argc; ++i)
    {
        if (argv[i]== NULL)
            continue;
        if (!strcmp(argv[i], "-w"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->width = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-h"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->height = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-iw"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->input_width = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-ih"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->input_height = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-dw"))
        {
            ASSURE_NEXT_ARG(i, argc);
            divx3_width = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-dh"))
        {
            ASSURE_NEXT_ARG(i, argc);
            divx3_height = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-r"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->rotation = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-of"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->output_format = map_format_string(argv[++i]);
            if (args->output_format == -1)
            {
                printf("unknown color format: %s\n", argv[i]);
                return -1;
            }
        }
        else if (!strcmp(argv[i], "-if"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->input_format = map_format_string(argv[++i]);
            if (args->input_format == -1)
            {
                printf("unknown color format: %s\n", argv[i]);
                return -1;
            }
        }
        else if (!strcmp(argv[i], "-ib"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->buffer_size_in = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-ob"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->buffer_size_out = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-id"))
        {
            ASSURE_NEXT_ARG(i, argc);
            const char* arg = argv[++i];
            args->test_id = (char*)malloc(strlen(arg)+1);
            memset(args->test_id, 0, strlen(arg)+1);
            strcpy(args->test_id, arg);
        }
        else if (!strcmp(argv[i], "-i"))
        {
            ASSURE_NEXT_ARG(i, argc);
            const char* arg = argv[++i];
            args->input = (char*)malloc(strlen(arg)+1);
            memset(args->input, 0, strlen(arg)+1);
            strcpy(args->input, arg);
        }
        else if (!strcmp(argv[i], "-bi"))
        {
            ASSURE_NEXT_ARG(i, argc);
            const char* arg = argv[++i];
            args->blend_input = (char*)malloc(strlen(arg)+1);
            memset(args->blend_input, 0, strlen(arg)+1);
            strcpy(args->blend_input, arg);
        }
        else if (!strcmp(argv[i], "-rf"))
        {
            ASSURE_NEXT_ARG(i, argc);
            const char* arg = argv[++i];
            args->reference_file = (char*)malloc(strlen(arg)+1);
            memset(args->reference_file, 0, strlen(arg)+1);
            strcpy(args->reference_file, arg);
        }
        else if (!strcmp(argv[i], "-rd"))
        {
            ASSURE_NEXT_ARG(i, argc);
            const char* arg = argv[++i];
            args->reference_dir = (char*)malloc(strlen(arg)+1);
            memset(args->reference_dir, 0, strlen(arg)+1);
            strcpy(args->reference_dir, arg);
        }
        else if (!strcmp(argv[i], "-cx"))
        {
            // crop x offset
            ASSURE_NEXT_ARG(i, argc);
            args->crop_x = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-cy"))
        {
            // crop y offset
            ASSURE_NEXT_ARG(i, argc);
            args->crop_y = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-cw"))
        {
            // crop width
            ASSURE_NEXT_ARG(i, argc);
            args->crop_width = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-ch"))
        {
            // crop height
            ASSURE_NEXT_ARG(i, argc);
            args->crop_height = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-mx"))
        {
            // mask origin X
            ASSURE_NEXT_ARG(i, argc);
            args->mask1.originX = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-my"))
        {
            // mask origin Y
            ASSURE_NEXT_ARG(i, argc);
            args->mask1.originY = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-mw"))
        {
            // mask width
            ASSURE_NEXT_ARG(i, argc);
            args->mask1.width = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-mh"))
        {
            // mask height
            ASSURE_NEXT_ARG(i, argc);
            args->mask1.height = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-m2x"))
        {
            // mask origin X
            ASSURE_NEXT_ARG(i, argc);
            args->mask2.originX = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-m2y"))
        {
            // mask origin Y
            ASSURE_NEXT_ARG(i, argc);
            args->mask2.originY = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-m2w"))
        {
            // mask width
            ASSURE_NEXT_ARG(i, argc);
            args->mask2.width = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-m2h"))
        {
            // mask height
            ASSURE_NEXT_ARG(i, argc);
            args->mask2.height = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-fl"))
        {
            // frame limit
            ASSURE_NEXT_ARG(i, argc);
            args->limit = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-fv"))
        {
            args->mirroring = 1;
        }
        else if (!strcmp(argv[i], "-fh"))
        {
            args->mirroring = 2;
        }
        else if (!strcmp(argv[i], "-nc"))
        {
            args->no_compare = 1;
        }
        else if(!strcmp(argv[i], "-dith"))
        {
            args->dithering = 1;
        }
        else if(!strcmp(argv[i], "-debloc"))
        {
            args->deblocking = 1;
        }
        else if(!strcmp(argv[i], "-contra"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->rgb.contrast = atoi(argv[++i]);
        }
        else if(!strcmp(argv[i], "-brig"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->rgb.brightness = atoi(argv[++i]);
        }
        else if(!strcmp(argv[i], "-satur"))
        {
            ASSURE_NEXT_ARG(i, argc);
            args->rgb.saturation = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-h263"))
            args->coding_type = OMX_VIDEO_CodingH263;
        else if (!strcmp(argv[i], "-h264"))
            args->coding_type = OMX_VIDEO_CodingAVC;
        else if (!strcmp(argv[i], "-mpeg4"))
            args->coding_type = OMX_VIDEO_CodingMPEG4;
        else if (!strcmp(argv[i], "-wmv"))
            args->coding_type = OMX_VIDEO_CodingWMV;
        else if (!strcmp(argv[i], "-mpeg2"))
            args->coding_type = OMX_VIDEO_CodingMPEG2;
        else if (!strcmp(argv[i], "-sorenson"))
            args->coding_type = OMX_VIDEO_CodingSORENSON;
        else if (!strcmp(argv[i], "-custom_1"))
            args->coding_type = OMX_VIDEO_CodingCUSTOM_1;
        else if (!strcmp(argv[i], "-custom_1_3"))
            args->coding_type = OMX_VIDEO_CodingCUSTOM_1_3;
        else if (!strcmp(argv[i], "-rv"))
            args->coding_type = OMX_VIDEO_CodingRV;
        else if (!strcmp(argv[i], "-vp6"))
            args->coding_type = OMX_VIDEO_CodingCUSTOM_2;
        else if (!strcmp(argv[i], "-avs"))
            args->coding_type = OMX_VIDEO_CodingCUSTOM_3;
        else if (!strcmp(argv[i], "-vp8"))
            args->coding_type = OMX_VIDEO_CodingVP8;
        else if (!strcmp(argv[i], "-mjpeg"))
            args->coding_type = OMX_VIDEO_CodingMJPEG;
        else if (!strcmp(argv[i], "-jpeg"))
        {
            args->coding_type = OMX_IMAGE_CodingJPEG;
            args->domain = CODEC_IMAGE;
        }
        else if (!strcmp(argv[i], "-webp"))
        {
            args->coding_type = OMX_IMAGE_CodingWEBP;
            args->domain = CODEC_IMAGE;
        }
        else if (!strcmp(argv[i], "-none"))
            args->coding_type = OMX_VIDEO_CodingUnused;
        else if (!strcmp(argv[i], "-fail"))
            args->fail = 1;
        else if (!strcmp(argv[i], "-v"))
            VERBOSE_OUTPUT = OMX_TRUE;
    else
        {
            printf("unknown argument: %s\n", argv[i]);
            return 0;
        }
    }

    return 1;
}

int parse_argument_line(const char* line, size_t len, TESTARGS* args)
{
    char* argv[100];
    int argc = 0;
    memset(argv, 0, sizeof(argv));

    const char* start = line;
    size_t tokenlen   = 0;

    size_t i;
    for (i=0; i<len; ++i)
    {
        if (line[i] == ' ')
        {
            if (tokenlen)
            {
                char* str = (char*)malloc(tokenlen+1);
                memset(str, 0, tokenlen+1);
                strncpy(str, start, tokenlen);
                argv[argc++] = str;
            }
            tokenlen = 0;
            start = &line[i+1];
        }
        else
        {
            ++tokenlen;
        }
    }
    if (tokenlen)
    {
        char* str = (char*)malloc(tokenlen+1);
        memset(str, 0, tokenlen+1);
        strncpy(str, start, tokenlen);
        argv[argc++] = str;
    }

    int ret = parse_argument_vector((const char**)argv, argc, args);

    for (i=0; i<sizeof(argv)/sizeof(const char*); ++i)
        if (argv[i])
            free(argv[i]);

    return ret;
}

int parse_script_var(const char* line, size_t len, SCRIPTVAR* var)
{
    assert(line[0] == '!');
    line++;
    len--;
    size_t i;
    for (i=0; i<len; ++i)
    {
        if (line[i] == '=')
            break;
    }
    var->name = (char*)malloc(i + 1);
    memset(var->name, 0, i + 1);
    strncat(var->name, line, i);
    ++i; // skip the '='
    line += i;
    len  -= i;
    var->value = (char*)malloc(len + 1);
    memset(var->value, 0, len + 1);
    strncat(var->value, line, len);
    return 1;
}

int set_variable(SCRIPTVAR** array, size_t arraysize, SCRIPTVAR* var)
{
    assert(var);
    assert(var->name && var->value);
    size_t i=0;
    for (i=0; i<arraysize; ++i)
    {
        const SCRIPTVAR* old = array[i];
        if (old == NULL)
            break;
        if (strcmp(old->name, var->name)==0)
            break;
    }
    if (!(i < arraysize))
        return 0;

    if (array[i] == NULL)
    {
        array[i] = var;
    }
    else
    {
        assert(strcmp(array[i]->name, var->name)==0);
        free(array[i]->value);
        array[i]->value = var->value;
        free(var->name);
        free(var);
    }
    return 0;
}

char* expand_variable(SCRIPTVAR** array, size_t arraysize, const char* str)
{
    char* expstr = NULL;
    size_t i=0;
    for (i=0; i<arraysize; ++i)
    {
        const SCRIPTVAR* var = array[i];
        if (var)
        {
            char* needle = (char*)malloc(strlen(var->name) + 4);
            memset(needle, 0, strlen(var->name)+4);
            strcat(needle, "$(");
            strcat(needle, var->name);
            strcat(needle, ")");
            char* ret = strstr(str, needle);
            if (ret)
            {
                int len = strlen(var->value) + strlen(str) - strlen(needle);
                expstr  = (char*)malloc(len + 1);
                memset(expstr, 0, len+1);
                strncat(expstr, str, ret-str);
                strcat(expstr, var->value);
                strcat(expstr, ret+strlen(needle));
                break;
            }
        }
    }
    return expstr;
}

char* find_variable(SCRIPTVAR** array, size_t arraysize, const char* name)
{
    size_t i=0;
    for (i=0; i<arraysize; ++i)
    {
        const SCRIPTVAR* var = array[i];
        if (var)
        {
            if (strcmp(name, var->name)==0)
                return var->value;
        }
    }
    return NULL;
}

typedef enum TEST_RESULT { TEST_FAIL, TEST_PASS, TEST_UNDECIDED } TEST_RESULT;

TEST_RESULT execute_test(const TESTARGS* test)
{
    VIDEOPROPS props;
    RCVSTATE rcv;
    memset(&props, 0, sizeof(props));
    memset(&rcv, 0, sizeof(rcv));

    if (test->test_id)
        printf("RUNNING: %s\n", test->test_id);

    const char* output = test->reference_file ? "temp.yuv" : NULL;
    read_func   func;

    if (test->domain == CODEC_VIDEO)
    {
        if (test->coding_type == OMX_VIDEO_CodingVP8)
        {
            formatCheck = 0;
            func = read_vp8_file;
        }
        else if (test->coding_type == OMX_VIDEO_CodingAVC)
        {
            func = read_h264_file;
        }
        else if (test->coding_type == OMX_VIDEO_CodingCUSTOM_3)
        {
            traceUsedStream = 0;
            previousUsed = 0;
            //func = read_any_file;
            func = read_avs_file;
        }
        else if (test->coding_type == OMX_VIDEO_CodingSORENSON)
        {
            traceUsedStream = 0;
            previousUsed = 0;
            func = read_sorenson_file;
        }
        else if (test->coding_type == OMX_VIDEO_CodingCUSTOM_2)
        {
            func = read_vp6_file;
        }
        else if (test->coding_type == OMX_VIDEO_CodingCUSTOM_1)
        {
            traceUsedStream = 0;
            previousUsed = 0;
            func = read_mpeg4_file;
        }
        else if (test->coding_type == OMX_VIDEO_CodingCUSTOM_1_3)
        {
            start_custom_1_3 = 0;
            offset = 0;
            func = read_custom_1_3_file;
        }
        else if (test->coding_type == OMX_VIDEO_CodingRV)
        {
            func = read_rv_file;
            rv_offset = 0;
            start_rv = 0;
            rawRVFile = OMX_FALSE;
            rawRV8 = OMX_FALSE;
        }
        else
        {
            func   = test->coding_type == OMX_VIDEO_CodingWMV ? read_rcv_file : read_any_file;
        }
    }
    else
    {
        if (test->coding_type == OMX_IMAGE_CodingJPEG)
        {
            func = read_any_file;
        }
        else if(test->coding_type == OMX_IMAGE_CodingWEBP)
        {
            func = read_webp_file;
        }
    }

    if (test->no_compare)
        output = "temp.yuv";

    TEST_RESULT res = TEST_FAIL;
    if (decode_file(test->input, output, &props, test, func, &rcv))
    {
        if (output && !test->no_compare)
        {
            res = TEST_FAIL;
            // if there's a single file output compare that
            char output[256];
            memset(output, 0, sizeof(output));
            strcat(output, test->reference_dir);
            strcat(output, "/");
            strcat(output, test->reference_file);
            if (compare_output(output, "temp.yuv"))
            {
                res = TEST_PASS;
                printf("PASS: %s\n", test->test_id);
            }


            if (res == TEST_FAIL && test->test_id)
            {
                memset(output, 0, sizeof(output));
                sprintf(output, "mv temp.yuv %s.fail.%s", test->test_id, format_to_file_ext(test->output_format));
                system(output);
                fprintf(stderr, "FAILED: %s stride:%d sliceh:%d\n", test->test_id, props.stride, props.sliceheight);
            }
        }
        else if (test->reference_dir && !test->no_compare && res != TEST_FAIL)
        {
            res = TEST_FAIL;
            // otherwise compare the frames we got to the
            // frames in the reference directory.
            char frame[255];
            char output[255];
            assert(props.framecount);
            int i=0;
            for (i=0; i<props.framecount; ++i)
            {
                memset(frame, 0, sizeof(frame));
                memset(output, 0, sizeof(output));
                sprintf(frame, "frame%d.yuv", i);
                sprintf(output, "%s/out_%d.yuv", test->reference_dir, i);
                if (!compare_output(output, frame))
                    break;
            }
            if (i==props.framecount) res = TEST_PASS;

            if (res == TEST_FAIL)
            {
                memset(frame, 0, sizeof(frame));
                sprintf(frame, "mv frame%d.yuv %s.frame%d.fail.%s", i, test->test_id, i, format_to_file_ext(test->output_format));
                system(frame);
                fprintf(stderr, "FAILED: %s stride:%d sliceh:%d\n", test->test_id, props.stride, props.sliceheight);
            }
        }
    }
    else
    {
        if (test->test_id)
            fprintf(stderr, "FAILED: %s\n", test->test_id);
    }
    return res;
}


int main(int argc, const char* argv[])
{
    printf("OMX component test video decoder\n");
    printf(__DATE__", "__TIME__"\n");

    OMX_ERRORTYPE err = OSAL_MutexCreate(&queue_mutex);
    if (err != OMX_ErrorNone)
    {
        printf("mutex create error: %s\n", HantroOmx_str_omx_err(err));
        return 1;
    }

    TESTARGS test;
    VIDEOPROPS props;
    RCVSTATE   rcv;
    memset(&test, 0, sizeof(test));
    memset(&props, 0, sizeof(props));
    memset(&rcv, 0, sizeof(rcv));
    int test_count = 0;
    int pass_count = 0;
    int fail_count = 0;
    int undc_count = 0;

    if (argc > 2)
    {
        //VERBOSE_OUTPUT = OMX_TRUE;
        argv[0] = NULL;
        test.buffer_size_in  = DEFAULT_BUFFER_SIZE_INPUT;
        test.buffer_size_out = DEFAULT_BUFFER_SIZE_OUTPUT;
        test.rgb.alpha       = 0xFF;
        if (parse_argument_vector(argv, argc, &test))
        {
            ++test_count;
            switch (execute_test(&test))
            {
                case TEST_PASS:      ++pass_count; break;
                case TEST_FAIL:      ++fail_count; break;
                case TEST_UNDECIDED: ++undc_count; break;
            }
            if (test.input) free(test.input);
            if (test.reference_file) free(test.reference_file);
            if (test.reference_dir) free(test.reference_dir);
            if (test.test_id) free(test.test_id);
        }
        else
        {
            printf("unknown arguments\n");
            return 1;
        }
    }
    else
    {
        FILE* strm = fopen("test_script.txt", "r");
        if (!strm)
        {
            printf("failed to open test_script.txt\n");
            OSAL_MutexDestroy(queue_mutex);
            return 1;
        }

        char* line = NULL;
        size_t linelen = 0;
        SCRIPTVAR* svars[100]; // TODO:bounds checking below
        memset(svars, 0, sizeof(svars));

        while (getline(&line, &linelen, strm) != -1)
        {
            assert(line);
            if (*line == '#' || *line == '\n')
                continue;

            linelen = strlen(line);
            if (line[linelen-1] == '\n')
                --linelen;

            if (line[0] == '!')
            {
                SCRIPTVAR* var = (SCRIPTVAR*)malloc(sizeof(SCRIPTVAR));
                parse_script_var(line, linelen, var);
                set_variable(svars, sizeof(svars)/sizeof(SCRIPTVAR*), var);
            }
            else
            {
                memset(&test, 0, sizeof(TESTARGS));

                // set some hardcoded defaults
                test.buffer_size_in  = DEFAULT_BUFFER_SIZE_INPUT;
                test.buffer_size_out = DEFAULT_BUFFER_SIZE_OUTPUT;
                test.rgb.alpha       = 0xFF;
                if (parse_argument_line(line, linelen, &test) != -1)
                {
                    char* exp = NULL;
                    exp = expand_variable(svars, sizeof(svars)/sizeof(SCRIPTVAR*), test.input);
                    if (exp)
                    {
                        free(test.input);
                        test.input = exp;
                    }
                    exp = expand_variable(svars, sizeof(svars)/sizeof(SCRIPTVAR*), test.reference_dir);
                    if (exp)
                    {
                        free(test.reference_dir);
                        test.reference_dir = exp;
                    }
                    if (test.blend_input)
                    {
                        exp = expand_variable(svars, sizeof(svars)/sizeof(SCRIPTVAR*), test.blend_input);
                        if (exp)
                        {
                            free(test.blend_input);
                            test.blend_input = exp;
                        }
                    }
                    ++test_count;
                    switch (execute_test(&test))
                    {
                        case TEST_PASS:
                            {
                                ++pass_count;
                            }
                            break;
                        case TEST_FAIL:
                            {
                                ++fail_count;
                                if (test.test_id)
                                    fprintf(stderr, "FAILED: %s\n", test.test_id);
                            }
                            break;
                        case TEST_UNDECIDED: ++undc_count; break;
                    }
                }
                else
                {
                    printf("unknown argument line:%s\n", line);
                    printf("skipping...\n");
                }
                if (test.input) free(test.input);
                if (test.reference_file) free(test.reference_file);
                if (test.reference_dir) free(test.reference_dir);
                if (test.test_id) free(test.test_id);
            }
            free(line);
            line = NULL;
            linelen = 0;
        }
        fclose(strm);
        size_t i;
        for (i=0; i<sizeof(svars)/sizeof(SCRIPTVAR*); ++i)
        {
            if (svars[i])
            {
                if (svars[i]->name)  free(svars[i]->name);
                if (svars[i]->value) free(svars[i]->value);
                free(svars[i]);
            }
        }
    }


    printf("\nResult summary:\n"
           "Tests run:\t%d\n"
           "Tests pass:\t%d\n"
           "Tests fail:\t%d\n"
           "Tests else:\t%d\n",
           test_count,
           pass_count,
           fail_count,
           undc_count);
    if (pass_count != test_count - undc_count)
    {
        printf("*** FAIL ***\n");
    }

    OSAL_MutexDestroy(queue_mutex);

    return 0;
}

void startRealFileReader(FILE * pFile, OMX_U32** pBuffer, OMX_U32* pAllocLen, OMX_BOOL* eof)
{
    int rc;

    buffering_data.pFile = pFile;
    buffering_data.pBuffer = pBuffer;
    buffering_data.pAllocLen = pAllocLen;
    buffering_data.eof = eof;

    rc = pthread_create(&threads[0], NULL, (void *)rv_display, (void *)&buffering_data);
}

/*------------------------------------------------------------------------------

    Function name:  PPTrace

    Purpose:
        Example implementation of PPTrace function. Prototype of this
        function is given in ppapi.h. This implementation appends
        trace messages to file named 'pp_api.trc'.

------------------------------------------------------------------------------*/
void PPTrace(const char *string)
{
    printf("%s", string);
}
