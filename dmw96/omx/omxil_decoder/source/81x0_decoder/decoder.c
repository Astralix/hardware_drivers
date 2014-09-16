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
--  Description :  OMX Decoder component.
--
------------------------------------------------------------------------------*/

//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_G1_decoder_component"
#include <utils/Log.h>
#define TRACE_PRINT(args...) LOGV(args)

#include <OSAL.h>
#include <basecomp.h>
#include <port.h>
#include <util.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "version.h"
#include "codec.h"
#include "codec_h264.h"
#include "codec_jpeg.h"
#include "codec_mpeg4.h"
#include "codec_vc1.h"
#include "codec_rv.h"
#include "codec_mpeg2.h"
#include "codec_vp6.h"
#include "codec_avs.h"
#include "codec_vp8.h"
#include "codec_webp.h"


#ifdef DYNAMIC_SCALING
  #define ALIGN_AND_DECREASE_OUTPUT 1
#endif

#ifdef ALIGN_AND_DECREASE_OUTPUT
  #include <linux/fb.h>
#endif

#ifdef ANDROID /*To be use by opencore multimedia framework*/
#define PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX 0xFF7A347
#endif

#ifdef INCLUDE_TB
#include "tb_cfg.h"
#include "tb_defs.h"
TBCfg tbCfg;
int bFrames = 0;
#endif

#include "codec_pp.h"

#define PORT_INDEX_INPUT     0
#define PORT_INDEX_OUTPUT    1
#define PORT_INDEX_PP        2
#define RETRY_INTERVAL       50
#define TIMEOUT              2000
#define MAX_RETRIES          10000

#define UNUSED_PARAMETER(p) (void)(p)

typedef struct TIMESTAMP_BUFFER
{
    OMX_TICKS *ts_data;
    OMX_U32 capacity;        // buffer capacity
    OMX_U32 count;            // how many count is in the buffer currently
} TIMESTAMP_BUFFER;

#define GET_DECODER(comp) (OMX_DECODER*)(((OMX_COMPONENTTYPE*)comp)->pComponentPrivate)

typedef struct FRAME_BUFFER
{
    OSAL_BUS_WIDTH bus_address;
    OMX_U8 *bus_data;
    OMX_U32 capacity;        // buffer size
    OMX_U32 size;            // how many bytes is in the buffer currently
} FRAME_BUFFER;

#define FRAME_BUFF_CAPACITY(fb) (fb)->capacity - (fb)->size

#define FRAME_BUFF_FREE(alloc, fb)                                  \
  OSAL_AllocatorFreeMem((alloc), (fb)->capacity, (fb)->bus_data, (fb)->bus_address)

typedef struct OMX_DECODER
{
    OMX_U8 privatetable[256]; // Bellagio has room to store it's own privates here
    BASECOMP base;
    volatile OMX_STATETYPE state;
    volatile OMX_STATETYPE statetrans;
    volatile OMX_BOOL run;
    OMX_U32 priority_group;
    OMX_U32 priority_id;
    OMX_BOOL deblock;
    OMX_CALLBACKTYPE callbacks;
    OMX_PTR appdata;
    BUFFER *buffer;          // outgoing buffer
    PORT in;                 // regular input port
    PORT inpp;               // post processor input port
    PORT out;                // output port
    OMX_HANDLETYPE self;     // handle to "this" component
    FILE *log;
    OSAL_ALLOCATOR alloc;
    CODEC_PROTOTYPE *codec;
    CODEC_STATE codecstate;
    FRAME_BUFFER frame_in;   // temporary input frame buffer
    FRAME_BUFFER frame_out;  // temporary output frame buffer
    FRAME_BUFFER mask;
    TIMESTAMP_BUFFER ts_buf;
    OMX_U8 role[128];
    OMX_CONFIG_ROTATIONTYPE conf_rotation;
    OMX_CONFIG_MIRRORTYPE conf_mirror;
    OMX_CONFIG_CONTRASTTYPE conf_contrast;
    OMX_CONFIG_BRIGHTNESSTYPE conf_brightness;
    OMX_CONFIG_SATURATIONTYPE conf_saturation;
    OMX_CONFIG_PLANEBLENDTYPE conf_blend;
    OMX_CONFIG_RECTTYPE conf_rect;
    OMX_CONFIG_POINTTYPE conf_mask_offset;
    OMX_CONFIG_RECTTYPE conf_mask;
    OMX_CONFIG_DITHERTYPE conf_dither;
    OMX_MARKTYPE marks[10];
    OMX_U32 mark_read_pos;
    OMX_U32 mark_write_pos;
    OMX_U32 imageSize;
    OMX_BOOL dispatchOnlyFrame;
    OMX_BOOL bIsRV8;
    OMX_U32 width;
    OMX_U32 height;
    OMX_U32 sliceInfoNum; /* real slice data */
    OMX_U8 *pSliceInfo; /* real slice data */
    OMX_BOOL portReconfigPending;
    OMX_VIDEO_WMVFORMATTYPE WMVFormat;
#ifdef ENABLE_CODEC_VP8
    OMX_BOOL isIntra;
    OMX_BOOL isGoldenOrAlternate;
#endif
#ifdef OMX_DECODER_IMAGE_DOMAIN
#ifdef CONFORMANCE
    OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE quant_table;
    OMX_IMAGE_PARAM_HUFFMANTTABLETYPE huffman_table;
#endif
#endif


#ifdef DYNAMIC_SCALING
	/* This will be the file descriptor of the fifo that will be
	   used by the renderer to pass us the desired scaling dimensions */
	int scalerFd;
#endif


#ifdef ALIGN_AND_DECREASE_OUTPUT
	/* We need to save the original movie dimensions to know what dimensions we can scale to
	   For example, we cannot scale down one dimension and scale up the other. */
	OMX_U32 originalWidth, originalHeight;

    /* This will hold the current screen size (of fb0)
     * We will initially scale the video to full screen based on the current screen size */
    OMX_U32 screenWidth, screenHeight;
#endif

#ifdef DSPG_TIMESTAMP_FIX
  #define TIMESTAMPS_QUEUE_SIZE 128
    OMX_TICKS timestampsQueue[TIMESTAMPS_QUEUE_SIZE];
    OMX_U32 timestampsQueueHead;
    OMX_U32 timestampsQueueTail;
#endif

#ifdef ANDROID_MOD
    OMX_BOOL outputPortFlushPending;
    OMX_BOOL dispatchOutputImmediately;
#endif

#ifdef DSPG_DUMP_STREAM
    int streamDumpFd;
#endif

} OMX_DECODER;

#define MAX_TICK_COUNTS 64

#ifdef OMX_SKIP64BIT
#define EARLIER(t1, t2) (((t1).nHighPart < (t2).nHighPart) || (((t1).nHighPart == (t2).nHighPart) && ((t1).nLowPart < (t2).nLowPart)))
#else
#define EARLIER(t1, t2) ((t1) < (t2))
#endif

static OMX_ERRORTYPE grow_timestamp_buffer(OMX_DECODER * dec, TIMESTAMP_BUFFER * fb,
                                    OMX_U32 capacity)
{
    CALLSTACK;
    assert(capacity >= fb->count);

    TIMESTAMP_BUFFER new_ts;

    memset(&new_ts, 0, sizeof(TIMESTAMP_BUFFER));
    new_ts.ts_data = malloc(capacity*sizeof(OMX_TICKS));

    memcpy(new_ts.ts_data, fb->ts_data, fb->count*sizeof(OMX_TICKS));
    new_ts.capacity = capacity;
    new_ts.count = fb->count;
    free(fb->ts_data);

    *fb = new_ts;
    return OMX_ErrorNone;
}

static void receive_timestamp(OMX_DECODER * dec, OMX_TICKS *timestamp)
{
    int i;
    TIMESTAMP_BUFFER *temp = &dec->ts_buf;

    if(temp->count >= temp->capacity)
    {
        OMX_U32 capacity = temp->capacity + 16;
        grow_timestamp_buffer(dec, temp, capacity);
    }

    for (i = 0; (OMX_U32)i < dec->ts_buf.count; i++)
    {
        if (EARLIER(*timestamp, dec->ts_buf.ts_data[i]))
        {
            break;
        }
    }

    if ((OMX_U32)i < dec->ts_buf.count)
    {
        memmove(&dec->ts_buf.ts_data[i+1], &dec->ts_buf.ts_data[i], sizeof(OMX_TICKS)*(dec->ts_buf.count-i));
    }

    dec->ts_buf.count++;

    //ts_queue.ticks[i] = timestamp;
    memcpy(&dec->ts_buf.ts_data[i], timestamp, sizeof(OMX_TICKS));
    /*printf("received timestamp %ld count %d\n", timestamp->nLowPart, dec->ts_buf.count);*/
}

static void pop_timestamp(OMX_DECODER * dec, OMX_TICKS *timestamp)
{
    if (dec->ts_buf.count == 0)
        return;

    if (timestamp != NULL)
    {
        memcpy(timestamp, &dec->ts_buf.ts_data[0], sizeof(OMX_TICKS));
    }

    if (dec->ts_buf.count > 1)
    {
        memmove(&dec->ts_buf.ts_data[0], &dec->ts_buf.ts_data[1], sizeof(OMX_TICKS)*(dec->ts_buf.count-1));
    }

    dec->ts_buf.count--;
    /*printf("Pop timestamp %ld count %d\n", timestamp->nLowPart, dec->ts_buf.count);*/
}

static OMX_ERRORTYPE async_decoder_set_state(OMX_COMMANDTYPE, OMX_U32, OMX_PTR,
                                             OMX_PTR);
static OMX_ERRORTYPE async_decoder_disable_port(OMX_COMMANDTYPE, OMX_U32,
                                                OMX_PTR, OMX_PTR);
static OMX_ERRORTYPE async_decoder_enable_port(OMX_COMMANDTYPE, OMX_U32,
                                               OMX_PTR, OMX_PTR);
static OMX_ERRORTYPE async_decoder_flush_port(OMX_COMMANDTYPE, OMX_U32, OMX_PTR,
                                              OMX_PTR);
static OMX_ERRORTYPE async_decoder_mark_buffer(OMX_COMMANDTYPE, OMX_U32,
                                               OMX_PTR, OMX_PTR);
static OMX_ERRORTYPE async_decoder_decode(OMX_DECODER * dec);
static OMX_ERRORTYPE async_decoder_set_mask(OMX_DECODER * dec);
static OMX_ERRORTYPE async_get_frame_buffer(OMX_DECODER * dec, FRAME * frm);

#ifdef DYNAMIC_SCALING
static OMX_ERRORTYPE async_set_new_pp_args(OMX_DECODER *dec, OMX_U32 newWidth, OMX_U32 newHeight);
#endif

static OMX_U32 decoder_thread_main(BASECOMP * base, OMX_PTR arg)
{
    CALLSTACK;

    assert(arg);
    OMX_DECODER *this = (OMX_DECODER *) arg;

    OMX_HANDLETYPE handles[3];

    handles[0] = this->base.queue.event;    // event handle for the command queue
    handles[1] = this->in.bufferevent;  // event handle for the normal input port input buffer queue
    handles[2] = this->inpp.bufferevent;    // event handle for the post-processor port input buffer queue

    OMX_ERRORTYPE err = OMX_ErrorNone;

    OMX_BOOL timeout = OMX_FALSE;

    OMX_BOOL signals[3];

    while(this->run)
    {
        // clear all signal indicators
        signals[0] = OMX_FALSE;
        signals[1] = OMX_FALSE;
        signals[2] = OMX_FALSE;

        // wait for command messages and buffers
        err =
            OSAL_EventWaitMultiple(handles, (OSAL_BOOL *)signals, 3, INFINITE_WAIT,
                                   (OSAL_BOOL*)&timeout);
        if(err != OMX_ErrorNone)
        {
            TRACE_PRINT("ASYNC: waiting for events failed: %s\n",
                        HantroOmx_str_omx_err(err));
            break;
        }
        if(signals[0] == OMX_TRUE)
        {
            CMD cmd;

            while(1)
            {
                OMX_BOOL ok = OMX_TRUE;

                err =
                    HantroOmx_basecomp_try_recv_command(&this->base, &cmd, &ok);
				TRACE_PRINT("ASYNC: command thread received command function = %p", cmd.arg.fun);
                if(err != OMX_ErrorNone)
                {
                    TRACE_PRINT("ASYNC: basecomp_try_recv_command failed: %s\n",
                                HantroOmx_str_omx_err(err));
                    this->run = OMX_FALSE;
                    break;
                }
                if(ok == OMX_FALSE)
                    break;
                if(cmd.type == CMD_EXIT_LOOP)
                {
                    TRACE_PRINT("ASYNC: got CMD_EXIT_LOOP\n");
                    this->run = OMX_FALSE;
                    break;
                }
				TRACE_PRINT("ASYNC: command thread dispatch command function = %p", cmd.arg.fun);
                HantroOmx_cmd_dispatch(&cmd, this);
            }
        }
        if(signals[1] == OMX_TRUE || signals[2] == OMX_TRUE)
        {
            if(signals[2] == OMX_TRUE && this->state == OMX_StateExecuting)
                // got a new input buffer for the post processor!
                async_decoder_set_mask(this);
#ifdef HANTRO_TESTBENCH
            if(signals[1] == OMX_TRUE && this->state == OMX_StateExecuting)
#else
            if(signals[1] == OMX_TRUE && this->state == OMX_StateExecuting &&
                !this->portReconfigPending)
#endif
                // got a new input buffer, process it
                async_decoder_decode(this);
        }
    }
    if(err != OMX_ErrorNone)
    {
        TRACE_PRINT("ASYNC: new state: %s\n",
                    HantroOmx_str_omx_state(OMX_StateInvalid));
        this->state = OMX_StateInvalid;
        this->callbacks.EventHandler(this->self, this->appdata, OMX_EventError,
                                     OMX_ErrorInvalidState, 0, NULL);
    }
    TRACE_PRINT("ASYNC: thread exit\n");
    return 0;
}

static
    OMX_ERRORTYPE decoder_get_version(OMX_IN OMX_HANDLETYPE hComponent,
                                      OMX_OUT OMX_STRING pComponentName,
                                      OMX_OUT OMX_VERSIONTYPE *
                                      pComponentVersion,
                                      OMX_OUT OMX_VERSIONTYPE * pSpecVersion,
                                      OMX_OUT OMX_UUIDTYPE * pComponentUUID)
{
    CALLSTACK;

    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pComponentName);
    CHECK_PARAM_NON_NULL(pComponentVersion);
    CHECK_PARAM_NON_NULL(pSpecVersion);
    CHECK_PARAM_NON_NULL(pComponentUUID);

    //OMX_DECODER *dec = GET_DECODER(hComponent);

    TRACE_PRINT("API: %s\n", __FUNCTION__);

#ifdef OMX_DECODER_VIDEO_DOMAIN
    strncpy(pComponentName, COMPONENT_NAME_VIDEO, OMX_MAX_STRINGNAME_SIZE - 1);
#else
    strncpy(pComponentName, COMPONENT_NAME_IMAGE, OMX_MAX_STRINGNAME_SIZE - 1);
#endif

    pComponentVersion->s.nVersionMajor = COMPONENT_VERSION_MAJOR;
    pComponentVersion->s.nVersionMinor = COMPONENT_VERSION_MINOR;
    pComponentVersion->s.nRevision = COMPONENT_VERSION_REVISION;
    pComponentVersion->s.nStep = COMPONENT_VERSION_STEP;

    pSpecVersion->s.nVersionMajor = 1;  // this is the OpenMAX IL version. Has nothing to do with component version.
    pSpecVersion->s.nVersionMinor = 1;
    pSpecVersion->s.nRevision = 2;
    pSpecVersion->s.nStep = 0;

    HantroOmx_generate_uuid(hComponent, pComponentUUID);
    return OMX_ErrorNone;
}

static void decoder_dealloc_buffers(OMX_DECODER * dec, PORT * p)
{
    CALLSTACK;
    TRACE_PRINT("API: %s\n", __FUNCTION__);

    assert(p);
    OMX_U32 count = HantroOmx_port_buffer_count(p);

    OMX_U32 i;

    for(i = 0; i < count; ++i)
    {
        BUFFER *buff = NULL;

        HantroOmx_port_get_allocated_buffer_at(p, &buff, i);
        assert(buff);
        if(buff->flags & BUFFER_FLAG_MY_BUFFER)
        {
            assert(buff->bus_address);
            assert(buff->bus_data);
            OSAL_AllocatorFreeMem(&dec->alloc, buff->allocsize, buff->bus_data,
                                  buff->bus_address);
        }
    }
}

static OMX_ERRORTYPE decoder_deinit(OMX_IN OMX_HANDLETYPE hComponent)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);

    if(((OMX_COMPONENTTYPE *) hComponent)->pComponentPrivate == NULL)
        // play nice and handle destroying a component that was never created nicely...
        return OMX_ErrorNone;

    OMX_DECODER *dec = GET_DECODER(hComponent);

    TRACE_PRINT("API: %s\n", __FUNCTION__);
    TRACE_PRINT("API: waiting for thread to finish\n");
    TRACE_PRINT("API: current state: %s\n",
                HantroOmx_str_omx_state(dec->state));

    if(dec->base.thread)
    {
        // bring down the component thread, and join it
        dec->run = OMX_FALSE;
        CMD c;

        INIT_EXIT_CMD(c);
        HantroOmx_basecomp_send_command(&dec->base, &c);
        OSAL_ThreadSleep(RETRY_INTERVAL);
        HantroOmx_basecomp_destroy(&dec->base);
    }

    assert(HantroOmx_port_is_allocated(&dec->in) == OMX_TRUE);
    assert(HantroOmx_port_is_allocated(&dec->out) == OMX_TRUE);
    assert(HantroOmx_port_is_allocated(&dec->inpp) == OMX_TRUE);

    //if (dec->state == OMX_StateInvalid)
    if(dec->state != OMX_StateLoaded)
    {
        // if there's stuff in the input/output port buffer queues
        // simply ignore it

        // deallocate allocated buffers (if any)
        // this could have catastrophic consequences if someone somewhere is
        // is still holding a pointer to these buffers... (what could be done here, except leak?)

        decoder_dealloc_buffers(dec, &dec->in);
        decoder_dealloc_buffers(dec, &dec->out);
        decoder_dealloc_buffers(dec, &dec->inpp);
        TRACE_PRINT("API: delloc buffers done\n");
        if(dec->codec)
            dec->codec->destroy(dec->codec);

        TRACE_PRINT("API: dealloc codec done\n");

        if(dec->frame_in.bus_address)
            FRAME_BUFF_FREE(&dec->alloc, &dec->frame_in);
        if(dec->frame_out.bus_address)
            FRAME_BUFF_FREE(&dec->alloc, &dec->frame_out);

        // free time stamp buffer queue.
        if(dec->ts_buf.ts_data)
            free(dec->ts_buf.ts_data);

        TRACE_PRINT("API: dealloc frame buffers done\n");
    }
    else
    {
        // ports should not have any queued buffers at this point anymore.
        // if there are this is a programming error somewhere.
        assert(HantroOmx_port_buffer_queue_count(&dec->in) == 0);
        assert(HantroOmx_port_buffer_queue_count(&dec->out) == 0);
        assert(HantroOmx_port_buffer_queue_count(&dec->inpp) == 0);

        // ports should not have any buffers allocated
        // at this point anymore
        assert(HantroOmx_port_has_buffers(&dec->in) == OMX_FALSE);
        assert(HantroOmx_port_has_buffers(&dec->out) == OMX_FALSE);
        assert(HantroOmx_port_has_buffers(&dec->inpp) == OMX_FALSE);

        // temporary frame buffers should not exist anymore
        assert(dec->frame_in.bus_data == NULL);
        assert(dec->frame_out.bus_data == NULL);
        assert(dec->ts_buf.ts_data== NULL);
        assert(dec->mask.bus_data == NULL);
    }
    HantroOmx_port_destroy(&dec->in);
    HantroOmx_port_destroy(&dec->out);
    HantroOmx_port_destroy(&dec->inpp);

#if 0 // OMX_DECODER_TRACE
    if(dec->log && dec->log != stdout)
    {
        int ret = fclose(dec->log);

        assert(ret == 0);
    }
#endif

#ifdef DYNAMIC_SCALING
	close(dec->scalerFd);
	unlink("/tmp/decoder-scaler");
#endif

#ifdef DSPG_DUMP_STREAM
    if (dec->streamDumpFd > 0) close(dec->streamDumpFd);
#endif

    OSAL_AllocatorDestroy(&dec->alloc);
    OSAL_Free(dec);
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE decoder_send_command(OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_IN OMX_COMMANDTYPE Cmd,
                                       OMX_IN OMX_U32 nParam1,
                                       OMX_IN OMX_PTR pCmdData)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    CHECK_STATE_INVALID(dec->state);

    TRACE_PRINT("API: %s\n", __FUNCTION__);
    TRACE_PRINT("API: Cmd:%s nParam1:%u pCmdData:%p\n",
                HantroOmx_str_omx_cmd(Cmd), (unsigned) nParam1, pCmdData);

    CMD c;

    OMX_ERRORTYPE err = OMX_ErrorNotImplemented;

    switch (Cmd)
    {
    case OMX_CommandStateSet:
        if(dec->statetrans != dec->state)
        {
            // transition is already pending
            LOGE("%s: OMX_CommandStateSet: %d != %d (transition already pending)", __func__, dec->statetrans, dec->state);
            return OMX_ErrorIncorrectStateTransition;
        }
        TRACE_PRINT("API: next state:%s\n",
                    HantroOmx_str_omx_state((OMX_STATETYPE) nParam1));
        INIT_SEND_CMD(c, Cmd, nParam1, NULL, async_decoder_set_state);
        dec->statetrans = (OMX_STATETYPE) nParam1;
        err = HantroOmx_basecomp_send_command(&dec->base, &c);
        break;

    case OMX_CommandFlush:
        if(nParam1 > PORT_INDEX_PP && (OMX_S32)nParam1 != -1)
        {
            TRACE_PRINT("API: bad port index:%u\n",
                        (unsigned) nParam1);
            //return OMX_ErrorBadParameter;
            return OMX_ErrorBadPortIndex;
        }
#ifdef ANDROID_MOD
        if (nParam1 == PORT_INDEX_OUTPUT) {
            dec->outputPortFlushPending = OMX_TRUE;
        }
#endif
        INIT_SEND_CMD(c, Cmd, nParam1, NULL, async_decoder_flush_port);
        err = HantroOmx_basecomp_send_command(&dec->base, &c);
        break;

    case OMX_CommandPortDisable:
        if(nParam1 > PORT_INDEX_PP && (OMX_S32)nParam1 != -1)
        {
            TRACE_PRINT("API: bad port index:%u\n",
                        (unsigned) nParam1);
            return OMX_ErrorBadPortIndex;
        }
        if(nParam1 == PORT_INDEX_INPUT || (OMX_S32)nParam1 == -1)
            dec->in.def.bEnabled = OMX_FALSE;
        if(nParam1 == PORT_INDEX_OUTPUT || (OMX_S32)nParam1 == -1)
            dec->out.def.bEnabled = OMX_FALSE;
        if(nParam1 == PORT_INDEX_PP || (OMX_S32)nParam1 == -1)
            dec->inpp.def.bEnabled = OMX_FALSE;

        INIT_SEND_CMD(c, Cmd, nParam1, NULL, async_decoder_disable_port);
        err = HantroOmx_basecomp_send_command(&dec->base, &c);
        break;

    case OMX_CommandPortEnable:
        TRACE_PRINT("OMX_CommandPortEnable\n");
        if(nParam1 > PORT_INDEX_PP && (OMX_S32)nParam1 != -1)
        {
            TRACE_PRINT("API: bad port index:%u\n",
                        (unsigned) nParam1);
            return OMX_ErrorBadPortIndex;
        }
        if(nParam1 == PORT_INDEX_INPUT || (OMX_S32)nParam1 == -1)
            dec->in.def.bEnabled = OMX_TRUE;
        if(nParam1 == PORT_INDEX_OUTPUT || (OMX_S32)nParam1 == -1)
            dec->out.def.bEnabled = OMX_TRUE;
        if(nParam1 == PORT_INDEX_PP || (OMX_S32)nParam1 == -1)
            dec->inpp.def.bEnabled = OMX_TRUE;

        INIT_SEND_CMD(c, Cmd, nParam1, NULL, async_decoder_enable_port);
        err = HantroOmx_basecomp_send_command(&dec->base, &c);

#ifndef ANDROID_MOD
        if(nParam1 == PORT_INDEX_OUTPUT || (OMX_S32)nParam1 == -1)
            dec->portReconfigPending = OMX_FALSE;
#endif
        break;

    case OMX_CommandMarkBuffer:
        if((nParam1 != PORT_INDEX_INPUT) && (nParam1 != PORT_INDEX_PP))
        {
            TRACE_PRINT("API: bad port index:%u\n",
                        (unsigned) nParam1);
            return OMX_ErrorBadPortIndex;
        }
        CHECK_PARAM_NON_NULL(pCmdData);
        OMX_MARKTYPE *mark = (OMX_MARKTYPE *) OSAL_Malloc(sizeof(OMX_MARKTYPE));

        if(!mark)
        {
            TRACE_PRINT("API: cannot marshall mark\n");
            return OMX_ErrorInsufficientResources;
        }
        memcpy(mark, pCmdData, sizeof(OMX_MARKTYPE));
        INIT_SEND_CMD(c, Cmd, nParam1, mark, async_decoder_mark_buffer);
        err = HantroOmx_basecomp_send_command(&dec->base, &c);
        if(err != OMX_ErrorNone)
            OSAL_Free(mark);
        break;

    default:
        TRACE_PRINT("API: bad command:%u\n", (unsigned) Cmd);
        err = OMX_ErrorBadParameter;
        break;
    }
    if(err != OMX_ErrorNone)
        TRACE_PRINT("API: error: %s\n", HantroOmx_str_omx_err(err));
    return err;
}

