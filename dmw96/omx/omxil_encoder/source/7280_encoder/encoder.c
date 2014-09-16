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
--  Description :  OMX Encoder component.
--
------------------------------------------------------------------------------*/

//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_7280_encoder_component"
#include <utils/Log.h>
#define TRACE_PRINT(a,b...) LOGV(b)

#include <OSAL.h>
#include <basecomp.h>
#include <port.h>
#include <util.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include "encoder_version.h"
#include "encoder.h"

#include "encoder_h264.h"
#include "encoder_jpeg.h"
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
#include "encoder_mpeg4.h"
#include "encoder_h263.h"
#endif

#ifdef ENCH1
#include "encoder_vp8.h"
#include "encoder_webp.h"
#endif

#define PORT_INDEX_INPUT     0
#define PORT_INDEX_OUTPUT    1
#define RETRY_INTERVAL       5
#define TIMEOUT              2000

static OMX_ERRORTYPE async_encoder_set_state(OMX_COMMANDTYPE, OMX_U32, OMX_PTR, OMX_PTR);
static OMX_ERRORTYPE async_encoder_disable_port(OMX_COMMANDTYPE, OMX_U32, OMX_PTR, OMX_PTR);
static OMX_ERRORTYPE async_encoder_enable_port(OMX_COMMANDTYPE, OMX_U32, OMX_PTR, OMX_PTR);
static OMX_ERRORTYPE async_encoder_flush_port(OMX_COMMANDTYPE, OMX_U32, OMX_PTR, OMX_PTR);
static OMX_ERRORTYPE async_encoder_mark_buffer(OMX_COMMANDTYPE, OMX_U32, OMX_PTR, OMX_PTR);


// video domain specific functions
#ifdef OMX_ENCODER_VIDEO_DOMAIN
static OMX_ERRORTYPE set_avc_defaults(OMX_ENCODER* pEnc);
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
static OMX_ERRORTYPE set_mpeg4_defaults(OMX_ENCODER* pEnc);
static OMX_ERRORTYPE set_h263_defaults(OMX_ENCODER* pEnc);
#endif
#ifdef ENCH1
static OMX_ERRORTYPE set_vp8_defaults(OMX_ENCODER* pEnc);
#endif
static OMX_ERRORTYPE set_bitrate_defaults(OMX_ENCODER* pEnc);
static OMX_ERRORTYPE async_video_encoder_encode(OMX_ENCODER* pEnc);
static OMX_ERRORTYPE async_encode_stabilized_data(OMX_ENCODER* pEnc, OMX_U8* bus_data, OSAL_BUS_WIDTH bus_address,
                                OMX_U32 datalen, BUFFER* buff, OMX_U32* retlen, OMX_U32 frameSize );
static OMX_ERRORTYPE async_encode_video_data(OMX_ENCODER* pEnc, OMX_U8* bus_data, OSAL_BUS_WIDTH bus_address,
                                OMX_U32 datalen, BUFFER* buff, OMX_U32* retlen, OMX_U32 frameSize );
static OMX_ERRORTYPE set_avc_intra_period(OMX_ENCODER* pEnc);
static OMX_ERRORTYPE set_frame_rate(OMX_ENCODER* pEnc);
#endif
// image domain specific functions
#ifdef OMX_ENCODER_IMAGE_DOMAIN
static OMX_ERRORTYPE set_jpeg_defaults(OMX_ENCODER* pEnc);
#ifdef ENCH1
static OMX_ERRORTYPE set_webp_defaults(OMX_ENCODER* pEnc);
#endif
static OMX_ERRORTYPE async_image_encoder_encode(OMX_ENCODER* pEnc);
static OMX_ERRORTYPE async_encode_image_data(OMX_ENCODER* pEnc, OMX_U8* bus_data, OSAL_BUS_WIDTH bus_address,
                                OMX_U32 datalen, BUFFER* buff, OMX_U32* retlen, OMX_U32 frameSize );
#endif
static OMX_ERRORTYPE set_preprocessor_defaults(OMX_ENCODER* pEnc);
static OMX_ERRORTYPE calculate_frame_size(OMX_ENCODER* pEnc, OMX_U32* frameSize);

static OMX_ERRORTYPE calculate_new_outputBufferSize(OMX_ENCODER* pEnc);



#define FRAME_BUFF_CAPACITY(fb) (fb)->capacity - (fb)->size

#define FRAME_BUFF_FREE(alloc, fb) \
 OSAL_AllocatorFreeMem((alloc), (fb)->capacity, (fb)->bus_data, (fb)->bus_address)

#define GET_ENCODER(comp) (OMX_ENCODER*)(((OMX_COMPONENTTYPE*)(comp))->pComponentPrivate)



/**
 * static OMX_U32 encoder_thread_main(BASECOMP* base, OMX_PTR arg)
 * Thread function.
 * 1. Initializes signals for command and buffer queue
 * 2. Starts to wait signals for command or buffer queue
 * 3. Processes commands
 * 4. Encodes received frames
 * 5. On exit sets state invalid
 * @param BASECOMP* base - Base component instance
 * @param OMX_PTR arg - Instance of OMX encoder component
 */
static
OMX_U32 encoder_thread_main(BASECOMP* base, OMX_PTR arg)
{
    assert(arg);
    OMX_ENCODER* this = (OMX_ENCODER*)arg;

    OMX_HANDLETYPE handles[2];
    handles[0] = this->base.queue.event;  // event handle for the command queue
    handles[1] = this->inputPort.bufferevent;    // event handle for the normal input port input buffer queue

    OMX_ERRORTYPE err = OMX_ErrorNone;
    OSAL_BOOL timeout = OMX_FALSE;
    OSAL_BOOL signals[2];
    
    OSAL_BOOL bInFlushState = OMX_FALSE ;

    while (this->run)
    {
        // clear all signal indicators
        signals[0] = OMX_FALSE;
        signals[1] = OMX_FALSE;

        // wait for command messages and buffers
        err = OSAL_EventWaitMultiple(handles, signals, 2, INFINITE_WAIT, &timeout);
        if (err != OMX_ErrorNone)
        {
            TRACE_PRINT(this->log, "ASYNC: waiting for events failed: %s\n", HantroOmx_str_omx_err(err));
            break;
        }
        if (signals[0] == OMX_TRUE)
        {
            CMD cmd;
            while (1)
            {
                OMX_BOOL ok = OMX_TRUE;
                err = HantroOmx_basecomp_try_recv_command(&this->base, &cmd, &ok);
                if (err != OMX_ErrorNone)
                {
                    TRACE_PRINT(this->log, "ASYNC: basecomp_try_recv_command failed: %s\n", HantroOmx_str_omx_err(err));
                    this->run = OMX_FALSE;
                    break;
                }
                if (ok == OMX_FALSE)
                    break;
                if (cmd.type == CMD_EXIT_LOOP)
                {
                    TRACE_PRINT(this->log, "ASYNC: got CMD_EXIT_LOOP\n");
                    this->run = OMX_FALSE;
                    break;
                }
                
                if (cmd.arg.cmd == OMX_CommandFlush) {
                    bInFlushState = OMX_TRUE;
                    TRACE_PRINT(this->log, "Entering into FLUSHING STATE");
                } else if (cmd.arg.cmd == OMX_CommandPortEnable) {
                    bInFlushState = OMX_FALSE;
                    TRACE_PRINT(this->log, "Coming out of FLUSHING STATE");
                }
                
                HantroOmx_cmd_dispatch(&cmd, this);
            }
        }

		/* Dont encode during flush mode, utill you get PortEnable */
        if (signals[1] == OMX_TRUE && this->state == OMX_StateExecuting && bInFlushState == OMX_FALSE)
        {
#ifdef OMX_ENCODER_VIDEO_DOMAIN
            async_video_encoder_encode(this);
#endif
#ifdef OMX_ENCODER_IMAGE_DOMAIN
            async_image_encoder_encode(this);
#endif
        }
    }
    if (err != OMX_ErrorNone)
    {
        TRACE_PRINT(this->log, "ASYNC: error: %s\n", HantroOmx_str_omx_err(err));
        TRACE_PRINT(this->log, "ASYNC: new state: %s\n", HantroOmx_str_omx_state(OMX_StateInvalid));
        this->state = OMX_StateInvalid;
        this->app_callbacks.EventHandler(this->self, this->app_data, OMX_EventError, OMX_ErrorInvalidState, 0, NULL);
    }
    TRACE_PRINT(this->log, "ASYNC: thread exit\n");
    return 0;
}

static
OMX_ERRORTYPE encoder_get_version(OMX_IN  OMX_HANDLETYPE   hComponent,
                                  OMX_OUT OMX_STRING       pComponentName,
                                  OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
                                  OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
                                  OMX_OUT OMX_UUIDTYPE*    pComponentUUID)
{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pComponentName);
    CHECK_PARAM_NON_NULL(pComponentVersion);
    CHECK_PARAM_NON_NULL(pSpecVersion);
    CHECK_PARAM_NON_NULL(pComponentUUID);

    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);

#ifdef OMX_ENCODER_VIDEO_DOMAIN
    strncpy(pComponentName, VIDEO_COMPONENT_NAME, OMX_MAX_STRINGNAME_SIZE-1);
#endif
#ifdef OMX_ENCODER_IMAGE_DOMAIN
    strncpy(pComponentName, IMAGE_COMPONENT_NAME, OMX_MAX_STRINGNAME_SIZE-1);
#endif

    pComponentVersion->s.nVersionMajor = COMPONENT_VERSION_MAJOR;
    pComponentVersion->s.nVersionMinor = COMPONENT_VERSION_MINOR;
    pComponentVersion->s.nRevision     = COMPONENT_VERSION_REVISION;
    pComponentVersion->s.nStep         = COMPONENT_VERSION_STEP;

    pSpecVersion->s.nVersionMajor      = 1; // this is the OpenMAX IL version. Has nothing to do with component version.
    pSpecVersion->s.nVersionMinor      = 1;
    pSpecVersion->s.nRevision          = 2;
    pSpecVersion->s.nStep              = 0;

    HantroOmx_generate_uuid(hComponent, pComponentUUID);
    return OMX_ErrorNone;
}

/**
 * void encoder_dealloc_buffers(OMX_ENCODER* pEnc, PORT* p)
 * Deallocates all buffers that are owned by this component
 */
static
void encoder_dealloc_buffers(OMX_ENCODER* pEnc, PORT* p)
{
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);

    assert(p);
    OMX_U32 count = HantroOmx_port_buffer_count(p);
    OMX_U32 i;
    for (i=0; i<count; ++i)
    {
        BUFFER* buff = NULL;
        HantroOmx_port_get_allocated_buffer_at(p, &buff, i);
        assert(buff);
        if (buff->flags & BUFFER_FLAG_MY_BUFFER)
        {
            assert(buff->header == &buff->headerdata);
            assert(buff->bus_address);
            assert(buff->bus_data);
            OSAL_AllocatorFreeMem(&pEnc->alloc, buff->allocsize, buff->bus_data, buff->bus_address);
        }
    }
}


/**
 * OMX_ERRORTYPE encoder_deinit( OMX_HANDLETYPE hComponent )
 * Deinitializes encoder component
 */
static
OMX_ERRORTYPE encoder_deinit( OMX_HANDLETYPE hComponent )
{
    CHECK_PARAM_NON_NULL(hComponent);

    if (((OMX_COMPONENTTYPE*)hComponent)->pComponentPrivate == NULL)
        return OMX_ErrorNone;

    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: waiting for thread to finish\n");

    if(pEnc->base.thread)
    {
        // bring down the component thread, and join it
        pEnc->run = OMX_FALSE;
        CMD c;
        INIT_EXIT_CMD(c);
        HantroOmx_basecomp_send_command(&pEnc->base, &c);
        OSAL_ThreadSleep(RETRY_INTERVAL);
        HantroOmx_basecomp_destroy(&pEnc->base);
    }

    assert(HantroOmx_port_is_allocated(&pEnc->inputPort) == OMX_TRUE);
    assert(HantroOmx_port_is_allocated(&pEnc->outputPort) == OMX_TRUE);

    if (pEnc->state != OMX_StateLoaded)
    {
        // if there's stuff in the input/output port buffer queues
        // simply ignore it

        // deallocate allocated buffers (if any)
        // this could have catastrophic consequences if someone somewhere is
        // is still holding a pointer to these buffers... (what could be done here, except leak?)

        encoder_dealloc_buffers(pEnc, &pEnc->inputPort);
        encoder_dealloc_buffers(pEnc, &pEnc->outputPort);
        if (pEnc->codec)
            pEnc->codec->destroy(pEnc->codec);
        /*if (pEnc->frame_in.bus_address)
            FRAME_BUFF_FREE(&pEnc->alloc, &pEnc->frame_in);*/
        if (pEnc->frame_out.bus_address)
            FRAME_BUFF_FREE(&pEnc->alloc, &pEnc->frame_out);
    }
    else
    {
        // ports should not have any queued buffers at this point anymore.
        assert(HantroOmx_port_buffer_queue_count(&pEnc->inputPort) == 0);
        assert(HantroOmx_port_buffer_queue_count(&pEnc->outputPort) == 0);

        // ports should not have any buffers allocated at this point anymore
        assert(HantroOmx_port_has_buffers(&pEnc->inputPort) == OMX_FALSE);
        assert(HantroOmx_port_has_buffers(&pEnc->outputPort) == OMX_FALSE);

        // temporary frame buffers should not exist anymore
        assert(pEnc->frame_in.bus_data == NULL);
        assert(pEnc->frame_out.bus_data == NULL);
    }
    HantroOmx_port_destroy(&pEnc->inputPort);
    HantroOmx_port_destroy(&pEnc->outputPort);


	TRACE_PRINT(pEnc->log, "%s: almost done", __FUNCTION__);
#ifdef ENCODER_TRACE
    if (pEnc->log && pEnc->log != stdout)
    {
        assert(fclose(pEnc->log) == 0);
    }
#endif

    OSAL_AllocatorDestroy(&pEnc->alloc);
    OSAL_Free(pEnc);
    return OMX_ErrorNone;
}

static
OMX_ERRORTYPE encoder_send_command(OMX_IN OMX_HANDLETYPE  hComponent,
                                   OMX_IN OMX_COMMANDTYPE Cmd,
                                   OMX_IN OMX_U32         nParam1,
                                   OMX_IN OMX_PTR         pCmdData)
{
    CHECK_PARAM_NON_NULL(hComponent);

    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);
    CHECK_STATE_INVALID(pEnc->state);

    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: Cmd:%s nParam1:%u pCmdData:%p\n", HantroOmx_str_omx_cmd(Cmd), (unsigned)nParam1, pCmdData);

    CMD c;
    OMX_ERRORTYPE err = OMX_ErrorNotImplemented;

    switch (Cmd)
    {
        case OMX_CommandStateSet:
            if (pEnc->statetrans != pEnc->state)
            {
                // transition is already pending
                return OMX_ErrorIncorrectStateTransition;
            }
            TRACE_PRINT(pEnc->log, "API: next state:%s\n", HantroOmx_str_omx_state((OMX_STATETYPE)nParam1));
            INIT_SEND_CMD(c, Cmd, nParam1, NULL, async_encoder_set_state);
            pEnc->statetrans = (OMX_STATETYPE)nParam1;
            err = HantroOmx_basecomp_send_command(&pEnc->base, &c);
            break;

        case OMX_CommandFlush:
            if (nParam1 > PORT_INDEX_OUTPUT && nParam1 != -1)
            {
                TRACE_PRINT(pEnc->log, "API: bad port index:%u\n", (unsigned)nParam1);
                return OMX_ErrorBadPortIndex;
            }
            INIT_SEND_CMD(c, Cmd, nParam1, NULL, async_encoder_flush_port);
            err = HantroOmx_basecomp_send_command(&pEnc->base, &c);
            break;

        case OMX_CommandPortDisable:
            if (nParam1 > PORT_INDEX_OUTPUT && nParam1 != -1)
            {
                TRACE_PRINT(pEnc->log, "API: bad port index:%u\n", (unsigned)nParam1);
                return OMX_ErrorBadPortIndex;
            }
            if ( nParam1 == PORT_INDEX_INPUT || nParam1 == -1 )
                pEnc->inputPort.def.bEnabled = OMX_FALSE;
            if ( nParam1 == PORT_INDEX_OUTPUT || nParam1 == -1 )
                pEnc->outputPort.def.bEnabled = OMX_FALSE;


            INIT_SEND_CMD(c, Cmd, nParam1, NULL, async_encoder_disable_port);
            err = HantroOmx_basecomp_send_command(&pEnc->base, &c);
            break;

        case OMX_CommandPortEnable:
            if (nParam1 > PORT_INDEX_OUTPUT && nParam1 != -1)
            {
                TRACE_PRINT(pEnc->log, "API: bad port index:%u\n", (unsigned)nParam1);
                return OMX_ErrorBadPortIndex;
            }
            if (nParam1 == PORT_INDEX_INPUT || nParam1 == -1)
                pEnc->inputPort.def.bEnabled = OMX_TRUE;
            if (nParam1 == PORT_INDEX_OUTPUT || nParam1 == -1)
                pEnc->outputPort.def.bEnabled = OMX_TRUE;


            INIT_SEND_CMD(c, Cmd, nParam1, NULL, async_encoder_enable_port);
            err = HantroOmx_basecomp_send_command(&pEnc->base, &c);
            break;

        case OMX_CommandMarkBuffer:
            if ((nParam1 != PORT_INDEX_INPUT) && (nParam1 != PORT_INDEX_OUTPUT))
            {
                TRACE_PRINT(pEnc->log, "API: bad port index:%u\n", (unsigned)nParam1);
                return OMX_ErrorBadPortIndex;
            }
            CHECK_PARAM_NON_NULL(pCmdData);
            OMX_MARKTYPE* mark = (OMX_MARKTYPE*)OSAL_Malloc(sizeof(OMX_MARKTYPE));
            if (!mark)
            {
                TRACE_PRINT(pEnc->log, "API: cannot marshall mark\n");
                return OMX_ErrorInsufficientResources;
            }
            memcpy(mark, pCmdData, sizeof(OMX_MARKTYPE));
            INIT_SEND_CMD(c, Cmd, nParam1, mark, async_encoder_mark_buffer);
            err = HantroOmx_basecomp_send_command(&pEnc->base, &c);
            if (err != OMX_ErrorNone)
                OSAL_Free(mark);

            break;

        default:
            TRACE_PRINT(pEnc->log, "API: bad command:%u\n", (unsigned)Cmd);
            err = OMX_ErrorBadParameter;
            break;
    }
    if (err != OMX_ErrorNone)
        TRACE_PRINT(pEnc->log, "API: error: %s\n", HantroOmx_str_omx_err(err));
    return err;
}

static
OMX_ERRORTYPE encoder_set_callbacks(OMX_IN OMX_HANDLETYPE    hComponent,
                                    OMX_IN OMX_CALLBACKTYPE* pCallbacks,
                                    OMX_IN OMX_PTR           pAppData)

{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pCallbacks);
    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);

    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: pCallbacks:%p pAppData:%p\n", pCallbacks, pAppData);

    pEnc->app_callbacks = *pCallbacks;
    pEnc->app_data   = pAppData;
    return OMX_ErrorNone;
}

static
OMX_ERRORTYPE encoder_get_state(OMX_IN  OMX_HANDLETYPE hComponent,
                                OMX_OUT OMX_STATETYPE* pState)
{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pState);
    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);
    *pState = pEnc->state;
    return OMX_ErrorNone;
}


/**
 * static PORT* encoder_map_index_to_port(OMX_ENCODER* pEnc, OMX_U32 portIndex)
 * Static function.
 * @param OMX_ENCODER - OMX encoder instance
 * @param OMX_U32 - Index indicating required port
 * @return PORT - input or output port depending on given index
 */
static
PORT* encoder_map_index_to_port(OMX_ENCODER* pEnc, OMX_U32 portIndex)
{
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    switch (portIndex)
    {
        TRACE_PRINT( pEnc->log, "ASYNC: portIndex - %d\n", (int)portIndex);
        case PORT_INDEX_INPUT:  return &pEnc->inputPort;
        case PORT_INDEX_OUTPUT: return &pEnc->outputPort;
    }
    return NULL;
}

/**
 * static OMX_ERRORTYPE encoder_verify_buffer_allocation(OMX_ENCODER* pEnc, PORT* p, OMX_U32 buffSize)
 * @param OMX_ENCODER pEnc - OMX Encoder instance
 * @param PORT* p - Instance of port holding buffer
 * @param OMX_U32 buffSize -
 */
static
OMX_ERRORTYPE encoder_verify_buffer_allocation(OMX_ENCODER* pEnc, PORT* p, OMX_U32 buffSize)
{
    assert(pEnc);
    assert(p);

    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);

    OMX_ERRORTYPE err = OMX_ErrorIncorrectStateOperation;

    // buffers can only be allocated when the component is in one of the following states:
    // 1. OMX_StateLoaded and has already sent a request for state transition to OMX_StateIdle
    // 2. OMX_StateWaitForResources, the resources are available and the component
    //    is ready to go to the OMX_StateIdle state
    // 3. OMX_StateExecuting, OMX_StatePause or OMX_StateIdle and the port is disabled.
    if (p->def.bPopulated)
    {
        TRACE_PRINT(pEnc->log, "API: port is already populated\n");
        return err;
    }
    if (buffSize < p->def.nBufferSize)
    {
        TRACE_PRINT(pEnc->log, "API: buffer is too small required:%u given:%u\n",
                    (unsigned)p->def.nBufferSize,
                    (unsigned)buffSize);
        return OMX_ErrorBadParameter;
    }
    // 3.2.2.15
    switch (pEnc->state)
    {
        case OMX_StateLoaded:
            if (pEnc->statetrans != OMX_StateIdle)
            {
                TRACE_PRINT(pEnc->log, "API: not in transition to idle\n");
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
            if (p->def.bEnabled)
            {
                TRACE_PRINT(pEnc->log, "API: port is not disabled\n");
                return err;
            }
    }
    return OMX_ErrorNone;
}

/**
 * OMX_ERRORTYPE encoder_use_buffer(OMX_IN    OMX_HANDLETYPE hComponent,
                                    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
                                    OMX_IN    OMX_U32 nPortIndex,
                                    OMX_IN    OMX_PTR pAppPrivate,
                                    OMX_IN    OMX_U32 nSizeBytes,
                                    OMX_IN    OMX_U8* pBuffer)
 * Client gives allocated buffer for OMX Component
 * Component must allocate its own temporary buffer to support correct memory mapping.
 * Call can be made when:
 * Component State is Loaded and it has requested to transition for Idle state  ||
 * Component state is WaitingForResources and it is ready to go Idle state ||
 * Port is disabled and Component State is Executing or Loaded or Idle
 *
 */
static
OMX_ERRORTYPE encoder_use_buffer(OMX_IN    OMX_HANDLETYPE hComponent,
                                 OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
                                 OMX_IN    OMX_U32 nPortIndex,
                                 OMX_IN    OMX_PTR pAppPrivate,
                                 OMX_IN    OMX_U32 nSizeBytes,
                                 OMX_IN    OMX_U8* pBuffer)
{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(ppBuffer);
    CHECK_PARAM_NON_NULL(pBuffer);

    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);

    CHECK_STATE_INVALID(pEnc->state);

    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: nPortIndex:%u pAppPrivate:%p nSizeBytes:%u pBuffer:%p\n",
                (unsigned)nPortIndex, pAppPrivate, (unsigned)nSizeBytes, pBuffer);

    PORT* port = encoder_map_index_to_port(pEnc, nPortIndex);
    if (port == NULL)
    {
        TRACE_PRINT(pEnc->log, "API: bad port index:%u\n", (unsigned)nPortIndex);
        return OMX_ErrorBadPortIndex;
    }
    OMX_ERRORTYPE err = encoder_verify_buffer_allocation(pEnc, port, nSizeBytes);
    if (err != OMX_ErrorNone)
        return err;

    TRACE_PRINT(pEnc->log, "API: port index:%u\n",  (unsigned)nPortIndex);
    TRACE_PRINT(pEnc->log, "API: buffer size:%u\n", (unsigned)nSizeBytes);

    BUFFER* buff = NULL;
    HantroOmx_port_allocate_next_buffer(port, &buff);
    if (buff == NULL)
    {
        TRACE_PRINT(pEnc->log, "API: no more buffers\n");
        return OMX_ErrorInsufficientResources;
    }

    // save the pointer here into the header. The data in this buffer
    // needs to be copied in to the DMA accessible frame buffer before encoding.

    INIT_OMX_VERSION_PARAM(*buff->header);
    buff->flags             &= ~BUFFER_FLAG_MY_BUFFER;
    buff->header->pBuffer     = pBuffer;
    buff->header->pAppPrivate = pAppPrivate;
    buff->header->nAllocLen   = nSizeBytes;
    buff->bus_address        = 0;
    buff->allocsize          = nSizeBytes;
    if (HantroOmx_port_buffer_count(port) == port->def.nBufferCountActual)
    {
        TRACE_PRINT(pEnc->log, "API: port is populated\n");
        port->def.bPopulated = OMX_TRUE;
    }
    if ( nPortIndex == PORT_INDEX_INPUT )
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
    TRACE_PRINT(pEnc->log, "API: pBufferHeader:%p\n", *ppBuffer);
    return OMX_ErrorNone;
}

/**
 * OMX_ERRORTYPE encoder_allocate_buffer(OMX_IN    OMX_HANDLETYPE hComponent,
                                      OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
                                      OMX_IN    OMX_U32 nPortIndex,
                                      OMX_IN    OMX_PTR pAppPrivate,
                                      OMX_IN    OMX_U32 nSizeBytes)
 * @param OMX_BUFFERHEADERTYPE** ppBuffer - Buffer header information for buffer that will be allocated
 * @param OMX_U32 nPortIndex - Indes of port, 1 input port, 2 output port
 * @param OMX_U32 nSizeBytes - Size of buffer that will be allocated
 * OMX Component allocates new buffer for encoding.
 */
static
OMX_ERRORTYPE encoder_allocate_buffer(OMX_IN    OMX_HANDLETYPE hComponent,
                                      OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
                                      OMX_IN    OMX_U32 nPortIndex,
                                      OMX_IN    OMX_PTR pAppPrivate,
                                      OMX_IN    OMX_U32 nSizeBytes)
{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(ppBuffer);

    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);

    CHECK_STATE_INVALID(pEnc->state);

    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: nPortIndex:%u pAppPrivate:%p nSizeBytes:%u\n",
        (unsigned)nPortIndex, pAppPrivate, (unsigned)nSizeBytes);
    PORT* port = encoder_map_index_to_port(pEnc, nPortIndex);
    if (port == NULL)
    {
        TRACE_PRINT(pEnc->log, "API: bad port index:%u\n", (unsigned)nPortIndex);
        return OMX_ErrorBadPortIndex;
    }
    OMX_ERRORTYPE err = encoder_verify_buffer_allocation(pEnc, port, nSizeBytes);
    if (err != OMX_ErrorNone)
        return err;

    TRACE_PRINT(pEnc->log, "API: port index:%u\n", (unsigned)nPortIndex);
    TRACE_PRINT(pEnc->log, "API: buffer size:%u\n", (unsigned)nSizeBytes);

    // about locking here.
    // Only in case of tunneling is the component thread accessing the port's
    // buffer's directly. However this function should not be called at that time.
    // note: perhaps we could lock/somehow make sure that a misbehaving client thread
    // wont get a change to mess everything up?

    OMX_U8*         bus_data    = NULL;
    OSAL_BUS_WIDTH  bus_address = 0;
    OMX_U32        allocsize   = nSizeBytes;

    // the conformance tester assumes that the allocated buffers equal nSizeBytes in size.
    // however when running on HW this might not be a valid assumption because the memory allocator
    // allocates memory always in fixed size chunks.
    err = OSAL_AllocatorAllocMem(&pEnc->alloc, &allocsize, &bus_data, &bus_address);
    if (err != OMX_ErrorNone)
        return err;

    assert(allocsize >= nSizeBytes);
    TRACE_PRINT(pEnc->log, "API: allocated buffer size:%u\n", (unsigned)allocsize);

    BUFFER* buff = NULL;
    HantroOmx_port_allocate_next_buffer(port, &buff);
    if (!buff)
    {
        TRACE_PRINT(pEnc->log, "API: no more buffers\n");
        OSAL_AllocatorFreeMem(&pEnc->alloc, nSizeBytes, bus_data, bus_address);
        return OMX_ErrorInsufficientResources;
    }

    INIT_OMX_VERSION_PARAM(*buff->header);
    buff->flags              |= BUFFER_FLAG_MY_BUFFER;
    buff->bus_data            = bus_data;
    buff->bus_address         = bus_address;
    buff->allocsize           = allocsize;
    buff->header->pBuffer     = bus_data;
    buff->header->pAppPrivate = pAppPrivate;
    buff->header->nAllocLen   = nSizeBytes; // the conformance tester assumes that the allocated buffers equal nSizeBytes in size.
    if (nPortIndex == PORT_INDEX_INPUT )
    {
        buff->header->nInputPortIndex  = nPortIndex;
        buff->header->nOutputPortIndex = 0;
    }
    else
    {
        buff->header->nInputPortIndex  = 0;
        buff->header->nOutputPortIndex = nPortIndex;
    }
    if (HantroOmx_port_buffer_count(port) == port->def.nBufferCountActual)
    {
        TRACE_PRINT(pEnc->log, "API: port is populated\n");
        port->def.bPopulated = OMX_TRUE;
    }
    *ppBuffer = buff->header;
    TRACE_PRINT(pEnc->log, "API: data (virtual address):%p pBufferHeader:%p\n", bus_data, *ppBuffer);

    return OMX_ErrorNone;

}

/**
 * OMX_ERRORTYPE encoder_free_buffer(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_U32 nPortIndex,
                                  OMX_IN OMX_BUFFERHEADERTYPE* pBufferHeader)
 * @param OMX_U32 nPortIndex - Indes of port, 1 input port, 2 output port
 * @param OMX_BUFFERHEADERTYPE pBufferHeader -  Buffer to be freed.
 */
static
OMX_ERRORTYPE encoder_free_buffer(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_U32 nPortIndex,
                                  OMX_IN OMX_BUFFERHEADERTYPE* pBufferHeader)
{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pBufferHeader);

    OMX_ENCODER* pEnc = GET_ENCODER( hComponent );
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: nPortIndex:%u pBufferHeader:%p\n", (unsigned)nPortIndex, pBufferHeader);

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

    PORT* port = encoder_map_index_to_port(pEnc, nPortIndex);
    if (port == NULL)
    {
        TRACE_PRINT(pEnc->log, "API: bad port index:%u\n", (unsigned)nPortIndex);
        return OMX_ErrorBadPortIndex;
    }
    OMX_BOOL violation = OMX_FALSE;

    if (port->def.bEnabled)
    {
        if ( pEnc->state == OMX_StateIdle && pEnc->statetrans != OMX_StateLoaded )
        {
            violation = OMX_TRUE;
        }
    }

    BUFFER* buff = HantroOmx_port_find_buffer(port, pBufferHeader);
    if (!buff)
    {
        TRACE_PRINT(pEnc->log, "API: no such buffer\n");
        //HantroOmx_port_unlock_buffers(port);
        return OMX_ErrorBadParameter;
    }

    if (!(buff->flags & BUFFER_FLAG_IN_USE))
    {
        TRACE_PRINT(pEnc->log, "API: buffer is not allocated\n");
        //HantroOmx_port_unlock_buffers(port);
        return OMX_ErrorBadParameter;
    }

    if (buff->flags & BUFFER_FLAG_MY_BUFFER)
    {
        assert(buff->bus_address && buff->bus_data);
        assert(buff->bus_data == buff->header->pBuffer);
        assert(buff->allocsize);
        OSAL_AllocatorFreeMem(&pEnc->alloc, buff->allocsize, buff->bus_data, buff->bus_address);
    }

    HantroOmx_port_release_buffer(port, buff);

    if ( HantroOmx_port_buffer_count(port) < port->def.nBufferCountActual )
        port->def.bPopulated = OMX_FALSE;

    // this is a hack because of the conformance tester.
    if (port->def.bPopulated == OMX_FALSE && violation == OMX_TRUE)
    {
        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventError, OMX_ErrorPortUnpopulated, 0, NULL);
    }
    return OMX_ErrorNone;
}

#define CHECK_PORT_STATE(encoder, port)                    \
  if ((encoder)->state != OMX_StateLoaded && (port)->bEnabled)      \
  {                                                             \
      TRACE_PRINT((encoder)->log, "API: port is not disabled\n"); \
      return OMX_ErrorIncorrectStateOperation;                  \
  }