static
    OMX_ERRORTYPE decoder_set_callbacks(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_CALLBACKTYPE * pCallbacks,
                                        OMX_IN OMX_PTR pAppData)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pCallbacks);
    OMX_DECODER *dec = GET_DECODER(hComponent);

    TRACE_PRINT("API: %s\n", __FUNCTION__);
    TRACE_PRINT("API: pCallbacks:%p pAppData:%p\n", pCallbacks,
                pAppData);

    dec->callbacks = *pCallbacks;
    dec->appdata = pAppData;
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE decoder_get_state(OMX_IN OMX_HANDLETYPE hComponent,
                                    OMX_OUT OMX_STATETYPE * pState)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pState);
    OMX_DECODER *dec = GET_DECODER(hComponent);

    *pState = dec->state;
    return OMX_ErrorNone;
}

static PORT *decoder_map_index_to_port(OMX_DECODER * dec, OMX_U32 portIndex)
{
    CALLSTACK;
    switch (portIndex)
    {
    case PORT_INDEX_INPUT:
        return &dec->in;
    case PORT_INDEX_OUTPUT:
        return &dec->out;
    case PORT_INDEX_PP:
        return &dec->inpp;
    }
    return NULL;
}

static
    OMX_ERRORTYPE decoder_verify_buffer_allocation(OMX_DECODER * dec, PORT * p,
                                                   OMX_U32 buffSize)
{
    CALLSTACK;
    assert(dec);
    assert(p);

    OMX_ERRORTYPE err = OMX_ErrorIncorrectStateOperation;

    // buffers can only be allocated when the component is in one of the following states:
    // 1. OMX_StateLoaded and has already sent a request for state transition to OMX_StateIdle
    // 2. OMX_StateWaitForResources, the resources are available and the component
    //    is ready to go to the OMX_StateIdle state
    // 3. OMX_StateExecuting, OMX_StatePause or OMX_StateIdle and the port is disabled.
    if(p->def.bPopulated)
    {
        TRACE_PRINT("API: port is already populated\n");
        return err;
    }
    if(buffSize < p->def.nBufferSize)
    {
        TRACE_PRINT("API: buffer is too small required:%u given:%u\n",
                    (unsigned) p->def.nBufferSize, (unsigned) buffSize);
        return OMX_ErrorBadParameter;
    }
    // 3.2.2.15
    switch (dec->state)
    {
    case OMX_StateLoaded:
        if(dec->statetrans != OMX_StateIdle)
        {
            TRACE_PRINT("API: not in transition to idle\n");
            return err;
        }
        break;
    case OMX_StateWaitForResources:
        return OMX_ErrorNotImplemented;

        //
        // These cases are in disagreement with the OMX_AllocateBuffer definition
        // in the specification in chapter 3.2.2.15. (And that chapter in turn seems to be
        // conflicting with chapter 3.2.2.6.)
        //
        // The bottom line is that the conformance tester sets the component to these states
        // and then wants to disable and enable ports and then allocate buffers. For example
        // Conformance tester PortCommunicationTest sets the component into executing state,
        // then disables all ports (-1), frees all buffers on those ports and finally tries
        // to enable all ports (-1), and then allocate buffers.
        // The specification says that if component is in executing state (or pause or idle)
        // the port must be disabled when allocating a buffer, but in order to pass the test
        // we must violate that requirement.
        //
        // A common guideline seems to be that when the tester and specification are in disagreement
        // the tester wins.
        //
    case OMX_StateIdle:
    case OMX_StateExecuting:
        break;

    default:
        if(p->def.bEnabled)
        {
            TRACE_PRINT("API: port is not disabled\n");
            return err;
        }
    }
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE decoder_use_buffer(OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_INOUT OMX_BUFFERHEADERTYPE ** ppBuffer,
                                     OMX_IN OMX_U32 nPortIndex,
                                     OMX_IN OMX_PTR pAppPrivate,
                                     OMX_IN OMX_U32 nSizeBytes,
                                     OMX_IN OMX_U8 * pBuffer)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(ppBuffer);
    CHECK_PARAM_NON_NULL(pBuffer);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    CHECK_STATE_INVALID(dec->state);

    TRACE_PRINT("API: %s\n", __FUNCTION__);
    TRACE_PRINT("API: nPortIndex:%u pAppPrivate:%p nSizeBytes:%u pBuffer:%p\n",
                (unsigned) nPortIndex, pAppPrivate, (unsigned) nSizeBytes,
                pBuffer);

    PORT *port = decoder_map_index_to_port(dec, nPortIndex);

    if(port == NULL)
    {
        TRACE_PRINT("API: bad port index:%u\n",
                    (unsigned) nPortIndex);
        return OMX_ErrorBadPortIndex;
    }
    OMX_ERRORTYPE err = decoder_verify_buffer_allocation(dec, port, nSizeBytes);

    if(err != OMX_ErrorNone)
        return err;

    TRACE_PRINT("API: port index:%u\n", (unsigned) nPortIndex);
    TRACE_PRINT("API: buffer size:%u\n", (unsigned) nSizeBytes);

    BUFFER *buff = NULL;

    HantroOmx_port_allocate_next_buffer(port, &buff);
    if(buff == NULL)
    {
        TRACE_PRINT("API: no more buffers\n");
        return OMX_ErrorInsufficientResources;
    }

    // save the pointer here into the header. The data in this buffer
    // needs to be copied in to the DMA accessible frame buffer before decoding.

    INIT_OMX_VERSION_PARAM(*buff->header);
    buff->flags &= ~BUFFER_FLAG_MY_BUFFER;
    buff->header->pBuffer = pBuffer;
    buff->header->pAppPrivate = pAppPrivate;
    buff->header->nAllocLen = nSizeBytes;
    buff->bus_address = 0;
    buff->allocsize = nSizeBytes;
    if(HantroOmx_port_buffer_count(port) == port->def.nBufferCountActual)
    {
        TRACE_PRINT("API: port is populated\n");
        port->def.bPopulated = OMX_TRUE;
    }
    if(nPortIndex == PORT_INDEX_INPUT || nPortIndex == PORT_INDEX_PP)
    {
        buff->header->nInputPortIndex = nPortIndex;
        buff->header->nOutputPortIndex = 0;
    }
    else
    {
        buff->header->nInputPortIndex = 0;
        buff->header->nOutputPortIndex = nPortIndex;
    }

    *ppBuffer = buff->header;
    TRACE_PRINT("API: pBufferHeader:%p\n", *ppBuffer);
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE decoder_allocate_buffer(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_INOUT OMX_BUFFERHEADERTYPE **
                                          ppBuffer, OMX_IN OMX_U32 nPortIndex,
                                          OMX_IN OMX_PTR pAppPrivate,
                                          OMX_IN OMX_U32 nSizeBytes)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(ppBuffer);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    CHECK_STATE_INVALID(dec->state);

    TRACE_PRINT("API: %s\n", __FUNCTION__);
    TRACE_PRINT("API: nPortIndex:%u pAppPrivate:%p nSizeBytes:%u\n",
                (unsigned) nPortIndex, pAppPrivate, (unsigned) nSizeBytes);
    PORT *port = decoder_map_index_to_port(dec, nPortIndex);

    if(port == NULL)
    {
        TRACE_PRINT("API: bad port index:%u\n",
                    (unsigned) nPortIndex);
        return OMX_ErrorBadPortIndex;
    }
    OMX_ERRORTYPE err = decoder_verify_buffer_allocation(dec, port, nSizeBytes);

    if(err != OMX_ErrorNone)
        return err;

    TRACE_PRINT("API: port index:%u\n", (unsigned) nPortIndex);
    TRACE_PRINT("API: buffer size:%u\n", (unsigned) nSizeBytes);

    // about locking here.
    // Only in case of tunneling is the component thread accessing the port's
    // buffer's directly. However this function should not be called at that time.
    // note: perhaps we could lock/somehow make sure that a misbehaving client thread
    // wont get a change to mess everything up?

    OMX_U8 *bus_data = NULL;

    OSAL_BUS_WIDTH bus_address = 0;

    OMX_U32 allocsize = nSizeBytes;

    // the conformance tester assumes that the allocated buffers equal nSizeBytes in size.
    // however when running on HW this might not be a valid assumption because the memory allocator
    // allocates memory always in fixed size chunks.
    TRACE_PRINT("decoder_allocate_buffer: OSAL_AllocatorAllocMem\n");

    err =
        OSAL_AllocatorAllocMem(&dec->alloc, &allocsize, &bus_data,
                               &bus_address);
    if(err != OMX_ErrorNone)
    {
        TRACE_PRINT("API: memory allocation failed: %s\n",
                    HantroOmx_str_omx_err(err));
        return err;
    }

    assert(allocsize >= nSizeBytes);
    TRACE_PRINT("API: allocated buffer size:%u\n",
                (unsigned) allocsize);

    BUFFER *buff = NULL;

    TRACE_PRINT("decoder_allocate_buffer: HantroOmx_port_allocate_next_buffer\n");
    HantroOmx_port_allocate_next_buffer(port, &buff);
    if(!buff)
    {
        TRACE_PRINT("API: no more buffers\n");
        OSAL_AllocatorFreeMem(&dec->alloc, nSizeBytes, bus_data, bus_address);
        return OMX_ErrorInsufficientResources;
    }

    INIT_OMX_VERSION_PARAM(*buff->header);
    buff->flags |= BUFFER_FLAG_MY_BUFFER;
    buff->bus_data = bus_data;
    buff->bus_address = bus_address;
    buff->allocsize = allocsize;
    buff->header->pBuffer = bus_data;
    buff->header->pAppPrivate = pAppPrivate;
    buff->header->nAllocLen = nSizeBytes;   // the conformance tester assumes that the allocated buffers equal nSizeBytes in size.
    if(nPortIndex == PORT_INDEX_INPUT || nPortIndex == PORT_INDEX_PP)
    {
        buff->header->nInputPortIndex = nPortIndex;
        buff->header->nOutputPortIndex = 0;
    }
    else
    {
        buff->header->nInputPortIndex = 0;
        buff->header->nOutputPortIndex = nPortIndex;
    }
    if(HantroOmx_port_buffer_count(port) == port->def.nBufferCountActual)
    {
        TRACE_PRINT("API: port is populated\n");
        TRACE_PRINT("API: bufferqueue.size = %lu\n",port->bufferqueue.size);
        port->def.bPopulated = OMX_TRUE;
    }
    *ppBuffer = buff->header;
    TRACE_PRINT("API: data (virtual address):%p pBufferHeader:%p\n",
                bus_data, *ppBuffer);

    return OMX_ErrorNone;

}

static
    OMX_ERRORTYPE decoder_free_buffer(OMX_IN OMX_HANDLETYPE hComponent,
                                      OMX_IN OMX_U32 nPortIndex,
                                      OMX_IN OMX_BUFFERHEADERTYPE *
                                      pBufferHeader)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pBufferHeader);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    TRACE_PRINT("API: %s\n", __FUNCTION__);
    TRACE_PRINT("API: nPortIndex:%u pBufferHeader:%p\n",
                (unsigned) nPortIndex, pBufferHeader);

    // 3.2.2.16
    // The call should be performed under the following conditions:
    // o  While the component is in the OMX_StateIdle state and the IL client has already
    //    sent a request for the state transition to OMX_StateLoaded (e.g., during the
    //    stopping of the component)
    // o  On a disabled port when the component is in the OMX_StateExecuting, the
    //    OMX_StatePause, or the OMX_StateIdle state.

    // the above is "should" only, in other words meaning that free buffer CAN be performed
    // at anytime. However the conformance tester expects that the component generates
    // an OMX_ErrorPortUnpopulated event, even though in the specification this is described as "may generate OMX_ErrorPortUnpopulated".

    PORT *port = decoder_map_index_to_port(dec, nPortIndex);

    if(port == NULL)
    {
        TRACE_PRINT("API: bad port index:%u\n",
                    (unsigned) nPortIndex);
        return OMX_ErrorBadPortIndex;
    }
    OMX_BOOL violation = OMX_FALSE;

    if(port->def.bEnabled)
    {
        if(dec->state == OMX_StateIdle && dec->statetrans != OMX_StateLoaded)
        {
            violation = OMX_TRUE;
        }
    }

    // have to lock the buffers here, cause its possible that
    // we're trying to free up a buffer that is still in fact queued
    // up for the port in the input queue
    // if there is such a buffer this function should then fail gracefully

    // 25.2.2008, locking removed (for the worse). This is because
    // when transitioning to idle state from executing or disabling a port
    // the queued buffers are being returned the queue is locked.
    // However the conformance tester for example calls directly back at us
    // and then locks up.

    //OMX_ERRORTYPE err = port_lock_buffers(port);
    //if (err != OMX_ErrorNone)
    //return err;

    // theres still data queued up on the port
    // could be the same buffers we're freeing here
    //assert(port_buffer_queue_count(port) == 0);

    BUFFER *buff = HantroOmx_port_find_buffer(port, pBufferHeader);

    if(!buff)
    {
        TRACE_PRINT("API: no such buffer\n");
        //HantroOmx_port_unlock_buffers(port);
        return OMX_ErrorBadParameter;
    }

    if(!(buff->flags & BUFFER_FLAG_IN_USE))
    {
        TRACE_PRINT("API: buffer is not allocated\n");
        //HantroOmx_port_unlock_buffers(port);
        return OMX_ErrorBadParameter;
    }

    if(buff->flags & BUFFER_FLAG_MY_BUFFER)
    {
        assert(buff->bus_address && buff->bus_data);
        assert(buff->bus_data == buff->header->pBuffer);
        assert(buff->allocsize);

        OSAL_AllocatorFreeMem(&dec->alloc, buff->allocsize, buff->bus_data,
                              buff->bus_address);
    }

    HantroOmx_port_release_buffer(port, buff);

    if(HantroOmx_port_buffer_count(port) < port->def.nBufferCountActual)
        port->def.bPopulated = OMX_FALSE;

    // remember to unlock!
    //port_unlock_buffers(port);

    // this is a hack because of the conformance tester.
    if(port->def.bPopulated == OMX_FALSE && violation == OMX_TRUE)
    {
        dec->callbacks.EventHandler(dec->self, dec->appdata, OMX_EventError,
                                    OMX_ErrorPortUnpopulated, 0, NULL);
    }

	TRACE_PRINT("%s end ok", __func__);
    return OMX_ErrorNone;
}

#define CHECK_PORT_STATE(decoder, port)                    \
  if ((decoder)->state != OMX_StateLoaded && (port)->bEnabled)    \
  {                                                             \
      TRACE_PRINT("API: port is not disabled\n");               \
      return OMX_ErrorIncorrectStateOperation;                  \
  }

#define CHECK_PORT_OUTPUT(param) \
  if ((param)->nPortIndex != PORT_INDEX_OUTPUT) \
      return OMX_ErrorBadPortIndex;

#define CHECK_PORT_INPUT(param) \
  if ((param)->nPortIndex != PORT_INDEX_INPUT) \
      return OMX_ErrorBadPortIndex;

#if (defined OMX_DECODER_VIDEO_DOMAIN)
static
    OMX_ERRORTYPE decoder_set_parameter(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_INDEXTYPE nIndex,
                                        OMX_IN OMX_PTR pParam)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    CHECK_STATE_INVALID(dec->state);
    TRACE_PRINT("API: %s\n", __FUNCTION__);

    switch (nIndex)
    {
    case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *port = NULL;

            OMX_PARAM_PORTDEFINITIONTYPE *param =
                (OMX_PARAM_PORTDEFINITIONTYPE *) pParam;
            switch (param->nPortIndex)
            {
            case PORT_INDEX_INPUT:
                port = &dec->in.def;
                CHECK_PORT_STATE(dec, port);

                port->nBufferCountActual = param->nBufferCountActual;
                port->nBufferSize = param->nBufferSize;
                port->format.video.eCompressionFormat =
                    param->format.video.eCompressionFormat;
                port->format.video.eColorFormat =
                    param->format.video.eColorFormat;
                port->format.video.bFlagErrorConcealment =
                    param->format.video.bFlagErrorConcealment;
                // need these for raw YUV input
                port->format.video.nFrameWidth =
                    param->format.video.nFrameWidth;
                port->format.video.nFrameHeight =
                    param->format.video.nFrameHeight;
#ifdef CONFORMANCE
                if (strcmp((char*)dec->role, "video_decoder.mpeg4") == 0)
                    port->format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
                else if (strcmp((char*)dec->role, "video_decoder.avc") == 0)
                    port->format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
                else if (strcmp((char*)dec->role, "video_decoder.h263") == 0)
                    port->format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
                else if (strcmp((char*)dec->role, "video_decoder.wmv") == 0)
                    port->format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
                else if (strcmp((char*)dec->role, "video_decoder.vp8") == 0)
                    port->format.video.eCompressionFormat = OMX_VIDEO_CodingVP8;
#endif
                break;

            case PORT_INDEX_OUTPUT:
                port = &dec->out.def;
                CHECK_PORT_STATE(dec, port);

                port->nBufferCountActual = param->nBufferCountActual;
                port->nBufferSize = param->nBufferSize;
                port->format.video.eColorFormat =
                    param->format.video.eColorFormat;
                port->format.video.nFrameWidth =
                     param->format.video.nFrameWidth;
                port->format.video.nFrameHeight =
                     param->format.video.nFrameHeight;
                break;

            case PORT_INDEX_PP:
                port = &dec->inpp.def;
                CHECK_PORT_STATE(dec, port);
                port->nBufferCountActual = param->nBufferCountActual;
                port->format.image.nFrameWidth = param->format.image.nFrameWidth;   // mask1 width
                port->format.image.nFrameHeight = param->format.image.nFrameHeight; // mask2 height
                break;
            default:
                TRACE_PRINT("API: no such port: %u\n",
                            (unsigned) param->nPortIndex);
                return OMX_ErrorBadPortIndex;
            }
        }
        break;

    case OMX_IndexParamVideoPortFormat:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *port = NULL;

            OMX_VIDEO_PARAM_PORTFORMATTYPE *param =
                (OMX_VIDEO_PARAM_PORTFORMATTYPE *) pParam;
            switch (param->nPortIndex)
            {
            case PORT_INDEX_INPUT:
                port = &dec->in.def;
                CHECK_PORT_STATE(dec, port);
                port->format.video.eCompressionFormat =
                    param->eCompressionFormat;
                port->format.video.eColorFormat = param->eColorFormat;
                break;
            case PORT_INDEX_OUTPUT:
                port = &dec->out.def;
                CHECK_PORT_STATE(dec, port);
                port->format.video.eCompressionFormat =
                    param->eCompressionFormat;
                port->format.video.eColorFormat = param->eColorFormat;
                TRACE_PRINT("set OMX_IndexParamVideoPortFormat(output): eColorFormat = %d", param->eColorFormat);

                break;
            default:
                TRACE_PRINT("API: no such video port:%u\n",
                            (unsigned) param->nPortIndex);
                return OMX_ErrorBadPortIndex;
            }
        }
        break;

    case OMX_IndexParamImagePortFormat:
        {
            // note: this is not currently being used anywhere. the 2nd input port input format is set at construction.
        }
        break;

        // note: for conformance.
    case OMX_IndexParamVideoAvc:
    case OMX_IndexParamVideoMpeg4:
    case OMX_IndexParamVideoH263:
    case OMX_IndexParamVideoWmv:
        {
            OMX_VIDEO_PARAM_WMVTYPE *param = (OMX_VIDEO_PARAM_WMVTYPE *) pParam;
            dec->WMVFormat = param->eFormat;
        }
    case OMX_IndexParamVideoMpeg2:
    case OMX_IndexParamVideoVp8:
        break;

    case OMX_IndexParamVideoRv:
        {
            OMX_VIDEO_PARAM_RVTYPE *param = (OMX_VIDEO_PARAM_RVTYPE *) pParam;
            if (param->eFormat == OMX_VIDEO_RVFormat8)
            {
                dec->bIsRV8 = OMX_TRUE;
            }
            else
            {
                dec->bIsRV8 = OMX_FALSE;
            }
            dec->imageSize = param->nMaxEncodeFrameSize;
            dec->width = param->nPaddedWidth;
            dec->height = param->nPaddedHeight;
        }
        break;

        // component mandatory stuff
    case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE *param =
                (OMX_PARAM_BUFFERSUPPLIERTYPE *) pParam;
            PORT *port = decoder_map_index_to_port(dec, param->nPortIndex);

            if(!port)
                return OMX_ErrorBadPortIndex;
            CHECK_PORT_STATE(dec, &port->def);

            port->tunnel.eSupplier = param->eBufferSupplier;

            TRACE_PRINT("API: new buffer supplier value:%s port:%d\n",
                        HantroOmx_str_omx_supplier(param->eBufferSupplier),
                        (int) param->nPortIndex);

            if(port->tunnelcomp && port->def.eDir == OMX_DirInput)
            {
                TRACE_PRINT("API: propagating value to tunneled component: %p port: %d\n",
                            port->tunnelcomp, (int) port->tunnelport);
                OMX_ERRORTYPE err;

                OMX_PARAM_BUFFERSUPPLIERTYPE foo;

                memset(&foo, 0, sizeof(foo));
                INIT_OMX_VERSION_PARAM(foo);
                foo.nPortIndex = port->tunnelport;
                foo.eBufferSupplier = port->tunnel.eSupplier;
                err =
                    ((OMX_COMPONENTTYPE *) port->tunnelcomp)->
                    SetParameter(port->tunnelcomp,
                                 OMX_IndexParamCompBufferSupplier, &foo);
                if(err != OMX_ErrorNone)
                {
                    TRACE_PRINT("API: tunneled component refused buffer supplier config:%s\n",
                                HantroOmx_str_omx_err(err));
                    return err;
                }
            }
        }
        break;

    case OMX_IndexParamCommonDeblocking:
        {
            OMX_PARAM_DEBLOCKINGTYPE *param =
                (OMX_PARAM_DEBLOCKINGTYPE *) pParam;
            dec->deblock = param->bDeblocking;
        }
        break;

    case OMX_IndexParamStandardComponentRole:
        {
            OMX_PARAM_COMPONENTROLETYPE *param =
                (OMX_PARAM_COMPONENTROLETYPE *) pParam;
            strcpy((char *) dec->role, (const char *) param->cRole);
            //printf("Codec role %s\n", dec->role);
#ifdef CONFORMANCE
                if (strcmp((char*)dec->role, "video_decoder.mpeg4") == 0)
                    dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
                else if (strcmp((char*)dec->role, "video_decoder.avc") == 0)
                    dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
                else if (strcmp((char*)dec->role, "video_decoder.h263") == 0)
                    dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
                else if (strcmp((char*)dec->role, "video_decoder.wmv") == 0)
                    dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
                else if (strcmp((char*)dec->role, "video_decoder.vp8") == 0)
                    dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingVP8;
#endif
        }

    case OMX_IndexParamPriorityMgmt:
        {
            CHECK_STATE_LOADED(dec->state);
            OMX_PRIORITYMGMTTYPE *param = (OMX_PRIORITYMGMTTYPE *) pParam;

            dec->priority_group = param->nGroupPriority;
            dec->priority_id = param->nGroupID;
        }
        break;
    default:
        TRACE_PRINT("API: unsupported settings index\n");
        return OMX_ErrorUnsupportedIndex;
    }
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE decoder_get_parameter(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_INDEXTYPE nIndex,
                                        OMX_INOUT OMX_PTR pParam)
{
    CALLSTACK;
    static int input_formats[][2] = {
#ifdef ENABLE_CODEC_H264
        {OMX_VIDEO_CodingAVC, OMX_COLOR_FormatUnused},  // H264
#endif
#ifdef ENABLE_CODEC_H263
        {OMX_VIDEO_CodingH263, OMX_COLOR_FormatUnused},
#endif
#ifdef ENABLE_CODEC_MPEG4
        {OMX_VIDEO_CodingMPEG4, OMX_COLOR_FormatUnused},
        {OMX_VIDEO_CodingSORENSON, OMX_COLOR_FormatUnused},
#endif
#ifdef ENABLE_CODEC_CUSTOM_1
        {OMX_VIDEO_CodingCUSTOM_1, OMX_COLOR_FormatUnused},
        {OMX_VIDEO_CodingCUSTOM_1_3, OMX_COLOR_FormatUnused},
#endif
#ifdef ENABLE_CODEC_RV
        {OMX_VIDEO_CodingRV, OMX_COLOR_FormatUnused},
#endif
#ifdef ENABLE_CODEC_VP8
        {OMX_VIDEO_CodingVP8, OMX_COLOR_FormatUnused},
#endif
#ifdef ENABLE_CODEC_AVS
        {OMX_VIDEO_CodingAVS, OMX_COLOR_FormatUnused},
#endif
#ifdef ENABLE_CODEC_VP6
        {OMX_VIDEO_CodingVP6, OMX_COLOR_FormatUnused},
#endif
#ifdef ENABLE_CODEC_VC1
        {OMX_VIDEO_CodingWMV, OMX_COLOR_FormatUnused},
#endif
#ifdef ENABLE_CODEC_MPEG2
        {OMX_VIDEO_CodingMPEG2, OMX_COLOR_FormatUnused},
#endif
#ifdef ENABLE_CODEC_MJPEG
        {OMX_VIDEO_CodingMJPEG, OMX_COLOR_FormatUnused},
#endif
#ifdef ENABLE_PP
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420PackedPlanar},   // PP standalone input
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420Planar},         // PP standalone input
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420PackedSemiPlanar},   // PP standalone input
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420SemiPlanar},   // PP standalone input
#if defined (IS_8190) || defined (IS_G1_DECODER)
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatCrYCbY},
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatCbYCrY},
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYCrYCb},
#endif /* IS_8190 */
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYCbYCr}    // PP standalone input YUV4:2:2 interleaved
#endif /* ENABLE_PP */
    };
    // post-proc also seems to have 420tiled input. but there's no enum for that in openmax?

    static int output_formats[][2] = {
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420PackedSemiPlanar},   // normal video output format
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420SemiPlanar},   // normal video output format
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420PackedPlanar},
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420Planar},
#ifdef ENABLE_PP
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYCbYCr},   // post-processor output format
#if defined (IS_8190) || defined (IS_G1_DECODER)
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYCrYCb},
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatCbYCrY},
        {OMX_VIDEO_CodingUnused, OMX_COLOR_FormatCrYCbY},