#define CHECK_PORT_OUTPUT(param) \
  if ((param)->nPortIndex != PORT_INDEX_OUTPUT) \
      return OMX_ErrorBadPortIndex;

#define CHECK_PORT_INPUT(param) \
  if ((param)->nPortIndex != PORT_INDEX_INPUT) \
      return OMX_ErrorBadPortIndex;

#ifdef OMX_ENCODER_VIDEO_DOMAIN
static
OMX_ERRORTYPE encoder_set_parameter(OMX_IN OMX_HANDLETYPE hComponent,
                                    OMX_IN OMX_INDEXTYPE  nIndex,
                                    OMX_IN OMX_PTR        pParam)
{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);
    CHECK_STATE_INVALID(pEnc->state);
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
	TRACE_PRINT(pEnc->log, "Set param %s\n", HantroOmx_str_omx_index(nIndex));
    switch (nIndex)
    {
        case OMX_IndexParamAudioInit: //Fall through
        case OMX_IndexParamOtherInit:
        case OMX_IndexParamImageInit:
            return OMX_ErrorUnsupportedIndex;
        case OMX_IndexParamVideoInit:
            return OMX_ErrorNone;
            break;

        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE* port  = NULL;
                OMX_PARAM_PORTDEFINITIONTYPE* param = (OMX_PARAM_PORTDEFINITIONTYPE*)pParam;
                switch (param->nPortIndex)
                {
                    case PORT_INDEX_INPUT:
                        port = &pEnc->inputPort.def;
                        CHECK_PORT_STATE(pEnc, port);

                        if ( param->nBufferCountActual >= port->nBufferCountMin )
                            port->nBufferCountActual          = param->nBufferCountActual;
                        port->format.video.xFramerate         = param->format.video.xFramerate;
                        port->format.video.nFrameWidth        = param->format.video.nFrameWidth;
                        port->format.video.nFrameHeight       = param->format.video.nFrameHeight;
                        port->format.video.nStride            = param->format.video.nStride;
                        port->format.video.eColorFormat       = param->format.video.eColorFormat;
						TRACE_PRINT("  OMX_IndexParamPortDefinition: port->nBufferSize = %d, param->nBufferSize = %d", port->nBufferSize, param->nBufferSize);
                        break;

                    case PORT_INDEX_OUTPUT:
                        port = &pEnc->outputPort.def;
                        CHECK_PORT_STATE(pEnc, port);

                        if ( param->nBufferCountActual >= port->nBufferCountMin )
                            port->nBufferCountActual              = param->nBufferCountActual;
                        port->format.video.nBitrate           = param->format.video.nBitrate;
                        port->format.video.xFramerate         = param->format.video.xFramerate;
                        port->format.video.nFrameWidth        = param->format.video.nFrameWidth;
                        port->format.video.nFrameHeight       = param->format.video.nFrameHeight;
                        port->format.video.eCompressionFormat = param->format.video.eCompressionFormat;
                        switch ( port->format.video.eCompressionFormat )
                        {
                            case OMX_VIDEO_CodingAVC:
                                set_avc_defaults( pEnc );
                                break;
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
                            case OMX_VIDEO_CodingH263:
                                set_h263_defaults( pEnc );
                                break;
                            case OMX_VIDEO_CodingMPEG4:
                                set_mpeg4_defaults( pEnc );
                                break;
#endif
#ifdef ENCH1
                            case OMX_VIDEO_CodingVP8:
                                set_vp8_defaults( pEnc );
                                break;
#endif
                            default:
                                return OMX_ErrorUnsupportedIndex;
                        }

                        calculate_new_outputBufferSize( pEnc );

                        //if width or height is changed for input port it's affecting also
                        //to preprocessor crop
                        OMX_CONFIG_RECTTYPE* crop  = 0;
                        crop = &pEnc->encConfig.crop;
                        crop->nWidth = param->format.video.nStride;
                        crop->nHeight = param->format.video.nFrameHeight;
                        break;
                    default:
                        TRACE_PRINT(pEnc->log, "API: no such port:%u\n", (unsigned)param->nPortIndex);
                        return OMX_ErrorBadPortIndex;
                }
            }
            break;
        case OMX_IndexParamVideoPortFormat:
            {
                OMX_PARAM_PORTDEFINITIONTYPE* port    = NULL;
                OMX_VIDEO_PARAM_PORTFORMATTYPE* param = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)pParam;
                switch (param->nPortIndex)
                {
                    TRACE_PRINT( pEnc->log, "param->nIndex - %u ", (int)param->nIndex);
                    case PORT_INDEX_INPUT:
                        port = &pEnc->inputPort.def;
                        CHECK_PORT_STATE(pEnc, port);
                        port->format.video.eCompressionFormat = param->eCompressionFormat;
                        port->format.video.eColorFormat       = param->eColorFormat;
                        break;
                    case PORT_INDEX_OUTPUT:
                        port = &pEnc->outputPort.def;
                        CHECK_PORT_STATE(pEnc, port);
                        port->format.video.eCompressionFormat = param->eCompressionFormat;
                        port->format.video.eColorFormat       = param->eColorFormat;
                        switch ( port->format.video.eCompressionFormat )
                        {
                            case OMX_VIDEO_CodingAVC:
                                set_avc_defaults( pEnc );
                                break;
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
                            case OMX_VIDEO_CodingH263:
                                set_h263_defaults( pEnc );
                                break;
                            case OMX_VIDEO_CodingMPEG4:
                                set_mpeg4_defaults( pEnc );
                                break;
#endif
#ifdef ENCH1
                            case OMX_VIDEO_CodingVP8:
                                set_vp8_defaults( pEnc );
                                break;
#endif
                            default:
                                return OMX_ErrorUnsupportedIndex;
                        }
                        break;
                    default:
                        TRACE_PRINT(pEnc->log, "API: no such video port:%u\n", (unsigned)param->nPortIndex);
                        return OMX_ErrorBadPortIndex;
                }
            }
            break;
        case OMX_IndexParamVideoAvc:
            {
                OMX_VIDEO_PARAM_AVCTYPE* config  = 0;
                OMX_PARAM_PORTDEFINITIONTYPE* port  = 0;
                OMX_VIDEO_PARAM_AVCTYPE* param = (OMX_VIDEO_PARAM_AVCTYPE*)pParam;
                switch ( param->nPortIndex )
                {
                    case PORT_INDEX_INPUT:
                        TRACE_PRINT(pEnc->log, "No H264 config for input.\n");
                        break;
                    case PORT_INDEX_OUTPUT:
                        port = &pEnc->outputPort.def;
                        CHECK_PORT_STATE(pEnc, port );
                        config = &pEnc->encConfig.avc;
                        config->eLevel = param->eLevel;
                        config->eProfile = param->eProfile;
                        config->nPFrames = param->nPFrames;
                        break;
                    default:
                        TRACE_PRINT(pEnc->log, "API: No such port\n");
                        return OMX_ErrorBadPortIndex;
                        break;
                }
            }
            break;
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
        case OMX_IndexParamVideoH263: //OMX_VIDEO_PARAM_H263TYPE
            {
                OMX_VIDEO_PARAM_H263TYPE* config  = 0;
                OMX_PARAM_PORTDEFINITIONTYPE* port  = 0;
                OMX_VIDEO_PARAM_H263TYPE* param = (OMX_VIDEO_PARAM_H263TYPE*)pParam;
                switch (param->nPortIndex)
                {
                    case PORT_INDEX_INPUT:
                        TRACE_PRINT(pEnc->log, "No H263 config for input.\n");
                        break;
                    case PORT_INDEX_OUTPUT:
                        port = &pEnc->outputPort.def;
                        CHECK_PORT_STATE(pEnc, port );
                        config = &pEnc->encConfig.h263;
                        config->eProfile = param->eProfile;
                        config->eLevel = param->eLevel;
                        config->nPFrames = param->nPFrames;
                        break;
                    default:
                        TRACE_PRINT(pEnc->log, "API: No such port\n");
                        return OMX_ErrorBadPortIndex;
                        break;
                }
            }
            break;
        case OMX_IndexParamVideoMpeg4: //OMX_VIDEO_PARAM_MPEG4TYPE
            {
                OMX_VIDEO_PARAM_MPEG4TYPE* config  = 0;
                OMX_PARAM_PORTDEFINITIONTYPE* port  = 0;
                OMX_VIDEO_PARAM_MPEG4TYPE* param = (OMX_VIDEO_PARAM_MPEG4TYPE*)pParam;
                switch ( param->nPortIndex )
                {
                    case PORT_INDEX_INPUT:
                            TRACE_PRINT(pEnc->log, "No mpeg4 config for input.\n");
                            break;
                    case PORT_INDEX_OUTPUT:
                        port = &pEnc->outputPort.def;
                        CHECK_PORT_STATE(pEnc, port );
                        config = &pEnc->encConfig.mpeg4;
                        config->eProfile = param->eProfile;
                        config->eLevel = param->eLevel;
                        config->bSVH = param->bSVH;
                        config->bGov = param->bGov;
                        config->nPFrames = param->nPFrames;
                        config->nMaxPacketSize = param->nMaxPacketSize;
                        config->nTimeIncRes = param->nTimeIncRes;
                        config->nHeaderExtension = param->nHeaderExtension;
                        config->bReversibleVLC = param->bReversibleVLC;
                        break;
                    default:
                        TRACE_PRINT(pEnc->log, "API: No such port\n");
                        return OMX_ErrorBadPortIndex;
                        break;
                }
            }
            break;
#endif /*ndef ENC8270 & 8290*/
#ifdef ENCH1
        case OMX_IndexParamVideoVp8:
            {
                OMX_VIDEO_PARAM_VP8TYPE* config  = 0;
                OMX_PARAM_PORTDEFINITIONTYPE* port  = 0;
                OMX_VIDEO_PARAM_VP8TYPE* param = (OMX_VIDEO_PARAM_VP8TYPE*)pParam;
                switch ( param->nPortIndex )
                {
                    case PORT_INDEX_INPUT:
                        TRACE_PRINT(pEnc->log, "No VP8 config for input.\n");
                        break;
                    case PORT_INDEX_OUTPUT:
                        port = &pEnc->outputPort.def;
                        CHECK_PORT_STATE(pEnc, port );
                        config = &pEnc->encConfig.vp8;
                        config->eLevel = param->eLevel;
                        config->eProfile = param->eProfile;
                        break;
                    default:
                        TRACE_PRINT(pEnc->log, "API: No such port\n");
                        return OMX_ErrorBadPortIndex;
                        break;
                }
            }
            break;
#endif

       case OMX_IndexParamCompBufferSupplier:
            {
                OMX_PARAM_BUFFERSUPPLIERTYPE* param = (OMX_PARAM_BUFFERSUPPLIERTYPE*)pParam;
                PORT* port = encoder_map_index_to_port(pEnc, param->nPortIndex);
                if (!port) return OMX_ErrorBadPortIndex;
                CHECK_PORT_STATE(pEnc, &port->def);

                port->tunnel.eSupplier = param->eBufferSupplier;

                TRACE_PRINT(pEnc->log, "API: new buffer supplier value:%s port:%d\n", HantroOmx_str_omx_supplier(param->eBufferSupplier), (int)param->nPortIndex);

                if (port->tunnelcomp && port->def.eDir == OMX_DirInput)
                {
                    TRACE_PRINT(pEnc->log, "API: propagating value to tunneled component: %p port: %d\n", port->tunnelcomp, (int)port->tunnelport);
                    OMX_ERRORTYPE err;
                    OMX_PARAM_BUFFERSUPPLIERTYPE foo;
                    memset(&foo, 0, sizeof(foo));
                    INIT_OMX_VERSION_PARAM(foo);
                    foo.nPortIndex      = port->tunnelport;
                    foo.eBufferSupplier = port->tunnel.eSupplier;
                    err = ((OMX_COMPONENTTYPE*)port->tunnelcomp)->SetParameter(port->tunnelcomp, OMX_IndexParamCompBufferSupplier, &foo);
                    if (err != OMX_ErrorNone)
                    {
                        TRACE_PRINT(pEnc->log, "API: tunneled component refused buffer supplier config:%s\n", HantroOmx_str_omx_err(err));
                        return err;
                    }
                }
            }
            break;

        case OMX_IndexParamCommonDeblocking: //OMX_PARAM_DEBLOCKINGTYPE
            {
                OMX_PARAM_DEBLOCKINGTYPE* deblocking  = 0;
                OMX_PARAM_PORTDEFINITIONTYPE* port = 0;
                OMX_PARAM_DEBLOCKINGTYPE* param = (OMX_PARAM_DEBLOCKINGTYPE*)pParam;
                switch ( param->nPortIndex )
                {
                    case PORT_INDEX_INPUT:
                        TRACE_PRINT(pEnc->log, "No mpeg4 config for input.\n");
                        break;
                    case PORT_INDEX_OUTPUT:
                        port = &pEnc->outputPort.def;
                        CHECK_PORT_STATE(pEnc, port );
                        deblocking = &pEnc->encConfig.deblocking;
                        break;
                    default:
                        TRACE_PRINT(pEnc->log, "API: No such port\n");
                        return OMX_ErrorBadPortIndex;
                        break;
                }
            }
            break;
        case OMX_IndexParamVideoBitrate:
        {
            OMX_VIDEO_PARAM_BITRATETYPE* paramBitrate;
            paramBitrate = (OMX_VIDEO_PARAM_BITRATETYPE *)pParam;
            OMX_VIDEO_PARAM_BITRATETYPE* portBitrate;
            portBitrate = &pEnc->encConfig.bitrate;
            if ( paramBitrate->nPortIndex == PORT_INDEX_OUTPUT )
            {
                portBitrate->eControlRate = paramBitrate->eControlRate;
                if ( paramBitrate->nTargetBitrate != pEnc->outputPort.def.format.video.nBitrate )
                {
                    //Port configuration is used when codec is initialized
                    pEnc->outputPort.def.format.video.nBitrate = paramBitrate->nTargetBitrate;
                    //Store target bitrate to OMX_VIDEO_PARAM_BITRATETYPE also
                    //so we can return correct target bitrate easily when
                    // client makes get_param(OMX_IndexParamVideoBitrate) call
                    portBitrate->nTargetBitrate = paramBitrate->nTargetBitrate;

					TRACE_PRINT( pEnc->log, "before PortSettingsChanged event: %d",__LINE__);
#ifdef ANDROID_MOD
					if (pEnc->state == OMX_StateExecuting) 
#endif
					{
						pEnc->app_callbacks.EventHandler(pEnc->self,
														 pEnc->app_data,
														 OMX_EventPortSettingsChanged,
														 PORT_INDEX_OUTPUT,
														 0,
														 NULL);
					}
                }
            }
            else if ( paramBitrate->nPortIndex == PORT_INDEX_INPUT )
                return OMX_ErrorNone;
            else
                return OMX_ErrorBadPortIndex;
        }
        break;
        case OMX_IndexParamVideoErrorCorrection:
            {
                OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE* paramEc;
                paramEc = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE*) pParam;
                OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE* portEc;
                portEc = &pEnc->encConfig.ec;

                if ( paramEc->nPortIndex == PORT_INDEX_OUTPUT )
                {
                    portEc->bEnableHEC = paramEc->bEnableHEC;
                    portEc->bEnableResync = paramEc->bEnableResync;
                    portEc->nResynchMarkerSpacing = paramEc->nResynchMarkerSpacing;
                    portEc->bEnableDataPartitioning = paramEc->bEnableDataPartitioning;
                    portEc->bEnableRVLC = paramEc->bEnableRVLC;
                }
                else if ( paramEc->nPortIndex == PORT_INDEX_INPUT )
                    return OMX_ErrorNone;
                else
                {
                    TRACE_PRINT(pEnc->log, "API: No such port\n");
                    return OMX_ErrorBadPortIndex;
                }
                break;
            }
            break;
        case OMX_IndexParamVideoQuantization:
            {
                TRACE_PRINT(pEnc->log, "API: got VideoQuantization param\n");
                OMX_VIDEO_PARAM_QUANTIZATIONTYPE* paramQ;
                paramQ = (OMX_VIDEO_PARAM_QUANTIZATIONTYPE*) pParam;
                OMX_VIDEO_PARAM_QUANTIZATIONTYPE* portQ;
                portQ = &pEnc->encConfig.videoQuantization;

                if ( paramQ->nPortIndex == PORT_INDEX_OUTPUT )
                {
                    portQ->nQpI = paramQ->nQpI;
                    portQ->nQpP = paramQ->nQpP;
                    TRACE_PRINT(pEnc->log, "API:  QpI value: %d\n", (int)portQ->nQpI);
                    TRACE_PRINT(pEnc->log, "API:  QpP value: %d\n", (int)portQ->nQpP);
                }
                else if ( paramQ->nPortIndex == PORT_INDEX_INPUT )
                    return OMX_ErrorNone;
                else
                {
                    TRACE_PRINT(pEnc->log, "API: No such port\n");
                    return OMX_ErrorBadPortIndex;
                }
            }
            break;
        case OMX_IndexParamStandardComponentRole:
            {
                OMX_PARAM_COMPONENTROLETYPE* param = (OMX_PARAM_COMPONENTROLETYPE*)pParam;
                strcpy((char*)pEnc->role, (const char*)param->cRole);
#ifdef CONFORMANCE
                if (strcmp((char*)pEnc->role, "video_encoder.avc") == 0)
                    pEnc->outputPort.def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
                else if (strcmp((char*)pEnc->role, "video_encoder.mpeg4") == 0)
                    pEnc->outputPort.def.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
                else if (strcmp((char*)pEnc->role, "video_encoder.h263") == 0)
                    pEnc->outputPort.def.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
#endif
#ifdef ENCH1
                else if (strcmp((char*)pEnc->role, "video_encoder.vp8") == 0)
                    pEnc->outputPort.def.format.video.eCompressionFormat = OMX_VIDEO_CodingVP8;
#endif
#endif // CONFORMANCE
            }
            break;
        case OMX_IndexParamPriorityMgmt:
            {
                CHECK_STATE_LOADED(pEnc->state);
                OMX_PRIORITYMGMTTYPE* param = (OMX_PRIORITYMGMTTYPE*)pParam;
                pEnc->priority_group = param->nGroupPriority;
                pEnc->priority_id    = param->nGroupID;
            }
            break;
        case OMX_IndexParamVideoProfileLevelCurrent:
            {
                OMX_VIDEO_PARAM_PROFILELEVELTYPE* param = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pParam;
                //CHECK_PORT_INPUT(param); // should this be check port output?
                switch ( pEnc->outputPort.def.format.video.eCompressionFormat )
                {
                    case OMX_VIDEO_CodingAVC:
                        pEnc->encConfig.avc.eProfile = param->eProfile;
                        pEnc->encConfig.avc.eLevel = param->eLevel;
                        break;
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
                    case OMX_VIDEO_CodingH263:
                        pEnc->encConfig.h263.eProfile = param->eProfile;
                        pEnc->encConfig.h263.eLevel = param->eLevel;
                        break;
                    case OMX_VIDEO_CodingMPEG4:
                        pEnc->encConfig.mpeg4.eProfile = param->eProfile;
                        pEnc->encConfig.mpeg4.eLevel = param->eLevel;
                        break;
#endif
#ifdef ENCH1
                    case OMX_VIDEO_CodingVP8:
                        pEnc->encConfig.vp8.eProfile = param->eProfile;
                        pEnc->encConfig.vp8.eLevel = param->eLevel;
                        break;
#endif
                    default:
                        return OMX_ErrorNoMore;
                }
            }
            break;
        default:
            TRACE_PRINT(pEnc->log, "API: unsupported settings index\n");
            return OMX_ErrorUnsupportedIndex;
    }
    return OMX_ErrorNone;
}

static
OMX_ERRORTYPE encoder_get_parameter(OMX_IN    OMX_HANDLETYPE hComponent,
                                    OMX_IN    OMX_INDEXTYPE  nIndex,
                                    OMX_INOUT OMX_PTR        pParam)
{
    static int output_formats[][2] = {
        { OMX_VIDEO_CodingAVC,    OMX_COLOR_FormatUnused },
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
        { OMX_VIDEO_CodingH263,   OMX_COLOR_FormatUnused },
        { OMX_VIDEO_CodingMPEG4,  OMX_COLOR_FormatUnused },   
#endif
#ifdef ENCH1
        { OMX_VIDEO_CodingVP8,    OMX_COLOR_FormatUnused },
#endif
     };

    static int input_formats[][2] = {
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420Planar },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420PackedPlanar },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420SemiPlanar },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420PackedSemiPlanar },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYCbYCr },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatCbYCrY },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitRGB565 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitBGR565 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format12bitRGB444 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitARGB4444 },
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV422Planar },
#endif
#ifdef ENCH1
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format24bitRGB888 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format24bitBGR888 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format25bitARGB1888 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format32bitARGB8888 },
#endif
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitARGB1555 }
    };

/*    static int avc_profiles[][2] = {
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel1  },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel1b },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel11 },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel12 },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel13 },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel2 },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel21 },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel22 },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel3 },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel31 },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel32 }
#if defined (ENC8270) || defined (ENC8290) || defined (ENCH1)
        ,
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel4 },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel41 }
#if defined (ENC8290) || defined (ENCH1)
        ,
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel42 },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel5 },
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel51 }
#endif
#endif
    };*/

    static int avc_profiles[][2] = {
        { OMX_VIDEO_AVCProfileBaseline,    OMX_VIDEO_AVCLevel51 },
        { OMX_VIDEO_AVCProfileMain,    OMX_VIDEO_AVCLevel51 },
        { OMX_VIDEO_AVCProfileHigh,    OMX_VIDEO_AVCLevel51 }
    };
#ifdef ENCH1
    static int vp8_profiles[][2] = {
        { OMX_VIDEO_VP8ProfileMain,     OMX_VIDEO_VP8Level_Version0 }/*,
        { OMX_VIDEO_VP8ProfileMain,     OMX_VIDEO_VP8Level_Version1 },
        { OMX_VIDEO_VP8ProfileMain,     OMX_VIDEO_VP8Level_Version2 },
        { OMX_VIDEO_VP8ProfileMain,     OMX_VIDEO_VP8Level_Version3 }*/
    };
#endif
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
    static int H263_profiles[][2] = {
        /*{ OMX_VIDEO_H263ProfileBaseline,    OMX_VIDEO_H263Level10  },
        { OMX_VIDEO_H263ProfileBaseline,    OMX_VIDEO_H263Level20 },
        { OMX_VIDEO_H263ProfileBaseline,    OMX_VIDEO_H263Level30 },
        { OMX_VIDEO_H263ProfileBaseline,    OMX_VIDEO_H263Level40 },
        { OMX_VIDEO_H263ProfileBaseline,    OMX_VIDEO_H263Level50 },
        { OMX_VIDEO_H263ProfileBaseline,   OMX_VIDEO_H263Level60 },*/
        { OMX_VIDEO_H263ProfileBaseline,   OMX_VIDEO_H263Level70 }
    };

    static int mpeg4_profiles[][2] = {
        /*{ OMX_VIDEO_MPEG4ProfileSimple,    OMX_VIDEO_MPEG4Level0  },
        { OMX_VIDEO_MPEG4ProfileSimple,    OMX_VIDEO_MPEG4Level0b  },
        { OMX_VIDEO_MPEG4ProfileSimple,    OMX_VIDEO_MPEG4Level1  },
        { OMX_VIDEO_MPEG4ProfileSimple,    OMX_VIDEO_MPEG4Level2  },
        { OMX_VIDEO_MPEG4ProfileSimple,    OMX_VIDEO_MPEG4Level3  },
        { OMX_VIDEO_MPEG4ProfileSimple,    OMX_VIDEO_MPEG4Level4  },
        { OMX_VIDEO_MPEG4ProfileSimple,    OMX_VIDEO_MPEG4Level4a  },*/
        { OMX_VIDEO_MPEG4ProfileSimple,    OMX_VIDEO_MPEG4Level5  },
        { OMX_VIDEO_MPEG4ProfileMain,      OMX_VIDEO_MPEG4Level4  },
        /*{ OMX_VIDEO_MPEG4ProfileAdvancedCore,    OMX_VIDEO_MPEG4Level3  },
        { OMX_VIDEO_MPEG4ProfileAdvancedCore,    OMX_VIDEO_MPEG4Level4  },*/
        { OMX_VIDEO_MPEG4ProfileAdvancedCore,    OMX_VIDEO_MPEG4Level5  }
    };
#endif
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);

    CHECK_STATE_INVALID(pEnc->state);
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: getting index:%s\n", HantroOmx_str_omx_index(nIndex));

    switch (nIndex)
    {
        case OMX_IndexParamAudioInit:
        case OMX_IndexParamOtherInit:
        case OMX_IndexParamImageInit:
            {
                OMX_PORT_PARAM_TYPE* pPortDomain = 0;
                pPortDomain = (OMX_PORT_PARAM_TYPE*) pParam;
                pPortDomain->nPorts = 0;
                pPortDomain->nStartPortNumber = 0;
            }
            break;
        case OMX_IndexParamVideoInit:
            {
                OMX_PORT_PARAM_TYPE* pPortDomain = 0;
                pPortDomain = (OMX_PORT_PARAM_TYPE*) pParam;
                memcpy( pPortDomain, &pEnc->ports, pPortDomain->nSize);
            }
            break;

        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE* port  = 0;
                OMX_PARAM_PORTDEFINITIONTYPE* param = (OMX_PARAM_PORTDEFINITIONTYPE*)pParam;
                switch (param->nPortIndex)
                {
                    case PORT_INDEX_INPUT:
                        port = &pEnc->inputPort.def;
						TRACE_PRINT(pEnc->log, "requested input port definition. port->nBufferSize = %d", port->nBufferSize);
                        break;
                    case PORT_INDEX_OUTPUT:
                        port = &pEnc->outputPort.def;
                        break;
                    default: return OMX_ErrorBadPortIndex;
                }
                assert(param->nSize);
                memcpy(param, port, param->nSize);
            }
            break;
        case OMX_IndexParamVideoPortFormat:
            {
                OMX_VIDEO_PARAM_PORTFORMATTYPE* param = (OMX_VIDEO_PARAM_PORTFORMATTYPE*)pParam;
                switch (param->nPortIndex)
                {
                    case PORT_INDEX_INPUT:
                        TRACE_PRINT(pEnc->log, "OMX_IndexParamVideoPortFormat (input:%d)", param->nIndex);
                        if (param->nIndex >= sizeof(input_formats)/(sizeof(int)*2)) {
                            TRACE_PRINT(pEnc->log, "returning OMX_ErrorNoMore");
                            return OMX_ErrorNoMore;
                        }
                        param->xFramerate = 0;
                        param->eCompressionFormat = input_formats[param->nIndex][0];
                        param->eColorFormat       = input_formats[param->nIndex][1];
                        break;
                    case PORT_INDEX_OUTPUT:
                        if (param->nIndex >= sizeof(output_formats)/(sizeof(int)*2))
                            return OMX_ErrorNoMore;
                        param->xFramerate = 0;
                        param->eCompressionFormat = output_formats[param->nIndex][0];
                        param->eColorFormat       = output_formats[param->nIndex][1];
                        break;
                    default: return OMX_ErrorBadPortIndex;
                }
            }
            break;
        case OMX_IndexParamVideoAvc:
            {
                OMX_VIDEO_PARAM_AVCTYPE* avc;
                avc = (OMX_VIDEO_PARAM_AVCTYPE *)pParam;
                if ( avc->nPortIndex == PORT_INDEX_OUTPUT )
                    memcpy( avc, &pEnc->encConfig.avc, avc->nSize) ;
                else
                    return OMX_ErrorBadPortIndex;
            }
            break;
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
        case OMX_IndexParamVideoMpeg4:
            {
                OMX_VIDEO_PARAM_MPEG4TYPE* mpeg4;
                mpeg4 = (OMX_VIDEO_PARAM_MPEG4TYPE *)pParam;
                if ( mpeg4->nPortIndex == PORT_INDEX_OUTPUT )
                    memcpy( mpeg4, &pEnc->encConfig.mpeg4, mpeg4->nSize) ;
                else
                {
                    return OMX_ErrorBadPortIndex;
                }
            }
            break;
        case OMX_IndexParamVideoH263:
            {
                OMX_VIDEO_PARAM_H263TYPE* h263;
                h263 = (OMX_VIDEO_PARAM_H263TYPE *)pParam;
                if ( h263->nPortIndex == PORT_INDEX_OUTPUT )
                    memcpy( h263, &pEnc->encConfig.h263, h263->nSize) ;
                else
                    return OMX_ErrorBadPortIndex;
            }
            break;
#else
        case OMX_IndexParamVideoH263:
        case OMX_IndexParamVideoMpeg4:
            break;
#endif
#ifdef ENCH1
        case OMX_IndexParamVideoVp8:
            {
                OMX_VIDEO_PARAM_VP8TYPE* vp8;
                vp8 = (OMX_VIDEO_PARAM_VP8TYPE *)pParam;
                if ( vp8->nPortIndex == PORT_INDEX_OUTPUT )
                    memcpy( vp8, &pEnc->encConfig.vp8, vp8->nSize) ;
                else
                    return OMX_ErrorBadPortIndex;
            }
            break;
#endif
        case OMX_IndexParamVideoErrorCorrection:
            {
                OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE* ec;
                ec = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE*) pParam;
                if ( ec->nPortIndex == PORT_INDEX_OUTPUT )
                    memcpy( ec, &pEnc->encConfig.ec, ec->nSize);
                else
                    return OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoProfileLevelCurrent:
            {
                OMX_VIDEO_PARAM_PROFILELEVELTYPE* param = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pParam;
                //CHECK_PORT_INPUT(param); // should this be check port output?
                switch ( pEnc->outputPort.def.format.video.eCompressionFormat )
                {
                    case OMX_VIDEO_CodingAVC:
                        param->eProfile = pEnc->encConfig.avc.eProfile;
                        param->eLevel   = pEnc->encConfig.avc.eLevel;
                        break;
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
                    case OMX_VIDEO_CodingH263:
                        param->eProfile = pEnc->encConfig.h263.eProfile;
                        param->eLevel   = pEnc->encConfig.h263.eLevel;
                        break;
                    case OMX_VIDEO_CodingMPEG4:
                        param->eProfile = pEnc->encConfig.mpeg4.eProfile;
                        param->eLevel   = pEnc->encConfig.mpeg4.eLevel;
                        break;
#endif
#ifdef ENCH1
                    case OMX_VIDEO_CodingVP8:
                        param->eProfile = pEnc->encConfig.vp8.eProfile;
                        param->eLevel   = pEnc->encConfig.vp8.eLevel;
                        break;
#endif
                    default:
                        return OMX_ErrorNoMore;
                }
            }
            break;

        case OMX_IndexParamVideoProfileLevelQuerySupported:
            {
                OMX_VIDEO_PARAM_PROFILELEVELTYPE* param = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pParam;
                //CHECK_PORT_INPUT(param); // conformance tester uses output port for query
                switch ( pEnc->outputPort.def.format.video.eCompressionFormat )
                {
                    case OMX_VIDEO_CodingAVC:
                        if (param->nProfileIndex >= sizeof(avc_profiles)/(sizeof(int)*2)) {
							TRACE_PRINT(pEnc->log, "error: param->nProfileIndex >= sizeof(avc_profiles)/(sizeof(int)*2)");
                            return OMX_ErrorNoMore;
						}
                        param->eProfile = avc_profiles[param->nProfileIndex][0];
                        param->eLevel   = avc_profiles[param->nProfileIndex][1];
						TRACE_PRINT(pEnc->log, "param->eProfile = %d, param->eLevel = %d", param->eProfile, param->eLevel);
                        break;
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
                    case OMX_VIDEO_CodingH263:
                        if (param->nProfileIndex >= sizeof(H263_profiles)/(sizeof(int)*2))
                            return OMX_ErrorNoMore;
                        param->eProfile = H263_profiles[param->nProfileIndex][0];
                        param->eLevel   = H263_profiles[param->nProfileIndex][1];
                    break;
                    case OMX_VIDEO_CodingMPEG4:
                        if (param->nProfileIndex >= sizeof(mpeg4_profiles)/(sizeof(int)*2))
                            return OMX_ErrorNoMore;
                        param->eProfile = mpeg4_profiles[param->nProfileIndex][0];
                        param->eLevel   = mpeg4_profiles[param->nProfileIndex][1];
                    break;
#endif
#ifdef ENCH1
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
                OMX_PARAM_DEBLOCKINGTYPE* deb;
                deb = (OMX_PARAM_DEBLOCKINGTYPE *)pParam;
                if ( deb->nPortIndex == PORT_INDEX_OUTPUT )
                    memcpy( deb, &pEnc->encConfig.deblocking, deb->nSize) ;
                else
                    return OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoQuantization:
            {
                OMX_VIDEO_PARAM_QUANTIZATIONTYPE* qp;
                qp = (OMX_VIDEO_PARAM_QUANTIZATIONTYPE*)pParam;
                if ( qp->nPortIndex == PORT_INDEX_OUTPUT )
                    memcpy( qp, &pEnc->encConfig.videoQuantization, qp->nSize);
                else
                    return OMX_ErrorBadPortIndex;
            }
            break;

        case OMX_IndexParamVideoBitrate:
            {
                OMX_VIDEO_PARAM_BITRATETYPE* bitrate;
                bitrate = (OMX_VIDEO_PARAM_BITRATETYPE *)pParam;
                if ( bitrate->nPortIndex == PORT_INDEX_OUTPUT )
                    memcpy( bitrate, &pEnc->encConfig.bitrate, bitrate->nSize) ;
                else
                    return OMX_ErrorBadPortIndex;
            }
            break;
        case OMX_IndexParamCompBufferSupplier:
            {
                OMX_PARAM_BUFFERSUPPLIERTYPE* param = (OMX_PARAM_BUFFERSUPPLIERTYPE*)pParam;
                PORT* port = encoder_map_index_to_port(pEnc, param->nPortIndex);
                if (!port) return OMX_ErrorBadPortIndex;
                param->eBufferSupplier = port->tunnel.eSupplier;
            }
            break;

        case OMX_IndexParamStandardComponentRole:
            {
                OMX_PARAM_COMPONENTROLETYPE* param = (OMX_PARAM_COMPONENTROLETYPE*)pParam;
                strcpy((char*)param->cRole, (const char*)pEnc->role);
            }
            break;

        case OMX_IndexParamPriorityMgmt:
            {
                OMX_PRIORITYMGMTTYPE* param = (OMX_PRIORITYMGMTTYPE*)pParam;
                param->nGroupPriority = pEnc->priority_group;
                param->nGroupID       = pEnc->priority_id;
            }
            break;

        default:
            TRACE_PRINT(pEnc->log, "API: unsupported settings index\n");
            return OMX_ErrorUnsupportedIndex;
    }

	TRACE_PRINT(pEnc->log, "[%s] return ok",__FUNCTION__);
    return OMX_ErrorNone;
}

#endif //OMX_ENCODER_VIDEO_DOMAIN
#ifdef OMX_ENCODER_IMAGE_DOMAIN
static
OMX_ERRORTYPE encoder_set_parameter(OMX_IN OMX_HANDLETYPE hComponent,
                                    OMX_IN OMX_INDEXTYPE  nIndex,
                                    OMX_IN OMX_PTR        pParam)
{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);

    CHECK_STATE_INVALID(pEnc->state);
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: settings index:%s\n", HantroOmx_str_omx_index(nIndex));

    switch (nIndex)
    {
        case OMX_IndexParamAudioInit: //Fall through
        case OMX_IndexParamOtherInit:
        case OMX_IndexParamVideoInit:
            return OMX_ErrorUnsupportedIndex;
        case OMX_IndexParamImageInit:
            return OMX_ErrorNone;
            break;

        case OMX_IndexParamImagePortFormat:
        {
            OMX_PARAM_PORTDEFINITIONTYPE* port    = NULL;
            OMX_IMAGE_PARAM_PORTFORMATTYPE* param = (OMX_IMAGE_PARAM_PORTFORMATTYPE*)pParam;
            switch (param->nPortIndex)
            {
                case PORT_INDEX_INPUT:
                    port = &pEnc->inputPort.def;
                    CHECK_PORT_STATE(pEnc, port);
                    port->format.image.eColorFormat       = param->eColorFormat;
                    port->format.image.eCompressionFormat = param->eCompressionFormat;
                    break;
                case PORT_INDEX_OUTPUT:
                    port = &pEnc->outputPort.def;
                    CHECK_PORT_STATE(pEnc, port);
                    port->format.image.eColorFormat       = param->eColorFormat;
                    port->format.image.eCompressionFormat = param->eCompressionFormat;
                    switch ( port->format.image.eCompressionFormat )
                    {
                        case OMX_IMAGE_CodingJPEG:
                            set_jpeg_defaults( pEnc );
                            break;
#ifdef ENCH1
                        case OMX_IMAGE_CodingWEBP:
                            set_webp_defaults( pEnc );
                            break;
#endif
                        default:
                            return OMX_ErrorUnsupportedIndex;
                    }
                    break;
                default:
                    TRACE_PRINT(pEnc->log, "API: no such image port:%u\n", (unsigned)param->nPortIndex);
                    return OMX_ErrorBadPortIndex;
            }
        }
        break;

        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE* port  = NULL;
                OMX_PARAM_PORTDEFINITIONTYPE* param = (OMX_PARAM_PORTDEFINITIONTYPE*)pParam;
                if ( param->eDomain != OMX_PortDomainImage )
                {
                    TRACE_PRINT(pEnc->log, "API: unsupported port domain\n");
                    return OMX_ErrorUnsupportedSetting;
                }
                switch (param->nPortIndex)
                {
                    case PORT_INDEX_INPUT:
                        port = &pEnc->inputPort.def;
                        CHECK_PORT_STATE(pEnc, port);
                        if (param->format.image.eCompressionFormat != OMX_IMAGE_CodingUnused )
                            return OMX_ErrorUnsupportedSetting;
                        switch (param->format.image.eColorFormat)
                        {
                            case OMX_COLOR_FormatYUV420Planar:
                            case OMX_COLOR_FormatYUV420PackedPlanar:
                            case OMX_COLOR_FormatYUV420SemiPlanar:
                            case OMX_COLOR_FormatYUV420PackedSemiPlanar:
                            case OMX_COLOR_FormatYCbYCr: // YCbCr 4:2:2 interleaved (YUYV)
                            case OMX_COLOR_FormatCbYCrY:
                            case OMX_COLOR_Format16bitRGB565:
                            case OMX_COLOR_Format16bitBGR565:
                            case OMX_COLOR_Format12bitRGB444:
                            case OMX_COLOR_Format16bitARGB4444:
                            case OMX_COLOR_Format16bitARGB1555:
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
                            case OMX_COLOR_FormatYUV422Planar:
#endif
#ifdef ENCH1
                            case OMX_COLOR_Format24bitRGB888:
                            case OMX_COLOR_Format24bitBGR888:
                            case OMX_COLOR_Format25bitARGB1888:
                            case OMX_COLOR_Format32bitARGB8888:
#endif
                                break;
                            default:
                                TRACE_PRINT(pEnc->log, "API: unsupported image color format\n");
                                return OMX_ErrorUnsupportedSetting;
                        }
                        if ( param->nBufferCountActual >= port->nBufferCountMin )
                            port->nBufferCountActual              = param->nBufferCountActual;
                        port->format.image.nStride            = param->format.image.nStride;
                        port->format.image.nSliceHeight       = param->format.image.nSliceHeight;
                        port->format.image.eColorFormat       = param->format.image.eColorFormat;
                        port->format.image.nFrameWidth        = param->format.image.nFrameWidth;
                        port->format.image.nFrameHeight       = param->format.image.nFrameHeight;
                        break;

                    case PORT_INDEX_OUTPUT:
                        port = &pEnc->outputPort.def;
                        CHECK_PORT_STATE(pEnc, port);
                        assert(pEnc->outputPort.def.format.image.eCompressionFormat == OMX_IMAGE_CodingJPEG ||
                                pEnc->outputPort.def.format.image.eCompressionFormat == OMX_IMAGE_CodingWEBP);
                        if ( param->nBufferCountActual >= port->nBufferCountMin )
                            port->nBufferCountActual          = param->nBufferCountActual;
                        port->format.image.nFrameWidth        = param->format.image.nFrameWidth;
                        port->format.image.nFrameHeight       = param->format.image.nFrameHeight;
                        port->format.image.nSliceHeight       = param->format.image.nSliceHeight;
                        port->format.image.eCompressionFormat = param->format.image.eCompressionFormat;
                        switch ( port->format.image.eCompressionFormat )
                        {
                            case OMX_IMAGE_CodingJPEG:
                                set_jpeg_defaults( pEnc );
                                break;
#ifdef ENCH1
                            case OMX_IMAGE_CodingWEBP:
                                set_webp_defaults( pEnc );
                                break;
#endif
                            default:
                                return OMX_ErrorUnsupportedIndex;
                        }

                        calculate_new_outputBufferSize( pEnc );

                        //if width or height is changed for input port it's affecting also
                        //to preprocessor crop
                        OMX_CONFIG_RECTTYPE* crop  = 0;
                        crop = &pEnc->encConfig.crop;
                        crop->nWidth = param->format.video.nStride;
                        crop->nHeight = param->format.video.nFrameHeight;
                        break;

                    default:
                        TRACE_PRINT(pEnc->log, "API: no such port:%u\n", (unsigned)param->nPortIndex);
                        return OMX_ErrorBadPortIndex;
                }
            }
            break;
        case OMX_IndexParamQFactor:
        {
            TRACE_PRINT(pEnc->log, "API: OMX_IndexParamQFactor\n");
            OMX_IMAGE_PARAM_QFACTORTYPE* paramQf;
            OMX_IMAGE_PARAM_QFACTORTYPE* portQf;
            paramQf = (OMX_IMAGE_PARAM_QFACTORTYPE*)pParam;
            portQf = &pEnc->encConfig.imageQuantization;

            if ( paramQf->nPortIndex == PORT_INDEX_OUTPUT )
            {
                portQf->nQFactor = paramQf->nQFactor;
                TRACE_PRINT(pEnc->log, "API: QFactor: %d\n", (int)portQf->nQFactor);
            }
            else
            {
                TRACE_PRINT( pEnc->log, "API: No such port: %d\n", (int)paramQf->nPortIndex);
                return OMX_ErrorBadPortIndex;
            }
        }
        break;

        case OMX_IndexParamCompBufferSupplier:
            {
                OMX_PARAM_BUFFERSUPPLIERTYPE* param = (OMX_PARAM_BUFFERSUPPLIERTYPE*)pParam;
                PORT* port = encoder_map_index_to_port(pEnc, param->nPortIndex);
                if (!port) return OMX_ErrorBadPortIndex;
                CHECK_PORT_STATE(pEnc, &port->def);

                port->tunnel.eSupplier = param->eBufferSupplier;

                TRACE_PRINT(pEnc->log, "API: new buffer supplier value:%s port:%d\n", HantroOmx_str_omx_supplier(param->eBufferSupplier), (int)param->nPortIndex);

                if (port->tunnelcomp && port->def.eDir == OMX_DirInput)
                {
                    TRACE_PRINT(pEnc->log, "API: propagating value to tunneled component: %p port: %d\n", port->tunnelcomp, (int)port->tunnelport);
                    OMX_ERRORTYPE err;
                    OMX_PARAM_BUFFERSUPPLIERTYPE foo;
                    memset(&foo, 0, sizeof(foo));
                    INIT_OMX_VERSION_PARAM(foo);
                    foo.nPortIndex      = port->tunnelport;
                    foo.eBufferSupplier = port->tunnel.eSupplier;
                    err = ((OMX_COMPONENTTYPE*)port->tunnelcomp)->SetParameter(port->tunnelcomp, OMX_IndexParamCompBufferSupplier, &foo);
                    if (err != OMX_ErrorNone)
                    {
                        TRACE_PRINT(pEnc->log, "API: tunneled component refused buffer supplier config:%s\n", HantroOmx_str_omx_err(err));
                        return err;
                    }
                }
            }
            break;

        case OMX_IndexParamStandardComponentRole:
        {
            OMX_PARAM_COMPONENTROLETYPE* param = (OMX_PARAM_COMPONENTROLETYPE*)pParam;
            strcpy((char*)pEnc->role, (const char*)param->cRole);
#ifdef CONFORMANCE
                if (strcmp((char*)pEnc->role, "image_encoder.jpeg") == 0)
                    pEnc->outputPort.def.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
#ifdef ENCH1
                else if (strcmp((char*)pEnc->role, "image_encoder.webp") == 0)
                    pEnc->outputPort.def.format.image.eCompressionFormat = OMX_IMAGE_CodingWEBP;
#endif
#endif // CONFORMANCE
        }
        break;
        case OMX_IndexParamPriorityMgmt:
        {
            CHECK_STATE_LOADED(pEnc->state);
            OMX_PRIORITYMGMTTYPE* param = (OMX_PRIORITYMGMTTYPE*)pParam;
            pEnc->priority_group = param->nGroupPriority;
            pEnc->priority_id    = param->nGroupID;
        }
        break;
#ifdef CONFORMANCE
        case OMX_IndexParamQuantizationTable:
        {
            OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *param =
                (OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *) pParam;
            memcpy(&pEnc->quant_table, param, param->nSize);
        }
        break;
#endif
        default:
            TRACE_PRINT(pEnc->log, "API: unsupported settings index\n");
            return OMX_ErrorUnsupportedIndex;
    }
    return OMX_ErrorNone;
}

static
OMX_ERRORTYPE encoder_get_parameter(OMX_IN    OMX_HANDLETYPE hComponent,
                                    OMX_IN    OMX_INDEXTYPE  nIndex,
                                    OMX_INOUT OMX_PTR        pParam)
{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);

    CHECK_STATE_INVALID(pEnc->state);
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: settings index:%s\n", HantroOmx_str_omx_index(nIndex));

static int output_formats[][2] = {
        { OMX_IMAGE_CodingJPEG,    OMX_COLOR_FormatUnused },
#ifdef ENCH1
        { OMX_IMAGE_CodingWEBP,    OMX_COLOR_FormatUnused },
#endif
     };

static int input_formats[][2] = {
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420Planar },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420PackedPlanar },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420SemiPlanar },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV420PackedSemiPlanar },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYCbYCr },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatCbYCrY },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitRGB565 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitBGR565 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitARGB4444 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format12bitRGB444 },
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
        { OMX_VIDEO_CodingUnused, OMX_COLOR_FormatYUV422Planar },
#endif
#ifdef ENCH1
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format24bitRGB888 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format24bitBGR888 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format25bitARGB1888 },
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format32bitARGB8888 },
#endif
        { OMX_VIDEO_CodingUnused, OMX_COLOR_Format16bitARGB1555 }
    };

    switch (nIndex)
    {
        case OMX_IndexParamAudioInit:
        case OMX_IndexParamOtherInit:
        case OMX_IndexParamVideoInit:
            {
                OMX_PORT_PARAM_TYPE* pPortDomain;
                pPortDomain = (OMX_PORT_PARAM_TYPE*) pParam;
                pPortDomain->nPorts = 0;
                pPortDomain->nStartPortNumber = 0;
            }
            break;
        case OMX_IndexParamImageInit:
            {
                OMX_PORT_PARAM_TYPE* pPortDomain;
                pPortDomain = (OMX_PORT_PARAM_TYPE*) pParam;
                memcpy( pPortDomain, &pEnc->ports, pPortDomain->nSize);
            }
            break;

        case OMX_IndexParamPortDefinition:
            {
                OMX_PARAM_PORTDEFINITIONTYPE* port  = 0;
                OMX_PARAM_PORTDEFINITIONTYPE* param = (OMX_PARAM_PORTDEFINITIONTYPE*)pParam;
                switch (param->nPortIndex)
                {
                    case PORT_INDEX_INPUT:
                        port = &pEnc->inputPort.def;
                        break;
                    case PORT_INDEX_OUTPUT:
                        port = &pEnc->outputPort.def;
                        break;
                    default:
                        TRACE_PRINT(pEnc->log, "API: no such port:%u\n", (unsigned)param->nPortIndex);
                        return OMX_ErrorBadPortIndex;
                }
                assert(param->nSize);
                memcpy(param, port, param->nSize);
            }
            break;

        case OMX_IndexParamImagePortFormat:
            {
                OMX_IMAGE_PARAM_PORTFORMATTYPE* param = (OMX_IMAGE_PARAM_PORTFORMATTYPE*)pParam;
                switch (param->nPortIndex)
                {
                    case PORT_INDEX_INPUT:
                        if (param->nIndex >= sizeof(input_formats)/(sizeof(int)*2))
                            return OMX_ErrorNoMore;
                        //param->xFramerate = 0;
                        param->eCompressionFormat = input_formats[param->nIndex][0];
                        param->eColorFormat       = input_formats[param->nIndex][1];
                        break;
                    case PORT_INDEX_OUTPUT:
                        if (param->nIndex >= sizeof(output_formats)/(sizeof(int)*2))
                            return OMX_ErrorNoMore;
                        //param->xFramerate = 0;
                        param->eCompressionFormat = output_formats[param->nIndex][0];
                        param->eColorFormat       = output_formats[param->nIndex][1];
                        break;
                    default: return OMX_ErrorBadPortIndex;
                }
            }
            break;

        case OMX_IndexParamQFactor:
            {
                OMX_IMAGE_PARAM_QFACTORTYPE* qf;
                qf = (OMX_IMAGE_PARAM_QFACTORTYPE *)pParam;
                if ( qf->nPortIndex == PORT_INDEX_OUTPUT )
                    memcpy( qf, &pEnc->encConfig.imageQuantization, qf->nSize) ;
                else
                    return OMX_ErrorBadPortIndex;
            }
            break;
        // these are specified for standard JPEG decoder
#ifdef CONFORMANCE
        case OMX_IndexParamQuantizationTable:
        {
            OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *param =
                (OMX_IMAGE_PARAM_QUANTIZATIONTABLETYPE *) pParam;
            memcpy(param, &pEnc->quant_table, param->nSize);
        }
        break;
#endif
        case OMX_IndexParamCompBufferSupplier:
            {
                OMX_PARAM_BUFFERSUPPLIERTYPE* param = (OMX_PARAM_BUFFERSUPPLIERTYPE*)pParam;
                PORT* port = encoder_map_index_to_port(pEnc, param->nPortIndex);
                if (!port) return OMX_ErrorBadPortIndex;
                param->eBufferSupplier = port->tunnel.eSupplier;
            }
            break;

        case OMX_IndexParamStandardComponentRole:
            {
                OMX_PARAM_COMPONENTROLETYPE* param = (OMX_PARAM_COMPONENTROLETYPE*)pParam;
                strcpy((char*)param->cRole, (const char*)pEnc->role);
            }
            break;
        case OMX_IndexParamPriorityMgmt:
            {
                OMX_PRIORITYMGMTTYPE* param = (OMX_PRIORITYMGMTTYPE*)pParam;
                param->nGroupPriority = pEnc->priority_group;
                param->nGroupID       = pEnc->priority_id;
            }
            break;
        default:
            TRACE_PRINT(pEnc->log, "API: unsupported settings index\n");
            return OMX_ErrorUnsupportedIndex;
    }

    return OMX_ErrorNone;
}
#endif //#ifdef OMX_ENCODER_IMAGE_DOMAIN

static
OMX_ERRORTYPE encoder_set_config(OMX_IN OMX_HANDLETYPE hComponent,
                                 OMX_IN OMX_INDEXTYPE  nIndex,
                                 OMX_IN OMX_PTR        pParam)

{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_ENCODER* pEnc = GET_ENCODER( hComponent );
    CHECK_STATE_INVALID(pEnc->state);

    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: config index:%s\n", HantroOmx_str_omx_index(nIndex));

    if (pEnc->state != OMX_StateLoaded && pEnc->state != OMX_StateIdle && pEnc->state != OMX_StateExecuting)
    {
        // by an agreement with the client. Simplify parameter setting by allowing
        // parameters to be set only in the loaded/idle state.
        // OMX specification does not know about such constraint, but an implementation is allowed to do this.
        TRACE_PRINT(pEnc->log, "API: unsupported state: %s\n", HantroOmx_str_omx_state(pEnc->state));
        return OMX_ErrorUnsupportedSetting;
    }

    switch (nIndex)
    {
        case OMX_IndexConfigCommonRotate:
            {
                OMX_CONFIG_ROTATIONTYPE* param = (OMX_CONFIG_ROTATIONTYPE*)pParam;
                CHECK_PORT_INPUT(param);
                memcpy(&pEnc->encConfig.rotation, param, param->nSize);
            }
            break;
        case OMX_IndexConfigCommonInputCrop:
            {
                OMX_CONFIG_RECTTYPE* param = (OMX_CONFIG_RECTTYPE*)pParam;
                CHECK_PORT_INPUT(param);
                memcpy(&pEnc->encConfig.crop, param, param->nSize);
                //if crop is changed it's affecting to output port
                //height and width.
                TRACE_PRINT( pEnc->log, "SetConfig Crop, nLeft : %u\n", (unsigned)param->nLeft );
                TRACE_PRINT( pEnc->log, "SetConfig Crop, nTop : %u\n", (unsigned)param->nTop );

                OMX_PARAM_PORTDEFINITIONTYPE* port  = 0;
                port = &pEnc->outputPort.def;
#ifdef OMX_ENCODER_VIDEO_DOMAIN
                port->format.video.nFrameHeight = param->nHeight;
                port->format.video.nFrameWidth = param->nWidth;
#endif
#ifdef OMX_ENCODER_IMAGE_DOMAIN
                port->format.image.nFrameHeight = param->nHeight;
                port->format.image.nFrameWidth = param->nWidth;
#endif
                TRACE_PRINT( pEnc->log, "SetConfig Crop, port->format.image.nFrameHeight : %u\n", (unsigned)port->format.image.nFrameHeight );
                TRACE_PRINT( pEnc->log, "SetConfig Crop, port->format.image.nFrameWidth : %u\n", (unsigned)port->format.image.nFrameWidth );
                calculate_new_outputBufferSize( pEnc );

#ifdef ANDROID_MOD
				if (pEnc->state == OMX_StateExecuting) 
#endif
				{
					TRACE_PRINT( pEnc->log, "before PortSettingsChanged event: %d",__LINE__);
					pEnc->app_callbacks.EventHandler(
						pEnc->self,
						pEnc->app_data,
						OMX_EventPortSettingsChanged,
						PORT_INDEX_OUTPUT,
						0,
						NULL);
				}
            }
            break;
#ifdef OMX_ENCODER_VIDEO_DOMAIN
        case OMX_IndexConfigCommonFrameStabilisation:
            {
                OMX_CONFIG_FRAMESTABTYPE* param = (OMX_CONFIG_FRAMESTABTYPE*) pParam;
                if ( param->nPortIndex == PORT_INDEX_OUTPUT )
                {
                    TRACE_PRINT(pEnc->log, "API: wrong port index\n");
                    return OMX_ErrorBadPortIndex;
                }
                OMX_CONFIG_FRAMESTABTYPE* stab = 0;
                stab = &pEnc->encConfig.stab;
                stab->bStab = param->bStab;
            }
            break;
        case OMX_IndexConfigVideoBitrate:
            {
                OMX_VIDEO_CONFIG_BITRATETYPE* param = (OMX_VIDEO_CONFIG_BITRATETYPE*) pParam;

				TRACE_PRINT(pEnc->log, "encoder_set_config: OMX_IndexConfigVideoBitrate: %d", param->nEncodeBitrate);

                OMX_VIDEO_PARAM_BITRATETYPE* encBitrate = 0;
                encBitrate = &pEnc->encConfig.bitrate;
                encBitrate->nTargetBitrate = param->nEncodeBitrate;
                //Store target bitrate to output port and to encoder config bitrate
                OMX_PARAM_PORTDEFINITIONTYPE* port  = 0;
                port = &pEnc->outputPort.def;
                port->format.video.nBitrate = param->nEncodeBitrate;
                //Send port settings changed event to client

				TRACE_PRINT(pEnc->log, "encoder_set_config: OMX_IndexConfigVideoBitrate: done");

#ifdef ANDROID_MOD
				if (pEnc->state == OMX_StateExecuting) 
#endif
				{
					TRACE_PRINT( pEnc->log, "before PortSettingsChanged event: %d",__LINE__);
					/* Avoiding sending of PortSettingsChanged event to OMXCodec as it causes un-necessary port flushing,
					 disabling and enabling operations. Anyways these values will be reflected next time when encode_video 
					 API is called */
					/*pEnc->app_callbacks.EventHandler(
						pEnc->self,
						pEnc->app_data,
						OMX_EventPortSettingsChanged,
						PORT_INDEX_OUTPUT,
						0,
						NULL);*/
				}
            }
            break;

        case OMX_IndexConfigVideoFramerate:
            {
                OMX_CONFIG_FRAMERATETYPE* param = (OMX_CONFIG_FRAMERATETYPE*) pParam;
                //Store target bitrate to output port and to encoder config bitrate
                OMX_PARAM_PORTDEFINITIONTYPE* port  = 0;
                port = &pEnc->outputPort.def;
                port->format.video.xFramerate = param->xEncodeFramerate;
                set_frame_rate(pEnc);
                //Send port settings changed event to client

#ifdef ANDROID_MOD
				if (pEnc->state == OMX_StateExecuting) 
#endif
				{
					TRACE_PRINT( pEnc->log, "before PortSettingsChanged event: %d",__LINE__);
					/* Avoiding sending of PortSettingsChanged event to OMXCodec as it causes un-necessary port flushing,
					 disabling and enabling operations. Anyways these values will be reflected next time when encode_video 
					 API is called */
					/*pEnc->app_callbacks.EventHandler(
						pEnc->self,
						pEnc->app_data,
						OMX_EventPortSettingsChanged,
						PORT_INDEX_OUTPUT,
						0,
						NULL);*/
				}
            }
            break;
#ifdef ENCH1
        case OMX_IndexConfigVideoVp8ReferenceFrame:
            {
                OMX_VIDEO_VP8REFERENCEFRAMETYPE* param = (OMX_VIDEO_VP8REFERENCEFRAMETYPE*) pParam;
                CHECK_PORT_OUTPUT(param);
                memcpy(&pEnc->encConfig.vp8Ref, param, param->nSize);
            }
            break;
#endif /* ENCH1 */
        case OMX_IndexConfigVideoIntraVOPRefresh:
            {
                OMX_CONFIG_INTRAREFRESHVOPTYPE* param = (OMX_CONFIG_INTRAREFRESHVOPTYPE*) pParam;
                CHECK_PORT_OUTPUT(param);
                memcpy(&pEnc->encConfig.intraRefresh, param, param->nSize);
            }
            break;
        case OMX_IndexConfigVideoAVCIntraPeriod:
            {
                OMX_VIDEO_CONFIG_AVCINTRAPERIOD* param = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD*) pParam;
                CHECK_PORT_OUTPUT(param);
                memcpy(&pEnc->encConfig.avcIdr, param, param->nSize);
                pEnc->encConfig.avc.nPFrames = param->nPFrames;
                set_avc_intra_period(pEnc);
            }
            break;
#endif /* OMX_ENCODER_VIDEO_DOMAIN */
        default: return OMX_ErrorUnsupportedIndex;
    }

    return OMX_ErrorNone;
}

static
OMX_ERRORTYPE encoder_get_config(OMX_IN    OMX_HANDLETYPE hComponent,
                                 OMX_IN    OMX_INDEXTYPE  nIndex,
                                 OMX_INOUT OMX_PTR        pParam)
{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pParam);

    OMX_ENCODER* pEnc = GET_ENCODER( hComponent );
    CHECK_STATE_INVALID(pEnc->state);
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: config index: %s\n", HantroOmx_str_omx_index(nIndex));

    switch (nIndex)
    {
        case OMX_IndexConfigCommonRotate:
            {
                OMX_CONFIG_ROTATIONTYPE* param = (OMX_CONFIG_ROTATIONTYPE*) pParam;
                CHECK_PORT_INPUT(param);
                memcpy(param, &pEnc->encConfig.rotation, param->nSize);
            }
            break;
        case OMX_IndexConfigCommonInputCrop:
            {
                OMX_CONFIG_RECTTYPE* param = (OMX_CONFIG_RECTTYPE*) pParam;
                CHECK_PORT_INPUT(param);
                memcpy(param, &pEnc->encConfig.crop, param->nSize);
            }
            break;

#ifdef OMX_ENCODER_VIDEO_DOMAIN
        case OMX_IndexConfigVideoFramerate:
            {
                OMX_CONFIG_FRAMERATETYPE* param = (OMX_CONFIG_FRAMERATETYPE*) pParam;
                CHECK_PORT_OUTPUT(param);
                param->xEncodeFramerate = pEnc->outputPort.def.format.video.xFramerate;
            }
            break;
        case OMX_IndexConfigVideoBitrate:
            {
                OMX_VIDEO_CONFIG_BITRATETYPE* param = (OMX_VIDEO_CONFIG_BITRATETYPE*) pParam;
                CHECK_PORT_OUTPUT(param);
                param->nEncodeBitrate = pEnc->outputPort.def.format.video.nBitrate;
            }
            break;
        case OMX_IndexConfigCommonFrameStabilisation:
            {
                OMX_CONFIG_FRAMESTABTYPE* param = (OMX_CONFIG_FRAMESTABTYPE*) pParam;
                CHECK_PORT_INPUT(param);
                memcpy(param, &pEnc->encConfig.stab, param->nSize);
            }
            break;
#ifdef ENCH1
        case OMX_IndexConfigVideoVp8ReferenceFrame:
            {
                OMX_VIDEO_VP8REFERENCEFRAMETYPE* param = (OMX_VIDEO_VP8REFERENCEFRAMETYPE*) pParam;
                CHECK_PORT_OUTPUT(param);
                memcpy(param, &pEnc->encConfig.vp8Ref, param->nSize);
            }
            break;
#endif /* ENCH1 */
        case OMX_IndexConfigVideoIntraVOPRefresh:
            {
                OMX_CONFIG_INTRAREFRESHVOPTYPE* param = (OMX_CONFIG_INTRAREFRESHVOPTYPE*) pParam;
                CHECK_PORT_OUTPUT(param);
                memcpy(param, &pEnc->encConfig.intraRefresh, param->nSize);
            }
            break;
        case OMX_IndexConfigVideoAVCIntraPeriod:
            {
                OMX_VIDEO_CONFIG_AVCINTRAPERIOD* param = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD*) pParam;
                CHECK_PORT_OUTPUT(param);
                memcpy(param, &pEnc->encConfig.avcIdr, param->nSize);
            }
            break;
#endif /* OMX_ENCODER_VIDEO_DOMAIN */

        default: return OMX_ErrorUnsupportedIndex;
    }
    return OMX_ErrorNone;
}



/**
 * static OMX_ERRORTYPE encoder_push_buffer(OMX_HANDLETYPE hComponent,
                                  OMX_BUFFERHEADERTYPE* pBufferHeader,
                                  OMX_U32 portindex)
 * @param OMX_BUFFERHEADERTYPE* pBufferHeader - Buffer to be
 */
static
OMX_ERRORTYPE encoder_push_buffer(OMX_HANDLETYPE hComponent,
                                  OMX_BUFFERHEADERTYPE* pBufferHeader,
                                  OMX_U32 portindex)
{
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pBufferHeader);

    OMX_ENCODER* pEnc  = GET_ENCODER(hComponent);

    CHECK_STATE_INVALID(pEnc->state);
    TRACE_PRINT(pEnc->log, "API: header:%p port index:%u\n", pBufferHeader, (unsigned)portindex);

    if (pEnc->state != OMX_StateExecuting && pEnc->state != OMX_StatePause)
    {
        TRACE_PRINT(pEnc->log, "API: incorrect encoder state: %s\n", HantroOmx_str_omx_state(pEnc->state));
        return OMX_ErrorIncorrectStateOperation;
    }

    PORT* port = encoder_map_index_to_port(pEnc, portindex);
    if (!port)
    {
        TRACE_PRINT(pEnc->log, "API: no such port %u\n",(unsigned)portindex );
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
    // port is still disabled.
    //
    if (!HantroOmx_port_is_tunneled(port))
    {
        if (!port->def.bEnabled)
        {
            TRACE_PRINT(pEnc->log, "API: port is disabled\n");
            return OMX_ErrorIncorrectStateOperation;
        }
    }

    // Lock the port's buffer queue here.
    OMX_ERRORTYPE err = HantroOmx_port_lock_buffers(port);
    if (err != OMX_ErrorNone)
    {
        TRACE_PRINT(pEnc->log, "API: failed to lock port: %s\n", HantroOmx_str_omx_err(err));
        return err;
    }

    BUFFER* buff = HantroOmx_port_find_buffer(port, pBufferHeader);
    if (!buff)
    {
        HantroOmx_port_unlock_buffers(port);
        TRACE_PRINT(pEnc->log, "API: no such buffer\n");
        return OMX_ErrorBadParameter;
    }

    err = HantroOmx_port_push_buffer(port, buff);
    if (err != OMX_ErrorNone)
        TRACE_PRINT(pEnc->log, "API: failed to queue buffer: %s\n", HantroOmx_str_omx_err(err));

    // remember to unlock the queue too
    HantroOmx_port_unlock_buffers(port);
    return err;
}