#endif /* IS_8190 */
        {OMX_VIDEO_CodingUnused, OMX_COLOR_Format32bitARGB8888},    // post-processor output format
        {OMX_VIDEO_CodingUnused, OMX_COLOR_Format32bitBGRA8888},    // post-processor output format
        {OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitARGB1555},    // post-processor output format
        {OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitARGB4444},    // post-processor output format
        {OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitRGB565},  // post-processor output format
        {OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitBGR565}   // post-processor output format
#endif /* ENABLE_PP */
    };

    /*static int profiles[][2] = {
        {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31},   // OMX_VIDEO_AVCLevel31 is not accepted by the conformance tester (standard component test)
        {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level5},
        {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70},
        {OMX_VIDEO_MPEG2ProfileSimple, OMX_VIDEO_MPEG2LevelHL},
        {OMX_VIDEO_VP8ProfileMain, OMX_VIDEO_VP8Level_Version0},
        {0, 0}  //  WMV doesn't have these
    };*/

#ifdef ENABLE_CODEC_H263
    static int h263_profiles[][2] = {
        {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70}
    };
#endif
#ifdef ENABLE_CODEC_MPEG2
    static int mpeg2_profiles[][2] = {
        { OMX_VIDEO_MPEG2ProfileSimple,    OMX_VIDEO_MPEG2LevelHL },
        { OMX_VIDEO_MPEG2ProfileMain,      OMX_VIDEO_MPEG2LevelHL }
    };
#endif
#ifdef ENABLE_CODEC_MPEG4
    static int mpeg4_profiles[][2] = {
        { OMX_VIDEO_MPEG4ProfileSimple,    OMX_VIDEO_MPEG4Level5 },
        { OMX_VIDEO_MPEG4ProfileAdvancedSimple,      OMX_VIDEO_MPEG4Level5 }
    };
#endif
#ifdef ENABLE_CODEC_VC1
static int wmv_profiles[][2] = {
        {OMX_VIDEO_WMVFormat7, 0},
        {OMX_VIDEO_WMVFormat8, 0},
        {OMX_VIDEO_WMVFormat9, 0},
    };
#endif
#ifdef ENABLE_CODEC_RV
    static int rv_profiles[][2] = {
        {OMX_VIDEO_RVFormat8, 0},
        {OMX_VIDEO_RVFormat9, 0},
        {OMX_VIDEO_RVFormatG2, 0}
    };
#endif
#ifdef ENABLE_CODEC_H264
    static int avc_profiles[][2] = {
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel51 },
        { OMX_VIDEO_AVCProfileMain,        OMX_VIDEO_AVCLevel51 },
        { OMX_VIDEO_AVCProfileHigh,        OMX_VIDEO_AVCLevel51 }
    };
#endif
#ifdef ENABLE_CODEC_VP8
    static int vp8_profiles[][2] = {
        { OMX_VIDEO_VP8ProfileMain,     OMX_VIDEO_VP8Level_Version0 }
    };
#endif

    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    CHECK_STATE_INVALID(dec->state);
    TRACE_PRINT("API: %s\n", __FUNCTION__);
    TRACE_PRINT("API: getting index: %s\n",
                HantroOmx_str_omx_index(nIndex));

    switch (nIndex)
    {
    case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *port = NULL;

            OMX_PARAM_PORTDEFINITIONTYPE *param =
                (OMX_PARAM_PORTDEFINITIONTYPE *) pParam;
            switch (param->nPortIndex)
            {
            case PORT_INDEX_INPUT:
                port = &dec->in.def;
                break;
            case PORT_INDEX_OUTPUT:
                port = &dec->out.def;
                break;
            case PORT_INDEX_PP:
                port = &dec->inpp.def;
                break;
            default:
                return OMX_ErrorBadPortIndex;
            }
            assert(param->nSize);
            memcpy(param, port, param->nSize);
        }
        break;

        // this is used to enumerate the formats supported by the ports
    case OMX_IndexParamVideoPortFormat:
        {
            OMX_VIDEO_PARAM_PORTFORMATTYPE *param =
                (OMX_VIDEO_PARAM_PORTFORMATTYPE *) pParam;
            switch (param->nPortIndex)
            {
            case PORT_INDEX_INPUT:
                if(param->nIndex >= sizeof(input_formats) / (sizeof(int) * 2))
                    return OMX_ErrorNoMore;
                param->xFramerate = 0;
                param->eCompressionFormat = input_formats[param->nIndex][0];
                param->eColorFormat = input_formats[param->nIndex][1];
                break;
            case PORT_INDEX_OUTPUT:
                if(param->nIndex >= sizeof(output_formats) / (sizeof(int) * 2))
                    return OMX_ErrorNoMore;
                param->xFramerate = 0;
                param->eCompressionFormat = output_formats[param->nIndex][0];
                //param->eColorFormat = OMX_COLOR_Format16bitRGB565;
                //param->eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
                param->eColorFormat = output_formats[param->nIndex][1];
                TRACE_PRINT("get OMX_IndexParamVideoPortFormat: param->eColorFormat = %d", output_formats[param->nIndex][1]);
                break;
            default:
                return OMX_ErrorBadPortIndex;
            }
        }
        break;

// this is for the post-processor input
    case OMX_IndexParamImagePortFormat:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *port = NULL;

            OMX_IMAGE_PARAM_PORTFORMATTYPE *param =
                (OMX_IMAGE_PARAM_PORTFORMATTYPE *) pParam;
            switch (param->nPortIndex)
            {
            case PORT_INDEX_PP:
                if(param->nIndex >= 1)
                    return OMX_ErrorNoMore;
                port = &dec->inpp.def;
                param->eCompressionFormat = port->format.image.eCompressionFormat;  // image coding not used
                param->eColorFormat = port->format.image.eColorFormat;  // ARGB8888
                break;
            default:
                return OMX_ErrorBadPortIndex;
            }
        }
        break;

    case OMX_IndexParamVideoAvc:
        {
            OMX_VIDEO_PARAM_AVCTYPE *param = (OMX_VIDEO_PARAM_AVCTYPE *) pParam;

            if(param->nPortIndex != PORT_INDEX_INPUT)
                return OMX_ErrorBadPortIndex;
            param->eProfile = OMX_VIDEO_AVCProfileBaseline;
            param->eLevel = OMX_VIDEO_AVCLevel31;   // not good according to the conformance tester
#ifdef CONFORMANCE
            param->eLevel               = OMX_VIDEO_AVCLevel1;
#endif
            param->nAllowedPictureTypes =
                OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
        }
        break;

    case OMX_IndexParamVideoMpeg4:
        {
            OMX_VIDEO_PARAM_MPEG4TYPE *param =
                (OMX_VIDEO_PARAM_MPEG4TYPE *) pParam;
            CHECK_PORT_INPUT(param);
            param->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
            param->eLevel = OMX_VIDEO_MPEG4Level5;
#ifdef CONFORMANCE
            param->eLevel = OMX_VIDEO_MPEG4Level1;
#endif
            param->nAllowedPictureTypes =
                OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
        }
        break;

    case OMX_IndexParamVideoH263:
        {
            OMX_VIDEO_PARAM_H263TYPE *param =
                (OMX_VIDEO_PARAM_H263TYPE *) pParam;
            CHECK_PORT_INPUT(param);
            param->eProfile = OMX_VIDEO_H263ProfileBaseline;
            param->eLevel = OMX_VIDEO_H263Level70;
            param->bPLUSPTYPEAllowed = OMX_TRUE;
#ifdef CONFORMANCE
            param->eLevel = OMX_VIDEO_H263Level10;
            param->bPLUSPTYPEAllowed = OMX_FALSE;
            param->bForceRoundingTypeToZero = OMX_TRUE;
#endif
            param->nAllowedPictureTypes =
                OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
        }
        break;

   case OMX_IndexParamVideoRv:
        {
            OMX_VIDEO_PARAM_RVTYPE *param = (OMX_VIDEO_PARAM_RVTYPE *) pParam;
            CHECK_PORT_INPUT(param);

            if (dec->bIsRV8)
            {
                 param->eFormat = OMX_VIDEO_RVFormat8;
            }
            else
            {
                param->eFormat = OMX_VIDEO_RVFormat9;
            }
            param->nMaxEncodeFrameSize = dec->imageSize;
            param->nPaddedWidth = dec->width;
            param->nPaddedHeight = dec->height;
        }
        break;

    case OMX_IndexParamVideoWmv:
        {
            OMX_VIDEO_PARAM_WMVTYPE *param = (OMX_VIDEO_PARAM_WMVTYPE *) pParam;

            CHECK_PORT_INPUT(param);
            param->eFormat = dec->WMVFormat;
        }
        break;

    case OMX_IndexParamVideoMpeg2:
        {
            OMX_VIDEO_PARAM_MPEG2TYPE *param =
                (OMX_VIDEO_PARAM_MPEG2TYPE *) pParam;
            CHECK_PORT_INPUT(param);
            param->eProfile = OMX_VIDEO_MPEG2ProfileSimple;
            param->eLevel = OMX_VIDEO_MPEG2LevelHL;
        }
        break;

    case OMX_IndexParamVideoVp8:
        {
            OMX_VIDEO_PARAM_VP8TYPE *param = (OMX_VIDEO_PARAM_VP8TYPE *) pParam;

            CHECK_PORT_INPUT(param);
            param->eProfile = OMX_VIDEO_VP8ProfileMain;
            param->eLevel = OMX_VIDEO_VP8Level_Version0;
        }
        break;

    case OMX_IndexParamVideoProfileLevelCurrent:
    case OMX_IndexParamVideoProfileLevelQuerySupported:
        {
            OMX_VIDEO_PARAM_PROFILELEVELTYPE *param =
                (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) pParam;
            CHECK_PORT_INPUT(param);
            switch ( dec->in.def.format.video.eCompressionFormat )
            {
#ifdef ENABLE_CODEC_H263
                case OMX_VIDEO_CodingH263:
                    if (param->nProfileIndex >= sizeof(h263_profiles)/(sizeof(int)*2))
                        return OMX_ErrorNoMore;
                    param->eProfile = h263_profiles[param->nProfileIndex][0];
                    param->eLevel   = h263_profiles[param->nProfileIndex][1];
                    break;
#endif
#ifdef ENABLE_CODEC_MPEG2
                case OMX_VIDEO_CodingMPEG2:
                    if (param->nProfileIndex >= sizeof(mpeg2_profiles)/(sizeof(int)*2))
                        return OMX_ErrorNoMore;
                    param->eProfile = mpeg2_profiles[param->nProfileIndex][0];
                    param->eLevel   = mpeg2_profiles[param->nProfileIndex][1];
                    break;
#endif
#ifdef ENABLE_CODEC_MPEG4
                case OMX_VIDEO_CodingMPEG4:
                    if (param->nProfileIndex >= sizeof(mpeg4_profiles)/(sizeof(int)*2))
                        return OMX_ErrorNoMore;
                    param->eProfile = mpeg4_profiles[param->nProfileIndex][0];
                    param->eLevel   = mpeg4_profiles[param->nProfileIndex][1];
                    break;
#endif
#ifdef ENABLE_CODEC_VC1
                case OMX_VIDEO_CodingWMV:
                    if (param->nProfileIndex >= sizeof(wmv_profiles)/(sizeof(int)*2))
                        return OMX_ErrorNoMore;
                    param->eProfile = wmv_profiles[param->nProfileIndex][0];
                    param->eLevel   = wmv_profiles[param->nProfileIndex][1];
                    break;
#endif
#ifdef ENABLE_CODEC_RV
                case OMX_VIDEO_CodingRV:
                    if (param->nProfileIndex >= sizeof(rv_profiles)/(sizeof(int)*2))
                        return OMX_ErrorNoMore;
                    param->eProfile = rv_profiles[param->nProfileIndex][0];
                    param->eLevel   = rv_profiles[param->nProfileIndex][1];
                    break;
#endif
#ifdef ENABLE_CODEC_H264
                case OMX_VIDEO_CodingAVC:
                    if (param->nProfileIndex >= sizeof(avc_profiles)/(sizeof(int)*2))
                        return OMX_ErrorNoMore;
                    param->eProfile = avc_profiles[param->nProfileIndex][0];
                    param->eLevel   = avc_profiles[param->nProfileIndex][1];
                    break;
#endif
#ifdef ENABLE_CODEC_VP8
                case OMX_VIDEO_CodingVP8:
                    if (param->nProfileIndex >= sizeof(vp8_profiles)/(sizeof(int)*2))
                        return OMX_ErrorNoMore;
                    param->eProfile = vp8_profiles[param->nProfileIndex][0];
                    param->eLevel   = vp8_profiles[param->nProfileIndex][1];
                    break;
#endif
                default:
                    return OMX_ErrorNoMore;
            }
        }
        break;

    case OMX_IndexParamCommonDeblocking:
        {
            OMX_PARAM_DEBLOCKINGTYPE *param =
                (OMX_PARAM_DEBLOCKINGTYPE *) pParam;
            param->bDeblocking = dec->deblock;
        }
        break;

        // component mandatory stuff

    case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE *param =
                (OMX_PARAM_BUFFERSUPPLIERTYPE *) pParam;
            PORT *port = decoder_map_index_to_port(dec, param->nPortIndex);

            if(!port)
                return OMX_ErrorBadPortIndex;
            param->eBufferSupplier = port->tunnel.eSupplier;
        }
        break;

    case OMX_IndexParamStandardComponentRole:
        {
            OMX_PARAM_COMPONENTROLETYPE *param =
                (OMX_PARAM_COMPONENTROLETYPE *) pParam;
            strcpy((char *) param->cRole, (const char *) dec->role);
        }
        break;

    case OMX_IndexParamPriorityMgmt:
        {
            OMX_PRIORITYMGMTTYPE *param = (OMX_PRIORITYMGMTTYPE *) pParam;

            param->nGroupPriority = dec->priority_group;
            param->nGroupID = dec->priority_id;
        }
        break;

    case OMX_IndexParamAudioInit:
    case OMX_IndexParamOtherInit:
        {
            OMX_PORT_PARAM_TYPE *param = (OMX_PORT_PARAM_TYPE *) pParam;

            param->nPorts = 0;
            param->nStartPortNumber = 0;
        }
        break;
    case OMX_IndexParamVideoInit:
        {
            OMX_PORT_PARAM_TYPE *param = (OMX_PORT_PARAM_TYPE *) pParam;

            param->nPorts = 2;
            param->nStartPortNumber = 0;
        }
        break;
    case OMX_IndexParamImageInit:
        {
            OMX_PORT_PARAM_TYPE *param = (OMX_PORT_PARAM_TYPE *) pParam;

            param->nPorts = 1;
            param->nStartPortNumber = 2;
        }
        break;
#ifdef ANDROID
     /* Opencore specific */
     case (OMX_INDEXTYPE) PV_OMX_COMPONENT_CAPABILITY_TYPE_INDEX:
        {
            PV_OMXComponentCapabilityFlagsType *param = (PV_OMXComponentCapabilityFlagsType *) pParam;
            param->iIsOMXComponentMultiThreaded = OMX_TRUE;
            param->iOMXComponentSupportsExternalOutputBufferAlloc = OMX_FALSE;
            param->iOMXComponentSupportsExternalInputBufferAlloc = OMX_FALSE;
            param->iOMXComponentSupportsMovableInputBuffers = OMX_TRUE;
            param->iOMXComponentSupportsPartialFrames = OMX_FALSE;
            param->iOMXComponentUsesNALStartCode = OMX_TRUE;
            param->iOMXComponentCanHandleIncompleteFrames = OMX_FALSE;
            param->iOMXComponentUsesFullAVCFrames = OMX_TRUE;
            TRACE_PRINT("PV Capability Flags set");
        }
        break;
#endif
    default:
        TRACE_PRINT("API: unsupported settings index\n");
        return OMX_ErrorUnsupportedIndex;
    }

    return OMX_ErrorNone;
}

#endif
#ifdef OMX_DECODER_IMAGE_DOMAIN
static
    OMX_ERRORTYPE decoder_set_parameter(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_INDEXTYPE nIndex,
                                        OMX_IN OMX_PTR pParam)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    CHECK_STATE_INVALID(dec->state);
    CHECK_STATE_IDLE(dec->state);
    TRACE_PRINT("API: %s\n", __FUNCTION__);

    switch (nIndex)
    {
    case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *port = NULL;

            OMX_PARAM_PORTDEFINITIONTYPE *param =
                (OMX_PARAM_PORTDEFINITIONTYPE *) pParam;
            switch (param->nPortIndex)
            {
            case PORT_INDEX_INPUT:
                port = &dec->in.def;
                CHECK_PORT_STATE(dec, port);
                port->nBufferCountActual = param->nBufferCountActual;
                port->format.image.eColorFormat =
                    param->format.image.eColorFormat;
                port->format.image.eCompressionFormat =
                    param->format.image.eCompressionFormat;
                // need these for raw YUV input
                port->format.image.nFrameWidth =
                    param->format.image.nFrameWidth;
                port->format.image.nFrameHeight =
                    param->format.image.nFrameHeight;
#ifdef CONFORMANCE
                if (strcmp((char*)dec->role, "image_decoder.jpeg") == 0)
                    port->format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
                else if (strcmp((char*)dec->role, "image_decoder.webp") == 0)
                    port->format.image.eCompressionFormat = OMX_IMAGE_CodingWEBP;
#endif
                break;
            case PORT_INDEX_OUTPUT:
                port = &dec->out.def;
                CHECK_PORT_STATE(dec, port);
                port->nBufferCountActual = param->nBufferCountActual;
                port->format.image.eColorFormat =
                    param->format.image.eColorFormat;
                port->format.image.nFrameHeight =
                    param->format.image.nFrameHeight;
                port->format.image.nFrameWidth =
                    param->format.image.nFrameWidth;
                break;
            case PORT_INDEX_PP:
                port = &dec->inpp.def;
                CHECK_PORT_STATE(dec, port);
                port->nBufferCountActual = param->nBufferCountActual;
                port->format.image.nFrameWidth = param->format.image.nFrameWidth;   // mask1 width
                port->format.image.nFrameHeight = param->format.image.nFrameHeight; // mask1 height
                break;
            default:
                TRACE_PRINT("API: no such port: %u\n",
                            (unsigned) param->nPortIndex);
                return OMX_ErrorBadPortIndex;
            }
        }
        break;

    case OMX_IndexParamImagePortFormat:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *port = NULL;

            OMX_IMAGE_PARAM_PORTFORMATTYPE *param =
                (OMX_IMAGE_PARAM_PORTFORMATTYPE *) pParam;
            switch (param->nPortIndex)
            {
            case PORT_INDEX_INPUT:
                port = &dec->in.def;
                CHECK_PORT_STATE(dec, port);
                port->format.image.eCompressionFormat =
                    param->eCompressionFormat;
                port->format.image.eColorFormat = param->eColorFormat;
                break;
            case PORT_INDEX_OUTPUT:
                port = &dec->out.def;
                CHECK_PORT_STATE(dec, port);
                port->format.image.eCompressionFormat =
                    param->eCompressionFormat;
                port->format.image.eColorFormat = param->eColorFormat;
                break;
            case PORT_INDEX_PP:
                break;
            default:
                TRACE_PRINT("API: no such image port:%u\n",
                            (unsigned) param->nPortIndex);
                return OMX_ErrorBadPortIndex;
            }
        }

    case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE *param =
                (OMX_PARAM_BUFFERSUPPLIERTYPE *) pParam;
            PORT *port = decoder_map_index_to_port(dec, param->nPortIndex);

            if(!port)
                return OMX_ErrorBadPortIndex;
            CHECK_PORT_STATE(dec, &port->def);

            port->tunnel.eSupplier = param->eBufferSupplier;

            TRACE_PRINT("API: new buffer supplier value:%s port:%d\n",
                        HantroOmx_str_omx_supplier(param->eBufferSupplier),
                        (int) param->nPortIndex);

            if(port->tunnelcomp && port->def.eDir == OMX_DirInput)
            {
                TRACE_PRINT("API: propagating value to tunneled component: %p port: %d\n",
                            port->tunnelcomp, (int) port->tunnelport);
                OMX_ERRORTYPE err;

                OMX_PARAM_BUFFERSUPPLIERTYPE foo;

                memset(&foo, 0, sizeof(foo));
                INIT_OMX_VERSION_PARAM(foo);
                foo.nPortIndex = port->tunnelport;
                foo.eBufferSupplier = port->tunnel.eSupplier;
                err =
                    ((OMX_COMPONENTTYPE *) port->tunnelcomp)->
                    SetParameter(port->tunnelcomp,
                                 OMX_IndexParamCompBufferSupplier, &foo);
                if(err != OMX_ErrorNone)
                {
                    TRACE_PRINT("API: tunneled component refused buffer supplier config:%s\n",
                                HantroOmx_str_omx_err(err));
                    return err;
                }
            }
        }
        break;

    case OMX_IndexParamStandardComponentRole:
        {
            OMX_PARAM_COMPONENTROLETYPE *param =
                (OMX_PARAM_COMPONENTROLETYPE *) pParam;
            strcpy((char *) dec->role, (const char *) param->cRole);
#ifdef CONFORMANCE
                if (strcmp((char*)dec->role, "image_decoder.jpeg") == 0)
                    dec->in.def.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
                else if (strcmp((char*)dec->role, "image_decoder.webp") == 0)
                    dec->in.def.format.image.eCompressionFormat = OMX_IMAGE_CodingWEBP;
#endif
        }
        break;

    case OMX_IndexParamPriorityMgmt:
        {
            CHECK_STATE_LOADED(dec->state);
            OMX_PRIORITYMGMTTYPE *param = (OMX_PRIORITYMGMTTYPE *) pParam;

            dec->priority_group = param->nGroupPriority;
            dec->priority_id = param->nGroupID;
        }
        break;
#ifdef CONFORMANCE
        case OMX_IndexParamHuffmanTable:
        {
            OMX_IMAGE_PARAM_HUFFMANTTABLETYPE *param =
                (OMX_IMAGE_PARAM_HUFFMANTTABLETYPE *) pParam;
            memcpy(&dec->huffman_table, param, param->nSize);
        }
        break;

        case OMX_IndexParamQuantizationTable:
        {
            OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *param =
                (OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *) pParam;
            memcpy(&dec->quant_table, param, param->nSize);
        }
        break;
#endif
    default:
        TRACE_PRINT("API: unsupported settings index\n");
        return OMX_ErrorUnsupportedIndex;
    }
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE decoder_get_parameter(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_INDEXTYPE nIndex,
                                        OMX_INOUT OMX_PTR pParam)
{
    CALLSTACK;
    static int input_formats[][2] = {
#ifdef ENABLE_CODEC_JPEG
        {OMX_IMAGE_CodingJPEG, OMX_COLOR_FormatUnused}, // normal JPEG input
#endif
#ifdef ENABLE_CODEC_WEBP
        {OMX_IMAGE_CodingWEBP, OMX_COLOR_FormatUnused}, // normal WebP input
#endif
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatYUV420PackedPlanar},   // PP standalone input
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatYUV420Planar},         // PP standalone input
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatYUV420PackedSemiPlanar},   // PP standalone input
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatYUV420SemiPlanar},   // PP standalone input
#if defined (IS_8190) || defined (IS_G1_DECODER)
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatCbYCrY},
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatCrYCbY},
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatYCrYCb},
#endif
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatYCbYCr}    // PP standalone input YUV422-interleaved
    };

    static int output_formats[][2] = {
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatYUV420Planar},
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatYUV420PackedSemiPlanar},
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatYUV420SemiPlanar},
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatYUV422PackedSemiPlanar},
        {OMX_IMAGE_CodingUnused, OMX_COLOR_FormatL8}    // grayscale
    };

    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    CHECK_STATE_INVALID(dec->state);
    TRACE_PRINT("API: %s\n", __FUNCTION__);
    TRACE_PRINT("API: settings index: %s\n",
                HantroOmx_str_omx_index(nIndex));

    switch (nIndex)
    {
    case OMX_IndexParamPortDefinition:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *port = NULL;

            OMX_PARAM_PORTDEFINITIONTYPE *param =
                (OMX_PARAM_PORTDEFINITIONTYPE *) pParam;
            switch (param->nPortIndex)
            {
            case PORT_INDEX_INPUT:
                port = &dec->in.def;
                break;
            case PORT_INDEX_OUTPUT:
                port = &dec->out.def;
                break;
            case PORT_INDEX_PP:
                port = &dec->inpp.def;
                break;
            default:
                return OMX_ErrorBadPortIndex;
            }
            assert(param->nSize);
            memcpy(param, port, param->nSize);
        }
        break;

    case OMX_IndexParamImagePortFormat:
        {
            OMX_PARAM_PORTDEFINITIONTYPE *port = NULL;

            OMX_IMAGE_PARAM_PORTFORMATTYPE *param =
                (OMX_IMAGE_PARAM_PORTFORMATTYPE *) pParam;
            switch (param->nPortIndex)
            {
            case PORT_INDEX_INPUT:
                if(param->nIndex >= sizeof(input_formats) / (sizeof(int) * 2))
                    return OMX_ErrorNoMore;

                param->eCompressionFormat = input_formats[param->nIndex][0];
                param->eColorFormat = input_formats[param->nIndex][1];
                break;
            case PORT_INDEX_OUTPUT:
                if(param->nIndex >= sizeof(output_formats) / (sizeof(int) * 2))
                    return OMX_ErrorNoMore;

                param->eCompressionFormat = output_formats[param->nIndex][0];
                param->eColorFormat = output_formats[param->nIndex][1];
                break;
            case PORT_INDEX_PP:
                if(param->nIndex >= 1)
                    return OMX_ErrorNoMore;

                port = &dec->inpp.def;
                param->eCompressionFormat =
                    port->format.image.eCompressionFormat;
                param->eColorFormat = port->format.image.eColorFormat;
                break;
            default:
                return OMX_ErrorBadPortIndex;
            }
        }
        break;

        // these are specified for standard JPEG decoder
#ifdef CONFORMANCE
        case OMX_IndexParamHuffmanTable:
        {
            OMX_IMAGE_PARAM_HUFFMANTTABLETYPE *param =
                (OMX_IMAGE_PARAM_HUFFMANTTABLETYPE *) pParam;
            memcpy(param, &dec->huffman_table, param->nSize);
        }
        break;
        case OMX_IndexParamQuantizationTable:
        {
            OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *param =
                (OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *) pParam;
            memcpy(param, &dec->quant_table, param->nSize);
        }
        break;