static
OMX_ERRORTYPE encoder_fill_this_buffer(OMX_IN OMX_HANDLETYPE hComponent,
                                       OMX_IN OMX_BUFFERHEADERTYPE* pBufferHeader)
{
    // this function gives us a databuffer encoded data is stored
    CHECK_PARAM_NON_NULL(hComponent);
    CHECK_PARAM_NON_NULL(pBufferHeader);
    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: nOutputPortIndex %u\n", (unsigned) pBufferHeader->nOutputPortIndex );
    TRACE_PRINT(pEnc->log, "API: nInputPortIndex %u\n", (unsigned)pBufferHeader->nInputPortIndex );
    TRACE_PRINT(pEnc->log, "API: pBufferHeader %p\n", pBufferHeader );

    if (pBufferHeader->nSize != sizeof(OMX_BUFFERHEADERTYPE))
    {
        TRACE_PRINT(pEnc->log, "API: buffer header size mismatch\n");
        return OMX_ErrorBadParameter;
    }

    return encoder_push_buffer(hComponent, pBufferHeader, pBufferHeader->nOutputPortIndex);
}

static
OMX_ERRORTYPE encoder_empty_this_buffer( OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_BUFFERHEADERTYPE* pBufferHeader)
{
    CHECK_PARAM_NON_NULL( hComponent );
    CHECK_PARAM_NON_NULL( pBufferHeader );
    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "API: %p\n", pBufferHeader);
    TRACE_PRINT(pEnc->log, "API: nFilledLen %u\n", (unsigned)pBufferHeader->nFilledLen );
    TRACE_PRINT(pEnc->log, "API: nFilledLen %u\n", (unsigned)pBufferHeader->nFilledLen );
    TRACE_PRINT(pEnc->log, "API: nOffset %u\n", (unsigned)pBufferHeader->nOffset );
    TRACE_PRINT(pEnc->log, "API: nOutputPortIndex %u\n", (unsigned)pBufferHeader->nOutputPortIndex );
    TRACE_PRINT(pEnc->log, "API: nInputPortIndex %u\n", (unsigned)pBufferHeader->nInputPortIndex );


    if ( pBufferHeader->nFilledLen > pBufferHeader->nAllocLen )
    {
        TRACE_PRINT(pEnc->log, "API: incorrect nFilledLen value: %u nAllocLen: %u\n",
            (unsigned)pBufferHeader->nFilledLen,
            (unsigned)pBufferHeader->nAllocLen);
            return OMX_ErrorBadParameter;
    }

    if (pBufferHeader->nSize != sizeof(OMX_BUFFERHEADERTYPE))
    {
        TRACE_PRINT(pEnc->log, "API: buffer header size mismatch\n");
        return OMX_ErrorBadParameter;
    }

    TRACE_PRINT(pEnc->log, "API: nFilledLen:%u nFlags:%x\n", (unsigned)pBufferHeader->nFilledLen, (unsigned)pBufferHeader->nFlags);
    return encoder_push_buffer( hComponent, pBufferHeader, pBufferHeader->nInputPortIndex);
}

static
OMX_ERRORTYPE encoder_component_tunnel_request(OMX_IN OMX_HANDLETYPE hComponent,
                                               OMX_IN OMX_U32        nPort,
                                               OMX_IN OMX_HANDLETYPE hTunneledComp,
                                               OMX_IN OMX_U32        nTunneledPort,
                                               OMX_INOUT OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    CHECK_PARAM_NON_NULL(hComponent);
    OMX_ENCODER* pEnc = GET_ENCODER(hComponent);
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    OMX_ERRORTYPE err = OMX_ErrorNone;

    PORT* port = encoder_map_index_to_port(pEnc, nPort);
    if (port == NULL)
    {
        TRACE_PRINT(pEnc->log, "API: bad port index:%d\n", (int)nPort);
        return OMX_ErrorBadPortIndex;
    }
    if (pEnc->state != OMX_StateLoaded && port->def.bEnabled)
    {
        TRACE_PRINT(pEnc->log, "API: port is not disabled\n");
        return OMX_ErrorIncorrectStateOperation;
    }

    TRACE_PRINT(pEnc->log, "API: setting up tunnel on port: %d\n", (int)nPort);
    TRACE_PRINT(pEnc->log, "API: tunnel component:%p tunnel port:%d\n", hTunneledComp, (int)nTunneledPort);

    if (hTunneledComp == NULL)
    {
        HantroOmx_port_setup_tunnel(port, NULL, 0, OMX_BufferSupplyUnspecified);
        return OMX_ErrorNone;
    }

#ifndef OMX_ENCODER_TUNNELING_SUPPORT
    TRACE_PRINT(pEnc->log, "API: ERROR Tunneling unsupported\n");
    return OMX_ErrorTunnelingUnsupported;
#endif

    CHECK_PARAM_NON_NULL(pTunnelSetup);
    if (port->def.eDir == OMX_DirOutput)
    {
        // 3.3.11
        // the component that provides the output port of the tunneling has to do the following:
        // 1. Indicate its supplier preference in pTunnelSetup.
        // 2. Set the OMX_PORTTUNNELFLAG_READONLY flag to indicate that buffers
        //    from this output port are read-only and that the buffers cannot be shared
        //    through components or modified.

        // do not overwrite if something has been specified with SetParameter
        if (port->tunnel.eSupplier == OMX_BufferSupplyUnspecified)
            port->tunnel.eSupplier = OMX_BufferSupplyOutput;

        // if the component that provides the input port
        // wants to override the buffer supplier setting it will call our SetParameter
        // to override the setting put here.
        pTunnelSetup->eSupplier    = port->tunnel.eSupplier;
        pTunnelSetup->nTunnelFlags = OMX_PORTTUNNELFLAG_READONLY;
        HantroOmx_port_setup_tunnel(port, hTunneledComp, nTunneledPort, port->tunnel.eSupplier);
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
        err = ((OMX_COMPONENTTYPE*)hTunneledComp)->GetParameter(hTunneledComp, OMX_IndexParamPortDefinition, &other);
        if (err != OMX_ErrorNone)
            return err;

        // next do the port compatibility checking
        if (port->def.eDomain != other.eDomain)
        {
            TRACE_PRINT(pEnc->log, "API: ports are not compatible (incompatible domains)\n");
            return OMX_ErrorPortsNotCompatible;
        }
        if (port == &pEnc->inputPort)
        {
#ifdef OMX_ENCODER_VIDEO_DOMAIN
            switch (other.format.video.eCompressionFormat)
            {
                case OMX_VIDEO_CodingUnused:
                    switch (other.format.video.eColorFormat)
                    {
                        case OMX_COLOR_FormatYUV420PackedPlanar:     break;
                        case OMX_COLOR_FormatYUV420PackedSemiPlanar: break;
                        case OMX_COLOR_FormatYCbYCr:                 break;
                        default:
                            TRACE_PRINT(pEnc->log, "API: ports are not compatible (incompatible color format)\n");
                            return OMX_ErrorPortsNotCompatible;
                    }
                    break;
                default:
                    TRACE_PRINT(pEnc->log, "API: ports are not compatible (incompatible video coding)\n");
                    return OMX_ErrorPortsNotCompatible;
            }
#endif
#ifdef OMX_ENCODER_IMAGE_DOMAIN
            switch (other.format.image.eCompressionFormat)
            {
                case OMX_IMAGE_CodingUnused:
                    switch  (other.format.image.eColorFormat)
                    {
                        case OMX_COLOR_FormatYUV420PackedPlanar:     break;
                        case OMX_COLOR_FormatYUV420PackedSemiPlanar: break;
                        case OMX_COLOR_FormatYCbYCr:                 break;
                        default:
                            TRACE_PRINT(pEnc->log, "API: ports are not compatible (incompatible color format)\n");
                            return OMX_ErrorPortsNotCompatible;
                    }
                    break;
                default:
                    TRACE_PRINT(pEnc->log, "ASYNC: ports are not compatible (incompatible image coding)\n");
                    return OMX_ErrorPortsNotCompatible;
            }
#endif
        }
        if (pTunnelSetup->eSupplier == OMX_BufferSupplyUnspecified)
            pTunnelSetup->eSupplier = OMX_BufferSupplyInput;

        // need to send back the result of the buffer supply negotiation
        // to the component providing the output port.
        OMX_PARAM_BUFFERSUPPLIERTYPE param;
        memset(&param, 0, sizeof(param));
        INIT_OMX_VERSION_PARAM(param);

        param.eBufferSupplier = pTunnelSetup->eSupplier;
        param.nPortIndex      = nTunneledPort;
        err = ((OMX_COMPONENTTYPE*)hTunneledComp)->SetParameter(hTunneledComp, OMX_IndexParamCompBufferSupplier, &param);
        if (err != OMX_ErrorNone)
            return err;

        // save tunneling crap somewhere
        HantroOmx_port_setup_tunnel(port, hTunneledComp, nTunneledPort, pTunnelSetup->eSupplier);
        TRACE_PRINT(pEnc->log, "API: tunnel supplier: %s\n", HantroOmx_str_omx_supplier(pTunnelSetup->eSupplier));
    }
    return OMX_ErrorNone;
}


OMX_ERRORTYPE encoder_get_extension_index(OMX_HANDLETYPE hComponent, OMX_STRING cParameterName, OMX_INDEXTYPE* pIndexType)
{
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE encoder_useegl_image(OMX_HANDLETYPE hComponent, OMX_BUFFERHEADERTYPE** ppBufferHdr, OMX_U32 nPortIndex, OMX_PTR pAppPrivate, void* pBuffer)
{
    return OMX_ErrorNotImplemented;
}


OMX_ERRORTYPE encoder_enum_roles(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex)
{
    return OMX_ErrorNotImplemented;
}


/**
 * OMX_ERRORTYPE encoder_init( OMX_HANDLETYPE hComponent )
 * Initializes encoder component
 * constructor
 */
OMX_ERRORTYPE HantroHwEncOmx_encoder_init( OMX_HANDLETYPE hComponent )
{
    CHECK_PARAM_NON_NULL( hComponent );
    OMX_COMPONENTTYPE *pComp = (OMX_COMPONENTTYPE *) hComponent;

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
    pComp->ComponentDeInit = encoder_deinit;

    OMX_ENCODER* pEnc = (OMX_ENCODER*)OSAL_Malloc( sizeof( OMX_ENCODER ) );
    if ( pEnc == 0 )
        return OMX_ErrorInsufficientResources;

    memset(pEnc, 0, sizeof(OMX_ENCODER));

#ifdef ENCODER_TRACE
    char log_file_name[256];
    memset(log_file_name, 0, sizeof(log_file_name));
    sprintf(log_file_name, "%d", (unsigned int)pEnc);
    strcat(log_file_name, ".log");
    //pEnc->log = fopen(log_file_name, "w");
    pEnc->log = stdout;
    if (pEnc->log == 0)
    {
        OSAL_Free(pEnc);
        return OMX_ErrorUndefined;
    }
    if (pEnc->log)
        setvbuf(pEnc->log, 0, _IONBF, 0);
#endif
	int defaultWidth; 
	int defaultHeight;

#ifdef ANDROID_MOD
	defaultWidth = 640;
	defaultHeight = 480;
#else
	defaultWidth = 176;
	defaultHeight = 144;
#endif

	OMX_ERRORTYPE err = HantroOmx_port_init(&pEnc->inputPort, 2, 2, 5, defaultWidth*defaultHeight*3/2);
    if (err != OMX_ErrorNone)
        goto INIT_FAILURE;

    OMX_U32 outputBufferSize = 0;
#ifdef OMX_ENCODER_VIDEO_DOMAIN
    //if frame has over 1620 macro blocks encoding ratio is 4 otherwise its 2
    if ( ( ((defaultWidth+15)/16 )*((defaultHeight+15)/16) ) > 1620 )
        outputBufferSize = (defaultWidth*defaultHeight*3/2/8) * 2;
    else
        outputBufferSize = (defaultWidth*defaultHeight*3/2/4) * 2;
#else
    outputBufferSize = (defaultWidth*defaultHeight*3/2/4) * 2 * 20;
#endif

    err = HantroOmx_port_init(&pEnc->outputPort, 2, 2, 5, outputBufferSize );
    if (err != OMX_ErrorNone)
        goto INIT_FAILURE;

    err = OSAL_AllocatorInit(&pEnc->alloc);
    if (err != OMX_ErrorNone)
        goto INIT_FAILURE;

    INIT_OMX_VERSION_PARAM(pEnc->ports);
    INIT_OMX_VERSION_PARAM(pEnc->inputPort.def);
    INIT_OMX_VERSION_PARAM(pEnc->outputPort.def);

    pEnc->ports.nPorts                  = 2;
    pEnc->ports.nStartPortNumber        = 0;
    pEnc->state                         = OMX_StateLoaded;
    pEnc->statetrans                    = OMX_StateLoaded;
    pEnc->run                           = OMX_TRUE;
    pEnc->self                          = pComp;
    pEnc->streamStarted                 = OMX_FALSE;

#ifdef OMX_ENCODER_VIDEO_DOMAIN
    strcpy((char*)pEnc->role, "video_encoder.avc");

    //Input port ( Use H264 as an default )
    pEnc->inputPort.def.nPortIndex          = PORT_INDEX_INPUT;
    pEnc->inputPort.def.eDir                = OMX_DirInput;
    pEnc->inputPort.def.bEnabled            = OMX_TRUE;
    pEnc->inputPort.def.bPopulated          = OMX_FALSE;
    pEnc->inputPort.def.eDomain             = OMX_PortDomainVideo;
    pEnc->inputPort.def.format.video.xFramerate         = FLOAT_Q16(30);
    pEnc->inputPort.def.format.video.nFrameWidth        = defaultWidth;
    pEnc->inputPort.def.format.video.nFrameHeight       = defaultHeight;
    pEnc->inputPort.def.format.video.nStride            = defaultWidth;
    pEnc->inputPort.def.format.video.nSliceHeight       = defaultHeight;
    pEnc->inputPort.def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
#ifdef ANDROID_MOD
	pEnc->inputPort.def.format.video.eColorFormat       = OMX_COLOR_FormatYUV420PackedSemiPlanar;
#else
    pEnc->inputPort.def.format.video.eColorFormat       = OMX_COLOR_FormatYUV420PackedPlanar;
#endif

    //Output port ( Use H264 as an default )
    pEnc->outputPort.def.nPortIndex                         = PORT_INDEX_OUTPUT;
    pEnc->outputPort.def.eDir                               = OMX_DirOutput;
    pEnc->outputPort.def.bEnabled                           = OMX_TRUE;
    pEnc->outputPort.def.bPopulated                         = OMX_FALSE;
    pEnc->outputPort.def.eDomain                            = OMX_PortDomainVideo;
    pEnc->outputPort.def.format.video.nBitrate              = 128000;
    pEnc->outputPort.def.format.video.xFramerate            = FLOAT_Q16(30);
    pEnc->outputPort.def.format.video.eCompressionFormat    = OMX_VIDEO_CodingAVC;
    pEnc->outputPort.def.format.video.eColorFormat          = OMX_COLOR_FormatUnused;
    pEnc->outputPort.def.format.video.nFrameWidth           = defaultWidth;
    pEnc->outputPort.def.format.video.nFrameHeight          = defaultHeight;
#endif
#ifdef OMX_ENCODER_IMAGE_DOMAIN

    strcpy((char*)pEnc->role, "image_encoder.jpeg");

    pEnc->inputPort.def.nPortIndex          = PORT_INDEX_INPUT;
    pEnc->inputPort.def.eDir                = OMX_DirInput;
    pEnc->inputPort.def.bEnabled            = OMX_TRUE;
    pEnc->inputPort.def.bPopulated          = OMX_FALSE;
    pEnc->inputPort.def.eDomain             = OMX_PortDomainImage;
    pEnc->inputPort.def.format.image.nFrameWidth        = 96;
    pEnc->inputPort.def.format.image.nFrameHeight       = 96;
    pEnc->inputPort.def.format.image.nStride            = 96;
    pEnc->inputPort.def.format.image.nSliceHeight       = 16;
    pEnc->inputPort.def.format.image.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pEnc->inputPort.def.format.image.eColorFormat       = OMX_COLOR_FormatYUV420PackedPlanar;
    pEnc->inputPort.def.format.image.bFlagErrorConcealment  = OMX_FALSE;
    pEnc->inputPort.def.format.image.pNativeWindow          = 0x0;

    pEnc->outputPort.def.nPortIndex                         = PORT_INDEX_OUTPUT;
    pEnc->outputPort.def.eDir                               = OMX_DirOutput;
    pEnc->outputPort.def.bEnabled                           = OMX_TRUE;
    pEnc->outputPort.def.bPopulated                         = OMX_FALSE;
    pEnc->outputPort.def.eDomain                            = OMX_PortDomainImage;
    pEnc->outputPort.def.format.image.pNativeRender         = 0;
    pEnc->outputPort.def.format.image.nFrameWidth           = 96;
    pEnc->outputPort.def.format.image.nFrameHeight          = 96;
    pEnc->outputPort.def.format.image.nStride               = 96;
    pEnc->outputPort.def.format.image.nSliceHeight          = 16; //No slices
    pEnc->outputPort.def.format.image.bFlagErrorConcealment = OMX_FALSE;
    pEnc->outputPort.def.format.image.eCompressionFormat    = OMX_IMAGE_CodingJPEG;
    pEnc->outputPort.def.format.image.eColorFormat          = OMX_COLOR_FormatUnused;
    pEnc->outputPort.def.format.image.pNativeWindow         = 0x0;

    pEnc->numOfSlices                   = 0;
    pEnc->sliceNum                      = 1;
#ifdef CONFORMANCE
    pEnc->inputPort.def.format.image.nFrameWidth        = 640;
    pEnc->inputPort.def.format.image.nFrameHeight       = 480;
    pEnc->inputPort.def.format.image.nStride            = 640;
    pEnc->inputPort.def.format.image.eColorFormat       = OMX_COLOR_FormatYUV420Planar;
    pEnc->outputPort.def.format.image.nFrameWidth           = 640;
    pEnc->outputPort.def.format.image.nFrameHeight          = 480;
    pEnc->outputPort.def.format.image.nStride               = 640;
    pEnc->outputPort.def.format.image.nSliceHeight          = 0;
#endif

#endif

    //Set up function pointers for component interface functions
    pComp->GetComponentVersion   = encoder_get_version;
    pComp->SendCommand           = encoder_send_command;
    pComp->GetParameter          = encoder_get_parameter;
    pComp->SetParameter          = encoder_set_parameter;
    pComp->SetCallbacks          = encoder_set_callbacks;
    pComp->GetConfig             = encoder_get_config;
    pComp->SetConfig             = encoder_set_config;
    pComp->GetExtensionIndex     = encoder_get_extension_index;
    pComp->GetState              = encoder_get_state;
    pComp->ComponentTunnelRequest= encoder_component_tunnel_request;
    pComp->UseBuffer             = encoder_use_buffer;
    pComp->AllocateBuffer        = encoder_allocate_buffer;
    pComp->FreeBuffer            = encoder_free_buffer;
    pComp->EmptyThisBuffer       = encoder_empty_this_buffer;
    pComp->FillThisBuffer        = encoder_fill_this_buffer;
    pComp->UseEGLImage           = encoder_useegl_image;
    pComp->ComponentRoleEnum     = encoder_enum_roles;
    // save the "this" pointer into component struct for later use
    pComp->pComponentPrivate     = pEnc;

    pEnc->frameCounter = 0 ;

#ifdef OMX_ENCODER_VIDEO_DOMAIN
    err = set_avc_defaults( pEnc );
    if (err != OMX_ErrorNone)
        goto INIT_FAILURE;
    err = set_preprocessor_defaults( pEnc );
    if ( err != OMX_ErrorNone )
        goto INIT_FAILURE;
    err = set_bitrate_defaults( pEnc );
    if ( err != OMX_ErrorNone)
        goto INIT_FAILURE;
#endif //~OMX_ENCODER_VIDEO DOMAIN
#ifdef OMX_ENCODER_IMAGE_DOMAIN
    err = set_jpeg_defaults( pEnc );
    if (err != OMX_ErrorNone)
        goto INIT_FAILURE;
    err = set_preprocessor_defaults( pEnc );
    if ( err != OMX_ErrorNone )
        goto INIT_FAILURE;
#endif

    err = HantroOmx_basecomp_init(&pEnc->base, encoder_thread_main, pEnc);
    if (err != OMX_ErrorNone)
        goto INIT_FAILURE;

    return OMX_ErrorNone;

 INIT_FAILURE:
    assert(pEnc);
    TRACE_PRINT(pEnc->log, "%s %s\n", "init failure", HantroOmx_str_omx_err(err));
#ifndef CONFORMANCE // conformance tester calls deinit
    if (HantroOmx_port_is_allocated(&pEnc->inputPort))
        HantroOmx_port_destroy(&pEnc->inputPort);
    if (HantroOmx_port_is_allocated(&pEnc->outputPort))
        HantroOmx_port_destroy(&pEnc->outputPort);
    if (pEnc->log)
        fclose(pEnc->log);
    if (OSAL_AllocatorIsReady(&pEnc->alloc))
        OSAL_AllocatorDestroy(&pEnc->alloc);
    OSAL_Free(pEnc);
#endif
    return err;

}

static
OMX_ERRORTYPE supply_tunneled_port(OMX_ENCODER* pEnc, PORT* port)
{
    assert(port->tunnelcomp);
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "ASYNC: supplying buffers for: %p (%d)\n", port->tunnelcomp, (int)port->tunnelport);

    OMX_ERRORTYPE err = OMX_ErrorNone;
    OMX_PARAM_PORTDEFINITIONTYPE param;
    memset(&param, 0, sizeof(param));
    INIT_OMX_VERSION_PARAM(param);
    param.nPortIndex = port->tunnelport;
    // get the port definition, cause we need the number of buffers
    // that we need to allocate for this port
    err = ((OMX_COMPONENTTYPE*)port->tunnelcomp)->GetParameter(port->tunnelcomp, OMX_IndexParamPortDefinition, &param);
    if (err != OMX_ErrorNone)
        return err;

    // this operation should be fine without locking.
    // there's no access to the supply queue through the public API,
    // so the component thread is the only thread doing thea access here.

    // 2.1.7.2
    // 2. Allocate buffers according to the maximum of its own requirements and the
    //    requirements of the tunneled port.

    OMX_U32 count = param.nBufferCountActual > port->def.nBufferCountActual ? param.nBufferCountActual : port->def.nBufferCountActual;
    OMX_U32 size  = param.nBufferSize        > port->def.nBufferSize        ? param.nBufferSize        : port->def.nBufferSize;
    TRACE_PRINT(pEnc->log, "ASYNC: allocating %d buffers\n", (int)count);

    OMX_U32 i=0;
    for (i=0; i<count; ++i)
    {
        OMX_U8*        bus_data    = NULL;
        OSAL_BUS_WIDTH bus_address = 0;
        OMX_U32        allocsize   = size;
        // allocate the memory chunk for the buffer
        err = OSAL_AllocatorAllocMem(&pEnc->alloc, &allocsize, &bus_data, &bus_address);
        if (err != OMX_ErrorNone)
        {
            TRACE_PRINT(pEnc->log, "ASYNC: failed to supply buffer (%d bytes)\n", (int)param.nBufferSize);
            goto FAIL;
        }

        // allocate the BUFFER object
        BUFFER* buff = NULL;
        HantroOmx_port_allocate_next_buffer(port, &buff);
        if (buff == NULL)
        {
            TRACE_PRINT(pEnc->log, "ASYNC: failed to supply buffer object\n");
            OSAL_AllocatorFreeMem(&pEnc->alloc, allocsize, bus_data, bus_address);
            goto FAIL;
        }
        buff->flags             |= BUFFER_FLAG_MY_BUFFER;
        buff->bus_data           = bus_data;
        buff->bus_address        = bus_address;
        buff->allocsize          = allocsize;
        buff->header->pBuffer     = bus_data;
        buff->header->pAppPrivate = NULL;
        buff->header->nAllocLen   = size;
        // the header will remain empty because the
        // tunneled port allocates it.
        buff->header = NULL;
        err = ((OMX_COMPONENTTYPE*)port->tunnelcomp)->UseBuffer(
            port->tunnelcomp,
            &buff->header,
            port->tunnelport,
            NULL,
            allocsize,
            bus_data);

        if (err != OMX_ErrorNone)
        {
            TRACE_PRINT(pEnc->log, "ASYNC: use buffer call failed on tunneled component:%s\n", HantroOmx_str_omx_err(err));
            goto FAIL;
        }
        // the tunneling component is responsible for allocating the
        // buffer header and filling in the buffer information.
        assert(buff->header);
        assert(buff->header != &buff->headerdata);
        assert(buff->header->nSize);
        assert(buff->header->nVersion.nVersion);
        assert(buff->header->nAllocLen);

        TRACE_PRINT(pEnc->log, "ASYNC: supplied buffer data virtual address:%p size:%d header:%p\n", bus_data, (int)allocsize, buff->header);
    }
    assert(HantroOmx_port_buffer_count(port) >= port->def.nBufferCountActual);
    TRACE_PRINT(pEnc->log, "ASYNC: port is populated\n");
    port->def.bPopulated = OMX_TRUE;
    return OMX_ErrorNone;
 FAIL:
    TRACE_PRINT(pEnc->log, "ASYNC: freeing allready supplied buffers\n");
    // must free any buffers we have allocated
    count = HantroOmx_port_buffer_count(port);
    for (i=0; i<count; ++i)
    {
        BUFFER* buff = NULL;
        HantroOmx_port_get_allocated_buffer_at(port, &buff, i);
        assert(buff);
        assert(buff->bus_data);
        assert(buff->bus_address);

        if (buff->header)
            ((OMX_COMPONENTTYPE*)port->tunnelcomp)->FreeBuffer(port->tunnelcomp, port->tunnelport, buff->header);

        OSAL_AllocatorFreeMem(&pEnc->alloc, buff->allocsize, buff->bus_data, buff->bus_address);
    }
    HantroOmx_port_release_all_allocated(port);
    return err;
}

static
OMX_ERRORTYPE unsupply_tunneled_port(OMX_ENCODER* pEnc, PORT* port)
{
    assert(port->tunnelcomp);
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "ASYNC: removing buffers from: %p (%d)\n", port->tunnelcomp, (int)port->tunnelport);

    // tell the non-supplier to release them buffers
    OMX_U32 count = HantroOmx_port_buffer_count(port);
    OMX_U32 i;
    for (i=0; i<count; ++i)
    {
        BUFFER* buff = NULL;
        HantroOmx_port_get_allocated_buffer_at(port, &buff, i);
        assert(buff);
        assert(buff->bus_data);
        assert(buff->bus_address);
        assert(buff->header != &buff->headerdata);
        ((OMX_COMPONENTTYPE*)port->tunnelcomp)->FreeBuffer(port->tunnelcomp, port->tunnelport, buff->header);
        OSAL_AllocatorFreeMem(&pEnc->alloc, buff->allocsize, buff->bus_data, buff->bus_address);
    }
    HantroOmx_port_release_all_allocated(port);
    port->def.bPopulated = OMX_FALSE;


    // since we've allocated the buffers, and they have been
    // destroyed empty the port's buffer queue
    OMX_BOOL loop = OMX_TRUE;
    while (loop)
    {
        loop = HantroOmx_port_pop_buffer(port);
    }
    return OMX_ErrorNone;
}


#ifdef OMX_ENCODER_VIDEO_DOMAIN
/**
 * OMX_U32 calculate_frame_size( OMX_ENCODER* pEnc )
 */
OMX_ERRORTYPE calculate_frame_size( OMX_ENCODER* pEnc, OMX_U32* frameSize )
{
    switch ( pEnc->inputPort.def.format.video.eColorFormat )
    {
        case OMX_COLOR_FormatYUV420Planar:
        case OMX_COLOR_FormatYUV420SemiPlanar:
        case OMX_COLOR_FormatYUV420PackedSemiPlanar:
        case OMX_COLOR_FormatYUV420PackedPlanar:
            *frameSize    =
            pEnc->inputPort.def.format.video.nFrameHeight *
            pEnc->inputPort.def.format.video.nStride * 3/2;
            break;
        case OMX_COLOR_Format16bitARGB4444:
        case OMX_COLOR_Format16bitARGB1555:
        case OMX_COLOR_Format12bitRGB444:
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
        case OMX_COLOR_FormatYUV422Planar:
#endif
        case OMX_COLOR_Format16bitRGB565:
        case OMX_COLOR_Format16bitBGR565:
        case OMX_COLOR_FormatCbYCrY:
        case OMX_COLOR_FormatYCbYCr: // YCbCr 4:2:2 interleaved (YUYV)
            *frameSize    =
            pEnc->inputPort.def.format.video.nFrameHeight *
            pEnc->inputPort.def.format.video.nStride * 2;
            break;
#ifdef ENCH1
        case OMX_COLOR_Format24bitRGB888:
        case OMX_COLOR_Format24bitBGR888:
        case OMX_COLOR_Format25bitARGB1888:
        case OMX_COLOR_Format32bitARGB8888:
            *frameSize    =
            pEnc->inputPort.def.format.video.nFrameHeight *
            pEnc->inputPort.def.format.video.nStride * 4;
            break;
#endif
        default:
            TRACE_PRINT(pEnc->log, "ASYNC: unsupported format\n");
            return OMX_ErrorUnsupportedIndex;
            break;
    }
    return OMX_ErrorNone;
}



/**
 * static OMX_ERRORTYPE set_avc_defaults( OMX_ENCODER* pEnc);
 */
OMX_ERRORTYPE set_avc_defaults( OMX_ENCODER* pEnc)
{
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);

    OMX_VIDEO_PARAM_AVCTYPE* config  =  0;
    config = &pEnc->encConfig.avc;
    config->nPortIndex = PORT_INDEX_OUTPUT;
    config->eProfile = OMX_VIDEO_AVCProfileBaseline;
    config->eLevel = OMX_VIDEO_AVCLevel3;
    config->eLoopFilterMode = OMX_VIDEO_AVCLoopFilterEnable;
    config->nRefFrames = 1;
	TRACE_PRINT(pEnc->log, "1)--> config->nAllowedPictureTypes = %d", config->nAllowedPictureTypes);
#if 0//def ANDROID_MOD
	config->nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
#else
    config->nAllowedPictureTypes &= OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
#endif
	TRACE_PRINT(pEnc->log, "2)--> config->nAllowedPictureTypes = %d", config->nAllowedPictureTypes);
    config->nPFrames = 150;
    config->nSliceHeaderSpacing = 0 ;
    config->nBFrames = 0;
    config->bUseHadamard = OMX_FALSE;
    config->nRefIdx10ActiveMinus1 = 0;
    config->nRefIdx11ActiveMinus1 = 0;
    config->bEnableUEP = OMX_FALSE;
    config->bEnableFMO = OMX_FALSE;
    config->bEnableASO = OMX_FALSE;
    config->bEnableRS = OMX_FALSE;
    config->bFrameMBsOnly = OMX_FALSE;
    config->bMBAFF = OMX_FALSE;
    config->bEntropyCodingCABAC = OMX_FALSE;
    config->bWeightedPPrediction = OMX_FALSE;
    config->nWeightedBipredicitonMode = OMX_FALSE;
    config->bconstIpred = OMX_FALSE;
    config->bDirect8x8Inference = OMX_FALSE;
    config->bDirectSpatialTemporal = OMX_FALSE;
    config->nCabacInitIdc = OMX_FALSE;
    config->eLoopFilterMode = OMX_VIDEO_AVCLoopFilterEnable;
#ifdef CONFORMANCE
    config->eLevel = OMX_VIDEO_AVCLevel1;
    config->bUseHadamard = OMX_TRUE;
#endif

    OMX_VIDEO_CONFIG_AVCINTRAPERIOD* avcIdr;
    avcIdr = &pEnc->encConfig.avcIdr;
    avcIdr->nPFrames = 150;
    avcIdr->nIDRPeriod = 150;

    OMX_PARAM_DEBLOCKINGTYPE* deb = 0;
    deb = &pEnc->encConfig.deblocking;

    deb->bDeblocking = OMX_FALSE;

    OMX_VIDEO_PARAM_QUANTIZATIONTYPE* quantization  = 0;
    quantization = &pEnc->encConfig.videoQuantization;
    quantization->nPortIndex = PORT_INDEX_OUTPUT;
    quantization->nQpI = 36;
    quantization->nQpP = 36;
    quantization->nQpB = 0; //Not used

    return OMX_ErrorNone;
}
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
/**
 * OMX_ERRORTYPE set_mpeg4_defaults( OMX_ENCODER* pEnc)
 */