#endif

    case OMX_IndexParamCompBufferSupplier:
        {
            OMX_PARAM_BUFFERSUPPLIERTYPE *param =
                (OMX_PARAM_BUFFERSUPPLIERTYPE *) pParam;
            PORT *port = decoder_map_index_to_port(dec, param->nPortIndex);

            if(!port)
                return OMX_ErrorBadPortIndex;
            param->eBufferSupplier = port->tunnel.eSupplier;
        }
        break;

    case OMX_IndexParamStandardComponentRole:
        {
            OMX_PARAM_COMPONENTROLETYPE *param =
                (OMX_PARAM_COMPONENTROLETYPE *) pParam;
            strcpy((char *) param->cRole, (const char *) dec->role);
        }
        break;

    case OMX_IndexParamPriorityMgmt:
        {
            OMX_PRIORITYMGMTTYPE *param = (OMX_PRIORITYMGMTTYPE *) pParam;

            param->nGroupPriority = dec->priority_group;
            param->nGroupID = dec->priority_id;
        }
        break;

    case OMX_IndexParamAudioInit:
    case OMX_IndexParamOtherInit:
    case OMX_IndexParamVideoInit:
        {
            OMX_PORT_PARAM_TYPE *param = (OMX_PORT_PARAM_TYPE *) pParam;

            param->nPorts = 0;
            param->nStartPortNumber = 0;
        }
        break;

    case OMX_IndexParamImageInit:
        {
            OMX_PORT_PARAM_TYPE *param = (OMX_PORT_PARAM_TYPE *) pParam;

            param->nPorts = 3;
            param->nStartPortNumber = 0;
        }
        break;
    default:
        TRACE_PRINT("API: unsupported settings index\n");
        return OMX_ErrorUnsupportedIndex;
    }

    return OMX_ErrorNone;
}

#endif // OMX_DECODER_IMAGE_DOMAIN