OMX_ERRORTYPE set_mpeg4_defaults( OMX_ENCODER* pEnc)
{
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);

    OMX_VIDEO_PARAM_MPEG4TYPE* config = 0;
    config = &pEnc->encConfig.mpeg4;
    config->nPortIndex = PORT_INDEX_OUTPUT;
    config->eProfile = OMX_VIDEO_MPEG4ProfileSimple;
    config->eLevel = OMX_VIDEO_MPEG4Level5;
    config->nAllowedPictureTypes &= OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
    config->bSVH = OMX_FALSE;
    config->bGov = OMX_FALSE;
    config->nTimeIncRes = TIME_RESOLUTION_MPEG4;
    config->nPFrames = 0;
    config->nMaxPacketSize = 0;
    config->nHeaderExtension = 0;
    config->bReversibleVLC = OMX_FALSE;
    config->bACPred = 0;
    config->nSliceHeaderSpacing = 0;
    config->nBFrames = 0;
    config->nIDCVLCThreshold = 0;

    OMX_VIDEO_PARAM_QUANTIZATIONTYPE* quantization  = 0;
    quantization = &pEnc->encConfig.videoQuantization;
    quantization->nPortIndex = PORT_INDEX_OUTPUT;
    quantization->nQpI = 20;
    quantization->nQpP = 20;
    quantization->nQpB = 0; //Not used

    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE* ec = 0;
    ec = &pEnc->encConfig.ec;
    ec->nPortIndex = PORT_INDEX_OUTPUT;
    ec->bEnableHEC = OMX_FALSE;
    ec->bEnableResync = OMX_FALSE;
    ec->nResynchMarkerSpacing = 0;
    ec->bEnableDataPartitioning = OMX_FALSE;
    ec->bEnableRVLC = OMX_FALSE;

    return OMX_ErrorNone;
}

/**
 * static OMX_ERRORTYPE set_h263_defaults( OMX_ENCODER* pEnc);
 */
OMX_ERRORTYPE set_h263_defaults( OMX_ENCODER* pEnc)
{
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);

    OMX_VIDEO_PARAM_H263TYPE* config = 0;
    config = &pEnc->encConfig.h263;
    config->nPortIndex = PORT_INDEX_OUTPUT;
    config->eProfile = OMX_VIDEO_H263ProfileBaseline;
    config->eLevel = OMX_VIDEO_H263Level70;
    config->bPLUSPTYPEAllowed = OMX_FALSE;
    config->nAllowedPictureTypes &= OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
    config->nPFrames = 0;

    OMX_VIDEO_PARAM_QUANTIZATIONTYPE* quantization  = 0;
    quantization = &pEnc->encConfig.videoQuantization;
    quantization->nPortIndex = PORT_INDEX_OUTPUT;
    quantization->nQpI = 20;
    quantization->nQpP = 20;
    quantization->nQpB = 0; //Not used

    return OMX_ErrorNone;
}
#endif /*ndef ENC8270 & ENC8290*/

#ifdef ENCH1
OMX_ERRORTYPE set_vp8_defaults( OMX_ENCODER* pEnc )
{
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);

    OMX_VIDEO_PARAM_VP8TYPE* config  =  0;
    config = &pEnc->encConfig.vp8;
    config->nPortIndex = PORT_INDEX_OUTPUT;
    config->eProfile = OMX_VIDEO_VP8ProfileMain;
    config->eLevel = OMX_VIDEO_VP8Level_Version0;
    config->nDCTPartitions = 0;
    config->bErrorResilientMode = OMX_FALSE;

    OMX_VIDEO_VP8REFERENCEFRAMETYPE* vp8Ref = 0;
    vp8Ref = &pEnc->encConfig.vp8Ref;
    vp8Ref->bPreviousFrameRefresh = OMX_TRUE;
    vp8Ref->bGoldenFrameRefresh = OMX_FALSE;
    vp8Ref->bAlternateFrameRefresh = OMX_FALSE;
    vp8Ref->bUsePreviousFrame = OMX_TRUE;
    vp8Ref->bUseGoldenFrame = OMX_FALSE;
    vp8Ref->bUseAlternateFrame = OMX_FALSE;

    return OMX_ErrorNone;
}
#endif // ENCH1
/**
 * static OMX_ERRORTYPE set_bitrate_defaults( OMX_ENCODER* pEnc );
 */
static OMX_ERRORTYPE set_bitrate_defaults( OMX_ENCODER* pEnc )
{
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    OMX_VIDEO_PARAM_BITRATETYPE* config = 0;
    config = &pEnc->encConfig.bitrate;
    config->nPortIndex = PORT_INDEX_OUTPUT;
    config->eControlRate = OMX_Video_ControlRateVariable;
    config->nTargetBitrate = pEnc->outputPort.def.format.video.nBitrate;
#ifdef CONFORMANCE
    config->eControlRate = OMX_Video_ControlRateConstant;
#endif
    return OMX_ErrorNone;
}

#endif // ~OMX_ENCODER_VIDEO_DOMAIN
#ifdef OMX_ENCODER_IMAGE_DOMAIN

/**
 * OMX_U32 calculate_frame_size( OMX_ENCODER* pEnc )
 */
OMX_ERRORTYPE calculate_frame_size( OMX_ENCODER* pEnc, OMX_U32* frameSize )
{
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT( pEnc->log, "ASYNC: pEnc->sliceNum - %u \n", (unsigned)pEnc->sliceNum);
    TRACE_PRINT( pEnc->log, "ASYNC: pEnc->numOfSlices - %u \n", (unsigned)pEnc->numOfSlices);

    switch ( pEnc->inputPort.def.format.image.eColorFormat )
    {
        case OMX_COLOR_Format16bitARGB4444:
        case OMX_COLOR_Format16bitARGB1555:
        case OMX_COLOR_Format12bitRGB444:
        case OMX_COLOR_FormatYUV420Planar:
        case OMX_COLOR_FormatYUV420SemiPlanar:
        case OMX_COLOR_FormatYUV420PackedSemiPlanar:
        case OMX_COLOR_FormatYUV420PackedPlanar:
            if ( !pEnc->sliceMode )
            {
                *frameSize    =
                pEnc->inputPort.def.format.image.nFrameHeight *
                pEnc->inputPort.def.format.image.nStride * 3/2;
            }
            else
            {
                if ( !(pEnc->sliceNum == pEnc->numOfSlices) )  //Not an last slice use sliceheight
                {
                    *frameSize    =
                    pEnc->inputPort.def.format.image.nSliceHeight *
                    pEnc->inputPort.def.format.image.nStride * 3/2;
                }
                else
                {

                    TRACE_PRINT(pEnc->log, "ASYNC: Encoding last slice...\n");
                    *frameSize = ( pEnc->outputPort.def.format.image.nFrameHeight -
                                  (pEnc->numOfSlices -1) * pEnc->inputPort.def.format.image.nSliceHeight ) *
                                  pEnc->inputPort.def.format.image.nStride * 3/2;
                }
            }
            break;
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
        case OMX_COLOR_FormatYUV422Planar:
#endif
        case OMX_COLOR_Format16bitRGB565:
        case OMX_COLOR_Format16bitBGR565:
        case OMX_COLOR_FormatCbYCrY:
        case OMX_COLOR_FormatYCbYCr: // YCbCr 4:2:2 interleaved (YUYV)
            if ( !pEnc->sliceMode )
            {
                *frameSize    =
                pEnc->inputPort.def.format.image.nFrameHeight *
                pEnc->inputPort.def.format.image.nStride * 2;
            }
            else
            {
                if ( !(pEnc->sliceNum == pEnc->numOfSlices) )  //Not an last slice use sliceheight
                {
                    *frameSize    =
                    pEnc->inputPort.def.format.image.nSliceHeight *
                    pEnc->inputPort.def.format.image.nStride * 2;
                }
                else
                {
                    TRACE_PRINT(pEnc->log, "ASYNC: Encoding last slice...\n");
                    *frameSize = ( pEnc->outputPort.def.format.image.nFrameHeight -
                                  (pEnc->numOfSlices -1) * pEnc->inputPort.def.format.image.nSliceHeight ) *
                                  pEnc->inputPort.def.format.image.nStride * 2;
                }
            }
            break;
#ifdef ENCH1
        case OMX_COLOR_Format24bitRGB888:
        case OMX_COLOR_Format24bitBGR888:
        case OMX_COLOR_Format25bitARGB1888:
        case OMX_COLOR_Format32bitARGB8888:
            if ( !pEnc->sliceMode )
            {
                *frameSize    =
                pEnc->inputPort.def.format.image.nFrameHeight *
                pEnc->inputPort.def.format.image.nStride * 4;
            }
            else
            {
                if ( !(pEnc->sliceNum == pEnc->numOfSlices) )  //Not an last slice use sliceheight
                {
                    *frameSize    =
                    pEnc->inputPort.def.format.image.nSliceHeight *
                    pEnc->inputPort.def.format.image.nStride * 4;
                }
                else
                {
                    TRACE_PRINT(pEnc->log, "ASYNC: Encoding last slice...\n");
                    *frameSize = ( pEnc->outputPort.def.format.image.nFrameHeight -
                                  (pEnc->numOfSlices -1) * pEnc->inputPort.def.format.image.nSliceHeight ) *
                                  pEnc->inputPort.def.format.image.nStride * 4;
                }
            }
            break;
#endif
        default:
            TRACE_PRINT(pEnc->log, "ASYNC: unsupported format\n");
            return OMX_ErrorUnsupportedIndex;
            break;
    }
    return OMX_ErrorNone;
}

/**
 * static OMX_ERRORTYPE set_jpeg_defaults( OMX_ENCODER* pEnc);
 */
OMX_ERRORTYPE set_jpeg_defaults( OMX_ENCODER* pEnc)
{
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    OMX_IMAGE_PARAM_QFACTORTYPE* config = 0;
    config = &pEnc->encConfig.imageQuantization;
    config->nQFactor = 8;

    pEnc->sliceMode = OMX_FALSE;

    return OMX_ErrorNone;
}
#ifdef ENCH1
/**
 * static OMX_ERRORTYPE set_webp_defaults( OMX_ENCODER* pEnc);
 */
OMX_ERRORTYPE set_webp_defaults( OMX_ENCODER* pEnc)
{
    OMX_PARAM_PORTDEFINITIONTYPE* portDef = 0;
    portDef = &pEnc->inputPort.def;
    
    printf("Set WEBP defaults\n");
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    OMX_IMAGE_PARAM_QFACTORTYPE* config = 0;
    config = &pEnc->encConfig.imageQuantization;
    config->nQFactor = 8;

    pEnc->sliceMode = OMX_FALSE;
    //portDef->format.image.nSliceHeight = 0;

    return OMX_ErrorNone;
}
#endif //ENCH1
#endif //#ifdef OMX_ENCODER_IMAGE_DOMAIN

/**
 * static OMX_ERRORTYPE set_preprocessor_defaults( OMX_ENCODER* pEnc);
 */
 OMX_ERRORTYPE set_preprocessor_defaults( OMX_ENCODER* pEnc)
 {
    TRACE_PRINT( pEnc->log, "API: %s\n", __FUNCTION__);

    OMX_CONFIG_ROTATIONTYPE* rotation = 0;
    rotation = &pEnc->encConfig.rotation;
    rotation->nPortIndex = PORT_INDEX_INPUT;
    rotation->nRotation = 0;

    OMX_CONFIG_RECTTYPE* crop = 0;
    crop = &pEnc->encConfig.crop;
    crop->nPortIndex = PORT_INDEX_INPUT;
    crop->nLeft = 0;
    crop->nTop = 0;
    crop->nWidth = pEnc->outputPort.def.format.video.nFrameWidth;
    crop->nHeight = pEnc->outputPort.def.format.video.nFrameHeight;

    return OMX_ErrorNone;
 }

 /**
  * static OMX_ERRORTYPE calculate_new_outputBufferSize( OMX_ENCODER* pEnc )
  */
 static OMX_ERRORTYPE calculate_new_outputBufferSize( OMX_ENCODER* pEnc )
 {
    TRACE_PRINT( pEnc->log, "API: %s\n", __FUNCTION__);

    OMX_U32 newBufferSize = 0;
    OMX_PARAM_PORTDEFINITIONTYPE* portDef = 0;
    portDef = &pEnc->outputPort.def;

#ifdef OMX_ENCODER_IMAGE_DOMAIN
    OMX_U32 frameWidth = portDef->format.image.nFrameWidth;
    OMX_U32 frameHeight;
    if (portDef->format.image.nSliceHeight > 0)
    {
        frameHeight = portDef->format.image.nSliceHeight;
    }
    else
    {
        frameHeight = portDef->format.image.nFrameHeight;
    }
#endif //#ifdef OMX_ENCODER_IMAGE_DOMAIN
#ifdef OMX_ENCODER_VIDEO_DOMAIN
    OMX_U32 frameWidth = portDef->format.video.nFrameWidth;
    OMX_U32 frameHeight = portDef->format.video.nFrameHeight;
#endif // ~OMX_ENCODER_VIDEO_DOMAIN

    TRACE_PRINT( pEnc->log, "API: framewidth - %u\n", (unsigned)frameWidth);
    TRACE_PRINT( pEnc->log, "API: frameHeight - %u\n", (unsigned)frameHeight);

    if ( ( ((frameWidth + 15)/16) * ((frameHeight + 15)/16) ) > 1620 )
        newBufferSize = (frameWidth*frameHeight*3/2/8) * 2;
    else
        newBufferSize = (frameWidth*frameHeight*3/2/4) * 2;

#ifdef OMX_ENCODER_IMAGE_DOMAIN
    if (newBufferSize > 16*1024*1024)
    {
        newBufferSize = 16*1024*1024;
    }
#endif // OMX_ENCODER_IMAGE_DOMAIN

    portDef->nBufferSize = newBufferSize;

    //Inform client about new output port buffer size
#ifndef ANDROID_MOD
	TRACE_PRINT( pEnc->log, "before PortSettingsChanged event: %d",__LINE__);
    pEnc->app_callbacks.EventHandler(
                    pEnc->self,
                    pEnc->app_data,
                    OMX_EventPortSettingsChanged,
                    PORT_INDEX_OUTPUT,
                    0,
	    			NULL);
#endif

    TRACE_PRINT( pEnc->log, "API: calculated new buffer size - %u\n", (unsigned)newBufferSize);
    TRACE_PRINT( pEnc->log, "API: portDef->nBufferSize - %u\n", (unsigned)portDef->nBufferSize);
    return OMX_ErrorNone;
 }


/**
 * static OMX_ERRORTYPE transition_to_idle_from_loaded(OMX_ENCODER* pEnc)
 */
static
OMX_ERRORTYPE transition_to_idle_from_loaded(OMX_ENCODER* pEnc)
{

    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    // the state transition cannot complete untill all
    // enabled ports are populated and the component has acquired
    // all of its static resources.
    assert(pEnc->state      == OMX_StateLoaded);
    assert(pEnc->statetrans == OMX_StateIdle);
    assert(pEnc->codec == NULL);

    OMX_ERRORTYPE err = OMX_ErrorHardware;
    ENCODER_PROTOTYPE*  codec = NULL;
    FRAME_BUFFER        in;
    FRAME_BUFFER        out;
    memset(&in,  0, sizeof(FRAME_BUFFER));
    memset(&out, 0, sizeof(FRAME_BUFFER));

    TRACE_PRINT(pEnc->log, "ASYNC: input port 0 is tunneled with: %p port: %d supplier:%s\n",
                pEnc->inputPort.tunnelcomp, (int)pEnc->inputPort.tunnelport, HantroOmx_str_omx_supplier(pEnc->inputPort.tunnel.eSupplier));

    TRACE_PRINT(pEnc->log, "ASYNC: output port 1 is tunneled with: %p port: %d supplier:%s\n",
                pEnc->outputPort.tunnelcomp, (int)pEnc->outputPort.tunnelport, HantroOmx_str_omx_supplier(pEnc->outputPort.tunnel.eSupplier));

    if (pEnc->inputPort.def.format.video.nStride & 0x0f)
    {
        err = OMX_ErrorUnsupportedSetting;
        goto FAIL;
    }

    if (HantroOmx_port_is_supplier(&pEnc->inputPort))
        if (supply_tunneled_port(pEnc, &pEnc->inputPort) != OMX_ErrorNone)
            goto FAIL;

    if (HantroOmx_port_is_supplier(&pEnc->outputPort))
        if (supply_tunneled_port(pEnc, &pEnc->outputPort) != OMX_ErrorNone)
            goto FAIL;

    TRACE_PRINT(pEnc->log, "ASYNC: waiting for buffers now!\n");

    while (!HantroOmx_port_is_ready(&pEnc->inputPort) ||
           !HantroOmx_port_is_ready(&pEnc->outputPort ) )
    {
        OSAL_ThreadSleep(RETRY_INTERVAL);
    }
    TRACE_PRINT(pEnc->log, "ASYNC: got all buffers\n");



#ifdef OMX_ENCODER_TEST_CODEC
TRACE_PRINT(pEnc->log, "TEST Codec\n");
    ENCODER_PROTOTYPE* test_codec_create();
    codec = test_codec_create();
#else
#ifdef OMX_ENCODER_VIDEO_DOMAIN
    switch ( pEnc->outputPort.def.format.video.eCompressionFormat )
    {
        case OMX_VIDEO_CodingAVC:
            {
            TRACE_PRINT(pEnc->log, "ASYNC: Creating H264 codec...\n");
            H264_CONFIG config;

            config.pp_config.origWidth = pEnc->inputPort.def.format.video.nStride;
            config.pp_config.origHeight = pEnc->inputPort.def.format.video.nFrameHeight;
            config.nSliceHeight = 0;
            config.pp_config.formatType = pEnc->inputPort.def.format.video.eColorFormat;
            config.common_config.nInputFramerate = pEnc->inputPort.def.format.video.xFramerate;
            config.pp_config.angle = pEnc->encConfig.rotation.nRotation;;
            if ( pEnc->encConfig.rotation.nRotation == 90 || pEnc->encConfig.rotation.nRotation == 270 )
            {
                config.common_config.nOutputWidth = pEnc->outputPort.def.format.video.nFrameHeight;
                config.common_config.nOutputHeight = pEnc->outputPort.def.format.video.nFrameWidth;
            }
            else
            {
                config.common_config.nOutputWidth = pEnc->outputPort.def.format.video.nFrameWidth;
                config.common_config.nOutputHeight = pEnc->outputPort.def.format.video.nFrameHeight;
            }
            config.pp_config.frameStabilization = pEnc->encConfig.stab.bStab;
            if ( config.pp_config.frameStabilization &&
                 pEnc->outputPort.def.format.video.nFrameWidth == pEnc->inputPort.def.format.video.nStride &&
                 pEnc->outputPort.def.format.video.nFrameHeight == pEnc->inputPort.def.format.video.nFrameHeight )
            {
                //Stabilization with default value ( 16 pixels)
                TRACE_PRINT( pEnc->log, "ASYNC: Using default stabilization values \n");
                config.pp_config.angle = pEnc->encConfig.rotation.nRotation;
                config.common_config.nOutputWidth -= 16;
                config.common_config.nOutputHeight -= 16;
                config.pp_config.xOffset = 0;
                config.pp_config.yOffset = 0;
            }
            else if ( config.pp_config.frameStabilization )
            {
                //Stabilization with user defined values
                //No other crop
                TRACE_PRINT( pEnc->log, "ASYNC: Using user defined stabilization values \n");
                config.pp_config.xOffset = 0;
                config.pp_config.yOffset = 0;
            }
            else
            {
                TRACE_PRINT( pEnc->log, "ASYNC: No stabilization \n");
                config.pp_config.xOffset = pEnc->encConfig.crop.nLeft;
                config.pp_config.yOffset = pEnc->encConfig.crop.nTop;
            }
            config.h264_config.eProfile = pEnc->encConfig.avc.eProfile;
            config.h264_config.eLevel = pEnc->encConfig.avc.eLevel;
            config.bDisableDeblocking = pEnc->encConfig.deblocking.bDeblocking;
            config.nPFrames = pEnc->encConfig.avc.nPFrames;
            config.rate_config.nTargetBitrate = pEnc->outputPort.def.format.video.nBitrate;
            config.rate_config.nQpDefault = pEnc->encConfig.videoQuantization.nQpI;
            config.rate_config.nQpMin = 10;
            config.rate_config.nQpMax = 51;

            config.rate_config.eRateControl = pEnc->encConfig.bitrate.eControlRate;
            switch ( config.rate_config.eRateControl )
            {
                case OMX_Video_ControlRateVariable:
                {
                    config.rate_config.nMbRcEnabled = 0;
                    config.rate_config.nHrdEnabled = 0;
                    config.rate_config.nPictureRcEnabled = 1;
                }
                break;
                case OMX_Video_ControlRateDisable:
                {
                    config.rate_config.nMbRcEnabled = 0;
                    config.rate_config.nHrdEnabled = 0;
                    config.rate_config.nPictureRcEnabled = 0;
                }
                break;
                case OMX_Video_ControlRateConstant:
                {
                    config.rate_config.nMbRcEnabled = 1;
                    config.rate_config.nHrdEnabled = 1;
                    config.rate_config.nPictureRcEnabled = 1;

                }
                break;
                case OMX_Video_ControlRateConstantSkipFrames:
                {
                    config.rate_config.nMbRcEnabled = 1;
                    config.rate_config.nHrdEnabled = 1;
                    config.rate_config.nPictureRcEnabled = 1;
                }
                break;
                case  OMX_Video_ControlRateVariableSkipFrames:
                {
                    config.rate_config.nMbRcEnabled = 0;
                    config.rate_config.nHrdEnabled = 0;
                    config.rate_config.nPictureRcEnabled = 1;
                }
                break;
                default:
                {
                    TRACE_PRINT(pEnc->log, "No rate control defined...using disabled value\n");
                    config.rate_config.eRateControl = OMX_Video_ControlRateDisable;
                    config.rate_config.nMbRcEnabled = 0;
                    config.rate_config.nHrdEnabled = 0;
                    config.rate_config.nPictureRcEnabled = 0;
                }
                break;
            }
            config.bSeiMessages = OMX_FALSE;

            codec = HantroHwEncOmx_encoder_create_h264( &config );
        }
        break;
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
    case OMX_VIDEO_CodingMPEG4:
    {
        TRACE_PRINT(pEnc->log, "ASYNC: Creating MPEG4 codec\n");

        MPEG4_CONFIG config;

        config.pp_config.origWidth = pEnc->inputPort.def.format.video.nStride;
        TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.origWidth - %u\n", (unsigned) config.pp_config.origWidth);
        config.pp_config.origHeight =  pEnc->inputPort.def.format.video.nFrameHeight;
        TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.origHeight - %u\n", (unsigned)config.pp_config.origHeight);
        config.common_config.nInputFramerate = pEnc->inputPort.def.format.video.xFramerate;
        TRACE_PRINT( pEnc->log, "ASYNC: config.common_config.nInputFramerate - %u\n", (unsigned) config.common_config.nInputFramerate);
        config.pp_config.formatType = pEnc->inputPort.def.format.video.eColorFormat;
        TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.formatType - %u\n", (unsigned)config.pp_config.formatType);
        config.pp_config.frameStabilization = pEnc->encConfig.stab.bStab;
        TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.frameStabilization - %u\n", (unsigned) config.pp_config.frameStabilization);
        config.pp_config.angle = pEnc->encConfig.rotation.nRotation;
#if 1
             if ( pEnc->encConfig.rotation.nRotation == 90 || pEnc->encConfig.rotation.nRotation == 270 )
            {
                config.common_config.nOutputWidth = pEnc->outputPort.def.format.video.nFrameHeight;
                config.common_config.nOutputHeight = pEnc->outputPort.def.format.video.nFrameWidth;
            }
            else
            {
                config.common_config.nOutputWidth = pEnc->outputPort.def.format.video.nFrameWidth;
                config.common_config.nOutputHeight = pEnc->outputPort.def.format.video.nFrameHeight;
            }
            config.pp_config.frameStabilization = pEnc->encConfig.stab.bStab;
            if ( config.pp_config.frameStabilization &&
                 pEnc->outputPort.def.format.video.nFrameWidth == pEnc->inputPort.def.format.video.nStride &&
                 pEnc->outputPort.def.format.video.nFrameHeight == pEnc->inputPort.def.format.video.nFrameHeight )
            {
                //Stabilization with default value ( 16 pixels)
                TRACE_PRINT( pEnc->log, "ASYNC: Using default stabilization values \n");
                config.pp_config.angle = pEnc->encConfig.rotation.nRotation;
                config.common_config.nOutputWidth -= 16;
                config.common_config.nOutputHeight -= 16;
                config.pp_config.xOffset = 0;
                config.pp_config.yOffset = 0;
            }
            else if ( config.pp_config.frameStabilization )
            {
                //Stabilization with user defined values
                //No other crop
                TRACE_PRINT( pEnc->log, "ASYNC: Using user defined stabilization values \n");
                config.pp_config.xOffset = 0;
                config.pp_config.yOffset = 0;
            }
            else
            {
                TRACE_PRINT( pEnc->log, "ASYNC: No stabilization \n");
                config.pp_config.xOffset = pEnc->encConfig.crop.nLeft;
                config.pp_config.yOffset = pEnc->encConfig.crop.nTop;
            }
#endif


        config.mp4_config.eProfile = pEnc->encConfig.mpeg4.eProfile;
        TRACE_PRINT( pEnc->log, "ASYNC: config.mp4_config.eProfile  - %u\n", (unsigned) config.mp4_config.eProfile);
        config.mp4_config.eLevel = pEnc->encConfig.mpeg4.eLevel;
        TRACE_PRINT( pEnc->log, "ASYNC: config.mp4_config.eLevel  - %u\n", (unsigned) config.mp4_config.eLevel);
        config.mp4_config.bSVH = pEnc->encConfig.mpeg4.bSVH;
        TRACE_PRINT( pEnc->log, "ASYNC: config.mp4_config.bSVH  - %u\n", (unsigned) config.mp4_config.bSVH);
        config.mp4_config.bReversibleVLC = pEnc->encConfig.mpeg4.bReversibleVLC;
        TRACE_PRINT( pEnc->log, "ASYNC: config.mp4_config.bReversibleVLC  - %u\n", (unsigned) config.mp4_config.bReversibleVLC );
        config.mp4_config.bGov = pEnc->encConfig.mpeg4.bGov;
        TRACE_PRINT( pEnc->log, "ASYNC: config.mp4_config.bGov  - %u\n", (unsigned) config.mp4_config.bGov );
        config.mp4_config.nPFrames = pEnc->encConfig.mpeg4.nPFrames;
        TRACE_PRINT( pEnc->log, "ASYNC: config.mp4_config.nPFrames  - %u\n", (unsigned) config.mp4_config.nPFrames );
        config.mp4_config.nTimeIncRes = pEnc->encConfig.mpeg4.nTimeIncRes;
        TRACE_PRINT( pEnc->log, "ASYNC: config.mp4_config.nTimeIncRes  - %u\n", (unsigned) config.mp4_config.nTimeIncRes );
        config.mp4_config.nHeaderExtension = pEnc->encConfig.mpeg4.nHeaderExtension;
        TRACE_PRINT( pEnc->log, "ASYNC: config.mp4_config.nHeaderExtension  - %u\n", (unsigned) config.mp4_config.nHeaderExtension );
        config.mp4_config.nMaxPacketSize = pEnc->encConfig.mpeg4.nMaxPacketSize;
        TRACE_PRINT( pEnc->log, "ASYNC: config.mp4_config.nMaxPacketSize  - %u\n", (unsigned) config.mp4_config.nMaxPacketSize );
        config.error_ctrl_config.bEnableDataPartitioning = pEnc->encConfig.ec.bEnableDataPartitioning;
        TRACE_PRINT( pEnc->log, "ASYNC: config.common_config.nOutputHeight  - %u\n", (unsigned) config.common_config.nOutputHeight );
        config.error_ctrl_config.bEnableResync = pEnc->encConfig.ec.bEnableResync;
        TRACE_PRINT( pEnc->log, "ASYNC: config.error_ctrl_config.bEnableResync - %u\n", (unsigned) config.error_ctrl_config.bEnableResync );
        config.error_ctrl_config.nResynchMarkerSpacing = pEnc->encConfig.ec.bEnableDataPartitioning;
        TRACE_PRINT( pEnc->log, "ASYNC: config.error_ctrl_config.nResynchMarkerSpacing  - %u\n", (unsigned) config.error_ctrl_config.nResynchMarkerSpacing );
        config.rate_config.nTargetBitrate = pEnc->outputPort.def.format.video.nBitrate;
        TRACE_PRINT( pEnc->log, "ASYNC: config.rate_config.nTargetBitrate  - %u\n", (unsigned) config.rate_config.nTargetBitrate );
        config.rate_config.nQpDefault = pEnc->encConfig.videoQuantization.nQpI;
        TRACE_PRINT( pEnc->log, "ASYNC: config.rate_config.nQpDefault  - %u\n", (unsigned) config.rate_config.nQpDefault );
        //config.rate_config.nQpMin = 4;
        config.rate_config.nQpMin = 1;
        TRACE_PRINT( pEnc->log, "ASYNC: config.rate_config.nQpMin  - %u\n", (unsigned) config.rate_config.nQpMin );
        config.rate_config.nQpMax = 31;
        TRACE_PRINT( pEnc->log, "ASYNC: config.rate_config.nQpMax  - %u\n", (unsigned) config.rate_config.nQpMax );
        config.rate_config.eRateControl = pEnc->encConfig.bitrate.eControlRate;
        //TRACE_PRINT( pEnc->log, "ASYNC: config.rate_config.eRateControl  - %u\n", (unsigned) config.rate_config.eRateControl );
        switch ( config.rate_config.eRateControl )
        {
            case OMX_Video_ControlRateVariable:
            {
                config.rate_config.nVbvEnabled = 0;
                config.rate_config.nMbRcEnabled = 0;
                config.rate_config.nHrdEnabled = 0;
                config.rate_config.nPictureRcEnabled = 1;
                                TRACE_PRINT(pEnc->log, "ASYNC: eRateControl: OMX_VIDEO_ControlRateVariable\n");
            }
            break;
            case OMX_Video_ControlRateDisable:
            {
                config.rate_config.nVbvEnabled = 0;
                config.rate_config.nMbRcEnabled = 0;
                config.rate_config.nHrdEnabled = 0;
                config.rate_config.nPictureRcEnabled = 0;
                                TRACE_PRINT(pEnc->log, "ASYNC: eRateControl: OMX_VIDEO_ControlRateDisable\n");
            }
            break;
            case OMX_Video_ControlRateConstant:
            {
                config.rate_config.nVbvEnabled = 1; // will in turn enable mbRC in EncRateControl
                config.rate_config.nMbRcEnabled = 1;
                config.rate_config.nHrdEnabled = 1;
                config.rate_config.nPictureRcEnabled = 1;
                                TRACE_PRINT(pEnc->log, "ASYNC: eRateControl: OMX_VIDEO_ControlRateConstant\n");
            }
            break;
            case OMX_Video_ControlRateConstantSkipFrames:
            {
                config.rate_config.nVbvEnabled = 1;
                config.rate_config.nMbRcEnabled = 1;
                config.rate_config.nHrdEnabled = 1;
                config.rate_config.nPictureRcEnabled = 1;
                                TRACE_PRINT(pEnc->log, "ASYNC: eRateControl: OMX_VIDEO_ControlRateConstantSkipFrames\n");
            }
            break;
            case  OMX_Video_ControlRateVariableSkipFrames:
            {
                config.rate_config.nVbvEnabled = 0;
                config.rate_config.nMbRcEnabled = 0;
                config.rate_config.nHrdEnabled = 0;
                config.rate_config.nPictureRcEnabled = 1;
                                TRACE_PRINT(pEnc->log, "ASYNC: eRateControl: OMX_VIDEO_ControlRateVariableSkipFrames\n");
            }
            break;
            default:
            {
                TRACE_PRINT(pEnc->log, "No rate control defined...using disabled value\n");
                config.rate_config.nVbvEnabled = 0;
                config.rate_config.eRateControl = OMX_Video_ControlRateDisable;
                config.rate_config.nMbRcEnabled = 0;
                config.rate_config.nHrdEnabled = 0;
                config.rate_config.nPictureRcEnabled = 0;
            }
            break;
        }
        TRACE_PRINT( pEnc->log, "ASYNC: config.rate_config.eRateControl - %u\n", (unsigned) config.rate_config.eRateControl );
        TRACE_PRINT( pEnc->log, "ASYNC: config.rate_config.nMbRcEnabled - %u\n", (unsigned) config.rate_config.nMbRcEnabled );
        TRACE_PRINT( pEnc->log, "ASYNC: config.rate_config.nHrdEnabled - %u\n", (unsigned) config.rate_config.nHrdEnabled );
        TRACE_PRINT( pEnc->log, "ASYNC: config.rate_config.nPictureRcEnabled - %u\n", (unsigned) config.rate_config.nPictureRcEnabled );
//        config.rate_config.nVbvEnabled = 1; // default, enabled. Should be always 1 (ON)

        codec = HantroHwEncOmx_encoder_create_mpeg4( &config );
    }
    break;
    case OMX_VIDEO_CodingH263:
    {
        TRACE_PRINT(pEnc->log, "ASYNC: Creating H263 codec\n");

        H263_CONFIG config;

        //Values from input port
        config.pp_config.origWidth = pEnc->inputPort.def.format.video.nStride;
        config.pp_config.origHeight =  pEnc->inputPort.def.format.video.nFrameHeight;
        config.common_config.nInputFramerate = pEnc->inputPort.def.format.video.xFramerate;
        config.pp_config.formatType = pEnc->inputPort.def.format.video.eColorFormat;

        config.pp_config.angle = pEnc->encConfig.rotation.nRotation;
#if 1
             if ( pEnc->encConfig.rotation.nRotation == 90 || pEnc->encConfig.rotation.nRotation == 270 )
            {
                config.common_config.nOutputWidth = pEnc->outputPort.def.format.video.nFrameHeight;
                config.common_config.nOutputHeight = pEnc->outputPort.def.format.video.nFrameWidth;
            }
            else
            {
                config.common_config.nOutputWidth = pEnc->outputPort.def.format.video.nFrameWidth;
                config.common_config.nOutputHeight = pEnc->outputPort.def.format.video.nFrameHeight;
            }
            config.pp_config.frameStabilization = pEnc->encConfig.stab.bStab;
            if ( config.pp_config.frameStabilization &&
                 pEnc->outputPort.def.format.video.nFrameWidth == pEnc->inputPort.def.format.video.nStride &&
                 pEnc->outputPort.def.format.video.nFrameHeight == pEnc->inputPort.def.format.video.nFrameHeight )
            {
                //Stabilization with default value ( 16 pixels)
                TRACE_PRINT( pEnc->log, "ASYNC: Using default stabilization values \n");
                config.pp_config.angle = pEnc->encConfig.rotation.nRotation;
                config.common_config.nOutputWidth -= 16;
                config.common_config.nOutputHeight -= 16;
                config.pp_config.xOffset = 0;
                config.pp_config.yOffset = 0;
            }
            else if ( config.pp_config.frameStabilization )
            {
                //Stabilization with user defined values
                //No other crop!
                TRACE_PRINT( pEnc->log, "ASYNC: Using user defined stabilization values \n");
                config.pp_config.xOffset = 0;
                config.pp_config.yOffset = 0;
            }
            else
            {
                TRACE_PRINT( pEnc->log, "ASYNC: No stabilization \n");
                config.pp_config.xOffset = pEnc->encConfig.crop.nLeft;
                config.pp_config.yOffset = pEnc->encConfig.crop.nTop;
            }
#endif

        //H263 specific values
        config.h263_config.eProfile =  pEnc->encConfig.h263.eProfile;
        config.h263_config.eLevel = pEnc->encConfig.h263.eLevel;
        config.h263_config.nPFrames = pEnc->encConfig.h263.nPFrames; // I frame interval
        config.h263_config.bPLUSPTYPEAllowed = OMX_FALSE;

        //Rate Control
        config.rate_config.nTargetBitrate = pEnc->outputPort.def.format.video.nBitrate;
        config.rate_config.nQpDefault = pEnc->encConfig.videoQuantization.nQpI;
        config.rate_config.nQpMin = 1;
        config.rate_config.nQpMax = 31;
        config.rate_config.eRateControl = pEnc->encConfig.bitrate.eControlRate;
        switch ( config.rate_config.eRateControl )
        {
            case OMX_Video_ControlRateVariable:
            {
                config.rate_config.nMbRcEnabled = 0;
                config.rate_config.nHrdEnabled = 0;
                config.rate_config.nPictureRcEnabled = 1;
            }
            break;
            case OMX_Video_ControlRateDisable:
            {
                config.rate_config.nMbRcEnabled = 0;
                config.rate_config.nHrdEnabled = 0;
                config.rate_config.nPictureRcEnabled = 0;
            }
            break;
            case OMX_Video_ControlRateConstant:
            {
                config.rate_config.nMbRcEnabled = 1;
                config.rate_config.nHrdEnabled = 1;
                config.rate_config.nPictureRcEnabled = 1;

            }
            break;
            case OMX_Video_ControlRateConstantSkipFrames:
            {
                config.rate_config.nMbRcEnabled = 1;
                config.rate_config.nHrdEnabled = 1;
                config.rate_config.nPictureRcEnabled = 1;
            }
            break;
            case  OMX_Video_ControlRateVariableSkipFrames:
            {
                config.rate_config.nMbRcEnabled = 0;
                config.rate_config.nHrdEnabled = 0;
                config.rate_config.nPictureRcEnabled = 1;
            }
            break;
            default:
            {
                TRACE_PRINT(pEnc->log, "No rate control defined...using disabled value\n");
                config.rate_config.eRateControl = OMX_Video_ControlRateDisable;
                config.rate_config.nMbRcEnabled = 0;
                config.rate_config.nHrdEnabled = 0;
                config.rate_config.nPictureRcEnabled = 0;
            }
            break;
        }

        config.rate_config.nVbvEnabled = 0;
        codec = HantroHwEncOmx_encoder_create_h263( &config );
        }
        break;

#endif /* ndef ENC8270 & ENC8290*/

#ifdef ENCH1
        case OMX_VIDEO_CodingVP8:
        {
            TRACE_PRINT(pEnc->log, "ASYNC: Creating VP8 codec...\n");
            VP8_CONFIG config;

            config.pp_config.origWidth = pEnc->inputPort.def.format.video.nStride;
            config.pp_config.origHeight = pEnc->inputPort.def.format.video.nFrameHeight;
            //config.nSliceHeight = 0;
            config.pp_config.formatType = pEnc->inputPort.def.format.video.eColorFormat;
            config.common_config.nInputFramerate = pEnc->inputPort.def.format.video.xFramerate;
            config.pp_config.angle = pEnc->encConfig.rotation.nRotation;;
            if ( pEnc->encConfig.rotation.nRotation == 90 || pEnc->encConfig.rotation.nRotation == 270 )
            {
                config.common_config.nOutputWidth = pEnc->outputPort.def.format.video.nFrameHeight;
                config.common_config.nOutputHeight = pEnc->outputPort.def.format.video.nFrameWidth;
            }
            else
            {
                config.common_config.nOutputWidth = pEnc->outputPort.def.format.video.nFrameWidth;
                config.common_config.nOutputHeight = pEnc->outputPort.def.format.video.nFrameHeight;
            }
            config.pp_config.frameStabilization = pEnc->encConfig.stab.bStab;
            if ( config.pp_config.frameStabilization &&
                 pEnc->outputPort.def.format.video.nFrameWidth == pEnc->inputPort.def.format.video.nStride &&
                 pEnc->outputPort.def.format.video.nFrameHeight == pEnc->inputPort.def.format.video.nFrameHeight )
            {
                //Stabilization with default value ( 16 pixels)
                TRACE_PRINT( pEnc->log, "ASYNC: Using default stabilization values \n");
                config.pp_config.angle = pEnc->encConfig.rotation.nRotation;
                config.common_config.nOutputWidth -= 16;
                config.common_config.nOutputHeight -= 16;
                config.pp_config.xOffset = 0;
                config.pp_config.yOffset = 0;
            }
            else if ( config.pp_config.frameStabilization )
            {
                //Stabilization with user defined values
                //No other crop
                TRACE_PRINT( pEnc->log, "ASYNC: Using user defined stabilization values \n");
                config.pp_config.xOffset = 0;
                config.pp_config.yOffset = 0;
            }
            else
            {
                TRACE_PRINT( pEnc->log, "ASYNC: No stabilization \n");
                config.pp_config.xOffset = pEnc->encConfig.crop.nLeft;
                config.pp_config.yOffset = pEnc->encConfig.crop.nTop;
            }
            config.vp8_config.eProfile = pEnc->encConfig.vp8.eProfile;
            config.vp8_config.eLevel = pEnc->encConfig.vp8.eLevel;
            config.vp8_config.nDCTPartitions = pEnc->encConfig.vp8.nDCTPartitions;
            config.vp8_config.bErrorResilientMode = pEnc->encConfig.vp8.bErrorResilientMode;
            config.rate_config.nTargetBitrate = pEnc->outputPort.def.format.video.nBitrate;
            config.rate_config.nQpDefault = pEnc->encConfig.videoQuantization.nQpI;
            config.rate_config.nQpMin = 0;
            config.rate_config.nQpMax = 127;

            config.rate_config.eRateControl = pEnc->encConfig.bitrate.eControlRate;
            switch ( config.rate_config.eRateControl )
            {
                case OMX_Video_ControlRateVariable:
                {
                    config.rate_config.nMbRcEnabled = 0;
                    config.rate_config.nHrdEnabled = 0;
                    config.rate_config.nPictureRcEnabled = 1;
                }
                break;
                case OMX_Video_ControlRateDisable:
                {
                    config.rate_config.nMbRcEnabled = 0;
                    config.rate_config.nHrdEnabled = 0;
                    config.rate_config.nPictureRcEnabled = 0;
                }
                break;
                case OMX_Video_ControlRateConstant:
                {
                    config.rate_config.nMbRcEnabled = 1;
                    config.rate_config.nHrdEnabled = 1;
                    config.rate_config.nPictureRcEnabled = 1;

                }
                break;
                case OMX_Video_ControlRateConstantSkipFrames:
                {
                    config.rate_config.nMbRcEnabled = 1;
                    config.rate_config.nHrdEnabled = 1;
                    config.rate_config.nPictureRcEnabled = 1;
                }
                break;
                case  OMX_Video_ControlRateVariableSkipFrames:
                {
                    config.rate_config.nMbRcEnabled = 0;
                    config.rate_config.nHrdEnabled = 0;
                    config.rate_config.nPictureRcEnabled = 1;
                }
                break;
                default:
                {
                    TRACE_PRINT(pEnc->log, "No rate control defined...using disabled value\n");
                    config.rate_config.eRateControl = OMX_Video_ControlRateDisable;
                    config.rate_config.nMbRcEnabled = 0;
                    config.rate_config.nHrdEnabled = 0;
                    config.rate_config.nPictureRcEnabled = 0;
                }
                break;
            }

            codec = HantroHwEncOmx_encoder_create_vp8( &config );
        }
        break;
#endif //ENCH1
    default:
        TRACE_PRINT(pEnc->log, "ASYNC: unsupported input format. no such codec\n");
        err = OMX_ErrorUnsupportedSetting;;
        break;
    }
#endif //~OMX_ENCODER_VIDEO_DOMAIN
#ifdef OMX_ENCODER_IMAGE_DOMAIN
    switch ( pEnc->outputPort.def.format.image.eCompressionFormat )
    {
        TRACE_PRINT(pEnc->log, "IMAGE Domain\n");
        case OMX_IMAGE_CodingJPEG:
        {
            TRACE_PRINT(pEnc->log, "Creating JPEG encoder\n");

            JPEG_CONFIG config;

            //Values from input port
            config.pp_config.origWidth = pEnc->inputPort.def.format.image.nStride;
            TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.origWidth - %u\n", (unsigned) config.pp_config.origWidth );
            config.pp_config.origHeight =  pEnc->inputPort.def.format.image.nFrameHeight;
            TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.origHeight - %u\n", (unsigned) config.pp_config.origHeight );
            config.pp_config.formatType = pEnc->inputPort.def.format.image.eColorFormat;
            TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.formatType - %u\n", (unsigned) config.pp_config.formatType );
            config.sliceHeight = pEnc->inputPort.def.format.image.nSliceHeight;
            TRACE_PRINT( pEnc->log, "ASYNC: config.sliceHeight - %u\n", (unsigned) config.sliceHeight );

            //Pre processor values
            config.pp_config.angle = pEnc->encConfig.rotation.nRotation;
            TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.angle - %u\n", (unsigned) config.pp_config.angle );
            if ( pEnc->encConfig.rotation.nRotation == 90 || pEnc->encConfig.rotation.nRotation == 270 )
            {

                config.pp_config.xOffset = pEnc->encConfig.crop.nTop;
                TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.xOffset - %u\n", (unsigned) config.pp_config.xOffset );
                config.pp_config.yOffset = pEnc->encConfig.crop.nLeft;
                TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.yOffset - %u\n", (unsigned) config.pp_config.yOffset );
                config.codingWidth = pEnc->outputPort.def.format.image.nFrameHeight;
                TRACE_PRINT( pEnc->log, "ASYNC: config.codingWidth - %u\n", (unsigned) config.codingWidth );
                config.codingHeight = pEnc->outputPort.def.format.image.nFrameWidth;
                TRACE_PRINT( pEnc->log, "ASYNC: config.codingHeight - %u\n", (unsigned) config.codingHeight );
            }
            else
            {
                config.pp_config.xOffset = pEnc->encConfig.crop.nLeft;
                TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.xOffset - %u\n", (unsigned) config.pp_config.xOffset );
                config.pp_config.yOffset = pEnc->encConfig.crop.nTop;
                TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.yOffset - %u\n", (unsigned) config.pp_config.yOffset );
                config.codingWidth = pEnc->outputPort.def.format.image.nFrameWidth;
                TRACE_PRINT( pEnc->log, "ASYNC: config.codingWidth - %u\n", (unsigned) config.codingWidth );
                config.codingHeight = pEnc->outputPort.def.format.image.nFrameHeight;
                TRACE_PRINT( pEnc->log, "ASYNC: config.codingHeight - %u\n", (unsigned) config.codingHeight );
            }

            config.qLevel = pEnc->encConfig.imageQuantization.nQFactor;
            TRACE_PRINT( pEnc->log, "ASYNC: config.qLevel - %u\n", (unsigned) config.qLevel );
            config.unitsType = JPEGENC_DOTS_PER_INCH;
            config.markerType = JPEGENC_MULTI_MARKER;
            if( config.sliceHeight == pEnc->inputPort.def.format.image.nFrameHeight ||
                config.sliceHeight == 0 )
            {
                TRACE_PRINT( pEnc->log, "config.codingType = JPEGENC_WHOLE_FRAME\n" );
                config.codingType = JPEGENC_WHOLE_FRAME;
            }
            else
            {
                TRACE_PRINT( pEnc->log, "config.codingType = JPEGENC_SLICED_FRAME;\n" );
                config.codingType = JPEGENC_SLICED_FRAME;
                pEnc->sliceMode = OMX_TRUE;
                if ( config.codingHeight % config.sliceHeight == 0 )
                {
                    TRACE_PRINT( pEnc->log, "config.codingHeight config.sliceHeight > 0 - TRUE \n" );
                    pEnc->numOfSlices = config.codingHeight / config.sliceHeight;
                }
                else
                {
                    TRACE_PRINT( pEnc->log, "config.codingHeight config.sliceHeight > 0 - FALSE \n" );
                    pEnc->numOfSlices = config.codingHeight / config.sliceHeight + 1;
                }
            }

            config.xDensity = 72;
            config.yDensity = 72;
            config.bAddHeaders = OMX_TRUE;

            codec = HantroHwEncOmx_encoder_create_jpeg( &config );
        }
            break;
#ifdef ENCH1
        case OMX_IMAGE_CodingWEBP:
        {
            TRACE_PRINT(pEnc->log, "Creating WEBP encoder\n");

            WEBP_CONFIG config;

            //Values from input port
            config.pp_config.origWidth = pEnc->inputPort.def.format.image.nStride;
            TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.origWidth - %u\n", (unsigned) config.pp_config.origWidth );
            config.pp_config.origHeight =  pEnc->inputPort.def.format.image.nFrameHeight;
            TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.origHeight - %u\n", (unsigned) config.pp_config.origHeight );
            config.pp_config.formatType = pEnc->inputPort.def.format.image.eColorFormat;
            TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.formatType - %u\n", (unsigned) config.pp_config.formatType );
            config.sliceHeight = pEnc->inputPort.def.format.image.nSliceHeight;
            TRACE_PRINT( pEnc->log, "ASYNC: config.sliceHeight - %u\n", (unsigned) config.sliceHeight );

            //Pre processor values
            config.pp_config.angle = pEnc->encConfig.rotation.nRotation;
            TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.angle - %u\n", (unsigned) config.pp_config.angle );
            if ( pEnc->encConfig.rotation.nRotation == 90 || pEnc->encConfig.rotation.nRotation == 270 )
            {

                config.pp_config.xOffset = pEnc->encConfig.crop.nTop;
                TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.xOffset - %u\n", (unsigned) config.pp_config.xOffset );
                config.pp_config.yOffset = pEnc->encConfig.crop.nLeft;
                TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.yOffset - %u\n", (unsigned) config.pp_config.yOffset );
                config.codingWidth = pEnc->outputPort.def.format.image.nFrameHeight;
                TRACE_PRINT( pEnc->log, "ASYNC: config.codingWidth - %u\n", (unsigned) config.codingWidth );
                config.codingHeight = pEnc->outputPort.def.format.image.nFrameWidth;
                TRACE_PRINT( pEnc->log, "ASYNC: config.codingHeight - %u\n", (unsigned) config.codingHeight );
            }
            else
            {
                config.pp_config.xOffset = pEnc->encConfig.crop.nLeft;
                TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.xOffset - %u\n", (unsigned) config.pp_config.xOffset );
                config.pp_config.yOffset = pEnc->encConfig.crop.nTop;
                TRACE_PRINT( pEnc->log, "ASYNC: config.pp_config.yOffset - %u\n", (unsigned) config.pp_config.yOffset );
                config.codingWidth = pEnc->outputPort.def.format.image.nFrameWidth;
                TRACE_PRINT( pEnc->log, "ASYNC: config.codingWidth - %u\n", (unsigned) config.codingWidth );
                config.codingHeight = pEnc->outputPort.def.format.image.nFrameHeight;
                TRACE_PRINT( pEnc->log, "ASYNC: config.codingHeight - %u\n", (unsigned) config.codingHeight );
            }

            config.qLevel = pEnc->encConfig.imageQuantization.nQFactor;
            TRACE_PRINT( pEnc->log, "ASYNC: config.qLevel - %u\n", (unsigned) config.qLevel );
            /* not supported
                if( config.sliceHeight == pEnc->inputPort.def.format.image.nFrameHeight ||
                config.sliceHeight == 0 )
            {
                TRACE_PRINT( pEnc->log, "config.codingType = WEBPENC_WHOLE_FRAME\n" );
                config.codingType = WEBPENC_WHOLE_FRAME;
            }
            else
            {
                TRACE_PRINT( pEnc->log, "config.codingType = WEBPENC_SLICED_FRAME;\n" );
                config.codingType = WEBPENC_SLICED_FRAME;
                pEnc->sliceMode = OMX_TRUE;
                if ( config.codingHeight % config.sliceHeight == 0 )
                {
                    TRACE_PRINT( pEnc->log, "config.codingHeight config.sliceHeight > 0 - TRUE \n" );
                    pEnc->numOfSlices = config.codingHeight / config.sliceHeight;
                }
                else
                {
                    TRACE_PRINT( pEnc->log, "config.codingHeight config.sliceHeight > 0 - FALSE \n" );
                    pEnc->numOfSlices = config.codingHeight / config.sliceHeight + 1;
                }
            }*/

            codec = HantroHwEncOmx_encoder_create_webp( &config );
        }
            break;
#endif //ENCH1
        default:
            TRACE_PRINT(pEnc->log, "ASYNC: unsupported input format. no such codec\n");
            err = OMX_ErrorUnsupportedSetting;
            break;
    }
#endif //~OMX_ENCODER_IMAGE_DOMAIN
#endif //~OMX_ENCODER_TEST_CODEC

    if ( !codec )
    {
        TRACE_PRINT(pEnc->log, "ASYNC: Codec creating error\n");
        goto FAIL;
    }

    assert(codec->destroy);
    assert(codec->stream_start);
    assert(codec->stream_end);
    assert(codec->encode);

    OMX_U32 input_buffer_size  = pEnc->inputPort.def.nBufferSize;
    OMX_U32 output_buffer_size = pEnc->outputPort.def.nBufferSize;

    // create the temporary frame buffers. These are needed when
    // we are either using a buffer that was allocated by the client
    // or when the data in the buffers contains partial encoding units

//BS:: ZERO COPY
//    err = OSAL_AllocatorAllocMem(&pEnc->alloc, &input_buffer_size, &in.bus_data, &in.bus_address);
//    if (err != OMX_ErrorNone)
//        goto FAIL;
	in.bus_data = NULL;	/* will be filled later (zero-copy) */
	in.bus_address = 0;


    //NOTE: Specify internal outputbuffer size here according to compression format
	TRACE_PRINT(pEnc->log, "calling OMX_AllocatorAllocMem with size = %d",output_buffer_size);
    err = OSAL_AllocatorAllocMem(&pEnc->alloc, &output_buffer_size, &out.bus_data, &out.bus_address);
    if (err != OMX_ErrorNone)
        goto FAIL;
	TRACE_PRINT(pEnc->log, "calling OMX_AllocatorAllocMem success with address = 0x%x",out.bus_address);

    in.capacity  = input_buffer_size;
    out.capacity = output_buffer_size;
    pEnc->codec     = codec;
    pEnc->frame_in  = in;
    pEnc->frame_out = out;

    return OMX_ErrorNone;

 FAIL:
   if (out.bus_address)
        OSAL_AllocatorFreeMem(&pEnc->alloc, output_buffer_size, out.bus_data, out.bus_address);
    if (in.bus_address)
        OSAL_AllocatorFreeMem(&pEnc->alloc, input_buffer_size, in.bus_data, in.bus_address);

    if (codec)
        codec->destroy(codec);

    return err;

}


/**
 * static OMX_ERRORTYPE async_encoder_return_buffers(OMX_ENCODER* pEnc, PORT* p)
 */
static
OMX_ERRORTYPE async_encoder_return_buffers(OMX_ENCODER* pEnc, PORT* p)
{
    if (HantroOmx_port_is_supplier(p))
        return OMX_ErrorNone;

    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "ASYNC: returning allocated buffers on port %d to %p %d\n",
                (int)p->def.nPortIndex,
                p->tunnelcomp,
                (int)p->tunnelport);

    // return the queued buffers to the suppliers
    // Danger. The port's locked and there are callbacks into the unknown.

    OMX_ERRORTYPE err = HantroOmx_port_lock_buffers(p);
    if (err != OMX_ErrorNone)
        return err;

    OMX_U32 i;
    OMX_U32 count = HantroOmx_port_buffer_queue_count(p);
    for (i=0; i<count; ++i)
    {
        BUFFER* buff = 0;
        HantroOmx_port_get_buffer_at(p, &buff, i);
        assert(buff);
        if (HantroOmx_port_is_tunneled(p))
        {
            // tunneled but someone else is supplying. i.e. we have allocated the header.
            assert(buff->header == &buff->headerdata);
            // return the buffer to the supplier
            if (p->def.eDir == OMX_DirInput)
                ((OMX_COMPONENTTYPE*)p->tunnelcomp)->FillThisBuffer(p->tunnelcomp, buff->header);
            if (p->def.eDir == OMX_DirOutput)
                ((OMX_COMPONENTTYPE*)p->tunnelcomp)->EmptyThisBuffer(p->tunnelcomp, buff->header);
        }
        else
        {
            // return the buffer to the client
            if (p->def.eDir == OMX_DirInput)
                pEnc->app_callbacks.EmptyBufferDone(pEnc->self, pEnc->app_data, buff->header);
            if (p->def.eDir == OMX_DirOutput)
                pEnc->app_callbacks.FillBufferDone(pEnc->self, pEnc->app_data, buff->header);
        }
    }
    HantroOmx_port_buffer_queue_clear(p);
    HantroOmx_port_unlock_buffers(p);
    return OMX_ErrorNone;
}

/**
 * static OMX_ERRORTYPE transition_to_idle_from_paused(OMX_ENCODER* pEnc)
 */
static
OMX_ERRORTYPE transition_to_idle_from_paused(OMX_ENCODER* pEnc)
{
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    assert(pEnc->state == OMX_StatePause);
    assert(pEnc->statetrans == OMX_StateIdle);

    async_encoder_return_buffers(pEnc, &pEnc->inputPort);
    async_encoder_return_buffers(pEnc, &pEnc->outputPort);

    return OMX_ErrorNone;
}

static
OMX_ERRORTYPE wait_supplied_buffers(OMX_ENCODER* pEnc, PORT* port)
{
    if (!HantroOmx_port_is_supplier(port))
        return OMX_ErrorNone;

    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    // buffer supplier component cannot transition
    // to idle untill all the supplied buffers have been returned
    // by the buffer user via a call to our FillThisBuffer
    int loop = 1;
    while (loop)
    {
        HantroOmx_port_lock_buffers(port);
        OMX_U32 queued = HantroOmx_port_buffer_queue_count(port);

        if (HantroOmx_port_buffer_count(port) == queued)
            loop = 0;

        TRACE_PRINT(pEnc->log, "ASYNC: port %d has %d buffers out of %d supplied\n",
                    (int)port->def.nPortIndex,
                    (int)queued,
                    (int)HantroOmx_port_buffer_count(port));
        HantroOmx_port_unlock_buffers(port);
        if (loop)
            OSAL_ThreadSleep(RETRY_INTERVAL);
    }
    return OMX_ErrorNone;
}

/**
 * static OMX_ERRORTYPE transition_to_idle_from_executing(OMX_ENCODER* pEnc)
 */
static
OMX_ERRORTYPE transition_to_idle_from_executing(OMX_ENCODER* pEnc)
{
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    assert(pEnc->state      == OMX_StateExecuting);
    assert(pEnc->statetrans == OMX_StateIdle);

    async_encoder_return_buffers(pEnc, &pEnc->inputPort);
    async_encoder_return_buffers(pEnc, &pEnc->outputPort);

    wait_supplied_buffers(pEnc, &pEnc->inputPort);
    wait_supplied_buffers(pEnc, &pEnc->outputPort);

    //Check do we have to call stream_end for HW encoder
    if ( pEnc->streamStarted )
    {
        pEnc->streamStarted = OMX_FALSE;
    }


    return OMX_ErrorNone;
}

/**
 * static OMX_ERRORTYPE transition_to_loaded_from_idle(OMX_ENCODER* pEnc)
 */
static
OMX_ERRORTYPE transition_to_loaded_from_idle(OMX_ENCODER* pEnc)
{
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    // the state transition cannot complete untill all
    // enabled ports have their buffers freed

    assert(pEnc->state      == OMX_StateIdle);
    assert(pEnc->statetrans == OMX_StateLoaded);

    if (HantroOmx_port_is_supplier(&pEnc->inputPort))
        unsupply_tunneled_port(pEnc, &pEnc->inputPort);
    if (HantroOmx_port_is_supplier(&pEnc->outputPort))
        unsupply_tunneled_port(pEnc, &pEnc->outputPort);

    // should be okay without locking
    assert(HantroOmx_port_buffer_queue_count(&pEnc->inputPort)==0);
    assert(HantroOmx_port_buffer_queue_count(&pEnc->outputPort)==0);


    while (HantroOmx_port_has_buffers(&pEnc->inputPort)  ||
           HantroOmx_port_has_buffers(&pEnc->outputPort) )
    {
        OSAL_ThreadSleep(RETRY_INTERVAL);
    }

    assert(pEnc->codec);

    pEnc->codec->destroy(pEnc->codec);

//BS:: do not free input buffer, address is used as zero copy from upper layers
	/*if (pEnc->frame_in.bus_address)
	  FRAME_BUFF_FREE(&pEnc->alloc, &pEnc->frame_in);*/
    if (pEnc->frame_out.bus_address)
        FRAME_BUFF_FREE(&pEnc->alloc, &pEnc->frame_out);

    pEnc->codec    = NULL;
    memset(&pEnc->frame_in,  0, sizeof(FRAME_BUFFER));
    memset(&pEnc->frame_out, 0, sizeof(FRAME_BUFFER));

    return OMX_ErrorNone;
}

/**
 * static OMX_ERRORTYPE transition_to_loaded_from_waitForResources(pEnc)
 */
static
OMX_ERRORTYPE transition_to_loaded_from_waitForResources( OMX_ENCODER* pEnc)
{
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    // the state transition cannot complete untill all
    // enabled ports have their buffers freed

    assert(pEnc->state      == OMX_StateWaitForResources);
    assert(pEnc->statetrans == OMX_StateLoaded);

    //NOTE: External HWRM should be used here
    return OMX_ErrorNone;
}

/**
 * static OMX_ERRORTYPE transition_to_executing_from_paused(OMX_ENCODER* pEnc)
 */
static
OMX_ERRORTYPE transition_to_executing_from_paused(OMX_ENCODER* pEnc)
{
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    assert(pEnc->state == OMX_StatePause);
    assert(pEnc->statetrans == OMX_StateExecuting);

    return OMX_ErrorNone;
}

/**
 * static OMX_ERRORTYPE transition_to_executing_from_idle(OMX_ENCODER* pEnc)
 */
static
OMX_ERRORTYPE transition_to_executing_from_idle(OMX_ENCODER* pEnc)
{

    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    assert(pEnc->state == OMX_StateIdle);

    return OMX_ErrorNone;
}

static
OMX_ERRORTYPE startup_tunnel(OMX_ENCODER* pEnc, PORT* port)
{
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    OMX_U32 buffers   = 0;
    OMX_U32 i         = 0;
    OMX_ERRORTYPE err = OMX_ErrorNone;
    if (HantroOmx_port_is_enabled(port) && HantroOmx_port_is_supplier(port))
    {
        if (port == &pEnc->outputPort)
        {
            // queue the buffers into own queue since the
            // output port is the supplier the tunneling component
            // cannot fill the buffer queue.
            buffers = HantroOmx_port_buffer_count(port);
            for (i=0; i<buffers; ++i)
            {
                BUFFER* buff = NULL;
                HantroOmx_port_get_allocated_buffer_at(port, &buff, i);
                HantroOmx_port_push_buffer(port, buff);
            }
        }
        else
        {
            buffers = HantroOmx_port_buffer_count(port);
            for (; i<buffers; ++i)
            {
                BUFFER* buff = NULL;
                HantroOmx_port_get_allocated_buffer_at(port, &buff, i);
                assert(buff);
                assert(buff->header != &buff->headerdata);
                err = ((OMX_COMPONENTTYPE*)port->tunnelcomp)->FillThisBuffer(port->tunnelcomp, buff->header);
                if (err != OMX_ErrorNone)
                {
                    TRACE_PRINT(pEnc->log, "ASYNC: tunneling component failed to fill buffer: %s\n", HantroOmx_str_omx_err(err));
                    return err;
                }
            }
        }
    }
    return OMX_ErrorNone;
}


/**
 * OMX_ERRORTYPE transition_to_waitForResources_from_loaded( OMX_ENCODER* pEnc)
 */
static
OMX_ERRORTYPE transition_to_waitForResources_from_loaded( OMX_ENCODER* pEnc)
{
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    assert(pEnc->state == OMX_StateLoaded);

    //NOTE: External HWRM should be used here

    return OMX_ErrorNone;

}

/**
 * static OMX_ERRORTYPE async_encoder_set_state(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1,
                                             OMX_PTR pCmdData, OMX_PTR arg)

 */