static
    OMX_ERRORTYPE decoder_set_config(OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_INDEXTYPE nIndex,
                                     OMX_IN OMX_PTR pParam)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    CHECK_STATE_INVALID(dec->state);

    TRACE_PRINT("API: %s\n", __FUNCTION__);
    TRACE_PRINT("API: config index:%s\n",
                HantroOmx_str_omx_index(nIndex));
    if(dec->state != OMX_StateLoaded && dec->state != OMX_StateIdle)
    {
        // by an agreement with the client. Simplify parameter setting by allowing
        // parameters to be set only in the loaded/idle state.
        // OMX specification does not know about such constraint, but an implementation is allowed to do this.
        TRACE_PRINT("API: unsupported state: %s\n",
                    HantroOmx_str_omx_state(dec->state));
        return OMX_ErrorUnsupportedSetting;
    }

    switch (nIndex)
    {
    case OMX_IndexConfigCommonRotate:
        {
            OMX_CONFIG_ROTATIONTYPE *param = (OMX_CONFIG_ROTATIONTYPE *) pParam;

            memcpy(&dec->conf_rotation, param, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonMirror:
        {
            OMX_CONFIG_MIRRORTYPE *param = (OMX_CONFIG_MIRRORTYPE *) pParam;

            memcpy(&dec->conf_mirror, param, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonContrast:
        {
            OMX_CONFIG_CONTRASTTYPE *param = (OMX_CONFIG_CONTRASTTYPE *) pParam;

            memcpy(&dec->conf_contrast, param, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonBrightness:
        {
            OMX_CONFIG_BRIGHTNESSTYPE *param =
                (OMX_CONFIG_BRIGHTNESSTYPE *) pParam;
            memcpy(&dec->conf_brightness, param, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonSaturation:
        {
            OMX_CONFIG_SATURATIONTYPE *param =
                (OMX_CONFIG_SATURATIONTYPE *) pParam;
            memcpy(&dec->conf_saturation, param, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonPlaneBlend:
        {
            OMX_CONFIG_PLANEBLENDTYPE *param =
                (OMX_CONFIG_PLANEBLENDTYPE *) pParam;
            memcpy(&dec->conf_blend, param, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonDithering:
        {
            OMX_CONFIG_DITHERTYPE *param = (OMX_CONFIG_DITHERTYPE *) pParam;

            memcpy(&dec->conf_dither, param, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonInputCrop:
        {
            OMX_CONFIG_RECTTYPE *param = (OMX_CONFIG_RECTTYPE *) pParam;

            memcpy(&dec->conf_rect, param, param->nSize);
        }
        break;

        // for masking
    case OMX_IndexConfigCommonOutputPosition:
        {
            OMX_CONFIG_POINTTYPE *param = (OMX_CONFIG_POINTTYPE *) pParam;

            memcpy(&dec->conf_mask_offset, param, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonExclusionRect:
        {
            OMX_CONFIG_RECTTYPE *param = (OMX_CONFIG_RECTTYPE *) pParam;

            memcpy(&dec->conf_mask, param, param->nSize);
        }
        break;
    case OMX_IndexExtDSPGRealTimeOptimizations:
#ifdef ANDROID_MOD
        // DSPG: apply realtime optimizations
        dec->in.def.nBufferCountActual = 2;
        dec->out.def.nBufferCountActual = 4;
        dec->dispatchOutputImmediately = OMX_TRUE;
#endif
        break;
    default:
        return OMX_ErrorUnsupportedIndex;
    }
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE decoder_get_config(OMX_IN OMX_HANDLETYPE hComponent,
                                     OMX_IN OMX_INDEXTYPE nIndex,
                                     OMX_INOUT OMX_PTR pParam)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    CHECK_STATE_INVALID(dec->state);
    TRACE_PRINT("API: %s\n", __FUNCTION__);
    TRACE_PRINT("API: config index: %s\n",
                HantroOmx_str_omx_index(nIndex));

    switch (nIndex)
    {
    case OMX_IndexConfigCommonRotate:
        {
            OMX_CONFIG_ROTATIONTYPE *param = (OMX_CONFIG_ROTATIONTYPE *) pParam;

            memcpy(param, &dec->conf_rotation, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonMirror:
        {
            OMX_CONFIG_MIRRORTYPE *param = (OMX_CONFIG_MIRRORTYPE *) pParam;

            memcpy(param, &dec->conf_mirror, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonContrast:
        {
            OMX_CONFIG_CONTRASTTYPE *param = (OMX_CONFIG_CONTRASTTYPE *) pParam;

            memcpy(param, &dec->conf_contrast, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonBrightness:
        {
            OMX_CONFIG_BRIGHTNESSTYPE *param =
                (OMX_CONFIG_BRIGHTNESSTYPE *) pParam;
            memcpy(param, &dec->conf_brightness, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonSaturation:
        {
            OMX_CONFIG_SATURATIONTYPE *param =
                (OMX_CONFIG_SATURATIONTYPE *) pParam;
            memcpy(param, &dec->conf_saturation, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonPlaneBlend:
        {
            OMX_CONFIG_PLANEBLENDTYPE *param =
                (OMX_CONFIG_PLANEBLENDTYPE *) pParam;
            memcpy(param, &dec->conf_blend, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonDithering:
        {
            OMX_CONFIG_DITHERTYPE *param = (OMX_CONFIG_DITHERTYPE *) pParam;

            memcpy(param, &dec->conf_dither, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonInputCrop:
        {
            OMX_CONFIG_RECTTYPE *param = (OMX_CONFIG_RECTTYPE *) pParam;

            memcpy(param, &dec->conf_rect, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonOutputPosition:
        {
            OMX_CONFIG_POINTTYPE *param = (OMX_CONFIG_POINTTYPE *) pParam;

            memcpy(param, &dec->conf_mask_offset, param->nSize);
        }
        break;

    case OMX_IndexConfigCommonExclusionRect:
        {
            OMX_CONFIG_RECTTYPE *param = (OMX_CONFIG_RECTTYPE *) pParam;

            memcpy(param, &dec->conf_mask, param->nSize);
        }
        break;
#ifdef ENABLE_CODEC_VP8
    case OMX_IndexConfigVideoVp8ReferenceFrameType:
        {
            OMX_VIDEO_VP8REFERENCEFRAMEINFOTYPE *param =
                (OMX_VIDEO_VP8REFERENCEFRAMEINFOTYPE *) pParam;
            param->bIsIntraFrame = dec->isIntra;
            param->bIsGoldenOrAlternateFrame = dec->isGoldenOrAlternate;
        }
        break;
#endif
    default:
        return OMX_ErrorUnsupportedIndex;
    }
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE decoder_push_buffer(OMX_HANDLETYPE hComponent,
                                      OMX_BUFFERHEADERTYPE * pBufferHeader,
                                      OMX_U32 portindex)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pBufferHeader);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    CHECK_STATE_INVALID(dec->state);
    TRACE_PRINT("API: header:%p port index:%u\n", pBufferHeader,
                (unsigned) portindex);

    if(dec->state != OMX_StateExecuting && dec->state != OMX_StatePause &&
       dec->state != OMX_StateIdle)
    {
        TRACE_PRINT("API: incorrect decoder state: %s\n",
                    HantroOmx_str_omx_state(dec->state));
        return OMX_ErrorIncorrectStateOperation;
    }

    PORT *port = decoder_map_index_to_port(dec, portindex);

    if(!port)
    {
        TRACE_PRINT("API: no such port\n");
        return OMX_ErrorBadPortIndex;
    }

    // In case of a tunneled port the client will request to disable a port on the buffer supplier,
    // and then on the non-supplier. The non-supplier needs to be able to return the supplied
    // buffer to our queue. So in this case this function will be invoked on a disabled port.
    // Then on the other hand the conformance tester (PortCommunicationTest) tests to see that
    // when a port is disabled it returns an appropriate error when a buffer is sent to it.
    //
    // In IOP/PortDisableEnable test tester disables all of our ports. Then destroys the TTC
    // and creates a new TTC. The new TTC is told to allocate buffers for our output port.
    // The the tester tells the TTC to transit to Executing state, at which point it is trying to
    // initiate buffer trafficing by calling our FillThisBuffer. However at this point the
    // port is still disabled. Doh.
    //
    if(!HantroOmx_port_is_tunneled(port))
    {
        if(!port->def.bEnabled)
        {
            TRACE_PRINT("API: port is disabled\n");
            return OMX_ErrorIncorrectStateOperation;
        }
    }

    // Lock the port's buffer queue here.
    OMX_ERRORTYPE err = HantroOmx_port_lock_buffers(port);

    if(err != OMX_ErrorNone)
    {
        TRACE_PRINT("API: failed to lock port: %s\n",
                    HantroOmx_str_omx_err(err));
        return err;
    }

    BUFFER *buff = HantroOmx_port_find_buffer(port, pBufferHeader);

    if(!buff)
    {
        HantroOmx_port_unlock_buffers(port);
        TRACE_PRINT("API: no such buffer\n");
        return OMX_ErrorBadParameter;
    }

    err = HantroOmx_port_push_buffer(port, buff);
    if(err != OMX_ErrorNone)
        TRACE_PRINT("API: failed to queue buffer: %s\n",
                    HantroOmx_str_omx_err(err));

    // remember to unlock the queue too!
    HantroOmx_port_unlock_buffers(port);
    return err;
}

static
    OMX_ERRORTYPE decoder_fill_this_buffer(OMX_IN OMX_HANDLETYPE hComponent,
                                           OMX_IN OMX_BUFFERHEADERTYPE *
                                           pBufferHeader)
{
    CALLSTACK;
    // this function gives us a databuffer into which store
    // decoded data, i.e. the video frames
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pBufferHeader);

    //OMX_DECODER *dec = GET_DECODER(hComponent);

    TRACE_PRINT("API: %s\n", __FUNCTION__);

    // note: version checks should be made backwards compatible.
    if(pBufferHeader->nSize != sizeof(OMX_BUFFERHEADERTYPE))
    {
        TRACE_PRINT("API: buffer header size mismatch\n");
        return OMX_ErrorBadParameter;
    }
/*    if (pBufferHeader->nVersion.nVersion != HantroOmx_make_int_ver(COMPONENT_VERSION_MAJOR, COMPONENT_VERSION_MINOR))
    {
        TRACE_PRINT("API: buffer header version mismatch\n");
        return OMX_ErrorVersionMismatch;
    }
*/
    return decoder_push_buffer(hComponent, pBufferHeader,
                               pBufferHeader->nOutputPortIndex);
}

static
    OMX_ERRORTYPE decoder_empty_this_buffer(OMX_IN OMX_HANDLETYPE hComponent,
                                            OMX_IN OMX_BUFFERHEADERTYPE *
                                            pBufferHeader)
{
    CALLSTACK;
    // this function gives us an input data buffer from
    // which data is decoded
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pBufferHeader);

    OMX_DECODER *dec = GET_DECODER(hComponent);

    TRACE_PRINT("API: %s\n", __FUNCTION__);

    if( /* pBufferHeader->nFilledLen == 0 || */ pBufferHeader->nFilledLen >
       pBufferHeader->nAllocLen)
    {
        TRACE_PRINT("API: incorrect nFilledLen value: %u nAllocLen: %u\n",
                    (unsigned) pBufferHeader->nFilledLen,
                    (unsigned) pBufferHeader->nAllocLen);
        return OMX_ErrorBadParameter;
    }

    // note: version checks should be backwards compatible.
    if(pBufferHeader->nSize != sizeof(OMX_BUFFERHEADERTYPE))
    {
        TRACE_PRINT("API: buffer header size mismatch\n");
        return OMX_ErrorBadParameter;
    }   /*
         * if (pBufferHeader->nVersion.nVersion != HantroOmx_make_int_ver(COMPONENT_VERSION_MAJOR, COMPONENT_VERSION_MINOR))
         * {
         * TRACE_PRINT("API: buffer header version mismatch\n");
         * return OMX_ErrorVersionMismatch;
         * } */
    TRACE_PRINT("API: nFilledLen:%u nFlags:%x\n",
                (unsigned) pBufferHeader->nFilledLen,
                (unsigned) pBufferHeader->nFlags);

#ifndef DSPG_TIMESTAMP_FIX  // DSPG timestamp fix do it differently
    // queue time stamp, except VP8 format
    if ((dec->in.def.format.video.eCompressionFormat) != OMX_VIDEO_CodingVP8)
    {
#ifdef OMX_SKIP64BIT
        if((pBufferHeader->nTimeStamp.nLowPart != -1) && (pBufferHeader->nTimeStamp.nHighPart != -1))
#else
        if(pBufferHeader->nTimeStamp != -1)
#endif

        {
            receive_timestamp(dec, &pBufferHeader->nTimeStamp);
        }
    }
#endif

#ifdef DSPG_DUMP_STREAM
    if (dec->streamDumpFd > 0) {
        write(dec->streamDumpFd, pBufferHeader->pBuffer, pBufferHeader->nFilledLen);
    }
#endif

    return decoder_push_buffer(hComponent, pBufferHeader,
                               pBufferHeader->nInputPortIndex);
}

static
    OMX_ERRORTYPE decoder_component_tunnel_request(OMX_IN OMX_HANDLETYPE
                                                   hComponent,
                                                   OMX_IN OMX_U32 nPort,
                                                   OMX_IN OMX_HANDLETYPE
                                                   hTunneledComp,
                                                   OMX_IN OMX_U32 nTunneledPort,
                                                   OMX_INOUT OMX_TUNNELSETUPTYPE
                                                   * pTunnelSetup)
{
    CALLSTACK;
    CHECK_PARAM_NON_NULL(hComponent);
    OMX_DECODER *dec = GET_DECODER(hComponent);

    TRACE_PRINT("API: %s\n", __FUNCTION__);
    OMX_ERRORTYPE err = OMX_ErrorNone;

    PORT *port = decoder_map_index_to_port(dec, nPort);

    if(port == NULL)
    {
        TRACE_PRINT("API: bad port index:%d\n", (int) nPort);
        return OMX_ErrorBadPortIndex;
    }
    if(dec->state != OMX_StateLoaded && port->def.bEnabled)
    {
        TRACE_PRINT("API: port is not disabled\n");
        return OMX_ErrorIncorrectStateOperation;
    }

    TRACE_PRINT("API: setting up tunnel on port: %d\n", (int) nPort);
    TRACE_PRINT("API: tunnel component:%p tunnel port:%d\n",
                hTunneledComp, (int) nTunneledPort);

    if(hTunneledComp == NULL)
    {
        HantroOmx_port_setup_tunnel(port, NULL, 0, OMX_BufferSupplyUnspecified);
        return OMX_ErrorNone;
    }
#ifndef OMX_DECODER_TUNNELING_SUPPORT
    TRACE_PRINT("API: ERROR Tunneling unsupported\n");
    return OMX_ErrorTunnelingUnsupported;
#endif
    CHECK_PARAM_NON_NULL(pTunnelSetup);
    if(port->def.eDir == OMX_DirOutput)
    {
        // 3.3.11
        // the component that provides the output port of the tunneling has to do the following:
        // 1. Indicate its supplier preference in pTunnelSetup.
        // 2. Set the OMX_PORTTUNNELFLAG_READONLY flag to indicate that buffers
        //    from this output port are read-only and that the buffers cannot be shared
        //    through components or modified.

        // do not overwrite if something has been specified with SetParameter
        if(port->tunnel.eSupplier == OMX_BufferSupplyUnspecified)
            port->tunnel.eSupplier = OMX_BufferSupplyOutput;

        // if the component that provides the input port
        // wants to override the buffer supplier setting it will call our SetParameter
        // to override the setting put here.
        pTunnelSetup->eSupplier = port->tunnel.eSupplier;
        pTunnelSetup->nTunnelFlags = OMX_PORTTUNNELFLAG_READONLY;
        HantroOmx_port_setup_tunnel(port, hTunneledComp, nTunneledPort,
                                    port->tunnel.eSupplier);
        return OMX_ErrorNone;
    }
    else
    {
        // the input port is responsible for checking that the
        // ports are compatible
        // so get the portdefinition from the output port
        OMX_PARAM_PORTDEFINITIONTYPE other;

        memset(&other, 0, sizeof(other));
        INIT_OMX_VERSION_PARAM(other);
        other.nPortIndex = nTunneledPort;
        err =
            ((OMX_COMPONENTTYPE *) hTunneledComp)->GetParameter(hTunneledComp,
                                                                OMX_IndexParamPortDefinition,
                                                                &other);
        if(err != OMX_ErrorNone)
            return err;

        // next do the port compatibility checking
        if(port->def.eDomain != other.eDomain)
        {
            TRACE_PRINT("API: ports are not compatible (incompatible domains)\n");
            return OMX_ErrorPortsNotCompatible;
        }
        if(port == &dec->in)
        {
#ifdef OMX_DECODER_VIDEO_DOMAIN
            switch (other.format.video.eCompressionFormat)
            {
            case OMX_VIDEO_CodingAVC:
                break;
            case OMX_VIDEO_CodingH263:
                break;
            case OMX_VIDEO_CodingMPEG4:
                break;
            case OMX_VIDEO_CodingSORENSON:
                break;
            case OMX_VIDEO_CodingCUSTOM_1:
                break;
            case OMX_VIDEO_CodingCUSTOM_1_3:
                break;
            case OMX_VIDEO_CodingAVS:
                break;
            case OMX_VIDEO_CodingVP8:
                break;
            case OMX_VIDEO_CodingRV:
                break;
            case OMX_VIDEO_CodingWMV:
                break;
            case OMX_VIDEO_CodingMPEG2:
                break;
            case OMX_VIDEO_CodingVP6:
                break;
            case OMX_VIDEO_CodingMJPEG:
                break;
            case OMX_VIDEO_CodingUnused:
                switch (other.format.video.eColorFormat)
                {
                case OMX_COLOR_FormatYUV420PackedPlanar:
                    break;
                case OMX_COLOR_FormatYUV420Planar:
                    break;
                case OMX_COLOR_FormatYUV420PackedSemiPlanar:
                    break;
                case OMX_COLOR_FormatYUV420SemiPlanar:
                    break;
                case OMX_COLOR_FormatYCbYCr:
                    break;
#if defined (IS_8190) || defined (IS_G1_DECODER)
                case OMX_COLOR_FormatYCrYCb:
                    break;
                case OMX_COLOR_FormatCbYCrY:
                    break;
                case OMX_COLOR_FormatCrYCbY:
                    break;
#endif
                default:
                    TRACE_PRINT("API: ports are not compatible (incompatible color format)\n");
                    return OMX_ErrorPortsNotCompatible;
                }
                break;
            default:
                TRACE_PRINT("API: ports are not compatible (incompatible video coding)\n");
                return OMX_ErrorPortsNotCompatible;
            }
#endif
#ifdef OMX_DECODER_IMAGE_DOMAIN
            switch (other.format.image.eCompressionFormat)
            {
            case OMX_IMAGE_CodingJPEG:
                break;
            case OMX_IMAGE_CodingWEBP:
                break;
            case OMX_IMAGE_CodingUnused:
                break;
                switch (other.format.image.eColorFormat)
                {
                case OMX_COLOR_FormatYUV420PackedPlanar:
                    break;
                case OMX_COLOR_FormatYUV420Planar:
                    break;
                case OMX_COLOR_FormatYUV420PackedSemiPlanar:
                    break;
                case OMX_COLOR_FormatYUV420SemiPlanar:
                    break;
                case OMX_COLOR_FormatYCbYCr:
                    break;
#if defined (IS_8190) || defined (IS_G1_DECODER)
                case OMX_COLOR_FormatYCrYCb:
                    break;
                case OMX_COLOR_FormatCbYCrY:
                    break;
                case OMX_COLOR_FormatCrYCbY:
                    break;
#endif
                default:
                    TRACE_PRINT("API: ports are not compatible (incompatible color format)\n");
                    return OMX_ErrorPortsNotCompatible;
                }
                break;
            default:
                TRACE_PRINT("ASYNC: ports are not compatible (incompatible image coding)\n");
                return OMX_ErrorPortsNotCompatible;
            }
#endif
        }
        if(port == &dec->inpp)
        {
            // was there any other post-processor input formats?
            if(other.format.image.eCompressionFormat != OMX_IMAGE_CodingUnused
               || other.format.image.eColorFormat !=
               OMX_COLOR_Format32bitARGB8888)
            {
                TRACE_PRINT("API: ports are not compatible (post-processor)\n");
                return OMX_ErrorPortsNotCompatible;
            }
        }
        if(pTunnelSetup->eSupplier == OMX_BufferSupplyUnspecified)
            pTunnelSetup->eSupplier = OMX_BufferSupplyInput;

        // need to send back the result of the buffer supply negotiation
        // to the component providing the output port.
        OMX_PARAM_BUFFERSUPPLIERTYPE param;

        memset(&param, 0, sizeof(param));
        INIT_OMX_VERSION_PARAM(param);

        param.eBufferSupplier = pTunnelSetup->eSupplier;
        param.nPortIndex = nTunneledPort;
        err =
            ((OMX_COMPONENTTYPE *) hTunneledComp)->SetParameter(hTunneledComp,
                                                                OMX_IndexParamCompBufferSupplier,
                                                                &param);
        if(err != OMX_ErrorNone)
            return err;

        // save tunneling details somewhere
        HantroOmx_port_setup_tunnel(port, hTunneledComp, nTunneledPort,
                                    pTunnelSetup->eSupplier);
        TRACE_PRINT("API: tunnel supplier: %s\n",
                    HantroOmx_str_omx_supplier(pTunnelSetup->eSupplier));
    }
    return OMX_ErrorNone;
}

//DEFINE_STUB(decoder_get_extension_index);

static OMX_ERRORTYPE decoder_get_extension_index(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_STRING cParameterName,
        OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    if (!strcmp(cParameterName, "OMX.hantro.G1.index.RealTimeMode")) {
        *pIndexType = OMX_IndexExtDSPGRealTimeOptimizations;
        return OMX_ErrorNone;
    }

    return OMX_ErrorNotImplemented;
}

//DEFINE_STUB(decoder_useegl_image);
OMX_ERRORTYPE decoder_useegl_image(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, void* pBuffer)
{
    UNUSED_PARAMETER(hComponent);
    UNUSED_PARAMETER(ppBufferHdr);
    UNUSED_PARAMETER(nPortIndex);
    UNUSED_PARAMETER(pAppPrivate);
    UNUSED_PARAMETER(pBuffer);

    return OMX_ErrorNotImplemented;
}

//DEFINE_STUB(decoder_enum_roles);
static OMX_ERRORTYPE decoder_enum_roles(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex)
{
    UNUSED_PARAMETER(hComponent);
    UNUSED_PARAMETER(cRole);
    UNUSED_PARAMETER(nIndex);

    return OMX_ErrorNotImplemented;
}

#ifdef OMX_DECODER_VIDEO_DOMAIN
OMX_ERRORTYPE HantroHwDecOmx_video_constructor(OMX_COMPONENTTYPE * comp,
                                               OMX_STRING name)
#endif
#ifdef OMX_DECODER_IMAGE_DOMAIN
    OMX_ERRORTYPE HantroHwDecOmx_image_constructor(OMX_COMPONENTTYPE * comp,
                                                   OMX_STRING name)
#endif
{
    CALLSTACK;

    CHECK_PARAM_NON_NULL(comp);
    CHECK_PARAM_NON_NULL(name);

    assert(comp->pComponentPrivate == 0);

#ifdef OMX_DECODER_VIDEO_DOMAIN
#ifdef ANDROID_MOD
    /* Check that the prefix is correct */
    if(strncmp(name, COMPONENT_NAME_VIDEO, strlen(COMPONENT_NAME_VIDEO)))
        return OMX_ErrorInvalidComponentName;

    /* Check for the specific name and set the decoder according to it */
    char *specificName = name + strlen(COMPONENT_NAME_VIDEO);
    TRACE_PRINT("specific component name = %s", specificName);

#else /* old code from on2 */
    if(strcmp(name, COMPONENT_NAME_VIDEO))
        return OMX_ErrorInvalidComponentName;
#endif
#else
    if(strcmp(name, COMPONENT_NAME_IMAGE))
        return OMX_ErrorInvalidComponentName;
#endif

    // work around an issue in the conformance tester.
    // In the ResourceExhaustion test the conformance tester tries to create
    // components more than the system can handle. However if it is the constructor
    // that fails instead of the Idle state transition,
    // (the component is never actually created but only the handle IS created)
    // the tester passes the component to the core for cleaning up which then just ends up calling the DeInit function
    // on an object that hasn't ever even been constructed.
    // To work around this problem we set the DeInit function pointer here and then in the DeInit function
    // check if the pComponentPrivate is set to a non NULL value. (i.e. has the object really been created). Lame.
    //
    comp->ComponentDeInit = decoder_deinit;

#ifdef INCLUDE_TB
    TBSetDefaultCfg(&tbCfg);
/* TODO 8170 testing can be implemented here */
/*    tbCfg.decParams.h264Support = 1;
    tbCfg.decParams.hwVersion = 8170;*/
    tbCfg.decParams.hwVersion = 10000;

#endif

    OMX_DECODER *dec = (OMX_DECODER *) OSAL_Malloc(sizeof(OMX_DECODER));

    if(dec == 0)
        return OMX_ErrorInsufficientResources;
    memset(dec, 0, sizeof(OMX_DECODER));

    dec->frame_out.bus_address = 0;

#if 0//#ifdef OMX_DECODER_TRACE
    char log_file_name[256];

    memset(log_file_name, 0, sizeof(log_file_name));
    sprintf(log_file_name, "%d", (unsigned int) comp);
    strcat(log_file_name, ".log");
    dec->log = fopen(log_file_name, "w");
    if(dec->log == 0)
    {
        OSAL_Free(dec);
        return OMX_ErrorUndefined;
    }
    if(dec->log)
        setvbuf(dec->log, 0, _IONBF, 0);

    //dec->log = stdout;
#endif
    OMX_ERRORTYPE err = OMX_ErrorNone;

#ifdef HANTRO_TESTBENCH
    err = HantroOmx_port_init(&dec->in, 4, 4, 5, 262144);
#else
    err = HantroOmx_port_init(&dec->in, 2, 4, 4, 262144);
#endif
    if(err != OMX_ErrorNone)
        goto INIT_FAILURE;

    // these hacks are for the conformance tester.
#if (defined OMX_DECODER_VIDEO_DOMAIN)
    TRACE_PRINT("port init");
#ifdef HANTRO_TESTBENCH
    err = HantroOmx_port_init(&dec->out, 2, 2, 5, 152064);
#else
    // the size of the buffers below is just dummy,
    // it will be configured later according to the max screen resolution
    err = HantroOmx_port_init(&dec->out, 3, 6, 6, 1382400);
#endif
#endif
    
#ifdef OMX_DECODER_IMAGE_DOMAIN
    err = HantroOmx_port_init(&dec->out, 4, 4, 5, 1569792);
#endif
    if(err != OMX_ErrorNone)
        goto INIT_FAILURE;

    err = HantroOmx_port_init(&dec->inpp, 1, 1, 1, 4);
    if(err != OMX_ErrorNone)
        goto INIT_FAILURE;

    err = OSAL_AllocatorInit(&dec->alloc);
    if(err != OMX_ErrorNone)
        goto INIT_FAILURE;

    INIT_OMX_VERSION_PARAM(dec->in.def);
    INIT_OMX_VERSION_PARAM(dec->out.def);
    INIT_OMX_VERSION_PARAM(dec->inpp.def);

    dec->state = OMX_StateLoaded;
    dec->statetrans = OMX_StateLoaded;
    dec->run = OMX_TRUE;
    dec->portReconfigPending = OMX_FALSE;
    dec->self = comp;   // hold a backpointer to the component handle.
    // callback interface requires this

    dec->in.def.nPortIndex = PORT_INDEX_INPUT;
    dec->in.def.eDir = OMX_DirInput;
    dec->in.def.bEnabled = OMX_TRUE;
    dec->in.def.bPopulated = OMX_FALSE;

    dec->out.def.nPortIndex = PORT_INDEX_OUTPUT;
    dec->out.def.eDir = OMX_DirOutput;
    dec->out.def.bEnabled = OMX_TRUE;
    dec->out.def.bPopulated = OMX_FALSE;


#ifdef OMX_DECODER_VIDEO_DOMAIN
    if(specificName == NULL || strcmp(specificName,".avc")==0) {
        TRACE_PRINT("Creating h264 decoder");
        strcpy((char *) dec->role, "video_decoder.avc");
        dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    } 
    else if(strcmp(specificName,".mpeg4") == 0) {
        TRACE_PRINT("Creating mpeg4 decoder");
        strcpy((char *) dec->role, "video_decoder.mpeg4");
        dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
    }
    else if(strcmp(specificName,".h263") == 0) {
        TRACE_PRINT("Creating h263 decoder");
        strcpy((char *) dec->role, "video_decoder.h263");
        dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
    }
    else if(strcmp(specificName,".vp8") == 0) {
        TRACE_PRINT("Creating vp8 decoder");
        strcpy((char *) dec->role, "video_decoder.vp8");
        dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingVP8;
    }
    else if (strcmp(specificName, ".vp6") == 0) {
        TRACE_PRINT("Creating vp6 decoder");
        strcpy((char *) dec->role, "video_decoder.vp6");
        dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingVP6;
    }
    else if (strcmp(specificName, ".avs") == 0) {
        TRACE_PRINT("Creating avs decoder");
        strcpy((char *) dec->role, "video_decoder.avs");
        dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVS;
    }
    else if (strcmp(specificName, ".rv") == 0) {
        TRACE_PRINT("Creating rv decoder");
        strcpy((char *) dec->role, "video_decoder.rv");
        dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingRV;
    }
    else if (strcmp(specificName, ".mpeg2") == 0) {
        TRACE_PRINT("Creating mpeg2 decoder");
        strcpy((char *) dec->role, "video_decoder.mpeg2");
        dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG2;
    }
    else if (strcmp(specificName, ".wmv") == 0) {
        TRACE_PRINT("Creating vc1 decoder");
        strcpy((char *) dec->role, "video_decoder.wmv");
        dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
    }
    else {
        TRACE_PRINT("Creating h264 decoder");
        strcpy((char *) dec->role, "video_decoder.avc");
        dec->in.def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    }


    dec->in.def.eDomain = OMX_PortDomainVideo;
    dec->in.def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    dec->in.def.format.video.nFrameWidth = 0;
    dec->in.def.format.video.nFrameHeight = 0;
    dec->in.def.format.video.nBitrate = 0;
    dec->in.def.format.video.xFramerate = 0;
    // conformance tester wants to check these in standard component tests
#ifdef CONFORMANCE
    dec->in.def.format.video.nFrameWidth         = 176;
    dec->in.def.format.video.nFrameHeight        = 144;
    dec->in.def.format.video.nBitrate            = 64000;
    dec->in.def.format.video.xFramerate          = 15 << 16;
#endif

    dec->out.def.eDomain = OMX_PortDomainVideo;
    dec->out.def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    dec->out.def.format.video.eColorFormat =
        OMX_COLOR_FormatYUV420PackedSemiPlanar;
    dec->out.def.format.video.nFrameWidth = 0;
    dec->out.def.format.video.nFrameHeight = 0;
    dec->out.def.format.video.nStride = -1;
    dec->out.def.format.video.nSliceHeight = -1;
#ifdef CONFORMANCE
    dec->out.def.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    dec->out.def.format.video.nFrameWidth = 176;
    dec->out.def.format.video.nFrameHeight = 144;
    dec->out.def.format.video.nStride = 176;
    dec->out.def.format.video.nSliceHeight = 144;
#endif
    dec->WMVFormat = OMX_VIDEO_WMVFormat9; /* for conformance tester */
#endif
#ifdef OMX_DECODER_IMAGE_DOMAIN
    strcpy((char *) dec->role, "image_decoder.jpeg");

    dec->in.def.eDomain = OMX_PortDomainImage;
    dec->in.def.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
    dec->in.def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    dec->in.def.format.image.nFrameWidth = 640;
    dec->in.def.format.image.nFrameHeight = 480;

    dec->out.def.eDomain = OMX_PortDomainImage;
    dec->out.def.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
    dec->out.def.format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedSemiPlanar;
    dec->out.def.format.image.nFrameWidth = 0;
    dec->out.def.format.image.nFrameHeight = 0;
    dec->out.def.format.image.nStride = -1;
    dec->out.def.format.image.nSliceHeight = -1;
#ifdef CONFORMANCE
    dec->out.def.format.image.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    dec->out.def.format.image.nFrameWidth = 640;
    dec->out.def.format.image.nFrameHeight = 480;
    dec->out.def.format.image.nStride = 640;
    dec->out.def.format.image.nSliceHeight = 16;
#endif
    
#endif // OMX_DECODER_IMAGE_DOMAIN

    dec->inpp.def.nPortIndex = PORT_INDEX_PP;
    dec->inpp.def.eDir = OMX_DirInput;
#ifdef ANDROID_MOD
    dec->inpp.def.bEnabled = OMX_FALSE;
#else
    dec->inpp.def.bEnabled = OMX_TRUE;
#endif
    dec->inpp.def.bPopulated = OMX_FALSE;
    dec->inpp.def.eDomain = OMX_PortDomainImage;
    dec->inpp.def.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
    dec->inpp.def.format.image.eColorFormat = OMX_COLOR_Format32bitARGB8888;

    // set the component interfarce functions.
    // here one could use the nVersion of the OMX_COMPONENTTYPE
    // to set these functions specifially for OpenMAX 1.0 version.
    comp->GetComponentVersion = decoder_get_version;
    comp->SendCommand = decoder_send_command;
    comp->GetParameter = decoder_get_parameter;
    comp->SetParameter = decoder_set_parameter;
    comp->SetCallbacks = decoder_set_callbacks;
    comp->GetConfig = decoder_get_config;
    comp->SetConfig = decoder_set_config;
    comp->GetExtensionIndex = decoder_get_extension_index;
    comp->GetState = decoder_get_state;
    comp->ComponentTunnelRequest = decoder_component_tunnel_request;
    comp->UseBuffer = decoder_use_buffer;
    comp->AllocateBuffer = decoder_allocate_buffer;
    comp->FreeBuffer = decoder_free_buffer;
    comp->EmptyThisBuffer = decoder_empty_this_buffer;
    comp->FillThisBuffer = decoder_fill_this_buffer;
    comp->ComponentDeInit = decoder_deinit;
    comp->UseEGLImage = decoder_useegl_image;
    comp->ComponentRoleEnum = decoder_enum_roles;
    comp->pComponentPrivate = dec;

#ifdef DYNAMIC_SCALING
	/* create fifo for scaler setup messages */
	int ret;
	if ((ret = mkfifo("/tmp/decoder-scaler", O_NONBLOCK | 0666)) != 0) {
		if (ret == EEXIST) {
			unlink("/tmp/decoder-scaler");
			if ((ret = mkfifo("/tmp/decoder-scaler", O_NONBLOCK | 0666)) != 0) {
				TRACE_PRINT("error: cannot open scaler fifo file (%d)\n", ret);
				goto INIT_FAILURE;
			}
		}
	}

	if ((dec->scalerFd = open("/tmp/decoder-scaler", O_RDONLY | O_NONBLOCK)) < 0) {
		TRACE_PRINT("error: cannot open scaler fifo\n");
		goto INIT_FAILURE;
	}
#endif

#ifdef ALIGN_AND_DECREASE_OUTPUT
    /* get the current screen size */
    struct fb_var_screeninfo info;
    dec->screenWidth = 0;
    dec->screenHeight = 0;

    int fb0_fd = open("/dev/graphics/fb0", O_RDONLY);
    if (fb0_fd > 0) {
        if (ioctl(fb0_fd, FBIOGET_VSCREENINFO, &info) >= 0) {
            /* always treat the big scalar as the width */
            if (info.xres > info.yres) {
                dec->screenWidth = info.xres;
                dec->screenHeight = info.yres;
            }
            else {
                dec->screenWidth = info.yres;
                dec->screenHeight = info.xres;
            }

            // optimize output buffers size according to max screen size
            // we assume here that the post-processor will be
            // configured later (in the function 'transition_to_idle_from_loaded')
            // to scale down the video to the screen size.
            dec->out.def.nBufferSize = dec->screenWidth * dec->screenHeight * 3 / 2 + 4096;

        }
        close(fb0_fd);
    }
#endif

#ifdef DSPG_TIMESTAMP_FIX
    dec->timestampsQueueHead = 0;
    dec->timestampsQueueTail = 0;
#endif

#ifdef ANDROID_MOD
    dec->outputPortFlushPending = OMX_FALSE;
    dec->dispatchOutputImmediately = OMX_FALSE;
#endif

#ifdef DSPG_DUMP_STREAM
    dec->streamDumpFd = open("/tmp/input.stream", O_WRONLY | O_CREAT);
#endif

    // this needs to be the last thing to be done in the code
    // cause this will create the component thread which will access the
    // decoder object we set up above. So, we better make sure that the object
    // is fully constructed.
    // note: could set the interface pointers to zero or something should this fail
    err = HantroOmx_basecomp_init(&dec->base, decoder_thread_main, dec);
    if(err != OMX_ErrorNone)
    {
        TRACE_PRINT("basecomp_init failed\n");
        goto INIT_FAILURE;
    }
    return OMX_ErrorNone;

  INIT_FAILURE:
    assert(dec);
    if(dec->log)
        fclose(dec->log);
    TRACE_PRINT("%s %s\n", "init failure",
                HantroOmx_str_omx_err(err));
#ifndef CONFORMANCE // conformance tester calls deinit
    if(HantroOmx_port_is_allocated(&dec->in))
        HantroOmx_port_destroy(&dec->in);
    if(HantroOmx_port_is_allocated(&dec->out))
        HantroOmx_port_destroy(&dec->out);
    if(HantroOmx_port_is_allocated(&dec->inpp))
        HantroOmx_port_destroy(&dec->inpp);
    //HantroOmx_basecomp_destroy(&dec->base);
    if(OSAL_AllocatorIsReady(&dec->alloc))
        OSAL_AllocatorDestroy(&dec->alloc);
    OSAL_Free(dec);
#endif
    return err;
}

static OMX_ERRORTYPE supply_tunneled_port(OMX_DECODER * dec, PORT * port)
{
    CALLSTACK;
    assert(port->tunnelcomp);
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT("ASYNC: supplying buffers for: %p (%d)\n",
                port->tunnelcomp, (int) port->tunnelport);

    OMX_ERRORTYPE err = OMX_ErrorNone;

    OMX_PARAM_PORTDEFINITIONTYPE param;

    memset(&param, 0, sizeof(param));
    INIT_OMX_VERSION_PARAM(param);
    param.nPortIndex = port->tunnelport;
    // get the port definition, cause we need the number of buffers
    // that we need to allocate for this port
    err =
        ((OMX_COMPONENTTYPE *) port->tunnelcomp)->GetParameter(port->tunnelcomp,
                                                               OMX_IndexParamPortDefinition,
                                                               &param);
    if(err != OMX_ErrorNone)
        return err;

    // this operation should be fine without locking.
    // there's no access to the supply queue through the public API,
    // so the component thread is the only thread doing the access here.

    // 2.1.7.2
    // 2. Allocate buffers according to the maximum of its own requirements and the
    //    requirements of the tunneled port.

    OMX_U32 count =
        param.nBufferCountActual >
        port->def.nBufferCountActual ? param.nBufferCountActual : port->def.
        nBufferCountActual;
    OMX_U32 size =
        param.nBufferSize >
        port->def.nBufferSize ? param.nBufferSize : port->def.nBufferSize;
    TRACE_PRINT("ASYNC: allocating %d buffers\n", (int) count);

    OMX_U32 i = 0;

    for(i = 0; i < count; ++i)
    {
        OMX_U8 *bus_data = NULL;

        OSAL_BUS_WIDTH bus_address = 0;

        OMX_U32 allocsize = size;

        // allocate the memory chunk for the buffer
        TRACE_PRINT("supply_tunneled_port: OSAL_AllocatorAllocMem 1\n");

        err =
            OSAL_AllocatorAllocMem(&dec->alloc, &allocsize, &bus_data,
                                   &bus_address);
        if(err != OMX_ErrorNone)
        {
            TRACE_PRINT("ASYNC: failed to supply buffer (%d bytes)\n",
                        (int) param.nBufferSize);
            goto FAIL;
        }

        // allocate the BUFFER object
        BUFFER *buff = NULL;

        HantroOmx_port_allocate_next_buffer(port, &buff);
        if(buff == NULL)
        {
            TRACE_PRINT("ASYNC: failed to supply buffer object\n");
            OSAL_AllocatorFreeMem(&dec->alloc, allocsize, bus_data,
                                  bus_address);
            goto FAIL;
        }
        buff->flags |= BUFFER_FLAG_MY_BUFFER;
        buff->bus_data = bus_data;
        buff->bus_address = bus_address;
        buff->allocsize = allocsize;
        buff->header->pBuffer = bus_data;
        buff->header->pAppPrivate = NULL;
        buff->header->nAllocLen = size;
        // the header will remain empty because the
        // tunneled port allocates it.
        buff->header = NULL;
        err =
            ((OMX_COMPONENTTYPE *) port->tunnelcomp)->UseBuffer(port->
                                                                tunnelcomp,
                                                                &buff->header,
                                                                port->
                                                                tunnelport,
                                                                NULL, allocsize,
                                                                bus_data);

        if(err != OMX_ErrorNone)
        {
            TRACE_PRINT("ASYNC: use buffer call failed on tunneled component:%s\n",
                        HantroOmx_str_omx_err(err));
            goto FAIL;
        }
        // the tunneling component is responsible for allocating the
        // buffer header and filling in the buffer information.
        assert(buff->header);
        assert(buff->header != &buff->headerdata);
        assert(buff->header->nSize);
        assert(buff->header->nVersion.nVersion);
        assert(buff->header->nAllocLen);

        TRACE_PRINT("ASYNC: supplied buffer data virtual address:%p size:%d header:%p\n",
                    bus_data, (int) allocsize, buff->header);
    }
    assert(HantroOmx_port_buffer_count(port) >= port->def.nBufferCountActual);
    TRACE_PRINT("ASYNC: port is populated\n");
    port->def.bPopulated = OMX_TRUE;
    return OMX_ErrorNone;
  FAIL:
    TRACE_PRINT("ASYNC: freeing allready supplied buffers\n");
    // must free any buffers we have allocated
    count = HantroOmx_port_buffer_count(port);
    for(i = 0; i < count; ++i)
    {
        BUFFER *buff = NULL;

        HantroOmx_port_get_allocated_buffer_at(port, &buff, i);
        assert(buff);
        assert(buff->bus_data);
        assert(buff->bus_address);

        if(buff->header)
            ((OMX_COMPONENTTYPE *) port->tunnelcomp)->FreeBuffer(port->
                                                                 tunnelcomp,
                                                                 port->
                                                                 tunnelport,
                                                                 buff->header);

        OSAL_AllocatorFreeMem(&dec->alloc, buff->allocsize, buff->bus_data,
                              buff->bus_address);
    }
    HantroOmx_port_release_all_allocated(port);
    return err;
}

static OMX_ERRORTYPE unsupply_tunneled_port(OMX_DECODER * dec, PORT * port)
{
    CALLSTACK;
    assert(port->tunnelcomp);
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT("ASYNC: removing buffers from: %p (%d)\n",
                port->tunnelcomp, (int) port->tunnelport);

    // tell the non-supplier to release them buffers
    OMX_U32 count = HantroOmx_port_buffer_count(port);

    OMX_U32 i;

    for(i = 0; i < count; ++i)
    {
        BUFFER *buff = NULL;

        HantroOmx_port_get_allocated_buffer_at(port, &buff, i);
        assert(buff);
        assert(buff->bus_data);
        assert(buff->bus_address);
        assert(buff->header != &buff->headerdata);
        ((OMX_COMPONENTTYPE *) port->tunnelcomp)->FreeBuffer(port->tunnelcomp,
                                                             port->tunnelport,
                                                             buff->header);
        OSAL_AllocatorFreeMem(&dec->alloc, buff->allocsize, buff->bus_data,
                              buff->bus_address);
    }
    HantroOmx_port_release_all_allocated(port);
    port->def.bPopulated = OMX_FALSE;

    // since we've allocated the buffers, and they have been
    // destroyed empty the port's buffer queue
    OMX_BOOL loop = OMX_TRUE;

    while(loop)
    {
        loop = HantroOmx_port_pop_buffer(port);
    }

    if(port == &dec->out)
        dec->buffer = NULL;

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE transition_to_idle_from_loaded(OMX_DECODER * dec)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    // the state transition cannot complete untill all
    // enabled ports are populated and the component has acquired
    // all of its static resources.
    assert(dec->state == OMX_StateLoaded);
    assert(dec->statetrans == OMX_StateIdle);
    assert(dec->codec == NULL);
    assert(dec->frame_in.bus_data == NULL);
    assert(dec->ts_buf.ts_data== NULL);
    assert(dec->frame_out.bus_data == NULL);
    assert(dec->mask.bus_data == NULL);

    OMX_ERRORTYPE err = OMX_ErrorHardware;

    TRACE_PRINT("ASYNC: input port 0 is tunneled with: %p port: %d supplier:%s\n",
                dec->in.tunnelcomp, (int) dec->in.tunnelport,
                HantroOmx_str_omx_supplier(dec->in.tunnel.eSupplier));

    TRACE_PRINT("ASYNC: input port 2 is tunneled with: %p port: %d supplier:%s\n",
                dec->inpp.tunnelcomp, (int) dec->inpp.tunnelport,
                HantroOmx_str_omx_supplier(dec->inpp.tunnel.eSupplier));

    TRACE_PRINT("ASYNC: output port 1 is tunneled with: %p port: %d supplier:%s\n",
                dec->out.tunnelcomp, (int) dec->out.tunnelport,
                HantroOmx_str_omx_supplier(dec->out.tunnel.eSupplier));

    if(HantroOmx_port_is_supplier(&dec->in))
        if(supply_tunneled_port(dec, &dec->in) != OMX_ErrorNone)
            goto FAIL;

    if(HantroOmx_port_is_supplier(&dec->out))
        if(supply_tunneled_port(dec, &dec->out) != OMX_ErrorNone)
            goto FAIL;

    if(HantroOmx_port_is_supplier(&dec->inpp))
        if(supply_tunneled_port(dec, &dec->inpp) != OMX_ErrorNone)
            goto FAIL;

    TRACE_PRINT("ASYNC: waiting for buffers now!\n");

    while(!HantroOmx_port_is_ready(&dec->in) ||
          !HantroOmx_port_is_ready(&dec->out) /*||
          !HantroOmx_port_is_ready(&dec->inpp)*/)
    {
		TRACE_PRINT("Waiting for input and output buffers");
        OSAL_ThreadSleep(RETRY_INTERVAL);
    }
    TRACE_PRINT("ASYNC: got all buffers\n");

#if (defined OMX_DECODER_VIDEO_DOMAIN)
    switch (dec->in.def.format.video.eCompressionFormat)
    {
#ifdef ENABLE_CODEC_H264
    case OMX_VIDEO_CodingAVC:
        strcpy((char *) dec->role, "video_decoder.avc");
        dec->codec =
            HantroHwDecOmx_decoder_create_h264(dec->in.def.format.video.
                                               bFlagErrorConcealment);
        TRACE_PRINT("ASYNC: created h264 codec\n");
        break;
#endif

#ifdef ENABLE_CODEC_MPEG4
    case OMX_VIDEO_CodingH263:
        strcpy((char *) dec->role, "video_decoder.h263");
        dec->codec =
            HantroHwDecOmx_decoder_create_mpeg4(dec->deblock,
                                                MPEG4FORMAT_MPEG4);
        TRACE_PRINT("ASYNC: created h263 codec\n");
        break;
    case OMX_VIDEO_CodingMPEG4:
        strcpy((char *) dec->role, "video_decoder.mpeg4");
        dec->codec =
            HantroHwDecOmx_decoder_create_mpeg4(dec->deblock,
                                                MPEG4FORMAT_MPEG4);
        TRACE_PRINT("ASYNC: created mpeg4 codec\n");
        break;
    case OMX_VIDEO_CodingSORENSON:
        strcpy((char *) dec->role, "video_decoder.mpeg4");
        dec->codec =
            HantroHwDecOmx_decoder_create_mpeg4(dec->deblock,
                                                MPEG4FORMAT_SORENSON);
        TRACE_PRINT("ASYNC: created Sorenson codec\n");
        break;
#endif

#ifdef ENABLE_CODEC_CUSTOM_1
    case OMX_VIDEO_CodingCUSTOM_1:
        strcpy((char *) dec->role, "video_decoder.mpeg4");
        dec->codec =
            HantroHwDecOmx_decoder_create_mpeg4(dec->deblock,
                                                MPEG4FORMAT_CUSTOM_1);
        TRACE_PRINT("ASYNC: created Custom 1 codec\n");
        break;
    case OMX_VIDEO_CodingCUSTOM_1_3:
        strcpy((char *) dec->role, "video_decoder.mpeg4");
        dec->codec =
            HantroHwDecOmx_decoder_create_mpeg4(dec->deblock,
                                                MPEG4FORMAT_CUSTOM_1_3);
        TRACE_PRINT("ASYNC: created Custom 1 Profile 3 codec\n");
        break;
#endif

#ifdef ENABLE_CODEC_RV
    case OMX_VIDEO_CodingRV:
        strcpy((char *) dec->role, "video_decoder.rv");
        dec->codec =
            HantroHwDecOmx_decoder_create_rv(dec->bIsRV8,dec->imageSize,dec->width,dec->height);
        TRACE_PRINT("ASYNC: created RV codec\n");
        break;
#endif

#ifdef ENABLE_CODEC_VC1
    case OMX_VIDEO_CodingWMV:
        strcpy((char *) dec->role, "video_decoder.wmv");
        dec->codec = HantroHwDecOmx_decoder_create_vc1();
        TRACE_PRINT("ASYNC: created vc1 codec\n");
        break;
#endif

#ifdef ENABLE_CODEC_VP8
    case OMX_VIDEO_CodingVP8:
        strcpy((char *) dec->role, "video_decoder.vp8");
        dec->codec = HantroHwDecOmx_decoder_create_vp8();
        TRACE_PRINT("ASYNC: created vp8 codec\n");
        break;
#endif

#ifdef ENABLE_CODEC_AVS
    case OMX_VIDEO_CodingAVS:
        strcpy((char *) dec->role, "video_decoder.avs");
        dec->codec = HantroHwDecOmx_decoder_create_avs();
        TRACE_PRINT("ASYNC: created avs codec\n");
        break;
#endif

#ifdef ENABLE_CODEC_VP6
    case OMX_VIDEO_CodingVP6:
        strcpy((char *) dec->role, "video_decoder.vp6");
        dec->codec = HantroHwDecOmx_decoder_create_vp6();
        // DSPG: for some odd reason vp6 outputs mirrored frames, so correct that.
        dec->conf_mirror.eMirror = OMX_MirrorVertical;
        TRACE_PRINT("ASYNC: created vp6 codec\n");
        break;
#endif

#ifdef ENABLE_CODEC_MPEG2
    case OMX_VIDEO_CodingMPEG2:
        strcpy((char *) dec->role, "video_decoder.mpeg2");
        dec->codec = HantroHwDecOmx_decoder_create_mpeg2();
        TRACE_PRINT("ASYNC: created mpeg2 codec\n");
        break;
#endif

#ifdef ENABLE_CODEC_MJPEG
    case OMX_VIDEO_CodingMJPEG:
        strcpy((char *) dec->role, "video_decoder.jpeg");
        dec->codec = HantroHwDecOmx_decoder_create_jpeg(OMX_TRUE);
        TRACE_PRINT("ASYNC: created mjpeg codec\n");
        break;
#endif

    case OMX_VIDEO_CodingUnused:
        switch (dec->in.def.format.video.eColorFormat)
        {
        case OMX_COLOR_FormatYUV420PackedPlanar:
        case OMX_COLOR_FormatYUV420PackedSemiPlanar:
        case OMX_COLOR_FormatYUV420Planar:
        case OMX_COLOR_FormatYUV420SemiPlanar:
        case OMX_COLOR_FormatYCbYCr:   // 422-interleaved
            // post-proc also supports 420 semi-planar in tiled format, but there's no OpenMAX color format for this.
            dec->codec =
                HantroHwDecOmx_decoder_create_pp(dec->in.def.format.video.
                                                 nFrameWidth,
                                                 dec->in.def.format.video.
                                                 nFrameHeight,
                                                 dec->in.def.format.video.
                                                 eColorFormat);
            TRACE_PRINT("ASYNC: created post processor\n");
            TRACE_PRINT("ASYNC: raw frame width: %d frame height: %d color format: %s\n",
                        (int) dec->in.def.format.video.nFrameWidth,
                        (int) dec->in.def.format.video.nFrameHeight,
                        HantroOmx_str_omx_color(dec->in.def.format.video.
                                                eColorFormat));
            break;
#if defined (IS_8190) || defined (IS_G1_DECODER)
        case OMX_COLOR_FormatYCrYCb:
        case OMX_COLOR_FormatCbYCrY:
        case OMX_COLOR_FormatCrYCbY:
            dec->codec =
                HantroHwDecOmx_decoder_create_pp(dec->in.def.format.video.
                                                 nFrameWidth,
                                                 dec->in.def.format.video.
                                                 nFrameHeight,
                                                 dec->in.def.format.video.
                                                 eColorFormat);
            TRACE_PRINT("ASYNC: created post processor\n");
            TRACE_PRINT("ASYNC: raw frame width: %d frame height: %d color format: %s\n",
                        (int) dec->in.def.format.video.nFrameWidth,
                        (int) dec->in.def.format.video.nFrameHeight,
                        HantroOmx_str_omx_color(dec->in.def.format.video.
                                                eColorFormat));
            break;
#endif
        default:
            TRACE_PRINT("ASYNC: unsupported input color format %x\n", dec->in.def.format.video.eColorFormat);
            err = OMX_ErrorUnsupportedSetting;
        }
        break;
    default:
        TRACE_PRINT("ASYNC: unsupported input format. no such video codec\n");
        err = OMX_ErrorUnsupportedSetting;
        break;
    }

#endif
#ifdef OMX_DECODER_IMAGE_DOMAIN
    switch (dec->in.def.format.image.eCompressionFormat)
    {
#ifdef ENABLE_CODEC_JPEG
    case OMX_IMAGE_CodingJPEG:
        strcpy((char *) dec->role, "image_decoder.jpeg");
        dec->codec = HantroHwDecOmx_decoder_create_jpeg(OMX_FALSE);
        TRACE_PRINT("ASYNC: created jpeg codec\n");
        break;
#endif
#ifdef ENABLE_CODEC_WEBP
    case OMX_IMAGE_CodingWEBP:
        strcpy((char *) dec->role, "image_decoder.webp");
        dec->codec = HantroHwDecOmx_decoder_create_webp();
        TRACE_PRINT("ASYNC: created webp codec\n");
        break;
#endif
    case OMX_IMAGE_CodingUnused:
        switch (dec->in.def.format.image.eColorFormat)
        {
        case OMX_COLOR_FormatYUV420PackedPlanar:
        case OMX_COLOR_FormatYUV420PackedSemiPlanar:
        case OMX_COLOR_FormatYUV420Planar:
        case OMX_COLOR_FormatYUV420SemiPlanar:
        case OMX_COLOR_FormatYCbYCr:   // 422 interleaved
            dec->codec =
                HantroHwDecOmx_decoder_create_pp(dec->in.def.format.image.
                                                 nFrameWidth,
                                                 dec->in.def.format.image.
                                                 nFrameHeight,
                                                 dec->in.def.format.image.
                                                 eColorFormat);
            TRACE_PRINT("ASYNC: created post processor\n");
            TRACE_PRINT("ASYNC: raw frame width: %d frame height: %d color format: %s\n",
                        (int) dec->in.def.format.image.nFrameWidth,
                        (int) dec->in.def.format.image.nFrameHeight,
                        HantroOmx_str_omx_color(dec->in.def.format.image.
                                                eColorFormat));
            break;
#if defined (IS_8190) || defined (IS_G1_DECODER)
        case OMX_COLOR_FormatYCrYCb:
        case OMX_COLOR_FormatCbYCrY:
        case OMX_COLOR_FormatCrYCbY:
            dec->codec =
                HantroHwDecOmx_decoder_create_pp(dec->in.def.format.image.
                                                 nFrameWidth,
                                                 dec->in.def.format.image.
                                                 nFrameHeight,
                                                 dec->in.def.format.image.
                                                 eColorFormat);
            TRACE_PRINT("ASYNC: created post processor\n");
            TRACE_PRINT("ASYNC: raw frame width: %d frame height: %d color format: %s\n",
                        (int) dec->in.def.format.image.nFrameWidth,
                        (int) dec->in.def.format.image.nFrameHeight,
                        HantroOmx_str_omx_color(dec->in.def.format.image.
                                                eColorFormat));
            break;
#endif // IS_8190
        default:
            TRACE_PRINT("ASYNC: unsupported input color format\n");
            err = OMX_ErrorUnsupportedSetting;
        }
        break;
    default:
        TRACE_PRINT("ASYNC: unsupported input format. No such image codec, %x\n",
			dec->in.def.format.image.eCompressionFormat);
        err = OMX_ErrorUnsupportedSetting;
        break;
    }
#endif // OMX_DECODER_IMAGE_DOMAIN
    if(!dec->codec) {
        LOGE("ASYNC: no codec was created");
        goto FAIL;
    }

    assert(dec->codec->destroy);
    assert(dec->codec->decode);
    assert(dec->codec->getinfo);
    assert(dec->codec->getframe);
    assert(dec->codec->scanframe);
    assert(dec->codec->setppargs);

    OMX_U32 input_buffer_size = dec->in.def.nBufferSize;

    OMX_U32 output_buffer_size = dec->out.def.nBufferSize;

	OMX_U32 ts_buffer_size = MAX_TICK_COUNTS;

    OMX_U32 mask_buffer_size =
        dec->inpp.def.format.image.nFrameWidth *
        dec->inpp.def.format.image.nFrameHeight * 4;

    // create the temporary frame buffers. These are needed when
    // we are either using a buffer that was allocated by the client
    // or when the data in the buffers contains partial decoding units
    err =
        OSAL_AllocatorAllocMem(&dec->alloc, &input_buffer_size,
                               &dec->frame_in.bus_data,
                               &dec->frame_in.bus_address);
    if(err != OMX_ErrorNone) {
        LOGE("ASYNC: error allocating temporary input frame buffer");
        goto FAIL;
    }
    dec->frame_in.capacity = input_buffer_size;
#ifndef ANDROID
    /* Only use internal allocated buffers with android */

    err =
        OSAL_AllocatorAllocMem(&dec->alloc, &output_buffer_size,
                               &dec->frame_out.bus_data,
                               &dec->frame_out.bus_address);
    if(err != OMX_ErrorNone) {
        LOGE("ASYNC: error allocating temporary output frame buffer");
        goto FAIL;
    }
    dec->frame_out.capacity = output_buffer_size;
#endif
    if(dec->inpp.def.bEnabled)
    {
        if (mask_buffer_size > 0)
        {
            err =
                OSAL_AllocatorAllocMem(&dec->alloc, &mask_buffer_size,
                                       &dec->mask.bus_data, &dec->mask.bus_address);
            if(err != OMX_ErrorNone) {
                LOGE("ASYNC: error allocating mask input frame buffer (1)");
                goto FAIL;
            }
        }
        else
        {
            //mask_buffer_size = 38016;
            err =
                OSAL_AllocatorAllocMem(&dec->alloc, &mask_buffer_size,
                                       &dec->mask.bus_data, &dec->mask.bus_address);
            if(err != OMX_ErrorNone) {
                LOGE("ASYNC: error allocating mask input frame buffer (2)");
                goto FAIL;
            }
        }
        dec->mask.capacity = mask_buffer_size;
    }

	// init pts buffer queue.
    dec->ts_buf.ts_data = malloc(ts_buffer_size * sizeof(OMX_TICKS));
    dec->ts_buf.capacity = ts_buffer_size;
    dec->ts_buf.count = 0;

    PP_ARGS args;

    memset(&args, 0, sizeof(PP_ARGS));
#ifdef OMX_DECODER_VIDEO_DOMAIN
    args.format = dec->out.def.format.video.eColorFormat;

#ifdef ALIGN_AND_DECREASE_OUTPUT
    unsigned int align = 16;

	/* save the original dimensions for later */
	dec->originalWidth = dec->out.def.format.video.nFrameWidth;
	dec->originalHeight = dec->out.def.format.video.nFrameHeight;

    /* DSPG: The output of the decoder (post-processor) must be aligned to 16 pixels. */
    args.scale.width = dec->out.def.format.video.nFrameWidth&(0xffffffff - align + 1);
    args.scale.height = dec->out.def.format.video.nFrameHeight&(0xffffffff - align + 1);

    /* Maximum scale should be limited by the screen dimensions */
    if (dec->screenWidth > 0) {
        if (dec->out.def.format.video.nFrameWidth > dec->screenWidth) {
            args.scale.width = dec->screenWidth&(0xffffffff - align + 1);
            args.scale.height = dec->out.def.format.video.nFrameHeight * args.scale.width / dec->out.def.format.video.nFrameWidth;
            if ((args.scale.height&0xf) > 8 && ((args.scale.height&(0xffffffff - align + 1)) + align) < dec->screenHeight) {
                args.scale.height = ((args.scale.height&(0xffffffff - align + 1)) + align);
            }
            else args.scale.height = args.scale.height&(0xffffffff - align + 1);
        }
        else if (dec->out.def.format.video.nFrameHeight > dec->screenHeight) {
            args.scale.height = dec->screenHeight&(0xffffffff - align + 1);
            args.scale.width = dec->out.def.format.video.nFrameWidth * args.scale.height / dec->out.def.format.video.nFrameHeight;
            if ((args.scale.width&0xf) > 8 && ((args.scale.width&(0xffffffff - align + 1)) + align) < dec->screenWidth) {
                args.scale.width = ((args.scale.width&(0xffffffff - align + 1)) + align);
            }
            else args.scale.width = args.scale.width&(0xffffffff - align + 1);
        }
    }

    TRACE_PRINT("-------------> set post-processor scaler: %ldx%ld", args.scale.width, args.scale.height);

#else 
    args.scale.width = dec->out.def.format.video.nFrameWidth;
    args.scale.height = dec->out.def.format.video.nFrameHeight;
#endif

#endif

#ifdef OMX_DECODER_IMAGE_DOMAIN
    args.format = dec->out.def.format.image.eColorFormat;
    args.scale.width = dec->out.def.format.image.nFrameWidth;
    args.scale.height = dec->out.def.format.image.nFrameHeight;
#endif
    args.crop.left = dec->conf_rect.nLeft;
    args.crop.top = dec->conf_rect.nTop;
    args.crop.width = dec->conf_rect.nWidth;
    args.crop.height = dec->conf_rect.nHeight;
    args.rotation = dec->conf_rotation.nRotation;
    if(dec->conf_mirror.eMirror != OMX_MirrorNone)
    {
        switch (dec->conf_mirror.eMirror)
        {
        case OMX_MirrorHorizontal:
            args.rotation = ROTATE_FLIP_HORIZONTAL;
            break;
        case OMX_MirrorVertical:
            args.rotation = ROTATE_FLIP_VERTICAL;
            break;
        default:
            TRACE_PRINT("ASYNC: unsupported mirror value\n");
            break;
        }
    }
    args.contrast = dec->conf_contrast.nContrast;
    args.brightness = dec->conf_brightness.nBrightness;
    args.saturation = dec->conf_saturation.nSaturation;
    args.alpha = dec->conf_blend.nAlpha;
    args.dither = dec->conf_dither.eDither;

    args.mask1.originX = dec->conf_mask_offset.nX;
    args.mask1.originY = dec->conf_mask_offset.nY;
    args.mask1.width = dec->inpp.def.format.image.nFrameWidth;
    args.mask1.height = dec->inpp.def.format.image.nFrameHeight;
    args.blend_mask_base = dec->mask.bus_address;

    args.mask2.originX = dec->conf_mask.nLeft;
    args.mask2.originY = dec->conf_mask.nTop;
    args.mask2.width = dec->conf_mask.nWidth;
    args.mask2.height = dec->conf_mask.nHeight;

    if(dec->codec->setppargs(dec->codec, &args) != CODEC_OK)
    {
        err = OMX_ErrorBadParameter;
        TRACE_PRINT("ASYNC: failed to set Post-Processor arguments\n");
        goto FAIL;
    }
    dec->codecstate = CODEC_OK;
    return OMX_ErrorNone;

  FAIL:
    TRACE_PRINT("ASYNC: error: %s\n", HantroOmx_str_omx_err(err));
    if(dec->frame_out.bus_address)
    {
        FRAME_BUFF_FREE(&dec->alloc, &dec->frame_out);
        memset(&dec->frame_out, 0, sizeof(FRAME_BUFFER));
    }
    if(dec->frame_in.bus_data)
    {
        FRAME_BUFF_FREE(&dec->alloc, &dec->frame_in);
        memset(&dec->frame_in, 0, sizeof(FRAME_BUFFER));
    }

	// reset pts buffer queue
    if(dec->ts_buf.ts_data)
    {
        free(dec->ts_buf.ts_data);
        memset(&dec->ts_buf, 0, sizeof(TIMESTAMP_BUFFER));
    }

    if(dec->mask.bus_data)
    {
        FRAME_BUFF_FREE(&dec->alloc, &dec->mask);
        memset(&dec->mask, 0, sizeof(FRAME_BUFFER));
    }
    if(dec->codec)
    {
        dec->codec->destroy(dec->codec);
        dec->codec = NULL;
    }
    return err;
}

#ifdef DYNAMIC_SCALING
static OMX_ERRORTYPE async_set_new_pp_args(OMX_DECODER *dec, OMX_U32 newWidth, OMX_U32 newHeight)
{
	if (!dec->codec->setscaling) return OMX_ErrorBadParameter;

	/* verify dimensions */
	if (newWidth < 64 ||
		newHeight < 64 ||
		newWidth > 1920 ||
		newHeight > 1080 ||
		(newWidth&0x3f)>0 ||
		(newHeight&0x3f)>0 ||
		((newWidth > dec->originalWidth) && (newHeight < dec->originalHeight)) ||
		((newWidth < dec->originalWidth) && (newHeight > dec->originalHeight))) 
	{
		LOGE("async_set_new_pp_args: wrong dimensions (%dx%d) original dimensions (%dx%d)", newWidth, newHeight, dec->originalWidth, dec->originalHeight);
		return OMX_ErrorBadParameter;
	}

	if(dec->codec->setscaling(dec->codec, newWidth, newHeight) != CODEC_OK)
	{
		TRACE_PRINT("ASYNC: failed to set Post-Processor scaling arguments\n");
		return OMX_ErrorBadParameter;
	}

	return OMX_ErrorNone;//async_get_info(dec);
}
#endif

static OMX_ERRORTYPE async_decoder_return_buffers(OMX_DECODER * dec, PORT * p)
{
    CALLSTACK;
    if(HantroOmx_port_is_supplier(p))
        return OMX_ErrorNone;

    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT("ASYNC: returning allocated buffers on port %d to %p %d\n",
                (int) p->def.nPortIndex, p->tunnelcomp, (int) p->tunnelport);

    if(p == &dec->out && dec->buffer)
    {
        // returned the held output buffer (if any)
        if(HantroOmx_port_is_tunneled(p))
        {
            ((OMX_COMPONENTTYPE *) p->tunnelcomp)->EmptyThisBuffer(p->
                                                                   tunnelcomp,
                                                                   dec->buffer->
                                                                   header);
            dec->buffer = NULL;
        }
        else
        {
            dec->callbacks.FillBufferDone(dec->self, dec->appdata,
                                          dec->buffer->header);
            dec->buffer = NULL;
        }
    }

    // return the queued buffers to the suppliers
    // Danger. The port's locked and there are callbacks into the unknown.

    OMX_ERRORTYPE err = HantroOmx_port_lock_buffers(p);

    if(err != OMX_ErrorNone)
        return err;

    OMX_U32 i;

    OMX_U32 count = HantroOmx_port_buffer_queue_count(p);

    for(i = 0; i < count; ++i)
    {
        BUFFER *buff = 0;

        HantroOmx_port_get_buffer_at(p, &buff, i);
        assert(buff);
        if(HantroOmx_port_is_tunneled(p))
        {
            // tunneled but someone else is supplying. i.e. we have allocated the header.
            assert(buff->header == &buff->headerdata);
            // return the buffer to the supplier
            if(p->def.eDir == OMX_DirInput)
                ((OMX_COMPONENTTYPE *) p->tunnelcomp)->FillThisBuffer(p->
                                                                      tunnelcomp,
                                                                      buff->
                                                                      header);
            if(p->def.eDir == OMX_DirOutput)
                ((OMX_COMPONENTTYPE *) p->tunnelcomp)->EmptyThisBuffer(p->
                                                                       tunnelcomp,
                                                                       buff->
                                                                       header);
        }
        else
        {
            // return the buffer to the client
            if(p->def.eDir == OMX_DirInput)
                dec->callbacks.EmptyBufferDone(dec->self, dec->appdata,
                                               buff->header);
            if(p->def.eDir == OMX_DirOutput)
                dec->callbacks.FillBufferDone(dec->self, dec->appdata,
                                              buff->header);
        }
    }

#ifdef DSPG_TIMESTAMP_FIX
    // if we return all input buffers, clear also the timestamps
    TRACE_PRINT("-------> [TIMESTAMP_FIX] async_decoder_return_buffers (%d) [head = %lu, tail = %lu]", p->def.eDir, dec->timestampsQueueHead, dec->timestampsQueueTail);
    if(p->def.eDir == OMX_DirInput)
    {
        dec->timestampsQueueHead = dec->timestampsQueueTail;
        TRACE_PRINT("-------> [TIMESTAMP_FIX] tail = head = %lu", dec->timestampsQueueHead);
    }
#endif

    HantroOmx_port_buffer_queue_clear(p);
    HantroOmx_port_unlock_buffers(p);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE transition_to_idle_from_paused(OMX_DECODER * dec)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    assert(dec->state == OMX_StatePause);
    assert(dec->statetrans == OMX_StateIdle);

    // return queued buffers
    async_decoder_return_buffers(dec, &dec->in);
    async_decoder_return_buffers(dec, &dec->out);
    async_decoder_return_buffers(dec, &dec->inpp);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE wait_supplied_buffers(OMX_DECODER * dec, PORT * port)
{
    CALLSTACK;
    if(!HantroOmx_port_is_supplier(port))
        return OMX_ErrorNone;

    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    // buffer supplier component cannot transition
    // to idle untill all the supplied buffers have been returned
    // by the buffer user via a call to our FillThisBuffer
    int loop = 1;

    while(loop)
    {
        HantroOmx_port_lock_buffers(port);
        OMX_U32 queued = HantroOmx_port_buffer_queue_count(port);

        if(port == &dec->out && dec->buffer)
            ++queued;   // include the held buffer in the queue size.

        //if (port_has_all_supplied_buffers(port))
        if(HantroOmx_port_buffer_count(port) == queued)
            loop = 0;

        TRACE_PRINT("ASYNC: port %d has %d buffers out of %d supplied\n",
                    (int) port->def.nPortIndex, (int) queued,
                    (int) HantroOmx_port_buffer_count(port));
        HantroOmx_port_unlock_buffers(port);
        if(loop)
            OSAL_ThreadSleep(RETRY_INTERVAL);
    }
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE transition_to_idle_from_executing(OMX_DECODER * dec)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    assert(dec->state == OMX_StateExecuting);
    assert(dec->statetrans == OMX_StateIdle);

    // return queued buffers
    async_decoder_return_buffers(dec, &dec->in);
    async_decoder_return_buffers(dec, &dec->out);
    async_decoder_return_buffers(dec, &dec->inpp);

    // wait for the buffers supplied by this component
    // to be returned by the buffer users.
    wait_supplied_buffers(dec, &dec->in);
    wait_supplied_buffers(dec, &dec->out);
    wait_supplied_buffers(dec, &dec->inpp);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE transition_to_loaded_from_idle(OMX_DECODER * dec)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    // the state transition cannot complete untill all
    // enabled ports have their buffers freed

    assert(dec->state == OMX_StateIdle);
    assert(dec->statetrans == OMX_StateLoaded);

    if(HantroOmx_port_is_supplier(&dec->in))
        unsupply_tunneled_port(dec, &dec->in);
    if(HantroOmx_port_is_supplier(&dec->out))
        unsupply_tunneled_port(dec, &dec->out);
    if(HantroOmx_port_is_supplier(&dec->inpp))
        unsupply_tunneled_port(dec, &dec->inpp);

    // should be okay without locking
    assert(HantroOmx_port_buffer_queue_count(&dec->in) == 0);
    assert(HantroOmx_port_buffer_queue_count(&dec->out) == 0);
    assert(HantroOmx_port_buffer_queue_count(&dec->inpp) == 0);

    TRACE_PRINT("ASYNC: waiting for ports to be non-populated\n");

    while(HantroOmx_port_has_buffers(&dec->in) ||
          HantroOmx_port_has_buffers(&dec->out) ||
          HantroOmx_port_has_buffers(&dec->inpp))
    {
		TRACE_PRINT("%s: waiting for buffers release..",__func__);
        OSAL_ThreadSleep(RETRY_INTERVAL);
    }

    TRACE_PRINT("ASYNC: destroying codec\n");
    assert(dec->codec);
    dec->codec->destroy(dec->codec);
    TRACE_PRINT("ASYNC: freeing internal frame buffers\n");
    if(dec->frame_in.bus_address)
        FRAME_BUFF_FREE(&dec->alloc, &dec->frame_in);

    if(dec->ts_buf.ts_data)
        free(dec->ts_buf.ts_data);

    if(dec->frame_out.bus_address)
        FRAME_BUFF_FREE(&dec->alloc, &dec->frame_out);
    if(dec->mask.bus_address)
        FRAME_BUFF_FREE(&dec->alloc, &dec->mask);

    dec->codec = NULL;
    memset(&dec->frame_in, 0, sizeof(FRAME_BUFFER));
    memset(&dec->ts_buf, 0, sizeof(TIMESTAMP_BUFFER));
    memset(&dec->frame_out, 0, sizeof(FRAME_BUFFER));
    memset(&dec->mask, 0, sizeof(FRAME_BUFFER));
    TRACE_PRINT("ASYNC: freed internal frame buffers\n");
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE transition_to_loaded_from_wait_for_resources(OMX_DECODER *
                                                               dec)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    assert(dec->state == OMX_StateWaitForResources);
    assert(dec->statetrans == OMX_StateLoaded);

    // note: could unregister with a custom resource manager here
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE transition_to_executing_from_paused(OMX_DECODER * dec)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    assert(dec->state == OMX_StatePause);
    assert(dec->statetrans == OMX_StateExecuting);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE transition_to_executing_from_idle(OMX_DECODER * dec)
{
    CALLSTACK;
    // note: could check for suspension here

    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    assert(dec->state == OMX_StateIdle);
    assert(dec->statetrans == OMX_StateExecuting);

    return OMX_ErrorNone;
}

static OMX_ERRORTYPE startup_tunnel(OMX_DECODER * dec, PORT * port)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    OMX_U32 buffers = 0;

    OMX_U32 i = 0;

    OMX_ERRORTYPE err = OMX_ErrorNone;

    if(HantroOmx_port_is_enabled(port) && HantroOmx_port_is_supplier(port))
    {
        if(port == &dec->out)
        {
            // queue the buffers into own queue since the
            // output port is the supplier the tunneling component
            // cannot fill the buffer queue.
            buffers = HantroOmx_port_buffer_count(port);
            for(i = 0; i < buffers; ++i)
            {
                BUFFER *buff = NULL;

                HantroOmx_port_get_allocated_buffer_at(port, &buff, i);
                HantroOmx_port_push_buffer(port, buff);
            }
        }
        else
        {
            buffers = HantroOmx_port_buffer_count(port);
            for(; i < buffers; ++i)
            {
                BUFFER *buff = NULL;

                HantroOmx_port_get_allocated_buffer_at(port, &buff, i);
                assert(buff);
                assert(buff->header != &buff->headerdata);
                err =
                    ((OMX_COMPONENTTYPE *) port->tunnelcomp)->
                    FillThisBuffer(port->tunnelcomp, buff->header);
                if(err != OMX_ErrorNone)
                {
                    TRACE_PRINT("ASYNC: tunneling component failed to fill buffer: %s\n",
                                HantroOmx_str_omx_err(err));
                    return err;
                }
            }
        }
    }
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE transition_to_wait_for_resources_from_loaded(OMX_DECODER *
                                                               dec)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    assert(dec->state == OMX_StateLoaded);
    assert(dec->statetrans == OMX_StateWaitForResources);
    // note: could register with a custom resource manager for availability of resources.
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE async_decoder_set_state(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1,
                                          OMX_PTR pCmdData, OMX_PTR arg)
{
    CALLSTACK;
    OMX_ERRORTYPE err = OMX_ErrorIncorrectStateTransition;

    OMX_DECODER *dec = (OMX_DECODER *) arg;

    OMX_STATETYPE nextstate = (OMX_STATETYPE) nParam1;

    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    if(nextstate == dec->state)
    {
        TRACE_PRINT("ASYNC: same state:%s\n",
                    HantroOmx_str_omx_state(dec->state));
        dec->callbacks.EventHandler(dec->self, dec->appdata, OMX_EventError,
                                    OMX_ErrorSameState, 0, NULL);

        TRACE_PRINT("ASYNC: %s done\n", __FUNCTION__);
        return OMX_ErrorNone;
    }

    // state transition state switch. ugliest thing EVER.

    switch (nextstate)
    {
    case OMX_StateIdle:
        switch (dec->state)
        {
        case OMX_StateLoaded:
            err = transition_to_idle_from_loaded(dec);
            break;
        case OMX_StatePause:
            err = transition_to_idle_from_paused(dec);
            break;
        case OMX_StateExecuting:
            err = transition_to_idle_from_executing(dec);
            break;
        default:
            break;
        }
        break;
    case OMX_StateLoaded:
        if(dec->state == OMX_StateIdle)
            err = transition_to_loaded_from_idle(dec);
        if(dec->state == OMX_StateWaitForResources)
            err = transition_to_loaded_from_wait_for_resources(dec);
        break;
    case OMX_StateExecuting:
        if(dec->state == OMX_StatePause)    /// OMX_StatePause to OMX_StateExecuting
            err = transition_to_executing_from_paused(dec);
        if(dec->state == OMX_StateIdle)
            err = transition_to_executing_from_idle(dec);   /// OMX_StateIdle to OMX_StateExecuting
        break;
    case OMX_StatePause:
        {
            if(dec->state == OMX_StateIdle) /// OMX_StateIdle to OMX_StatePause
                err = OMX_ErrorNone;
            if(dec->state == OMX_StateExecuting)    /// OMX_StateExecuting to OMX_StatePause
                err = OMX_ErrorNone;
        }
        break;
    case OMX_StateWaitForResources:
        {
            if(dec->state == OMX_StateLoaded)
                err = transition_to_wait_for_resources_from_loaded(dec);
        }
        break;
    case OMX_StateInvalid:
        err = OMX_ErrorInvalidState;
        break;
    default:
        assert(!"weird state");
        break;
    }

    if(err != OMX_ErrorNone)
    {
        TRACE_PRINT("ASYNC: set state error:%s\n",
                    HantroOmx_str_omx_err(err));
        if(err != OMX_ErrorIncorrectStateTransition)
        {
            dec->state = OMX_StateInvalid;
            dec->run = OMX_FALSE;
        }
        else
        {
            // clear state transition flag
            dec->statetrans = dec->state;
        }
        dec->callbacks.EventHandler(dec->self, dec->appdata, OMX_EventError,
                                    err, 0, NULL);
    }
    else
    {
        // transition complete
        TRACE_PRINT("ASYNC: set state complete:%s\n",
                    HantroOmx_str_omx_state(nextstate));
        dec->statetrans = nextstate;
        dec->state = nextstate;
        dec->callbacks.EventHandler(dec->self, dec->appdata,
                                    OMX_EventCmdComplete, OMX_CommandStateSet,
                                    dec->state, NULL);

        if(dec->state == OMX_StateExecuting)
        {
            err = startup_tunnel(dec, &dec->in);
            if(err != OMX_ErrorNone)
            {
                TRACE_PRINT("ASYNC: failed to startup buffer processing: %s\n",
                            HantroOmx_str_omx_err(err));
                dec->state = OMX_StateInvalid;
                dec->run = OMX_FALSE;
            }
            err = startup_tunnel(dec, &dec->inpp);
            if(err != OMX_ErrorNone)
            {
                TRACE_PRINT("ASYNC: failed to startup buffer processing: %s\n",
                            HantroOmx_str_omx_err(err));
                dec->state = OMX_StateInvalid;
                dec->run = OMX_FALSE;
            }
            startup_tunnel(dec, &dec->out);
        }
    }

    TRACE_PRINT("ASYNC: %s done\n", __FUNCTION__);
    return err;
}

static void async_decoder_wait_port_buffer_dealloc(OMX_DECODER * dec, PORT * p)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    while(HantroOmx_port_has_buffers(p))
    {
		TRACE_PRINT("%s: waiting for port buffers dealloc: %lu",__func__, p->def.nPortIndex);
        OSAL_ThreadSleep(RETRY_INTERVAL);
    }
	TRACE_PRINT("ASYNC: %s done\n", __FUNCTION__);
}

static
    OMX_ERRORTYPE async_decoder_disable_port(OMX_COMMANDTYPE Cmd,
                                             OMX_U32 nParam1, OMX_PTR pCmdData,
                                             OMX_PTR arg)
{
    CALLSTACK;
    OMX_DECODER *dec = (OMX_DECODER *) arg;

    OMX_U32 portIndex = nParam1;

    // return the queue'ed buffers to the suppliers
    // and wait untill all the buffers have been free'ed.
    // The component must generate port disable event for each port disableed
    // 3.2.2.5

    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT("ASYNC: nParam1:%u pCmdData:%p\n", (unsigned) nParam1,
                pCmdData);

    if((OMX_S32)portIndex == -1)
    {
        TRACE_PRINT("ASYNC: disabling all ports\n");
        async_decoder_return_buffers(dec, &dec->in);
        async_decoder_return_buffers(dec, &dec->out);
        async_decoder_return_buffers(dec, &dec->inpp);

        wait_supplied_buffers(dec, &dec->in);
        wait_supplied_buffers(dec, &dec->out);
        wait_supplied_buffers(dec, &dec->inpp);

        if(HantroOmx_port_is_supplier(&dec->in))
            unsupply_tunneled_port(dec, &dec->in);
        if(HantroOmx_port_is_supplier(&dec->out))
            unsupply_tunneled_port(dec, &dec->out);
        if(HantroOmx_port_is_supplier(&dec->inpp))
            unsupply_tunneled_port(dec, &dec->inpp);

        async_decoder_wait_port_buffer_dealloc(dec, &dec->in);
        async_decoder_wait_port_buffer_dealloc(dec, &dec->out);
        async_decoder_wait_port_buffer_dealloc(dec, &dec->inpp);

        // generate port disable events
        int i = 0;

        for(i = 0; i < 3; ++i)
        {
            dec->callbacks.EventHandler(dec->self, dec->appdata,
                                        OMX_EventCmdComplete,
                                        OMX_CommandPortDisable, i, NULL);
        }
    }
    else
    {
        PORT *ports[] = { &dec->in, &dec->out, &dec->inpp };
        unsigned int i = 0;

        for(i = 0; i < sizeof(ports) / sizeof(PORT *); ++i)
        {
            if(portIndex == i)
            {
                TRACE_PRINT("ASYNC: disabling port: %d\n", i);
                async_decoder_return_buffers(dec, ports[i]);
                wait_supplied_buffers(dec, ports[i]);
                if(HantroOmx_port_is_supplier(ports[i]))
                    unsupply_tunneled_port(dec, ports[i]);

                async_decoder_wait_port_buffer_dealloc(dec, ports[i]);
                break;
            }
        }
        dec->callbacks.EventHandler(dec->self, dec->appdata,
                                    OMX_EventCmdComplete,
                                    OMX_CommandPortDisable, portIndex, NULL);
    }

    TRACE_PRINT("ASYNC: %s done\n", __FUNCTION__);
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE async_decoder_enable_port(OMX_COMMANDTYPE Cmd,
                                            OMX_U32 nParam1, OMX_PTR pCmdData,
                                            OMX_PTR arg)
{
    CALLSTACK;
    OMX_DECODER *dec = (OMX_DECODER *) arg;

    OMX_U32 portIndex = nParam1;

    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT("ASYNC: nParam1:%u pCmdData:%p\n", (unsigned) nParam1,
                pCmdData);

    // brutally wait untill all buffers for the enabled
    // port have been allocated
    OMX_ERRORTYPE err = OMX_ErrorNone;

    if(portIndex == PORT_INDEX_INPUT || (OMX_S32)portIndex == -1)
    {
        if(dec->state != OMX_StateLoaded)
        {
            if(HantroOmx_port_is_supplier(&dec->in))
            {
                err = supply_tunneled_port(dec, &dec->in);
                if(err != OMX_ErrorNone)
                    return err;
            }
            while(!HantroOmx_port_is_ready(&dec->in))
                OSAL_ThreadSleep(RETRY_INTERVAL);
        }
        if(dec->state == OMX_StateExecuting)
            startup_tunnel(dec, &dec->in);

        dec->callbacks.EventHandler(dec->self, dec->appdata,
                                    OMX_EventCmdComplete, OMX_CommandPortEnable,
                                    PORT_INDEX_INPUT, NULL);
        TRACE_PRINT("ASYNC: input port enabled\n");
    }
    if(portIndex == PORT_INDEX_OUTPUT || (OMX_S32)portIndex == -1)
    {
        if(dec->state != OMX_StateLoaded)
        {
            if(HantroOmx_port_is_supplier(&dec->out))
            {
                err = supply_tunneled_port(dec, &dec->out);
                if(err != OMX_ErrorNone)
                    return err;
            }
            while(!HantroOmx_port_is_ready(&dec->out))
                OSAL_ThreadSleep(RETRY_INTERVAL);
        }
        if(dec->state == OMX_StateExecuting)
            startup_tunnel(dec, &dec->out);

        dec->callbacks.EventHandler(dec->self, dec->appdata,
                                    OMX_EventCmdComplete, OMX_CommandPortEnable,
                                    PORT_INDEX_OUTPUT, NULL);
        TRACE_PRINT("ASYNC: output port enabled\n");

#ifdef ANDROID_MOD
        dec->portReconfigPending = OMX_FALSE;
#endif
    }
    if(portIndex == PORT_INDEX_PP || (OMX_S32)portIndex == -1)
    {
        if(dec->state != OMX_StateLoaded)
        {
            if(HantroOmx_port_is_supplier(&dec->inpp))
            {
                err = supply_tunneled_port(dec, &dec->inpp);
                if(err != OMX_ErrorNone)
                    return err;
            }
            while(!HantroOmx_port_is_ready(&dec->inpp))
                OSAL_ThreadSleep(RETRY_INTERVAL);
        }
        if(dec->state == OMX_StateExecuting)
            startup_tunnel(dec, &dec->inpp);

        dec->callbacks.EventHandler(dec->self, dec->appdata,
                                    OMX_EventCmdComplete, OMX_CommandPortEnable,
                                    PORT_INDEX_PP, NULL);
        TRACE_PRINT("ASYNC: post processor input port enabled\n");
    }

    TRACE_PRINT("ASYNC: %s done\n", __FUNCTION__);
    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE async_decoder_flush_port(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1,
                                           OMX_PTR pCmdData, OMX_PTR arg)
{
    CALLSTACK;
    assert(arg);
    OMX_DECODER *dec = (OMX_DECODER *) arg;

    OMX_U32 portindex = nParam1;

    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT("ASYNC: nParam1:%u pCmdData:%p\n", (unsigned) nParam1,
                pCmdData);

    OMX_ERRORTYPE err = OMX_ErrorNone;

    if((OMX_S32)portindex == -1)
    {
        TRACE_PRINT("ASYNC: flushing all ports\n");
        err = async_decoder_return_buffers(dec, &dec->in);
        if(err != OMX_ErrorNone)
            goto FAIL;
        err = async_decoder_return_buffers(dec, &dec->out);
        if(err != OMX_ErrorNone)
            goto FAIL;
        err = async_decoder_return_buffers(dec, &dec->inpp);
        if(err != OMX_ErrorNone)
            goto FAIL;

        int i = 0;

        for(i = 0; i < 3; ++i)
        {
            dec->callbacks.EventHandler(dec->self, dec->appdata,
                                        OMX_EventCmdComplete, OMX_CommandFlush,
                                        i, NULL);
        }
#ifdef ANDROID_MOD
        dec->outputPortFlushPending = OMX_FALSE;
#endif
        TRACE_PRINT("ASYNC: all buffers flushed\n");
    }
    else
    {
        if(portindex == PORT_INDEX_INPUT)
        {
            err = async_decoder_return_buffers(dec, &dec->in);
            if(err != OMX_ErrorNone)
                goto FAIL;

            dec->callbacks.EventHandler(dec->self, dec->appdata,
                                        OMX_EventCmdComplete, OMX_CommandFlush,
                                        portindex, NULL);
            TRACE_PRINT("ASYNC: input port flushed\n");
        }
        else if(portindex == PORT_INDEX_OUTPUT)
        {
			TRACE_PRINT("ASYNC: flushing output port\n");
            err = async_decoder_return_buffers(dec, &dec->out);
            if(err != OMX_ErrorNone)
                goto FAIL;

            dec->callbacks.EventHandler(dec->self, dec->appdata,
                                        OMX_EventCmdComplete, OMX_CommandFlush,
                                        portindex, NULL);
#ifdef ANDROID_MOD
            dec->outputPortFlushPending = OMX_FALSE;
#endif
            TRACE_PRINT("ASYNC: output port flushed\n");
        }
        else if(portindex == PORT_INDEX_PP)
        {
            err = async_decoder_return_buffers(dec, &dec->inpp);
            if(err != OMX_ErrorNone)
                goto FAIL;

            dec->callbacks.EventHandler(dec->self, dec->appdata,
                                        OMX_EventCmdComplete, OMX_CommandFlush,
                                        portindex, NULL);
            TRACE_PRINT("ASYNC: post-processor port flushed\n");
        }
    }

	// flush the PTS buffer
	memset(dec->ts_buf.ts_data, 0, sizeof(OMX_TICKS)*dec->ts_buf.capacity);
	dec->ts_buf.count = 0;

    TRACE_PRINT("ASYNC: %s done\n", __FUNCTION__);
    return OMX_ErrorNone;
  FAIL:
    assert(err != OMX_ErrorNone);
    TRACE_PRINT("ASYNC: error while flushing port: %s\n",
                HantroOmx_str_omx_err(err));
    dec->state = OMX_StateInvalid;
    dec->run = OMX_FALSE;
    dec->callbacks.EventHandler(dec->self, dec->appdata, OMX_EventError, err, 0,
                                NULL);
    LOGE("ASYNC: %s done with error\n", __FUNCTION__);
    return err;
}

static
    OMX_ERRORTYPE async_decoder_mark_buffer(OMX_COMMANDTYPE Cmd,
                                            OMX_U32 nParam1, OMX_PTR pCmdData,
                                            OMX_PTR arg)
{
    CALLSTACK;
    assert(arg);
    assert(pCmdData);
    OMX_MARKTYPE *mark = (OMX_MARKTYPE *) pCmdData;

    OMX_DECODER *dec = (OMX_DECODER *) arg;

    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    if(dec->mark_write_pos < sizeof(dec->marks) / sizeof(OMX_MARKTYPE))
    {
        dec->marks[dec->mark_write_pos] = *mark;
        TRACE_PRINT("ASYNC: set mark in index: %d\n",
                    (int) dec->mark_write_pos);
        dec->mark_write_pos++;
    }
    else
    {
        TRACE_PRINT("ASYNC: no space for mark\n");
        dec->callbacks.EventHandler(dec->self, dec->appdata, OMX_EventError,
                                    OMX_ErrorUndefined, 0, NULL);
    }
    OSAL_Free(mark);

    TRACE_PRINT("ASYNC: %s done\n", __FUNCTION__);
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE async_get_info(OMX_DECODER * dec)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    // get the information from the codec and create port settings changed event
    STREAM_INFO info;

    memset(&info, 0, sizeof(STREAM_INFO));

    CODEC_STATE ret = dec->codec->getinfo(dec->codec, &info);

    TRACE_PRINT("--> dec->codec->getinfo returned video size of %lux%lu",info.width, info.height);

    if(ret != CODEC_OK)
    {
        TRACE_PRINT("ASYNC: getinfo error: %d\n", ret);
        return OMX_ErrorHardware;
    }

//#ifdef OMX_DECODER_IMAGE_DOMAIN
    dec->imageSize = info.imageSize;
//#endif

#ifdef ANDROID_MOD
    OMX_BOOL portResChanged = OMX_FALSE;
    // Check if video width and height is different from width/height configured for port.
    // Signal PortSettingsChanged only if resolution changes.
    // TODO: Can other params change during get_info call and do they need
    // signalling portsettingschanged.
    if( (dec->out.def.format.video.nFrameWidth!=info.width) || (dec->out.def.format.video.nFrameHeight!=info.height) ) {
        portResChanged = OMX_TRUE;
        TRACE_PRINT("ASYNC: Video Frame Width/Height Changed.");
    }
#endif
    dec->out.def.nBufferSize = info.framesize;
    dec->out.def.format.video.nFrameWidth = info.width;
    dec->out.def.format.video.nFrameHeight = info.height;
    dec->out.def.format.video.nSliceHeight = info.sliceheight;
    dec->out.def.format.video.nStride = info.stride;
    if(dec->out.def.format.video.eColorFormat == OMX_COLOR_FormatUnused)
        dec->out.def.format.video.eColorFormat = info.format;

    TRACE_PRINT("ASYNC: video props: width: %u height: %u buffersize: %u stride: %u sliceheight: %u\n",
                (unsigned) info.width, (unsigned) info.height,
                (unsigned) info.framesize, (unsigned) info.stride,
                (unsigned) info.sliceheight);

#ifdef ANDROID_MOD
    if(portResChanged)
#endif
    {
        dec->portReconfigPending = OMX_TRUE;
        dec->callbacks.EventHandler(dec->self, dec->appdata,
                                OMX_EventPortSettingsChanged, PORT_INDEX_OUTPUT,
                                0, NULL);
    }

    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE grow_frame_buffer(OMX_DECODER * dec, FRAME_BUFFER * fb,
                                    OMX_U32 capacity)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT("ASYNC: fb size: %u capacity: %u new capacity:%u\n",
                (unsigned) fb->size, (unsigned) fb->capacity,
                (unsigned) capacity);

    assert(capacity >= fb->size);

    FRAME_BUFFER new;

    memset(&new, 0, sizeof(FRAME_BUFFER));
    TRACE_PRINT("grow_frame_buffer: OSAL_AllocatorAllocMem 1\n");
    OMX_ERRORTYPE err =
        OSAL_AllocatorAllocMem(&dec->alloc, &capacity, &new.bus_data,
                               &new.bus_address);
    if(err != OMX_ErrorNone)
    {
        TRACE_PRINT("ASYNC: frame buffer reallocation failed: %s\n",
                    HantroOmx_str_omx_err(err));
        return err;
    }
    memcpy(new.bus_data, fb->bus_data, fb->size);
    new.capacity = capacity;
    new.size = fb->size;
    OSAL_AllocatorFreeMem(&dec->alloc, fb->capacity, fb->bus_data,
                          fb->bus_address);
    *fb = new;
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE async_get_frame_buffer(OMX_DECODER * dec, FRAME * frm)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    OMX_U32 framesize = dec->out.def.nBufferSize;

    OMX_ERRORTYPE err = OMX_ErrorNone;

    BUFFER *buff = NULL;

    err = HantroOmx_port_lock_buffers(&dec->out);
    if(err != OMX_ErrorNone)
        return err;

    HantroOmx_port_get_buffer(&dec->out, &buff);
    HantroOmx_port_unlock_buffers(&dec->out);

    int retry = MAX_RETRIES; // needed for stagefright when video is paused (no transition to pause)
    while (buff == NULL && retry > 0 && dec->state != OMX_StatePause && dec->statetrans != OMX_StatePause && dec->statetrans != OMX_StateIdle &&
           dec->outputPortFlushPending != OMX_TRUE)
    {
		TRACE_PRINT("%s: waiting for buffer..", __func__);
        OSAL_ThreadSleep(RETRY_INTERVAL);

        err = HantroOmx_port_lock_buffers(&dec->out);
        if(err != OMX_ErrorNone) {
            LOGW("%s: error on call to HantroOmx_port_lock_buffer", __func__);
            return err;
        }

        HantroOmx_port_get_buffer(&dec->out, &buff);
        HantroOmx_port_unlock_buffers(&dec->out);
        retry--;
    }


    if(buff == NULL)
    {
        if(dec->state == OMX_StatePause || dec->statetrans == OMX_StatePause || dec->statetrans == OMX_StateIdle 
            || dec->outputPortFlushPending == OMX_TRUE )
        {
            TRACE_PRINT("%s: no output buffer, at pause or going into pause state or going into idle state or in flush state", __func__);
            return OMX_ErrorNone;
        }
        TRACE_PRINT("ASYNC: there's no output buffer!\n");
        return OMX_ErrorOverflow;
    }
    if(framesize > buff->header->nAllocLen)
    {
        TRACE_PRINT("ASYNC: frame is too big for output buffer. framesize:%u nAllocLen:%u\n",
                    (unsigned) framesize, (unsigned) buff->header->nAllocLen);
        return OMX_ErrorOverflow;
    }
    if(buff->flags & BUFFER_FLAG_MY_BUFFER)
    {
        // can stick the data directly into the output buffer
        assert(buff->bus_data);
        assert(buff->bus_address);
        assert(buff->header->pBuffer == buff->bus_data);

        frm->fb_bus_data = buff->bus_data;
        frm->fb_bus_address = buff->bus_address;
        frm->fb_size = buff->header->nAllocLen;
        TRACE_PRINT("ASYNC: using output buffer:%p\n", buff->header);
    }
    else
    {
        TRACE_PRINT("ASYNC: using temporary output buffer\n");
        // need to stick the data into the temporary output buffer
        FRAME_BUFFER *temp = &dec->frame_out;

        assert(temp->bus_data);
        assert(temp->bus_address);
        assert(temp->size == 0);
        if(framesize > temp->capacity)
        {
            OMX_U32 capacity = temp->capacity;

            if(capacity == 0)
                capacity = 384; // TODO

            while(capacity < framesize)
                capacity *= 2;
            if(capacity != temp->capacity)
            {
                err = grow_frame_buffer(dec, temp, capacity);
                if(err != OMX_ErrorNone)
                    return OMX_ErrorInsufficientResources;
            }
        }
        assert(temp->capacity >= framesize);
        frm->fb_bus_data = temp->bus_data;
        frm->fb_bus_address = temp->bus_address;
        frm->fb_size = temp->capacity;
    }

    TRACE_PRINT("ASYNC: %s done\n", __FUNCTION__);
    return OMX_ErrorNone;
}

static
    void async_dispatch_frame_buffer(OMX_DECODER * dec, OMX_BOOL EOS,
                                     BUFFER * inbuff, FRAME * frm)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    assert(frm->size);
    assert(frm->fb_bus_data);

    BUFFER *buff = NULL;

    // above assert doesnt work in release mode.
    // validate below params since get_frame_buffer returns no error when in flush mode and also
    // without output buffer
    if( (frm->fb_bus_data==0) || (frm->size==0) ) {
        return;
    }

    HantroOmx_port_lock_buffers(&dec->out);
    HantroOmx_port_get_buffer(&dec->out, &buff);
    HantroOmx_port_unlock_buffers(&dec->out);

    assert(buff);
    assert(inbuff);

    if(frm->fb_bus_data == dec->frame_out.bus_data)
    {
        assert(buff->header->nAllocLen >= frm->size);
        // copy data from the temporary output buffer
        // into the real output buffer
        memcpy(buff->header->pBuffer, frm->fb_bus_data, frm->size);
    }
    buff->header->nOffset = 0;
    buff->header->nFilledLen = frm->size;
    buff->header->nTimeStamp = inbuff->header->nTimeStamp;

    /* DSPG: pass corrupted MB count scalar in platform private */
    buff->header->pPlatformPrivate = (void*)frm->MB_err_count;

    if(strcmp((char*)dec->role, "video_decoder.jpeg") == 0)
    {
        dec->buffer = buff;
    }

#ifdef DSPG_TIMESTAMP_FIX
    /* If no timestamp in queue set the timestamp of the last input frame */
    if(dec->timestampsQueueHead == dec->timestampsQueueTail) 
    {
        LOGE("----> [TIMESTAMP_FIX] !!! no timestamp in queue !!!");
        buff->header->nTimeStamp = inbuff->header->nTimeStamp;
    }
    else 
    {
        buff->header->nTimeStamp = dec->timestampsQueue[dec->timestampsQueueHead];
        dec->timestampsQueueHead++;
        dec->timestampsQueueHead %= TIMESTAMPS_QUEUE_SIZE;
    }
#else
    if ((dec->in.def.format.video.eCompressionFormat) != OMX_VIDEO_CodingVP8)
    {
        pop_timestamp(dec, &buff->header->nTimeStamp);
    }
#endif
    buff->header->nFlags = inbuff->header->nFlags;
    buff->header->nFlags &= ~OMX_BUFFERFLAG_EOS;

    int markcount = dec->mark_write_pos - dec->mark_read_pos;

    TRACE_PRINT("---------> sending output frame with timestamp %llu", buff->header->nTimeStamp);

    if(markcount)
    {
        TRACE_PRINT("ASYNC: got %d marks pending\n", markcount);
        buff->header->hMarkTargetComponent =
            dec->marks[dec->mark_read_pos].hMarkTargetComponent;
        buff->header->pMarkData = dec->marks[dec->mark_read_pos].pMarkData;
        dec->mark_read_pos++;
        if(--markcount == 0)
        {
            dec->mark_read_pos = 0;
            dec->mark_write_pos = 0;
            TRACE_PRINT("ASYNC: mark buffer empty!\n");
        }
    }
    else
    {
        buff->header->hMarkTargetComponent =
            inbuff->header->hMarkTargetComponent;
        buff->header->pMarkData = inbuff->header->pMarkData;
    }

#ifdef ANDROID_MOD
    if (dec->dispatchOutputImmediately) 
    {
        if(buff)
        {
            if(HantroOmx_port_is_tunneled(&dec->out))
            {
                ((OMX_COMPONENTTYPE *) dec->out.tunnelcomp)->EmptyThisBuffer(dec->out.tunnelcomp,
                                                                             buff->header);
            }
            else
            {
                //TRACE_PRINT("ASYNC: firing FillBufferDone header:%p\n", &dec->buffer->header);
                dec->callbacks.FillBufferDone(dec->self, dec->appdata,buff->header);
            }
        }
    } 
    else 
#endif
    {
        if(dec->buffer)
        {
            if(HantroOmx_port_is_tunneled(&dec->out))
            {
                ((OMX_COMPONENTTYPE *) dec->out.tunnelcomp)->EmptyThisBuffer(dec->out.tunnelcomp,
                                                                             dec->buffer->header);
                dec->buffer = NULL;
            }
            else
            {
                //TRACE_PRINT("ASYNC: firing FillBufferDone header:%p\n", &dec->buffer->header);
                dec->callbacks.FillBufferDone(dec->self, dec->appdata,dec->buffer->header);
                dec->buffer = NULL;
            }
        }
        // defer returning the buffer untill later time.
        // this is because we dont know when we're sending the last buffer cause it
        // is possible that the codec has internal buffering for frames, and only after codec::getframe
        // has been invoked do we know when there are no more frames available. Unless the buffer sending
        // was deferred, there would be no way to stamp the last buffer with OMX_BUFFERFLAG_EOS
        dec->buffer = buff;
    }

    HantroOmx_port_lock_buffers(&dec->out);
    HantroOmx_port_pop_buffer(&dec->out);
    HantroOmx_port_unlock_buffers(&dec->out);
}

static void async_dispatch_last_frame(OMX_DECODER * dec)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

#ifdef OMX_DECODER_IMAGE_DOMAIN
    if(strcmp((char*)dec->role, "image_decoder.webp") != 0)
    {
        BUFFER *buff = NULL;

        HantroOmx_port_lock_buffers(&dec->out);
        HantroOmx_port_get_buffer(&dec->out, &buff);
        HantroOmx_port_unlock_buffers(&dec->out);

        buff->header->nOffset = 0;
        buff->header->nFilledLen = dec->imageSize;
#ifdef OMX_SKIP64BIT
        buff->header->nTimeStamp.nLowPart = buff->header->nTimeStamp.nHighPart = 0;
#else
        buff->header->nTimeStamp = 0;
#endif
        buff->header->nFlags |= OMX_BUFFERFLAG_EOS;

        //printf("IMAGE size %d\n", dec->imageSize);

        dec->buffer = buff;
    }
#endif

    if(dec->buffer == NULL)
        return;

    dec->buffer->header->nFlags |= OMX_BUFFERFLAG_EOS;
    if(HantroOmx_port_is_tunneled(&dec->out))
    {
        ((OMX_COMPONENTTYPE *) dec->out.tunnelcomp)->EmptyThisBuffer(dec->out.
                                                                     tunnelcomp,
                                                                     dec->
                                                                     buffer->
                                                                     header);
        dec->buffer = NULL;
    }
    else
    {
        dec->callbacks.FillBufferDone(dec->self, dec->appdata,
                                      dec->buffer->header);
        dec->buffer = NULL;
    }

#ifdef OMX_DECODER_IMAGE_DOMAIN
    if(strcmp((char*)dec->role, "image_decoder.webp") != 0)
    {
        HantroOmx_port_lock_buffers(&dec->out);
        HantroOmx_port_pop_buffer(&dec->out);
        HantroOmx_port_unlock_buffers(&dec->out);
    }
#endif

}

static
    OMX_ERRORTYPE async_get_frames(OMX_DECODER * dec, OMX_BOOL EOS,
                                   BUFFER * inbuff)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);
    OMX_ERRORTYPE err = OMX_ErrorNone;


#ifdef DSPG_TIMESTAMP_FIX
        // insert input buffer timestamp into the queue
        dec->timestampsQueue[dec->timestampsQueueTail] = inbuff->header->nTimeStamp;
        TRACE_PRINT("---------> [TIMESTAMP_FIX] added timestamp %llu at %lu (inbuff size = %lu)", inbuff->header->nTimeStamp, dec->timestampsQueueTail, inbuff->header->nFilledLen);
        dec->timestampsQueueTail++;
        dec->timestampsQueueTail %= TIMESTAMPS_QUEUE_SIZE;
#endif

    CODEC_STATE state = CODEC_HAS_FRAME;

    while(state == CODEC_HAS_FRAME && !dec->outputPortFlushPending)
    {
        FRAME frm;

        memset(&frm, 0, sizeof(FRAME));
        err = async_get_frame_buffer(dec, &frm);
        if(err != OMX_ErrorNone || dec->outputPortFlushPending)
            return err;
        state = dec->codec->getframe(dec->codec, &frm, EOS);
        //TRACE_PRINT("ASYNC getframe stat %d\n", state);
        //TRACE_PRINT("ASYNC getframe isIntra %d isGoldenOrAlternate %d\n", dec->isIntra, dec->isGoldenOrAlternate);
        if(state == CODEC_OK)
            break;
#ifdef ENABLE_CODEC_VP8
        dec->isIntra = frm.isIntra;
        dec->isGoldenOrAlternate = frm.isGoldenOrAlternate;
#endif
        if(state < 0)
        {
            TRACE_PRINT("ASYNC: getframe error:%d\n", state);
            return OMX_ErrorHardware;
        }
        if(frm.size)
            async_dispatch_frame_buffer(dec, OMX_FALSE, inbuff, &frm);
    }

#ifdef OMX_DECODER_VIDEO_DOMAIN
#ifdef ANDROID_MOD
    // this code and the expression that was added in the while loop above (outputPortFlushPending) solves the video seek bug.
    TRACE_PRINT("ASYNC: %s (state = %d, dec->outputPortFlushPending = %d)", __func__, state, dec->outputPortFlushPending);
    if (dec->outputPortFlushPending) {
        /* flush also the codec internal buffers, do nothing with them */
        while (state == CODEC_HAS_FRAME) {
            FRAME frm;
    
            memset(&frm, 0, sizeof(FRAME));
            err = async_get_frame_buffer(dec, &frm);
            if(err != OMX_ErrorNone) {
                TRACE_PRINT("error when calling async_get_frame_buffer, ignore.");
                break;
            }

            TRACE_PRINT("ASYNC: %s flushing internal codec frame using buffer bus address: 0x%lx", __func__, frm.fb_bus_address);         

            state = dec->codec->getframe(dec->codec, &frm, EOS);
#ifdef DSPG_TIMESTAMP_FIX
            if (state == CODEC_HAS_FRAME) {
                dec->timestampsQueueHead++;
                dec->timestampsQueueHead %= TIMESTAMPS_QUEUE_SIZE;
                TRACE_PRINT("---> [TIMESTAMP_FIX] increment head by 1 (tail = %lu, head = %lu)", dec->timestampsQueueTail, dec->timestampsQueueHead);
            }
#endif

        }
    }
#endif // ANDROID_MOD
#endif // OMX_DECODER_VIDEO_DOMAIN


    return OMX_ErrorNone;
}

static
    OMX_ERRORTYPE async_decode_data(OMX_DECODER * dec, OMX_U8 * bus_data,
                                    OSAL_BUS_WIDTH bus_address, OMX_U32 datalen,
                                    BUFFER * buff, OMX_U32 * retlen)
{
    CALLSTACK;
    assert(dec);
    assert(retlen);
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    OMX_BOOL EOS =
        buff->header->nFlags & OMX_BUFFERFLAG_EOS ? OMX_TRUE : OMX_FALSE;
    OMX_BOOL sendEOS = OMX_FALSE;

    OMX_ERRORTYPE err = OMX_ErrorNone;


#if 0
    {
        /* Dump input buffer into file for tracing */
        static int frameCtr = 0;

        char filename[30];

        FILE *fp;

        sprintf(filename, "input%d.dump", frameCtr++);
        fp = fopen(filename, "wb");
        if(fp)
        {
            fwrite(bus_data, datalen, 1, fp);
            fclose(fp);
        }
    }
#endif
    while( (datalen > 0) && (!dec->outputPortFlushPending) )
    {
        OMX_U32 first = 0;   // offset to the start of the first frame in the buffer

        OMX_U32 last = 0;    // offset to the start of the last frame in the buffer

        STREAM_BUFFER stream;

        stream.bus_data = bus_data; // + offset
        stream.bus_address = bus_address;   // + offset
        stream.streamlen = datalen; // - offset
        // see if we can find complete frames in the buffer
        int ret = dec->codec->scanframe(dec->codec, &stream, &first, &last);

        if(ret == -1 || first == last)
        {
            TRACE_PRINT("ASYNC: no decoding unit in input buffer\n");
            if(EOS == OMX_FALSE)
                break;
            // assume that remaining data contains a single complete decoding unit
            // fingers crossed..
            first = 0;
            last = datalen;
            // if there _isn't_ that remaining frame then an output buffer
            // with EOS flag is never sent. oh well.
            sendEOS = OMX_TRUE;
        }

        // got at least one complete frame between first and last
        stream.bus_data = bus_data + first;
        stream.bus_address = bus_address + first;   // is this ok?
        stream.streamlen = last - first;
        stream.sliceInfoNum =  dec->sliceInfoNum;
        stream.pSliceInfo =  dec->pSliceInfo;

        OMX_U32 bytes = 0;

        FRAME frm;

        memset(&frm, 0, sizeof(FRAME));
        err = async_get_frame_buffer(dec, &frm);
        TRACE_PRINT("async_get_frame_buffer done\n");
        if(err != OMX_ErrorNone)
        {
            if(err == OMX_ErrorOverflow)
            {
                TRACE_PRINT("ASYNC: firing OMX_EventErrorOverflow\n");
                dec->callbacks.EventHandler(dec->self, dec->appdata,
                                            OMX_EventError, OMX_ErrorOverflow,
                                            0, NULL);
                *retlen = datalen;
                return OMX_ErrorNone;
            }
            LOGE("[%s] error getting frame buffer (output buffer)", __func__);
            return err;
        }

        CODEC_STATE codec =
            dec->codec->decode(dec->codec, &stream, &bytes, &frm);

#ifndef OMX_DECODER_IMAGE_DOMAIN
        if(frm.size)
            async_dispatch_frame_buffer(dec, OMX_FALSE, buff, &frm);
#endif
        // note: handle stream errors

        OMX_BOOL dobreak = OMX_FALSE;

#ifdef DYNAMIC_SCALING
		{
			char buff[16];
			int requestedWidth, requestedHeight;
			size_t ret = read(dec->scalerFd, buff, 16);
			if (ret > 0) {
				sscanf(buff,"%dx%d",&requestedWidth, &requestedHeight);
				if (async_set_new_pp_args(dec, requestedWidth, requestedHeight) == OMX_ErrorNone) {
					codec = CODEC_HAS_INFO;
				}
			}
		}
#endif 

        switch (codec)
        {
        case CODEC_NEED_MORE:
            LOGV("streamlen = %lu bytes", stream.streamlen);
            LOGV("dec->codec->decode() returned CODEC_NEED_MORE");
            break;
        case CODEC_HAS_INFO:
            err = async_get_info(dec);
            if(err != OMX_ErrorNone)
                return err;
            break;
        case CODEC_HAS_FRAME:
            err = async_get_frames(dec, sendEOS, buff);
            if(err != OMX_ErrorNone)
            {
                if(err == OMX_ErrorOverflow)
                {
                    TRACE_PRINT("ASYNC: saving decode state\n");
                    dec->codecstate = CODEC_HAS_FRAME;
                    dobreak = OMX_TRUE;
                    break;
                }
                return err;
            }
            break;
        case CODEC_ERROR_STREAM:
            /* Stream error occured, force frame output
             * This is because in conformance tests the EOS flag
             * is forced and stream ends prematurely. */
            LOGW("dec->codec->decode() returned CODEC_ERROR_STREAM");
            frm.size = dec->out.def.nBufferSize;
            async_dispatch_frame_buffer(dec, EOS, buff, &frm);
            // TODO: need to find some logic to understand when to return error and when to conceal it
            //return OMX_ErrorUndefined;
            break;
        case CODEC_ERROR_STREAM_NOT_SUPPORTED:
            LOGE("dec->codec->decode() returned CODEC_ERROR_STREAM_NOT_SUPPORTED");
            return OMX_ErrorNotImplemented;
        case CODEC_ERROR_FORMAT_NOT_SUPPORTED:
            LOGE("dec->codec->decode() returned CODEC_ERROR_FORMAT_NOT_SUPPORTED");
            return OMX_ErrorNotImplemented;
        case CODEC_ERROR_INVALID_ARGUMENT:
            LOGE("dec->codec->decode() returned CODEC_ERROR_INVALID_ARGUMENT");
            return OMX_ErrorBadParameter;
        default:
            TRACE_PRINT("ASYNC: unexpected codec return value: %d\n",
                        codec);
            return OMX_ErrorUndefined;
        }
        if(bytes > 0)
        {
            // ugh, crop consumed bytes from the buffer, maintain alignment
            OMX_U32 rem = datalen - bytes - first;

            memmove(bus_data, bus_data + first + bytes, rem);
            datalen -= (bytes + first);
        }
        if(dobreak)
            break;
    }
    *retlen = datalen;
    if(EOS == OMX_TRUE)
    {
#ifndef OMX_DECODER_IMAGE_DOMAIN
        // flush decoder output buffers
        if(strcmp((char*)dec->role, "video_decoder.jpeg") != 0)
        {
            TRACE_PRINT("ASYNC: Flush video decoder output buffers\n");
            err = async_get_frames(dec, 1, buff);
            if(err != OMX_ErrorNone)
            {
                TRACE_PRINT("ASYNC: EOS get frames err: %d\n", err);
            }
        }
#endif
        async_dispatch_last_frame(dec);
        TRACE_PRINT("ASYNC: sending OMX_EventBufferFlag\n");
        dec->callbacks.EventHandler(dec->self, dec->appdata,
                                    OMX_EventBufferFlag, PORT_INDEX_OUTPUT,
                                    buff->header->nFlags, NULL);
    }

    if(err == OMX_ErrorOverflow)
    {
        TRACE_PRINT("ASYNC: firing OMX_ErrorOverflow\n");
        dec->callbacks.EventHandler(dec->self, dec->appdata, OMX_EventError,
                                    err, 0, NULL);
    }
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE async_decoder_decode(OMX_DECODER * dec)
{
    CALLSTACK;
    assert(dec->codec);

    // grab the next input buffer and decode data out of it
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    BUFFER *buff = NULL;

    OMX_ERRORTYPE err = OMX_ErrorNone;

    err = HantroOmx_port_lock_buffers(&dec->in);
    if(err != OMX_ErrorNone)
        goto INVALID_STATE;

    HantroOmx_port_get_buffer(&dec->in, &buff);
    HantroOmx_port_unlock_buffers(&dec->in);
    if(!buff) {
        TRACE_PRINT("ASYNC: %s no buffer\n", __FUNCTION__);
        return OMX_ErrorNone;
    }
#ifdef OMX_DECODER_IMAGE_DOMAIN
    if(buff->header->nFilledLen == 0)
    {
        // this hack is because in the BufferFlag test the component needs
        // to output a buffer and propage the EOS flag. However without
        // getting an input EOS there's never going to be any output, since
        // the input EOS is used to indicate the end of stream and that we have
        // a complete JPEG decoding unit in the buffer.
        buff->header->nFlags |= OMX_BUFFERFLAG_EOS;
        TRACE_PRINT("ASYNC: faking EOS flag!\n");
    }
#endif

    if(buff->header->nFlags & OMX_BUFFERFLAG_EXTRADATA)
    {
        OMX_OTHER_EXTRADATATYPE * segDataBuffer;
        TRACE_PRINT("ASYNC: Extracting EXTRA data from buffer\n");


        segDataBuffer = (OMX_OTHER_EXTRADATATYPE *) (((OMX_U32)buff->header->pBuffer + buff->header->nFilledLen +3)& ~3);

        dec->sliceInfoNum = segDataBuffer->nDataSize/8;
        dec->pSliceInfo = segDataBuffer->data;
    } else if(buff->header->nFlags & 0x10000000)
    {
        OMX_OTHER_EXTRADATATYPE * segDataBuffer;
        TRACE_PRINT("ASYNC: Extracting EXTRA data from buffer\n");

        OMX_U32 * pOrgDataSize = (OMX_U32 *) (((OMX_U32)buff->header->pBuffer + buff->header->nFilledLen +3)& ~3);
        OMX_U32 org_size = *--pOrgDataSize;
        buff->header->nFilledLen=org_size;
        int size1 = ((org_size +3)& ~3);
        segDataBuffer = (OMX_OTHER_EXTRADATATYPE *) ((OMX_U32)buff->header->pBuffer + size1);

        dec->sliceInfoNum = segDataBuffer->nDataSize/8;
        dec->pSliceInfo = segDataBuffer->data;
    }

    if(dec->codecstate == CODEC_HAS_FRAME)
    {
        TRACE_PRINT("has frame\n");
        OMX_BOOL EOS = buff->header->nFlags & OMX_BUFFERFLAG_EOS;

        err = async_get_frames(dec, EOS, buff);
        if(err == OMX_ErrorOverflow)
        {
            TRACE_PRINT("ASYNC: firing OMX_EventErrorOverflow\n");
            dec->callbacks.EventHandler(dec->self, dec->appdata, OMX_EventError,
                                        OMX_ErrorOverflow, 0, NULL);
            return OMX_ErrorNone;
        }
        if(err != OMX_ErrorNone)
            goto INVALID_STATE;

        dec->codecstate = CODEC_OK;
    }

    assert(buff);
    assert(buff->header->nInputPortIndex == 0 &&
           "post processor input buffer in wrong place");

    FRAME_BUFFER *temp = &dec->frame_in;

    // if there is previous data in the frame buffer left over from a previous call to decode
    // or if the buffer has possibly misaligned offset or if the buffer is allocated by the client
    // invoke the decoding through the temporary frame buffer
    if(temp->size != 0 || buff->header->nOffset != 0 ||
       !(buff->flags & BUFFER_FLAG_MY_BUFFER))
    {
        OMX_U32 capacity = temp->capacity;

        OMX_U32 needed = buff->header->nFilledLen;
        while(1)
        {
            OMX_U32 available = capacity - temp->size;

            if(available > needed)
                break;
            capacity *= 2;
        }
        if(capacity != temp->capacity)
        {
            err = grow_frame_buffer(dec, temp, capacity);
            if(err != OMX_ErrorNone)
                goto INVALID_STATE;
        }
        assert(temp->capacity - temp->size >= buff->header->nFilledLen);

        // copy input buffer data into the temporary input buffer
        OMX_U32 len = buff->header->nFilledLen;

        OMX_U8 *src = buff->header->pBuffer + buff->header->nOffset;

        OMX_U8 *dst = temp->bus_data + temp->size;  // append to the buffer

        memcpy(dst, src, len);
        temp->size += len;

        OMX_U32 retlen = 0;

        // decode data as much as we can
        err =
            async_decode_data(dec, temp->bus_data, temp->bus_address,
                              temp->size, buff, &retlen);
        if(err != OMX_ErrorNone)
            goto INVALID_STATE;

        temp->size = retlen;
    }
    else
    {
        // take input directly from the input buffer
        assert(buff->header->nOffset == 0);
        assert(buff->bus_data == buff->header->pBuffer);
        assert(buff->flags & BUFFER_FLAG_MY_BUFFER);

        OMX_U8 *bus_data = buff->bus_data;

        OSAL_BUS_WIDTH bus_address = buff->bus_address;

        OMX_U32 filledlen = buff->header->nFilledLen;

        OMX_U32 retlen = 0;

        // decode data as much as we can
        err =
            async_decode_data(dec, bus_data, bus_address, filledlen, buff,
                              &retlen);
        if(err != OMX_ErrorNone)
            goto INVALID_STATE;

        // if some data was left over (implying partial decoding units)
        // copy this data into the temporary buffer
        if(retlen)
        {
            OMX_U32 capacity = temp->capacity;

            OMX_U32 needed = retlen;
            while(1)
            {
                OMX_U32 available = capacity - temp->size;

                if(available > needed)
                    break;
                capacity *= 2;
            }
            if(capacity != temp->capacity)
            {
                err = grow_frame_buffer(dec, temp, capacity);
                if(err != OMX_ErrorNone)
                    goto INVALID_STATE;
            }
            assert(temp->capacity - temp->size >= retlen);

            OMX_U8 *dst = temp->bus_data + temp->size;

            OMX_U8 *src = bus_data;

            memcpy(dst, src, retlen);
            temp->size += retlen;
        }
    }

    // A component generates the OMX_EventMark event when it receives a marked buffer.
    // When a component receives a buffer, it shall compare its own pointer to the
    // pMarkTargetComponent field contained in the buffer. If the pointers match, then the
    // component shall send a mark event including pMarkData as a parameter, immediately
    // after the component has finished processing the buffer.

    // note that this *needs* to be before the emptybuffer done callback because the tester
    // clears the mark fields.
    if(buff->header->hMarkTargetComponent == dec->self)
    {
        dec->callbacks.EventHandler(dec->self, dec->appdata, OMX_EventMark, 0,
                                    0, buff->header->pMarkData);
        buff->header->hMarkTargetComponent = NULL;
        buff->header->pMarkData = NULL;
    }
    if(HantroOmx_port_is_tunneled(&dec->in))
    {
        ((OMX_COMPONENTTYPE *) dec->in.tunnelcomp)->FillThisBuffer(dec->in.
                                                                   tunnelcomp,
                                                                   buff->
                                                                   header);
    }
    else
    {
        dec->callbacks.EmptyBufferDone(dec->self, dec->appdata, buff->header);
    }

    HantroOmx_port_lock_buffers(&dec->in);
    HantroOmx_port_pop_buffer(&dec->in);
    HantroOmx_port_unlock_buffers(&dec->in);

    TRACE_PRINT("ASYNC: %s done\n", __FUNCTION__);
    return OMX_ErrorNone;

  INVALID_STATE:
    assert(err != OMX_ErrorNone);
    TRACE_PRINT("ASYNC: error while processing buffers: %s\n",
                HantroOmx_str_omx_err(err));
    if(buff != NULL)
        TRACE_PRINT("ASYNC: current buffer: %p\n", buff);
    dec->state = OMX_StateInvalid;
    dec->run = OMX_FALSE;
    dec->callbacks.EventHandler(dec->self, dec->appdata, OMX_EventError, err, 0,
                                NULL);

    TRACE_PRINT("ASYNC: %s done (error)\n", __FUNCTION__);
    return err;
}

static OMX_ERRORTYPE async_decoder_set_mask(OMX_DECODER * dec)
{
    CALLSTACK;
    TRACE_PRINT("ASYNC: %s\n", __FUNCTION__);

    BUFFER *buff = NULL;

    OMX_ERRORTYPE err = OMX_ErrorNone;

    err = HantroOmx_port_lock_buffers(&dec->inpp);
    if(err != OMX_ErrorNone)
        goto INVALID_STATE;

    HantroOmx_port_get_buffer(&dec->inpp, &buff);
    HantroOmx_port_unlock_buffers(&dec->inpp);
    if(!buff) {
        TRACE_PRINT("ASYNC: %s no buffer\n", __FUNCTION__);
        return OMX_ErrorNone;
    }

    assert(dec->mask.bus_data);
    assert(dec->mask.bus_address);

    if(buff->header->nFilledLen != dec->mask.capacity)
    {
        // note: should one generate some kind of an error
        // if there's less data provided than what has been specified
        // in the port dimensions??
        TRACE_PRINT("ASYNC: alpha blend mask buffer data length does not match expected data length nFilledLen: %d expected: %d\n",
                    (int) buff->header->nFilledLen, (int) dec->mask.capacity);
    }

    OMX_U32 minbytes =
        buff->header->nFilledLen <
        dec->mask.capacity ? buff->header->nFilledLen : dec->mask.capacity;

    assert(dec->mask.bus_data);
    //assert(dec->mask.capacity >= buff->header->nFilledLen);
    // copy the mask data into the internal buffer
    memcpy(dec->mask.bus_data, buff->header->pBuffer, minbytes);
    //memcpy(dec->mask.bus_data, buff->header->pBuffer, buff->header->nFilledLen);

    if(buff->header->hMarkTargetComponent == dec->self)
    {
        dec->callbacks.EventHandler(dec->self, dec->appdata, OMX_EventMark, 0,
                                    0, buff->header->pMarkData);
        buff->header->pMarkData = NULL;
        buff->header->hMarkTargetComponent = NULL;
    }
    if(HantroOmx_port_is_tunneled(&dec->inpp))
    {
        ((OMX_COMPONENTTYPE *) dec->inpp.tunnelcomp)->FillThisBuffer(dec->inpp.
                                                                     tunnelcomp,
                                                                     buff->
                                                                     header);
    }
    else
    {
        dec->callbacks.EmptyBufferDone(dec->self, dec->appdata, buff->header);
    }

    HantroOmx_port_lock_buffers(&dec->inpp);
    HantroOmx_port_pop_buffer(&dec->inpp);
    HantroOmx_port_unlock_buffers(&dec->inpp);

    TRACE_PRINT("ASYNC: %s done\n", __FUNCTION__);
    return OMX_ErrorNone;
  INVALID_STATE:
    assert(err != OMX_ErrorNone);
    TRACE_PRINT("ASYNC: error while processing buffer: %s\n",
                HantroOmx_str_omx_err(err));
    dec->state = OMX_StateInvalid;
    dec->run = OMX_FALSE;
    dec->callbacks.EventHandler(dec->self, dec->appdata, OMX_EventError, err, 0,
                                NULL);
    TRACE_PRINT("ASYNC: %s done (error)\n", __FUNCTION__);
    return err;
}