static
OMX_ERRORTYPE async_encoder_set_state(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1,
                                             OMX_PTR pCmdData, OMX_PTR arg)
{
    OMX_ERRORTYPE err = OMX_ErrorIncorrectStateTransition;
    OMX_ENCODER*  pEnc = (OMX_ENCODER*)arg;
    OMX_STATETYPE nextstate = (OMX_STATETYPE)nParam1;
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    if (nextstate == pEnc->state)
    {
        TRACE_PRINT(pEnc->log, "ASYNC: same state:%s\n", HantroOmx_str_omx_state(pEnc->state));
        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventError, OMX_ErrorSameState, 0, NULL);
        return OMX_ErrorNone;
    }
    switch (nextstate)
    {
        case OMX_StateIdle:
            switch (pEnc->state)
            {
                case OMX_StateLoaded:
                    err = transition_to_idle_from_loaded(pEnc);
                    break;
                case OMX_StatePause:
                    err = transition_to_idle_from_paused(pEnc);
                    break;
                case OMX_StateExecuting:
                    err = transition_to_idle_from_executing(pEnc);
                    break;
                default: break;
            }
            break;
        case OMX_StateLoaded:
            if (pEnc->state == OMX_StateIdle)
                err = transition_to_loaded_from_idle(pEnc);
            if (pEnc->state == OMX_StateWaitForResources)
                err = transition_to_loaded_from_waitForResources(pEnc);
            break;
        case OMX_StateExecuting:
            if (pEnc->state == OMX_StatePause)                   /// OMX_StatePause to OMX_StateExecuting
                err = transition_to_executing_from_paused(pEnc);
            if (pEnc->state == OMX_StateIdle)
                err = transition_to_executing_from_idle(pEnc);   /// OMX_StateIdle to OMX_StateExecuting
            break;
        case OMX_StatePause:
            {
                if (pEnc->state == OMX_StateIdle)                /// OMX_StateIdle to OMX_StatePause
                    err = OMX_ErrorNone;
                if (pEnc->state == OMX_StateExecuting)           /// OMX_StateExecuting to OMX_StatePause
                    err = OMX_ErrorNone;
            }
            break;
        case OMX_StateInvalid:
            err = OMX_ErrorInvalidState;
            break;
        case OMX_StateWaitForResources:
            if (pEnc->state == OMX_StateLoaded)
                    err = transition_to_waitForResources_from_loaded(pEnc);
            break;
        default:
            assert(!"Incorrect state");
            break;
    }

    if (err != OMX_ErrorNone)
    {
        TRACE_PRINT(pEnc->log, "ASYNC: set state error:%s\n", HantroOmx_str_omx_err(err));
        if (err != OMX_ErrorIncorrectStateTransition)
        {
            pEnc->state = OMX_StateInvalid;
            pEnc->run   = OMX_FALSE;
        }
        else
        {
            // clear state transition flag
            pEnc->statetrans = pEnc->state;
        }
       pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventError, err, 0, NULL);
    }
    else
    {
        // transition complete
        TRACE_PRINT(pEnc->log, "ASYNC: set state complete:%s\n", HantroOmx_str_omx_state(nextstate));
        pEnc->statetrans = nextstate;
        pEnc->state      = nextstate;
        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventCmdComplete, OMX_CommandStateSet, pEnc->state, NULL);

        if (pEnc->state == OMX_StateExecuting)
        {
            err = startup_tunnel(pEnc, &pEnc->inputPort);
            if (err != OMX_ErrorNone)
            {
                TRACE_PRINT(pEnc->log, "ASYNC: failed to startup buffer processing: %s\n", HantroOmx_str_omx_err(err));
                pEnc->state = OMX_StateInvalid;
                pEnc->run   = OMX_FALSE;
            }
            startup_tunnel(pEnc, &pEnc->outputPort);
        }
    }
    return err;
}

/**
 * static OMX_ERRORTYPE async_encoder_wait_port_buffer_dealloc(OMX_ENCODER* pEnc, PORT* p)
 */
static
OMX_ERRORTYPE async_encoder_wait_port_buffer_dealloc(OMX_ENCODER* pEnc, PORT* p)
{
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    while (HantroOmx_port_has_buffers(p))
    {
        OSAL_ThreadSleep(RETRY_INTERVAL);
    }
    return OMX_ErrorNone;
}


/**
 * OMX_ERRORTYPE async_encoder_disable_port(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1,
                                            OMX_PTR pCmdData, OMX_PTR arg)
 * Port cannot be populated when it is disabled
 */
static
OMX_ERRORTYPE async_encoder_disable_port(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1,
                                         OMX_PTR pCmdData, OMX_PTR arg)
{
    OMX_ENCODER*  pEnc = (OMX_ENCODER*)arg;
    OMX_U32 portIndex = nParam1;

    // return the queue'ed buffers to the suppliers
    // and wait untill all the buffers have been free'ed

    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "ASYNC: nParam1:%u pCmdData:%p\n", (unsigned)nParam1, pCmdData);

    if ( portIndex == -1)
    {
        TRACE_PRINT(pEnc->log, "ASYNC: disabling all ports\n");
        async_encoder_return_buffers(pEnc, &pEnc->inputPort);
        async_encoder_return_buffers(pEnc, &pEnc->outputPort);

        wait_supplied_buffers(pEnc, &pEnc->inputPort);
        wait_supplied_buffers(pEnc, &pEnc->outputPort);

        if (HantroOmx_port_is_supplier(&pEnc->inputPort))
            unsupply_tunneled_port(pEnc, &pEnc->inputPort);
        if (HantroOmx_port_is_supplier(&pEnc->outputPort))
            unsupply_tunneled_port(pEnc, &pEnc->outputPort);

        async_encoder_wait_port_buffer_dealloc(pEnc, &pEnc->inputPort);
        async_encoder_wait_port_buffer_dealloc(pEnc, &pEnc->outputPort);

        int i=0;
        for (i=0; i<2; ++i)
        {
            pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventCmdComplete, OMX_CommandPortDisable, i, NULL);
        }
        TRACE_PRINT(pEnc->log, "ASYNC: ports disabled\n");
    }
    else
    {
        PORT* ports[] = {&pEnc->inputPort, &pEnc->outputPort};
        int i = 0;
        for (i=0; i<sizeof(ports)/sizeof(PORT*); ++i)
        {
            if (portIndex == i)
            {
                TRACE_PRINT(pEnc->log, "ASYNC: disabling port: %d\n", i);
                async_encoder_return_buffers(pEnc, ports[i]);
                wait_supplied_buffers(pEnc, ports[i]);
                if (HantroOmx_port_is_supplier(ports[i]))
                    unsupply_tunneled_port(pEnc, ports[i]);

                async_encoder_wait_port_buffer_dealloc(pEnc, ports[i]);
                break;
            }
        }
        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventCmdComplete, OMX_CommandPortDisable, portIndex, NULL);
    }

    return OMX_ErrorNone;
}


/**
 * OMX_ERRORTYPE async_encoder_enable_port(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1,
                                            OMX_PTR pCmdData, OMX_PTR arg)
 */
static
OMX_ERRORTYPE async_encoder_enable_port(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1,
                                        OMX_PTR pCmdData, OMX_PTR arg)
{
    OMX_ENCODER* pEnc  = (OMX_ENCODER*)arg;
    OMX_U32 portIndex = nParam1;
    OMX_ERRORTYPE err = OMX_ErrorNone;

    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "ASYNC: nParam1:%u pCmdData:%p\n", (unsigned)nParam1, pCmdData);

    if (portIndex == PORT_INDEX_INPUT || portIndex == -1)
    {
        if (pEnc->state != OMX_StateLoaded)
        {
            if (HantroOmx_port_is_supplier(&pEnc->inputPort))
            {
                err = supply_tunneled_port(pEnc, &pEnc->inputPort);
                if (err != OMX_ErrorNone)
                    return err;
            }
            while (!HantroOmx_port_is_ready(&pEnc->inputPort))
                OSAL_ThreadSleep(RETRY_INTERVAL);
        }
        if (pEnc->state == OMX_StateExecuting)
            startup_tunnel(pEnc, &pEnc->inputPort);

        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventCmdComplete, OMX_CommandPortEnable, PORT_INDEX_INPUT, NULL);
        TRACE_PRINT(pEnc->log, "ASYNC: input port enabled\n");

    }
    if (portIndex == PORT_INDEX_OUTPUT || portIndex == -1)
    {
        if (pEnc->state != OMX_StateLoaded)
        {
            if (HantroOmx_port_is_supplier(&pEnc->outputPort))
            {
                err = supply_tunneled_port(pEnc, &pEnc->outputPort);
                if (err != OMX_ErrorNone)
                    return err;
            }
            while (!HantroOmx_port_is_ready(&pEnc->outputPort))
                OSAL_ThreadSleep(RETRY_INTERVAL);
        }
        if (pEnc->state == OMX_StateExecuting)
            startup_tunnel(pEnc, &pEnc->outputPort);

        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventCmdComplete, OMX_CommandPortEnable, PORT_INDEX_OUTPUT, NULL);
        TRACE_PRINT(pEnc->log, "ASYNC: output port enabled\n");
    }
    return OMX_ErrorNone;
}

/**
 * static
OMX_ERRORTYPE async_encoder_flush_port(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData, OMX_PTR arg)
 */
static
OMX_ERRORTYPE async_encoder_flush_port(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData, OMX_PTR arg)
{
    assert(arg);
    OMX_ENCODER* pEnc  = (OMX_ENCODER*)arg;
    OMX_U32 portindex = nParam1;

    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "ASYNC: nParam1:%u pCmdData:%p\n", (unsigned)nParam1, pCmdData);

    OMX_ERRORTYPE err = OMX_ErrorNone;
    if (portindex == -1)
    {
        TRACE_PRINT(pEnc->log, "ASYNC: flushing all ports\n");
        err = async_encoder_return_buffers(pEnc, &pEnc->inputPort);
        if (err != OMX_ErrorNone)
            goto FAIL;
        err = async_encoder_return_buffers(pEnc, &pEnc->outputPort);
        if (err != OMX_ErrorNone)
            goto FAIL;
        int i=0;
        for (i=0; i<2; ++i)
        {
            pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventCmdComplete, OMX_CommandFlush, i,NULL);
        }
        TRACE_PRINT(pEnc->log, "ASYNC: all buffers flushed\n");
    }
    else
    {
        if (portindex == PORT_INDEX_INPUT)
        {
            err = async_encoder_return_buffers(pEnc, &pEnc->inputPort);
            if (err != OMX_ErrorNone)
                goto FAIL;

            pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventCmdComplete, OMX_CommandFlush, portindex, NULL);
            TRACE_PRINT(pEnc->log, "ASYNC: input port flushed\n");
        }
        else if (portindex == PORT_INDEX_OUTPUT)
        {
            err = async_encoder_return_buffers(pEnc, &pEnc->outputPort);
            if (err != OMX_ErrorNone)
                goto FAIL;

            pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventCmdComplete, OMX_CommandFlush, portindex, NULL);
            TRACE_PRINT(pEnc->log, "ASYNC: output port flushed\n");
        }
    }
    return OMX_ErrorNone;
 FAIL:
    assert(err != OMX_ErrorNone);
    TRACE_PRINT(pEnc->log, "ASYNC: error while flushing port: %s\n", HantroOmx_str_omx_err(err));
    pEnc->state = OMX_StateInvalid;
    pEnc->run   = OMX_FALSE;
    pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventError, err, 0, NULL);
    return err;
}


static
OMX_ERRORTYPE async_encoder_mark_buffer(OMX_COMMANDTYPE Cmd, OMX_U32 nParam1, OMX_PTR pCmdData, OMX_PTR arg)
{
    assert(arg);
    assert(pCmdData);
    OMX_MARKTYPE* mark = (OMX_MARKTYPE*)pCmdData;
    OMX_ENCODER* dec   = (OMX_ENCODER*)arg;

    TRACE_PRINT(dec->log, "ASYNC: %s\n", __FUNCTION__);

    if (dec->mark_write_pos < sizeof(dec->marks)/sizeof(OMX_MARKTYPE))
    {
        dec->marks[dec->mark_write_pos] = *mark;
        TRACE_PRINT(dec->log, "ASYNC: set mark in index: %d\n", (int)dec->mark_write_pos);
        dec->mark_write_pos++;
    }
    else
    {
        TRACE_PRINT(dec->log, "ASYNC: no space for mark\n");
        dec->app_callbacks.EventHandler(dec->self, dec->app_data, OMX_EventError, OMX_ErrorUndefined, 0, NULL);
    }
    OSAL_Free(mark);
    return OMX_ErrorNone;
}

/**
 * OMX_ERRORTYPE grow_frame_buffer(OMX_ENCODER* pEnc, FRAME_BUFFER* fb, OMX_U32 capacity)
 */
static
OMX_ERRORTYPE grow_frame_buffer(OMX_ENCODER* pEnc, FRAME_BUFFER* fb, OMX_U32 capacity)
{
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "ASYNC: fb capacity: %u new capacity:%u\n",
                (unsigned)fb->capacity, (unsigned)capacity);

    FRAME_BUFFER new;
    memset(&new, 0, sizeof(FRAME_BUFFER));
    OMX_ERRORTYPE err = OSAL_AllocatorAllocMem(&pEnc->alloc, &capacity, &new.bus_data, &new.bus_address);
    if (err != OMX_ErrorNone)
    {
        TRACE_PRINT(pEnc->log, "ASYNC: frame buffer reallocation failed: %s\n", HantroOmx_str_omx_err(err));
        return err;
    }

    memcpy(new.bus_data, fb->bus_data, fb->size);
    new.capacity = capacity;
    new.size     = fb->size;
    OSAL_AllocatorFreeMem(&pEnc->alloc, fb->capacity, fb->bus_data, fb->bus_address);
    *fb = new;
    return OMX_ErrorNone;
}

/**
 * OMX_ERRRORTYPE async_start_stream( OMX_ENCODER* pEnc )
 */
static
OMX_ERRORTYPE async_start_stream( OMX_ENCODER* pEnc )
{
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    BUFFER*         outputBuffer        = NULL;
    STREAM_BUFFER   outputStream;

#ifdef USE_TEMP_OUTPUT_BUFFER
    FRAME_BUFFER*   tempBuffer = &pEnc->frame_out;
#endif

    OMX_ERRORTYPE err = OMX_ErrorNone;
#ifdef ANDROID_MOD
	int retry = 4000;
	while (outputBuffer == NULL && retry>0) {
		retry--;
		err = HantroOmx_port_lock_buffers(&pEnc->outputPort);
		if (err != OMX_ErrorNone)
			return err;
		HantroOmx_port_get_buffer(&pEnc->outputPort, &outputBuffer);
		HantroOmx_port_unlock_buffers(&pEnc->outputPort);
		if (outputBuffer == NULL)
			usleep(5000);
	}
#else
    err = HantroOmx_port_lock_buffers(&pEnc->outputPort);
    if (err != OMX_ErrorNone)
        return err;
    HantroOmx_port_get_buffer(&pEnc->outputPort, &outputBuffer);
    HantroOmx_port_unlock_buffers(&pEnc->outputPort);
#endif
    if (outputBuffer == NULL)
    {
        TRACE_PRINT(pEnc->log, "ASYNC: there's no output buffer!\n");
        return OMX_ErrorOverflow;
    }


    bzero( &outputStream, sizeof( STREAM_BUFFER ) );
#ifdef USE_TEMP_OUTPUT_BUFFER
    TRACE_PRINT(pEnc->log,"----> %s: tempBuffer->bus_address = 0x%x, tempBuffer->bus_data = 0x%x", __func__, tempBuffer->bus_address, tempBuffer->bus_data);
    outputStream.buf_max_size = tempBuffer->capacity;
    outputStream.bus_data = tempBuffer->bus_data;
    outputStream.bus_address = tempBuffer->bus_address;
#else
    outputStream.buf_max_size = outputBuffer->header->nAllocLen;
    outputStream.bus_data = outputBuffer->bus_data;
    outputStream.bus_address = outputBuffer->bus_address;
    LOGW("---> zero copy: max_size = %d, bus_data = 0x%x, bus_address = 0x%x", outputStream.buf_max_size, outputStream.bus_data, outputStream.bus_address);
#endif

    CODEC_STATE codecState = CODEC_ERROR_UNSPECIFIED;
    codecState = pEnc->codec->stream_start( pEnc->codec, &outputStream );
    if ( codecState != CODEC_OK )
    {
        return OMX_ErrorHardware;
    }

#ifdef USE_TEMP_OUTPUT_BUFFER
    memcpy( outputBuffer->header->pBuffer, outputStream.bus_data, outputStream.streamlen);
    outputBuffer->header->nOffset    = 0;
#endif

    outputBuffer->header->nFilledLen = outputStream.streamlen;
	outputBuffer->header->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;

    pEnc->streamStarted = OMX_TRUE;
    if (HantroOmx_port_is_tunneled(&pEnc->outputPort))
    {
        ((OMX_COMPONENTTYPE*)pEnc->outputPort.tunnelcomp)->EmptyThisBuffer(pEnc->outputPort.tunnelcomp, outputBuffer->header);
    }
    else
    {
        pEnc->app_callbacks.FillBufferDone(pEnc->self, pEnc->app_data, outputBuffer->header);
    }

#ifdef USE_TEMP_OUTPUT_BUFFER
    tempBuffer->offset = 0;
    tempBuffer->size = 0 ;
#endif

    HantroOmx_port_lock_buffers(&pEnc->outputPort);
    HantroOmx_port_pop_buffer(&pEnc->outputPort);
    HantroOmx_port_unlock_buffers(&pEnc->outputPort);

    return OMX_ErrorNone;
 }

 /**
 * OMX_ERRORTYPE async_end_stream( OMX_ENCODER* pEnc )
 */
static
OMX_ERRORTYPE async_end_stream( OMX_ENCODER* pEnc )
{
    TRACE_PRINT(pEnc->log, "API: %s\n", __FUNCTION__);
    BUFFER*         outputBuffer        = NULL;
    STREAM_BUFFER   outputStream;

#ifdef USE_TEMP_OUTPUT_BUFFER
    FRAME_BUFFER*   tempBuffer = &pEnc->frame_out;
#endif

    OMX_ERRORTYPE err = OMX_ErrorNone;
    err = HantroOmx_port_lock_buffers(&pEnc->outputPort);
    if (err != OMX_ErrorNone)
        return err;
    HantroOmx_port_get_buffer(&pEnc->outputPort, &outputBuffer);
    HantroOmx_port_unlock_buffers(&pEnc->outputPort);
    if (outputBuffer == NULL)
    {
        TRACE_PRINT(pEnc->log, "ASYNC: there's no output buffer!\n");
        return OMX_ErrorOverflow;
    }
    assert(outputBuffer->bus_data != NULL);
    assert(outputBuffer->bus_address != 0);
    assert(outputBuffer->header->pBuffer == outputBuffer->bus_data);

    bzero( &outputStream, sizeof( STREAM_BUFFER ) );
#ifdef USE_TEMP_OUTPUT_BUFFER
    outputStream.buf_max_size = tempBuffer->capacity;
    outputStream.bus_data = tempBuffer->bus_data;
    outputStream.bus_address = tempBuffer->bus_address;
#else
    outputStream.buf_max_size = outputBuffer->header->nAllocLen;
    outputStream.bus_data = outputBuffer->bus_data;
    outputStream.bus_address = outputBuffer->bus_address;
#endif

    CODEC_STATE codecState = CODEC_ERROR_UNSPECIFIED;
    codecState = pEnc->codec->stream_end( pEnc->codec, &outputStream );
    if ( codecState != CODEC_OK )
    {
        return OMX_ErrorHardware;
    }

#ifdef USE_TEMP_OUTPUT_BUFFER
    memcpy( outputBuffer->header->pBuffer, outputStream.bus_data, outputStream.streamlen);
#endif

    outputBuffer->header->nOffset    = 0;
    outputBuffer->header->nFilledLen = outputStream.streamlen;
    outputBuffer->header->nFlags |= OMX_BUFFERFLAG_EOS;

    pEnc->streamStarted = OMX_FALSE;
    if (HantroOmx_port_is_tunneled(&pEnc->outputPort))
    {
        ((OMX_COMPONENTTYPE*)pEnc->outputPort.tunnelcomp)->EmptyThisBuffer(pEnc->outputPort.tunnelcomp, outputBuffer->header);
    }
    else
    {
        pEnc->app_callbacks.FillBufferDone(pEnc->self, pEnc->app_data, outputBuffer->header);
    }

#ifdef USE_TEMP_OUTPUT_BUFFER
    tempBuffer->offset = 0;
    tempBuffer->size = 0 ;
#endif

    HantroOmx_port_lock_buffers(&pEnc->outputPort);
    HantroOmx_port_pop_buffer(&pEnc->outputPort);
    HantroOmx_port_unlock_buffers(&pEnc->outputPort);

    return OMX_ErrorNone;
 }



#ifdef OMX_ENCODER_VIDEO_DOMAIN
/**
 * OMX_ERRORTYPE async_video_encoder_encode(OMX_ENCODER* pEnc)
 * 1. Get next input buffer from buffer queue
 * 1.1 if stream is started call codec start stream
 * 2. Check that we have complete frame
 * 3. Make copy of inputbuffer if buffer is not allocated by us
 * 4. encode
 * 5 when EOS flag is received call stream end
 */
static
OMX_ERRORTYPE async_video_encoder_encode(OMX_ENCODER* pEnc)
{

    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    assert(pEnc->codec);
    BUFFER* inputBuffer         = NULL;
    OMX_ERRORTYPE err           = OMX_ErrorNone;
    OMX_U32 frameSize           = 0;

    err =  calculate_frame_size( pEnc, &frameSize);
    TRACE_PRINT( pEnc->log, "ASYNC: Calculated frameSize - %u ",(unsigned)frameSize );

    if ( err != OMX_ErrorNone )
        goto INVALID_STATE;

    if ( pEnc->streamStarted == OMX_FALSE )
    {
        err = async_start_stream( pEnc );
        if (err != OMX_ErrorNone)
        {
            if (err == OMX_ErrorOverflow)
            {
                TRACE_PRINT(pEnc->log, "ASYNC: firing OMX_EventErrorOverflow\n");
                pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventError, OMX_ErrorOverflow, 0, NULL);
                return OMX_ErrorNone;
            }
            else
                goto INVALID_STATE;
        }
    }

    err = HantroOmx_port_lock_buffers( &pEnc->inputPort );
    if (err != OMX_ErrorNone)
        goto INVALID_STATE;

    HantroOmx_port_get_buffer( &pEnc->inputPort, &inputBuffer );
    HantroOmx_port_unlock_buffers( &pEnc->inputPort );

    //For conformance tester
    if ( !inputBuffer )
        return OMX_ErrorNone;

    FRAME_BUFFER*   tempBuffer = &pEnc->frame_in;

	/* zero copy */

	tempBuffer->bus_address = *(unsigned int*)(inputBuffer->header->pBuffer + inputBuffer->header->nOffset);
	tempBuffer->bus_data = inputBuffer->header->pBuffer + inputBuffer->header->nOffset;

    // if there is previous data in the frame buffer left over from a previous call to encode
    // or if the buffer has possibly misaligned offset or if the buffer is allocated by the client
    // invoke the decoding through the temporary frame buffer
    if ( inputBuffer->header->nOffset != 0 ||
         tempBuffer->size != 0 ||
         !(inputBuffer->flags & BUFFER_FLAG_MY_BUFFER) ||
         pEnc->encConfig.stab.bStab ||
         inputBuffer->header->nOffset < frameSize  )
    {
        TRACE_PRINT( pEnc->log, "ASYNC: Using internal buffer\n");
        OMX_U32 capacity  = tempBuffer->capacity;
        OMX_U32 needed    = inputBuffer->header->nFilledLen;

        while (1)
        {
            OMX_U32 available = capacity - tempBuffer->size;

            if (available >= needed)
                break;
            capacity *= 2;
        }

        if (capacity >  tempBuffer->capacity)
        {
            err = grow_frame_buffer(pEnc, tempBuffer, capacity);
            if (err != OMX_ErrorNone )
                goto INVALID_STATE;
        }

        assert(tempBuffer->capacity - tempBuffer->size >= inputBuffer->header->nFilledLen);

//        OMX_U32 len = inputBuffer->header->nFilledLen;
        OMX_U32 len = frameSize;
//        OMX_U8* src = inputBuffer->header->pBuffer + inputBuffer->header->nOffset;
//        OMX_U8* dst = tempBuffer->bus_data + tempBuffer->size; // append to the buffer

//        memcpy(dst, src, len);
        tempBuffer->size += len;
        OMX_U32 retlen = 0;

        if ( pEnc->encConfig.stab.bStab )
        {
            if ( tempBuffer->size >= frameSize *2 )
            {
                err = async_encode_stabilized_data(pEnc, tempBuffer->bus_data, tempBuffer->bus_address, tempBuffer->size, inputBuffer, &retlen, frameSize);
                if (err != OMX_ErrorNone)
                {
                    if (err == OMX_ErrorOverflow)
                    {
                        TRACE_PRINT(pEnc->log, "ASYNC: firing OMX_EventErrorOverflow\n");
                        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventError, OMX_ErrorOverflow, 0, NULL);
                    }
                    else if (err == OMX_ErrorBadParameter)
                    {
                        TRACE_PRINT(pEnc->log, "ASYNC: firing OMX_ErrorBadParameter\n");
                        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventError, OMX_ErrorBadParameter, 0, NULL);
                    }
                    else
                        goto INVALID_STATE;
                }
                tempBuffer->size = retlen;
            }
        }
        else if( tempBuffer->size >= frameSize )
        {
           TRACE_PRINT(pEnc->log, "tempBuffer->bus_data = 0x%x, tempBuffer->bus_address = 0x%x, tempBuffer->size = %d, frameSize = %d\n",
					tempBuffer->bus_data, tempBuffer->bus_address, tempBuffer->size, frameSize);

            err = async_encode_video_data(pEnc, tempBuffer->bus_data, tempBuffer->bus_address, tempBuffer->size, inputBuffer, &retlen, frameSize);
            if (err != OMX_ErrorNone)
            {
                if (err == OMX_ErrorOverflow)
                {
                    TRACE_PRINT(pEnc->log, "ASYNC: firing OMX_EventErrorOverflow\n");
                    pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventError, OMX_ErrorOverflow, 0, NULL);
                }
                else if (err == OMX_ErrorBadParameter)
                {
                    TRACE_PRINT(pEnc->log, "ASYNC: firing OMX_ErrorBadParameter\n");
                    pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventError, OMX_ErrorBadParameter, 0, NULL);
                }
                else
                    goto INVALID_STATE;
            }
            tempBuffer->size = retlen;
        }
    }
    else
    {
        TRACE_PRINT( pEnc->log, "ASYNC: No internal buffer\n");
        assert(inputBuffer->header->nOffset == 0);
        assert(inputBuffer->bus_data == inputBuffer->header->pBuffer);
        assert(inputBuffer->flags & BUFFER_FLAG_MY_BUFFER);

        OMX_U8* bus_data = inputBuffer->bus_data;
        OSAL_BUS_WIDTH bus_address = inputBuffer->bus_address;
        OMX_U32 filledlen = inputBuffer->header->nFilledLen;
        OMX_U32 retlen    = 0;

        if( filledlen >= frameSize )
            err = async_encode_video_data( pEnc, bus_data, bus_address, filledlen, inputBuffer, &retlen, frameSize);
        if (err != OMX_ErrorNone)
            goto INVALID_STATE;

        if (retlen > 0)
        {
            OMX_U8* dst = tempBuffer->bus_data + tempBuffer->size;
            OMX_U8* src = bus_data;
            OMX_U32 capacity = tempBuffer->capacity;
            OMX_U32 needed   = retlen;
            while (1)
            {
                OMX_U32 available = capacity - tempBuffer->size;
                if (available > needed)
                    break;
                capacity *= 2;
            }
            if (capacity != tempBuffer->capacity)
            {
                err = grow_frame_buffer(pEnc, tempBuffer, capacity);
                if (err != OMX_ErrorNone)
                {
                    if (err == OMX_ErrorOverflow)
                    {
                        TRACE_PRINT(pEnc->log, "ASYNC: firing OMX_EventErrorOverflow\n");
                        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventError, OMX_ErrorOverflow, 0, NULL);
                    }
                    else
                        goto INVALID_STATE;
                }
            }

            memcpy(dst, src, retlen);
            tempBuffer->size += retlen;
        }
    }

    if (inputBuffer->header->nFlags & OMX_BUFFERFLAG_EOS)
    {
        TRACE_PRINT(pEnc->log, "ASYNC: got buffer EOS flag\n");
        err = async_end_stream( pEnc );
        if ( err != OMX_ErrorNone )
            return err;
        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventBufferFlag, PORT_INDEX_OUTPUT, inputBuffer->header->nFlags, NULL);
    }

    if (inputBuffer->header->hMarkTargetComponent == pEnc->self)
    {
        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventMark, 0, 0, inputBuffer->header->pMarkData);
        inputBuffer->header->hMarkTargetComponent = NULL;
        inputBuffer->header->pMarkData            = NULL;
    }

    if (HantroOmx_port_is_tunneled(&pEnc->inputPort))
    {
        ((OMX_COMPONENTTYPE*)pEnc->inputPort.tunnelcomp)->FillThisBuffer(pEnc->inputPort.tunnelcomp, inputBuffer->header);
    }
    else
    {
        pEnc->app_callbacks.EmptyBufferDone(pEnc->self, pEnc->app_data, inputBuffer->header);
    }


    HantroOmx_port_lock_buffers(&pEnc->inputPort);
    HantroOmx_port_pop_buffer(&pEnc->inputPort);
    HantroOmx_port_unlock_buffers(&pEnc->inputPort);

    return OMX_ErrorNone;

 INVALID_STATE:
    assert(err != OMX_ErrorNone);

    TRACE_PRINT(pEnc->log, "ASYNC: error while processing buffers: %s\n", HantroOmx_str_omx_err(err));

    pEnc->state = OMX_StateInvalid;
    pEnc->run   = OMX_FALSE;
    pEnc->app_callbacks.EventHandler(
        pEnc->self,
        pEnc->app_data,
        OMX_EventError,
        err,
        0,
        NULL);
    return err;
}

/**
 * static OMX_ERRORTYPE async_encode_stabilized_data(OMX_ENCODER* pEnc, OMX_U8* bus_data, OSAL_BUS_WIDTH bus_address,
                                OMX_U32 datalen, BUFFER* buff, OMX_U32* retlen, OMX_U32 frameSize )
 */
static
OMX_ERRORTYPE async_encode_stabilized_data(OMX_ENCODER* pEnc, OMX_U8* bus_data, OSAL_BUS_WIDTH bus_address,
                                OMX_U32 datalen, BUFFER* inputBuffer, OMX_U32* retlen, OMX_U32 frameSize )
{
    assert(pEnc);
    assert(retlen);
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    FRAME           frame;          //Frame for codec
    STREAM_BUFFER   stream;         //Encoder stream from codec
    BUFFER*         outputBuffer    = NULL;         //Output buffer from client
    OMX_ERRORTYPE   err             = OMX_ErrorNone;
    OMX_U32         i, dataSize;

#ifdef USE_TEMP_OUTPUT_BUFFER
    FRAME_BUFFER*   tempBuffer      = &pEnc->frame_out;
#endif

    while (datalen >= frameSize*2 )
    {

        err = HantroOmx_port_lock_buffers(&pEnc->outputPort);
        if (err != OMX_ErrorNone)
            return err;
        HantroOmx_port_get_buffer(&pEnc->outputPort, &outputBuffer);
        HantroOmx_port_unlock_buffers(&pEnc->outputPort);
        if (outputBuffer == NULL)
        {
            TRACE_PRINT(pEnc->log, "ASYNC: there's no output buffer!\n");
            return OMX_ErrorOverflow;
        }

        assert(outputBuffer->bus_data != NULL);
        assert(outputBuffer->bus_address != 0);

#ifdef USE_TEMP_OUTPUT_BUFFER
        if ( tempBuffer->capacity < outputBuffer->header->nAllocLen )
        {
            err = grow_frame_buffer( pEnc, tempBuffer, outputBuffer->header->nAllocLen);
        }

        if (err != OMX_ErrorNone )
            return err;
#endif

        frame.fb_bus_data    = bus_data;
        frame.fb_bus_address = bus_address;
        frame.fb_frameSize = frameSize;
        frame.fb_bufferSize = 2* frame.fb_frameSize;
        frame.bitrate = pEnc->outputPort.def.format.video.nBitrate;

        switch ( pEnc->outputPort.def.format.video.eCompressionFormat )
        {
            case OMX_VIDEO_CodingAVC:
                {
                    TRACE_PRINT( pEnc->log, "ASYNC: pEnc->encConfig.avc.nPFrames - %d\n", (unsigned) pEnc->encConfig.avc.nPFrames);
                    if ( pEnc->encConfig.avc.nPFrames == 0 )
                        frame.frame_type = INTRA_FRAME;
                    else
                        frame.frame_type = PREDICTED_FRAME;
                }
                break;
            case OMX_VIDEO_CodingMPEG4:
                {
                    TRACE_PRINT( pEnc->log, "ASYNC: pEnc->encConfig.mpeg4.nPFrames - %d\n", (unsigned) pEnc->encConfig.mpeg4.nPFrames);
                    if ( pEnc->encConfig.mpeg4.nPFrames == 0)
                        frame.frame_type = INTRA_FRAME;
                    else
                    frame.frame_type = PREDICTED_FRAME;
                }
                break;
            case OMX_VIDEO_CodingH263:
                {
                    frame.frame_type = PREDICTED_FRAME;
                }
                break;
#ifdef ENCH1
            case OMX_VIDEO_CodingVP8:
                {
                    frame.frame_type = PREDICTED_FRAME;

                    frame.bPrevRefresh = pEnc->encConfig.vp8Ref.bPreviousFrameRefresh;
                    frame.bGoldenRefresh = pEnc->encConfig.vp8Ref.bGoldenFrameRefresh;
                    frame.bAltRefresh = pEnc->encConfig.vp8Ref.bAlternateFrameRefresh;
                    frame.bUsePrev = pEnc->encConfig.vp8Ref.bUsePreviousFrame;
                    frame.bUseGolden = pEnc->encConfig.vp8Ref.bUseGoldenFrame;
                    frame.bUseAlt = pEnc->encConfig.vp8Ref.bUseAlternateFrame;
                }
                break;
#endif
            default:
                TRACE_PRINT( pEnc->log, "Illegal compression format..using 0 at nPFrames\n");
            break;
        }

        if ( pEnc->encConfig.intraRefresh.IntraRefreshVOP == OMX_TRUE)
        {
            frame.frame_type = INTRA_FRAME;
            pEnc->encConfig.intraRefresh.IntraRefreshVOP = OMX_FALSE;
        }

        bzero( &stream, sizeof( STREAM_BUFFER ) );
#ifdef USE_TEMP_OUTPUT_BUFFER
        stream.buf_max_size = tempBuffer->capacity;
        stream.bus_data = tempBuffer->bus_data;
        stream.bus_address = tempBuffer->bus_address;
#else
        stream.buf_max_size = outputBuffer->header->nAllocLen;
        stream.bus_data = outputBuffer->bus_data;
        stream.bus_address = outputBuffer->bus_address;
#endif
        CODEC_STATE codecState = CODEC_ERROR_UNSPECIFIED;

        codecState = pEnc->codec->encode(pEnc->codec,&frame, &stream);
        if ( codecState < 0 )
        {
            if (codecState == CODEC_ERROR_BUFFER_OVERFLOW)
            {
                LOGE("pEnc->codec->encode() returned CODEC_ERROR_BUFFER_OVERFLOW!");
                return OMX_ErrorOverflow;
            }
            else if (codecState == CODEC_ERROR_INVALID_ARGUMENT)
            {
                LOGE("pEnc->codec->encode() returned CODEC_ERROR_INVALID_ARGUMENT!");
                return OMX_ErrorBadParameter;
            }
            else
            {
                LOGE("pEnc->codec->encode() returned undefned error!");
                return OMX_ErrorUndefined;
            }
        }

        //Move frame start bit to the beginning of the frame buffer
        OMX_U32 rem = datalen - frame.fb_frameSize;

        memmove(bus_data, bus_data + frame.fb_frameSize, rem );
        datalen -=  frame.fb_frameSize;

        TRACE_PRINT(pEnc->log, "ASYNC: output buffer size: %d\n", (int)outputBuffer->header->nAllocLen);
        if (stream.streamlen > outputBuffer->header->nAllocLen)
        {
            TRACE_PRINT(pEnc->log, "ASYNC: output buffer is too small!!\n");
            return OMX_ErrorOverflow;
        }

#ifdef USE_TEMP_OUTPUT_BUFFER
        if(pEnc->outputPort.def.format.video.eCompressionFormat == OMX_VIDEO_CodingVP8)
        {
            dataSize = 0;
            // copy data from each partition to the output buffer
            for(i=0;i<9;i++)
            {
                if (stream.streamSize[i] <= outputBuffer->header->nAllocLen &&
                    dataSize <= outputBuffer->header->nAllocLen)
                    memcpy( outputBuffer->header->pBuffer + dataSize, stream.pOutBuf[i], stream.streamSize[i]);
                else
                    return OMX_ErrorOverflow;

                dataSize += stream.streamSize[i];
            }
        }
        else
            memcpy( outputBuffer->header->pBuffer, stream.bus_data, stream.streamlen);
#endif

        outputBuffer->header->nOffset    = 0;
        outputBuffer->header->nFilledLen = stream.streamlen;
        outputBuffer->header->nTimeStamp = inputBuffer->header->nTimeStamp;
        outputBuffer->header->nFlags     = inputBuffer->header->nFlags & ~OMX_BUFFERFLAG_EOS;
        TRACE_PRINT(pEnc->log, "ASYNC: output buffer OK\n");

        int markcount = pEnc->mark_write_pos - pEnc->mark_read_pos;
        if (markcount)
        {
            TRACE_PRINT(pEnc->log, "ASYNC: got %d marks pending\n", markcount);
            outputBuffer->header->hMarkTargetComponent = pEnc->marks[pEnc->mark_read_pos].hMarkTargetComponent;
            outputBuffer->header->pMarkData            = pEnc->marks[pEnc->mark_read_pos].pMarkData;
            pEnc->mark_read_pos++;
            if (--markcount == 0)
            {
                pEnc->mark_read_pos  = 0;
                pEnc->mark_write_pos = 0;
                TRACE_PRINT(pEnc->log, "ASYNC: mark buffer empty\n");
            }
        }
        else
        {
            outputBuffer->header->hMarkTargetComponent = inputBuffer->header->hMarkTargetComponent;
            outputBuffer->header->pMarkData            = inputBuffer->header->pMarkData;
        }

        HantroOmx_port_lock_buffers(&pEnc->outputPort);
        HantroOmx_port_pop_buffer(&pEnc->outputPort);
        HantroOmx_port_unlock_buffers(&pEnc->outputPort);

        if (HantroOmx_port_is_tunneled(&pEnc->outputPort))
        {
            ((OMX_COMPONENTTYPE*)pEnc->outputPort.tunnelcomp)->EmptyThisBuffer(pEnc->outputPort.tunnelcomp, outputBuffer->header);
        }
        else
        {
            pEnc->app_callbacks.FillBufferDone(pEnc->self, pEnc->app_data, outputBuffer->header);
        }
    }

    *retlen = datalen;
    return OMX_ErrorNone;
}

/**
 * OMX_ERRORTYPE async_encode_video_data(OMX_ENCODER* pEnc,
 *                                  OMX_U8* bus_data,
 *                                  OSAL_BUS_WIDTH bus_address,
 *                                  OMX_U32 datalen, BUFFER* buff,
                                    OMX_U32* retlen)
 */
static
OMX_ERRORTYPE async_encode_video_data(OMX_ENCODER* pEnc, OMX_U8* bus_data, OSAL_BUS_WIDTH bus_address,
                                OMX_U32 datalen, BUFFER* inputBuffer, OMX_U32* retlen, OMX_U32 frameSize )
{
    assert(pEnc);
    assert(retlen);
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    FRAME           frame;       //Frame for codec
    STREAM_BUFFER   stream;      //Encoder stream from codec
    BUFFER*         outputBuffer        = NULL;         //Output buffer from client
    OMX_ERRORTYPE   err                 = OMX_ErrorNone;
    OMX_U32         i, dataSize;

#ifdef USE_TEMP_OUTPUT_BUFFER
    FRAME_BUFFER*   tempBuffer          = &pEnc->frame_out;
#endif

    while (datalen >= frameSize )
    {
#ifdef ANDROID_MOD
		int retry = 4000;
		while (outputBuffer == NULL && retry > 0) {
			//check for transition to idle state, exit without error
			if( pEnc->statetrans == OMX_StateIdle){
	            TRACE_PRINT(pEnc->log,"%s: transition to idle, exit outputBuffer loop",__FUNCTION__);
				return	OMX_ErrorNone;
			}
			retry--;
			err = HantroOmx_port_lock_buffers(&pEnc->outputPort);
			if (err != OMX_ErrorNone)
				return err;
			HantroOmx_port_get_buffer(&pEnc->outputPort, &outputBuffer);
			HantroOmx_port_unlock_buffers(&pEnc->outputPort);
			if (outputBuffer == NULL)
				usleep(5000);
		}
#else
        err = HantroOmx_port_lock_buffers(&pEnc->outputPort);
        if (err != OMX_ErrorNone)
            return err;
        HantroOmx_port_get_buffer(&pEnc->outputPort, &outputBuffer);
        HantroOmx_port_unlock_buffers(&pEnc->outputPort);
#endif
        if (outputBuffer == NULL)
        {
            TRACE_PRINT(pEnc->log, "ASYNC: there's no output buffer!\n");
            return OMX_ErrorOverflow;
        }

#ifdef USE_TEMP_OUTPUT_BUFFER
        if ( tempBuffer->capacity < outputBuffer->header->nAllocLen )
        {
            err = grow_frame_buffer( pEnc, tempBuffer, outputBuffer->header->nAllocLen);
        }
        if (err != OMX_ErrorNone )
            return err;
#endif

        frame.fb_bus_data    = bus_data; // + offset
        frame.fb_bus_address = bus_address; // + offset
        frame.fb_frameSize = frameSize;
        frame.fb_bufferSize = frame.fb_frameSize;
        frame.bitrate = pEnc->outputPort.def.format.video.nBitrate;

        switch ( pEnc->outputPort.def.format.video.eCompressionFormat )
        {
            case OMX_VIDEO_CodingAVC:
                {
                    TRACE_PRINT( pEnc->log, "ASYNC: pEnc->encConfig.avc.nPFrames - %u\n", (unsigned)pEnc->encConfig.avc.nPFrames);
#ifdef ALWAYS_CODE_INTRA_FRAMES
#warning "!!! Always code intra frames !!!"
					frame.frame_type = INTRA_FRAME;
#else
                    if ( pEnc->encConfig.avc.nPFrames == 0 )
                        frame.frame_type = INTRA_FRAME;
                    else
                        frame.frame_type = PREDICTED_FRAME;
#endif
                }
                break;
            case OMX_VIDEO_CodingMPEG4:
                {
                    TRACE_PRINT( pEnc->log, "ASYNC: pEnc->encConfig.mpeg4.nPFrames - %u\n", (unsigned) pEnc->encConfig.mpeg4.nPFrames);
                    if ( pEnc->encConfig.mpeg4.nPFrames == 0)
                        frame.frame_type = INTRA_FRAME;
                    else
                    frame.frame_type = PREDICTED_FRAME;
                }
                break;
            case OMX_VIDEO_CodingH263:
                {
                    frame.frame_type = PREDICTED_FRAME;
                }
                break;
#ifdef ENCH1
            case OMX_VIDEO_CodingVP8:
                {
                    frame.frame_type = PREDICTED_FRAME;

                    frame.bPrevRefresh = pEnc->encConfig.vp8Ref.bPreviousFrameRefresh;
                    frame.bGoldenRefresh = pEnc->encConfig.vp8Ref.bGoldenFrameRefresh;
                    frame.bAltRefresh = pEnc->encConfig.vp8Ref.bAlternateFrameRefresh;
                    frame.bUsePrev = pEnc->encConfig.vp8Ref.bUsePreviousFrame;
                    frame.bUseGolden = pEnc->encConfig.vp8Ref.bUseGoldenFrame;
                    frame.bUseAlt = pEnc->encConfig.vp8Ref.bUseAlternateFrame;
                }
                break;
#endif
            default:
                TRACE_PRINT( pEnc->log, "Illegal compression format..using 0 at nPFrames\n");
            break;
        }

        if ( pEnc->encConfig.intraRefresh.IntraRefreshVOP == OMX_TRUE)
        {
            frame.frame_type = INTRA_FRAME;
            pEnc->encConfig.intraRefresh.IntraRefreshVOP = OMX_FALSE;
        }

        bzero( &stream, sizeof( STREAM_BUFFER ) );
#ifdef USE_TEMP_OUTPUT_BUFFER
        stream.buf_max_size = tempBuffer->capacity;
        stream.bus_data = tempBuffer->bus_data;
        stream.bus_address = tempBuffer->bus_address;
#else 
        stream.buf_max_size = outputBuffer->header->nAllocLen;
        stream.bus_data = outputBuffer->bus_data;
        stream.bus_address = outputBuffer->bus_address;
#endif
        CODEC_STATE codecState = CODEC_ERROR_UNSPECIFIED;

        codecState = pEnc->codec->encode(pEnc->codec, &frame, &stream);

        if ( codecState < 0 )
        {
            TRACE_PRINT( pEnc->log, "ASYNC: codecState - %d\n", codecState);
            if (codecState == CODEC_ERROR_BUFFER_OVERFLOW)
            {
                LOGE("pEnc->codec->encode returned CODEC_ERROR_BUFFER_OVERFLOW!");
                return OMX_ErrorOverflow;
            }
            else if (codecState == CODEC_ERROR_INVALID_ARGUMENT)
            {
                LOGE("pEnc->codec->encode returned CODEC_ERROR_INVALID_ARGUMENT!");
                return OMX_ErrorBadParameter;
            }
            else
            {
                LOGE("pEnc->codec->encode returned undefined error!");
                return OMX_ErrorUndefined;
            }
        }
        TRACE_PRINT( pEnc->log, "ASYNC: codecState - %u\n", (unsigned)codecState);


        OMX_U32 rem = datalen - frame.fb_frameSize;

        TRACE_PRINT( pEnc->log, "ASYNC: rem - %u\n", (unsigned)rem);

        memmove(bus_data, bus_data + frame.fb_frameSize, rem );
        datalen -=  frame.fb_frameSize;

        TRACE_PRINT( pEnc->log, "ASYNC: v - %u\n", (unsigned)datalen);
        TRACE_PRINT(pEnc->log, "ASYNC: output buffer size: %d\n", (int)outputBuffer->header->nAllocLen);
        if (stream.streamlen > outputBuffer->header->nAllocLen)
        {
            TRACE_PRINT(pEnc->log, "ASYNC: output buffer is too small!\n");
            return OMX_ErrorOverflow;
        }

#ifdef USE_TEMP_OUTPUT_BUFFER
        if(pEnc->outputPort.def.format.video.eCompressionFormat == OMX_VIDEO_CodingVP8)
        {
            dataSize = 0;
            // copy data from each partition to the output buffer
            for(i=0;i<9;i++)
            {
                if (stream.streamSize[i] <= outputBuffer->header->nAllocLen &&
                    dataSize <= outputBuffer->header->nAllocLen)
                    memcpy( outputBuffer->header->pBuffer + dataSize, stream.pOutBuf[i], stream.streamSize[i]);
                else
                    return OMX_ErrorOverflow;

                dataSize += stream.streamSize[i];
            }
        }
        else
            memcpy( outputBuffer->header->pBuffer, stream.bus_data, stream.streamlen);
#endif

        outputBuffer->header->nOffset    = 0;
        outputBuffer->header->nFilledLen = stream.streamlen;
        outputBuffer->header->nTimeStamp = inputBuffer->header->nTimeStamp;
        outputBuffer->header->nFlags     = inputBuffer->header->nFlags & ~OMX_BUFFERFLAG_EOS;

#ifdef ANDROID_MOD
		// mark intra frame as sync frame
		if (codecState == CODEC_CODED_INTRA) {
			outputBuffer->header->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
		}
#endif

        TRACE_PRINT(pEnc->log, "ASYNC: output buffer OK\n");

        int markcount = pEnc->mark_write_pos - pEnc->mark_read_pos;
        if (markcount)
        {
            TRACE_PRINT(pEnc->log, "ASYNC: got %d marks pending\n", markcount);
            outputBuffer->header->hMarkTargetComponent = pEnc->marks[pEnc->mark_read_pos].hMarkTargetComponent;
            outputBuffer->header->pMarkData            = pEnc->marks[pEnc->mark_read_pos].pMarkData;
            pEnc->mark_read_pos++;
            if (--markcount == 0)
            {
                pEnc->mark_read_pos  = 0;
                pEnc->mark_write_pos = 0;
                TRACE_PRINT(pEnc->log, "ASYNC: mark buffer empty\n");
            }
        }
        else
        {
            outputBuffer->header->hMarkTargetComponent = inputBuffer->header->hMarkTargetComponent;
            outputBuffer->header->pMarkData            = inputBuffer->header->pMarkData;
        }

        HantroOmx_port_lock_buffers(&pEnc->outputPort);
        HantroOmx_port_pop_buffer(&pEnc->outputPort);
        HantroOmx_port_unlock_buffers(&pEnc->outputPort);

        if (HantroOmx_port_is_tunneled(&pEnc->outputPort))
        {
            ((OMX_COMPONENTTYPE*)pEnc->outputPort.tunnelcomp)->EmptyThisBuffer(pEnc->outputPort.tunnelcomp, outputBuffer->header);
        }
        else
        {
            pEnc->app_callbacks.FillBufferDone(pEnc->self, pEnc->app_data, outputBuffer->header);
        }
    }
    *retlen = datalen;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE set_avc_intra_period(OMX_ENCODER* pEnc)
{
    assert(pEnc);
    TRACE_PRINT(pEnc->log, "%s\n", __FUNCTION__);

    HantroHwEncOmx_encoder_intra_period_h264(pEnc->codec, pEnc->encConfig.avcIdr.nPFrames);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE set_frame_rate(OMX_ENCODER* pEnc)
{
    assert(pEnc);
    TRACE_PRINT(pEnc->log, "%s\n", __FUNCTION__);

    switch ( pEnc->outputPort.def.format.video.eCompressionFormat )
    {
        case OMX_VIDEO_CodingAVC:
            HantroHwEncOmx_encoder_frame_rate_h264(pEnc->codec,
                        pEnc->outputPort.def.format.video.xFramerate);
            break;
#if !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
        case OMX_VIDEO_CodingH263:
            HantroHwEncOmx_encoder_frame_rate_h263(pEnc->codec,
                        pEnc->outputPort.def.format.video.xFramerate);
            break;
        case OMX_VIDEO_CodingMPEG4:
            HantroHwEncOmx_encoder_frame_rate_mpeg4(pEnc->codec,
                        pEnc->outputPort.def.format.video.xFramerate);
            break;
#endif
#ifdef ENCH1
        case OMX_VIDEO_CodingVP8:
            HantroHwEncOmx_encoder_frame_rate_vp8(pEnc->codec,
                        pEnc->outputPort.def.format.video.xFramerate);
            break;
#endif
        default:
            return OMX_ErrorUnsupportedIndex;
    }

    return OMX_ErrorNone;
}

#endif //~OMX_ENCODER_VIDEO_DOMAIN

#ifdef OMX_ENCODER_IMAGE_DOMAIN
/**
 * OMX_ERRORTYPE async_image_encoder_encode(OMX_ENCODER* pEnc)
 * 1. Get next input buffer from buffer queue
 * 1.1 if stream is started call codec start stream
 * 2. Check that we have complete frame
 * 3. Make copy of inputbuffer if buffer is not allocated by us
 * 4. encode
 * 5 when EOS flag is received call stream end
 */
static
OMX_ERRORTYPE async_image_encoder_encode(OMX_ENCODER* pEnc)
{

    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);

    assert(pEnc->codec);
    BUFFER* inputBuffer         = NULL;
    OMX_ERRORTYPE err           = OMX_ErrorNone;
    OMX_U32 frameSize           = 0;

    err = calculate_frame_size( pEnc, &frameSize );
    TRACE_PRINT( pEnc->log, "ASYNC:  calculated frameSize - %u\n", (unsigned)frameSize );

    if ( err != OMX_ErrorNone )
        goto INVALID_STATE;
    if ( pEnc->streamStarted == OMX_FALSE )
    {
        err = async_start_stream( pEnc );
        if (err != OMX_ErrorNone)
        {
            if (err == OMX_ErrorOverflow)
            {
                TRACE_PRINT(pEnc->log, "ASYNC: firing OMX_EventErrorOverflow\n");
                pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventError, OMX_ErrorOverflow, 0, NULL);
                return OMX_ErrorNone;
            }
            else
                goto INVALID_STATE;
        }
    }

    err = HantroOmx_port_lock_buffers( &pEnc->inputPort );
    if (err != OMX_ErrorNone)
        goto INVALID_STATE;
    HantroOmx_port_get_buffer( &pEnc->inputPort, &inputBuffer );
    HantroOmx_port_unlock_buffers( &pEnc->inputPort );

      //For conformance tester
    if ( !inputBuffer )
        return OMX_ErrorNone;
    FRAME_BUFFER*   tempBuffer = &pEnc->frame_in;

    // if there is previous data in the frame buffer left over from a previous call to encode
    // or if the buffer has possibly misaligned offset or if the buffer is allocated by the client
    // invoke the decoding through the temporary frame buffer
    if ( inputBuffer->header->nOffset != 0 || tempBuffer->size != 0 ||
        !(inputBuffer->flags & BUFFER_FLAG_MY_BUFFER) ||
        inputBuffer->header->nOffset < frameSize )
    {
        OMX_U32 capacity  = tempBuffer->capacity;
        OMX_U32 needed    = inputBuffer->header->nFilledLen;
        while (1)
        {
            OMX_U32 available = capacity - tempBuffer->size;

            if (available >= needed)
                break;
            capacity *= 2;
        }
        if (capacity >  tempBuffer->capacity)
        {
            err = grow_frame_buffer(pEnc, tempBuffer, capacity);
            if (err != OMX_ErrorNone )
                goto INVALID_STATE;
        }

        assert(tempBuffer->capacity - tempBuffer->size >= inputBuffer->header->nFilledLen);

        OMX_U32 len = inputBuffer->header->nFilledLen;
        OMX_U8* src = inputBuffer->header->pBuffer + inputBuffer->header->nOffset;
        OMX_U8* dst = tempBuffer->bus_data + tempBuffer->size; // append to the buffer

        memcpy(dst, src, len);
        tempBuffer->size += len;

        OMX_U32 retlen = 0;
        if( tempBuffer->size >= frameSize )
        {
            err = async_encode_image_data(pEnc, tempBuffer->bus_data, tempBuffer->bus_address, tempBuffer->size, inputBuffer, &retlen, frameSize);
            if (err != OMX_ErrorNone)
                goto INVALID_STATE;
            tempBuffer->size = retlen;
        }
    }
    else
    {
        assert(inputBuffer->header->nOffset == 0);
        assert(inputBuffer->bus_data == inputBuffer->header->pBuffer);
        assert(inputBuffer->flags & BUFFER_FLAG_MY_BUFFER);

        OMX_U8* bus_data = inputBuffer->bus_data;
        OSAL_BUS_WIDTH bus_address = inputBuffer->bus_address;
        OMX_U32 filledlen = inputBuffer->header->nFilledLen;
        OMX_U32 retlen    = 0;

        // encode if we have frames
        //or slices in slice mode
        if( filledlen >= frameSize )
            err = async_encode_image_data( pEnc, bus_data, bus_address, filledlen, inputBuffer, &retlen, frameSize);
        if (err != OMX_ErrorNone)
            goto INVALID_STATE;

        // Chech do we have encoded something and is leftover bits
        // Store leftovers to internal temp buffer
        if (retlen > 0)
        {
            OMX_U8* dst = tempBuffer->bus_data + tempBuffer->size;
            OMX_U8* src = bus_data;
            OMX_U32 capacity = tempBuffer->capacity;
            OMX_U32 needed   = retlen;
            while (1)
            {
                OMX_U32 available = capacity - tempBuffer->size;
                if (available > needed)
                    break;
                capacity *= 2;
            }
            if (capacity != tempBuffer->capacity)
            {
                err = grow_frame_buffer(pEnc, tempBuffer, capacity);
                if (err != OMX_ErrorNone)
                    goto INVALID_STATE;
            }

            memcpy(dst, src, retlen);
            tempBuffer->size += retlen;
        }
    }

    if (inputBuffer->header->nFlags & OMX_BUFFERFLAG_EOS)
    {
        TRACE_PRINT(pEnc->log, "ASYNC: got buffer EOS flag\n");
        err = async_end_stream( pEnc );
        if ( err != OMX_ErrorNone )
            return err;

        pEnc->app_callbacks.EventHandler(
            pEnc->self,
            pEnc->app_data,
            OMX_EventBufferFlag,
            PORT_INDEX_OUTPUT,
            inputBuffer->header->nFlags,
            NULL);
    }

    if (inputBuffer->header->hMarkTargetComponent == pEnc->self)
    {
        pEnc->app_callbacks.EventHandler(pEnc->self, pEnc->app_data, OMX_EventMark, 0, 0, inputBuffer->header->pMarkData);
        inputBuffer->header->hMarkTargetComponent = NULL;
        inputBuffer->header->pMarkData            = NULL;
    }

    if (HantroOmx_port_is_tunneled(&pEnc->inputPort))
    {
        ((OMX_COMPONENTTYPE*)pEnc->inputPort.tunnelcomp)->FillThisBuffer(pEnc->inputPort.tunnelcomp, inputBuffer->header);
    }
    else
    {
        pEnc->app_callbacks.EmptyBufferDone(pEnc->self, pEnc->app_data, inputBuffer->header);
    }

    HantroOmx_port_lock_buffers(&pEnc->inputPort);
    HantroOmx_port_pop_buffer(&pEnc->inputPort);
    HantroOmx_port_unlock_buffers(&pEnc->inputPort);
    return OMX_ErrorNone;

 INVALID_STATE:
    assert(err != OMX_ErrorNone);

    TRACE_PRINT(pEnc->log, "ASYNC: error while processing buffers: %s\n", HantroOmx_str_omx_err(err));

    pEnc->state = OMX_StateInvalid;
    pEnc->run   = OMX_FALSE;
    pEnc->app_callbacks.EventHandler(
        pEnc->self,
        pEnc->app_data,
        OMX_EventError,
        err,
        0,
        NULL);
    return err;
}

/**
 * OMX_ERRORTYPE async_encode_image_data(OMX_ENCODER* pEnc,
 *                                  OMX_U8* bus_data,
 *                                  OSAL_BUS_WIDTH bus_address,
 *                                  OMX_U32 datalen, BUFFER* buff,
                                    OMX_U32* retlen)
 */
static
OMX_ERRORTYPE async_encode_image_data(OMX_ENCODER* pEnc, OMX_U8* bus_data, OSAL_BUS_WIDTH bus_address,
                                OMX_U32 datalen, BUFFER* inputBuffer, OMX_U32* retlen, OMX_U32 frameSize )
{
    assert(pEnc);
    assert(retlen);
    TRACE_PRINT(pEnc->log, "ASYNC: %s\n", __FUNCTION__);
    TRACE_PRINT(pEnc->log, "ASYNC: datalen %u\n",(unsigned)datalen);

    FRAME           frame;       //Frame for codec
    STREAM_BUFFER   stream;      //Encoder stream from codec
    BUFFER*         outputBuffer        = NULL;         //Output buffer from client
    FRAME_BUFFER*   tempBuffer          = &pEnc->frame_out;
    OMX_ERRORTYPE   err                 = OMX_ErrorNone;
    OMX_U32         i, dataSize;

    while ( datalen >= frameSize )
    {

        err = HantroOmx_port_lock_buffers(&pEnc->outputPort);
        if (err != OMX_ErrorNone)
            return err;
        HantroOmx_port_get_buffer(&pEnc->outputPort, &outputBuffer);
        HantroOmx_port_unlock_buffers(&pEnc->outputPort);
        if (outputBuffer == NULL)
        {
            TRACE_PRINT(pEnc->log, "ASYNC: there's no output buffer!\n");
            return OMX_ErrorOverflow;
        }

        if ( tempBuffer->capacity < outputBuffer->header->nAllocLen )
        {
            err = grow_frame_buffer( pEnc, tempBuffer, outputBuffer->header->nAllocLen);
        }
        if (err != OMX_ErrorNone )
            return err;


        frame.fb_bus_data    = bus_data; // + offset
        frame.fb_bus_address = bus_address; // + offset

        frame.fb_frameSize = frameSize;
        frame.fb_bufferSize = frame.fb_frameSize;

        bzero( &stream, sizeof( STREAM_BUFFER ) );
        stream.buf_max_size = tempBuffer->capacity;
        stream.bus_data = tempBuffer->bus_data;
        stream.bus_address = tempBuffer->bus_address;
        CODEC_STATE codecState = CODEC_ERROR_UNSPECIFIED;

        codecState = pEnc->codec->encode(pEnc->codec,&frame, &stream);
        TRACE_PRINT( pEnc->log, "ASYNC: codecState - %d\n", codecState );
        if ( codecState < 0 )
        {
            return OMX_ErrorUndefined;
        }

        OMX_U32 rem = datalen - frame.fb_frameSize;

        TRACE_PRINT( pEnc->log, "ASYNC: stream.streamlen - %u \n", (unsigned)stream.streamlen);
        TRACE_PRINT( pEnc->log, "ASYNC: rem - %u \n", (unsigned)rem);

        memmove(bus_data, bus_data + frame.fb_frameSize, rem );
        datalen -=  frame.fb_frameSize;
        TRACE_PRINT( pEnc->log, "ASYNC: bytes left in temp buffer - %u \n",(unsigned)datalen );

        TRACE_PRINT(pEnc->log, "ASYNC: output buffer size: %d\n", (int)outputBuffer->header->nAllocLen);
        if (stream.streamlen > outputBuffer->header->nAllocLen)
        {
            // note: create overflow event, continue?
            TRACE_PRINT(pEnc->log, "ASYNC: output buffer is too small!\n");
            return OMX_ErrorOverflow;
        }
        if (pEnc->outputPort.def.format.image.eCompressionFormat == OMX_IMAGE_CodingWEBP)
        {
            dataSize = 0;
            // copy data from each partition to the output buffer
            for(i=0;i<9;i++)
            {
                if (stream.streamSize[i] <= outputBuffer->header->nAllocLen &&
                    dataSize <= outputBuffer->header->nAllocLen)
                    memcpy( outputBuffer->header->pBuffer + dataSize, stream.pOutBuf[i], stream.streamSize[i]);
                else
                    return OMX_ErrorOverflow;

                dataSize += stream.streamSize[i];
            }
        }
        else
            memcpy( outputBuffer->header->pBuffer, stream.bus_data, stream.streamlen);

        outputBuffer->header->nOffset    = 0;
        outputBuffer->header->nFilledLen = stream.streamlen;
        outputBuffer->header->nTimeStamp = inputBuffer->header->nTimeStamp;
        outputBuffer->header->nFlags     = inputBuffer->header->nFlags & ~OMX_BUFFERFLAG_EOS;

        TRACE_PRINT(pEnc->log, "ASYNC: output buffer OK\n");

        int markcount = pEnc->mark_write_pos - pEnc->mark_read_pos;
        if (markcount)
        {
            TRACE_PRINT(pEnc->log, "ASYNC: got %d marks pending\n", markcount);
            outputBuffer->header->hMarkTargetComponent = pEnc->marks[pEnc->mark_read_pos].hMarkTargetComponent;
            outputBuffer->header->pMarkData            = pEnc->marks[pEnc->mark_read_pos].pMarkData;
            pEnc->mark_read_pos++;
            if (--markcount == 0)
            {
                pEnc->mark_read_pos  = 0;
                pEnc->mark_write_pos = 0;
                TRACE_PRINT(pEnc->log, "ASYNC: mark buffer empty\n");
            }
        }
        else
        {
            outputBuffer->header->hMarkTargetComponent = inputBuffer->header->hMarkTargetComponent;
            outputBuffer->header->pMarkData            = inputBuffer->header->pMarkData;
        }

        if ( pEnc-> sliceMode )
        {
            TRACE_PRINT( pEnc->log, "ASYNC: pEnc->sliceCounter %u\n", (unsigned)pEnc->sliceNum);
            TRACE_PRINT( pEnc->log, "ASYNC: pEnc->numOfSlices %u\n", (unsigned)pEnc->numOfSlices );
            pEnc->sliceNum++;
            if ( pEnc->sliceNum > pEnc->numOfSlices )
            {
                pEnc->frameCounter++;
                TRACE_PRINT( pEnc->log, "OMX_BUFFERFLAG_ENDOFFRAME \n");

                outputBuffer->header->nFlags &= OMX_BUFFERFLAG_ENDOFFRAME;
                pEnc->sliceNum = 1;
            }
        }
        else
        {
            TRACE_PRINT( pEnc->log, "OMX_BUFFERFLAG_ENDOFFRAME \n");
            outputBuffer->header->nFlags &= OMX_BUFFERFLAG_ENDOFFRAME;
        }

        HantroOmx_port_lock_buffers(&pEnc->outputPort);
        HantroOmx_port_pop_buffer(&pEnc->outputPort);
        HantroOmx_port_unlock_buffers(&pEnc->outputPort);

        if (HantroOmx_port_is_tunneled(&pEnc->outputPort))
        {
            ((OMX_COMPONENTTYPE*)pEnc->outputPort.tunnelcomp)->EmptyThisBuffer(pEnc->outputPort.tunnelcomp, outputBuffer->header);
        }
        else
        {
            pEnc->app_callbacks.FillBufferDone(pEnc->self, pEnc->app_data, outputBuffer->header);
        }
    }
    *retlen = datalen;
    return OMX_ErrorNone;
}

#endif //~OMX_ENCODER_IMAGE_DOMAIN

