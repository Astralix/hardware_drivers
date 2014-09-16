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
--  Description :  Common code for test clients
--
------------------------------------------------------------------------------*/

#define _GNU_SOURCE // for getline

/* OMX includes */
#include <OMX_Core.h>
#include <OMX_Types.h>

/* system includes */
#include <stdarg.h> /* va_list ... */
#include <stdio.h>  /* vprintf */
#include <string.h> /* strcmp */
#include <assert.h>
#include <errno.h>
#include <unistd.h> /* usleep */
#include <stdlib.h>

/* NOTE: way to embed common code between projects */
#include <OSAL.h>
#include <msgque.h>
#include <util.h>

/* project includes */
#include "omxtestcommon.h"

typedef struct RCVSTATE
{
    int rcV1;
    int advanced;
    int filesize;
} RCVSTATE;

/* list function definitions */

OMX_U32 list_capacity(HEADERLIST * list)
{
    assert(list);
    return list->capacity - 1;
}

OMX_U32 list_available(HEADERLIST * list)
{
    assert(list);
    return (list->readpos < list->writepos)
        ? list->writepos - list->readpos
        : (list->capacity - list->readpos) + list->writepos;
}

void list_init(HEADERLIST * list, OMX_U32 capacity)
{
    assert(list);
    assert(capacity > 0);

    list->capacity = capacity + 1;
    list->readpos = 0;
    list->writepos = 0;

    list->hdrs =
        (OMX_BUFFERHEADERTYPE **) OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE *) *
                                              list->capacity);
    assert(list->hdrs);

    memset(list->hdrs, 0, sizeof(OMX_BUFFERHEADERTYPE *) * list->capacity);

}

void list_clear(HEADERLIST * list)
{
    assert(list);
    assert(list->hdrs);
    assert(list->capacity > 0);

    memset(list->hdrs, 0, sizeof(OMX_BUFFERHEADERTYPE *) * list->capacity);

    list->readpos = 0;
    list->writepos = 0;
}

void list_destroy(HEADERLIST * list)
{
    memset(list->hdrs, 0, sizeof(OMX_BUFFERHEADERTYPE *) * list->capacity);
    OSAL_Free((OMX_PTR) list->hdrs);

    memset(list, 0, sizeof(HEADERLIST));
}

void list_copy(HEADERLIST * dst, HEADERLIST * src)
{
    OMX_U32 diff;

    assert(dst && src);
    assert(dst->capacity >= src->capacity);

    dst->capacity = src->capacity;
    dst->readpos = src->readpos;
    dst->writepos = src->writepos;

    assert(dst->hdrs);
    assert(src->hdrs);

    memcpy(dst->hdrs, src->hdrs,
           sizeof(OMX_BUFFERHEADERTYPE *) * dst->capacity);

    diff = dst->capacity - src->capacity;
    if(diff > 0)
    {
        memset(dst->hdrs + diff, 0, diff);
    }
}

OMX_BOOL list_push_header(HEADERLIST * list, OMX_BUFFERHEADERTYPE * header)
{
    assert(list);
    assert(list->writepos < list->capacity);

    if(((list->writepos + 1) % list->capacity) != list->readpos)
    {
        list->hdrs[list->writepos] = header;
        list->writepos = ++(list->writepos) % list->capacity;
        return OMX_TRUE;
    }
    return OMX_FALSE;
}

void list_get_header(HEADERLIST * list, OMX_BUFFERHEADERTYPE ** header)
{
    assert(list);
    assert(header);

    if(list->readpos == list->writepos)
    {
        *header = NULL;
        return;
    }

    *header = list->hdrs[list->readpos++];
    list->readpos = list->readpos % list->capacity;
}

/* ---------------- TRACE-C -------------------- */

static OMX_HANDLETYPE gTraceMutex = 0;

OMX_U32 traceLevel =
    OMX_OSAL_TRACE_INFO | OMX_OSAL_TRACE_ERROR | OMX_OSAL_TRACE_DEBUG;

/**
 * error string table
 */
OMX_STRING OMX_OSAL_TraceErrorStr(OMX_IN OMX_ERRORTYPE omxError)
{
    switch (omxError)
    {
    case OMX_ErrorNone:
        return "OMX_ErrorNone";
    case OMX_ErrorInsufficientResources:
        return "OMX_ErrorInsufficientResources";
    case OMX_ErrorUndefined:
        return "OMX_ErrorUndefined";
    case OMX_ErrorInvalidComponentName:
        return "OMX_ErrrInvalidComponentName";
    case OMX_ErrorComponentNotFound:
        return "OMX_ErrorComponentNotFound";
    case OMX_ErrorInvalidComponent:
        return "OMX_ErrorInvalidComponent";
    case OMX_ErrorBadParameter:
        return "OMX_ErrorBadParameter";
    case OMX_ErrorNotImplemented:
        return "OMX_ErrorNotImplemented";
    case OMX_ErrorUnderflow:
        return "OMX_ErrorUnderflow";
    case OMX_ErrorOverflow:
        return "OMX_ErrorOverflow";
    case OMX_ErrorHardware:
        return "OMX_ErrorHardware";
    case OMX_ErrorInvalidState:
        return "OMX_ErrorInvalidState";
    case OMX_ErrorStreamCorrupt:
        return "OMX_ErrorStreamCorrupt";
    case OMX_ErrorPortsNotCompatible:
        return "OMX_ErrorPortsNotCompatible";
    case OMX_ErrorResourcesLost:
        return "OMX_ErrorResourcesLost";
    case OMX_ErrorNoMore:
        return "OMX_ErrorNoMore";
    case OMX_ErrorVersionMismatch:
        return "OMX_ErrorVersionMismatch";
    case OMX_ErrorNotReady:
        return "OMX_ErrorNotReady";
    case OMX_ErrorTimeout:
        return "OMX_ErrorTimeout";
    case OMX_ErrorSameState:
        return "OMX_ErrorSameState";
    case OMX_ErrorResourcesPreempted:
        return "OMX_ErrorResourcesPreempted";
    case OMX_ErrorPortUnresponsiveDuringAllocation:
        return "OMX_ErrorPortUnresponsiveDuringAllocation";
    case OMX_ErrorPortUnresponsiveDuringDeallocation:
        return "OMX_ErrorPortUnresponsiveDuringDeallocation";
    case OMX_ErrorPortUnresponsiveDuringStop:
        return "OMX_ErrorPortUnresponsiveDuringStop";
    case OMX_ErrorIncorrectStateTransition:
        return "OMX_ErrorIncorrectStateTransition";
    case OMX_ErrorIncorrectStateOperation:
        return "OMX_ErrorIncorrectStateOperation";
    case OMX_ErrorUnsupportedSetting:
        return "OMX_ErrorUnsupportedSetting";
    case OMX_ErrorUnsupportedIndex:
        return "OMX_ErrorUnsupportedIndex";
    case OMX_ErrorBadPortIndex:
        return "OMX_ErrorBadPortIndex";
    case OMX_ErrorPortUnpopulated:
        return "OMX_ErrorPortUnpopulated";
    case OMX_ErrorComponentSuspended:
        return "OMX_ErrorComponentSuspended";
    case OMX_ErrorDynamicResourcesUnavailable:
        return "OMX_ErrorDynamicResourcesUnavailable";
    case OMX_ErrorMbErrorsInFrame:
        return "OMX_ErrorMbErrorsInFrame";
    case OMX_ErrorFormatNotDetected:
        return "OMX_ErrorFormatNotDetected";
    case OMX_ErrorContentPipeOpenFailed:
        return "OMX_ErrorContentPipeOpenFailed";
    case OMX_ErrorContentPipeCreationFailed:
        return "OMX_ErrorContentPipeCreationFailed";
    case OMX_ErrorSeperateTablesUsed:
        return "OMX_ErrorSeperateTablesUsed";
    case OMX_ErrorTunnelingUnsupported:
        return "OMX_ErrorTunnelingUnsupported";
    case OMX_ErrorMax:
        return 0;
    }

    return 0;
}

OMX_STRING OMX_OSAL_TraceCodingTypeStr(OMX_IN OMX_VIDEO_CODINGTYPE coding)
{
    switch (coding)
    {
    case OMX_VIDEO_CodingUnused:
        return "OMX_VIDEO_CodingUnused";
    case OMX_VIDEO_CodingAutoDetect:
        return "OMX_VIDEO_CodingAutoDetect";
    case OMX_VIDEO_CodingMPEG2:
        return "OMX_VIDEO_CodingMPEG2";
    case OMX_VIDEO_CodingH263:
        return "OMX_VIDEO_CodingH263";
    case OMX_VIDEO_CodingMPEG4:
        return "OMX_VIDEO_CodingMPEG4";
    case OMX_VIDEO_CodingWMV:
        return "OMX_VIDEO_CodingWMV";
    case OMX_VIDEO_CodingRV:
        return "OMX_VIDEO_CodingRV";
    case OMX_VIDEO_CodingAVC:
        return "OMX_VIDEO_CodingAVC";
    case OMX_VIDEO_CodingMJPEG:
        return "OMX_VIDEO_CodingMJPEG";
    case OMX_VIDEO_CodingMax:
        return "OMX_VIDEO_CodingMax";
    }
    return 0;
}

OMX_STRING OMX_OSAL_TraceColorFormatStr(OMX_IN OMX_COLOR_FORMATTYPE color)
{
    switch (color)
    {
    case OMX_COLOR_FormatUnused:
        return "OMX_COLOR_FormatUnused";
    case OMX_COLOR_FormatMonochrome:
        return "OMX_COLOR_FormatMonochrome";
    case OMX_COLOR_Format8bitRGB332:
        return "OMX_COLOR_Format8bitRGB332";
    case OMX_COLOR_Format12bitRGB444:
        return "OMX_COLOR_Format12bitRGB444";
    case OMX_COLOR_Format16bitARGB4444:
        return "OMX_COLOR_Format16bitARGB4444";
    case OMX_COLOR_Format16bitARGB1555:
        return "OMX_COLOR_Format16bitARGB1555";
    case OMX_COLOR_Format16bitRGB565:
        return "OMX_COLOR_Format16bitRGB565";
    case OMX_COLOR_Format16bitBGR565:
        return "OMX_COLOR_Format16bitBGR565";
    case OMX_COLOR_Format18bitRGB666:
        return "OMX_COLOR_Format18bitRGB666";
    case OMX_COLOR_Format18bitARGB1665:
        return "OMX_COLOR_Format18bitARGB1665";
    case OMX_COLOR_Format19bitARGB1666:
        return "OMX_COLOR_Format19bitARGB1666";
    case OMX_COLOR_Format24bitRGB888:
        return "OMX_COLOR_Format24bitRGB888";
    case OMX_COLOR_Format24bitBGR888:
        return "OMX_COLOR_Format24bitBGR888";
    case OMX_COLOR_Format24bitARGB1887:
        return "OMX_COLOR_Format24bitARGB1887";
    case OMX_COLOR_Format25bitARGB1888:
        return "OMX_COLOR_Format25bitARGB1888";
    case OMX_COLOR_Format32bitBGRA8888:
        return "OMX_COLOR_Format32bitBGRA8888";
    case OMX_COLOR_Format32bitARGB8888:
        return "OMX_COLOR_Format32bitARGB8888";
    case OMX_COLOR_FormatYUV411Planar:
        return "OMX_COLOR_FormatYUV411Planar";
    case OMX_COLOR_FormatYUV411PackedPlanar:
        return "OMX_COLOR_FormatYUV411PackedPlanar";
    case OMX_COLOR_FormatYUV420Planar:
        return "OMX_COLOR_FormatYUV420Planar";
    case OMX_COLOR_FormatYUV420PackedPlanar:
        return "OMX_COLOR_FormatYUV420PackedPlanar";
    case OMX_COLOR_FormatYUV420SemiPlanar:
        return "OMX_COLOR_FormatYUV420SemiPlanar";
    case OMX_COLOR_FormatYUV422Planar:
        return "OMX_COLOR_FormatYUV422Planar";
    case OMX_COLOR_FormatYUV422PackedPlanar:
        return "OMX_COLOR_FormatYUV422PackedPlanar";
    case OMX_COLOR_FormatYUV422SemiPlanar:
        return "OMX_COLOR_FormatYUV422SemiPlanar";
    case OMX_COLOR_FormatYCbYCr:
        return "OMX_COLOR_FormatYCbYCr";
    case OMX_COLOR_FormatYCrYCb:
        return "OMX_COLOR_FormatYCrYCb";
    case OMX_COLOR_FormatCbYCrY:
        return "OMX_COLOR_FormatCbYCrY";
    case OMX_COLOR_FormatCrYCbY:
        return "OMX_COLOR_FormatCrYCbY";
    case OMX_COLOR_FormatYUV444Interleaved:
        return "OMX_COLOR_FormatYUV444Interleaved";
    case OMX_COLOR_FormatRawBayer8bit:
        return "OMX_COLOR_FormatRawBayer8bit";
    case OMX_COLOR_FormatRawBayer10bit:
        return "OMX_COLOR_FormatRawBayer10bit";
    case OMX_COLOR_FormatRawBayer8bitcompressed:
        return "OMX_COLOR_FormatRawBayer8bitcompressed";
    case OMX_COLOR_FormatL2:
        return "OMX_COLOR_FormatL2";
    case OMX_COLOR_FormatL4:
        return "OMX_COLOR_FormatL4";
    case OMX_COLOR_FormatL8:
        return "OMX_COLOR_FormatL8";
    case OMX_COLOR_FormatL16:
        return "OMX_COLOR_FormatL16";
    case OMX_COLOR_FormatL24:
        return "OMX_COLOR_FormatL24";
    case OMX_COLOR_FormatL32:
        return "OMX_COLOR_FormatL32";
    case OMX_COLOR_FormatYUV420PackedSemiPlanar:
        return "OMX_COLOR_FormatYUV420PackedSemiPlanar";
    case OMX_COLOR_FormatYUV422PackedSemiPlanar:
        return "OMX_COLOR_FormatYUV422PackedSemiPlanar";
    case OMX_COLOR_Format18BitBGR666:
        return "OMX_COLOR_Format18BitBGR666";
    case OMX_COLOR_Format24BitARGB6666:
        return "OMX_COLOR_Format24BitARGB6666";
    case OMX_COLOR_Format24BitABGR6666:
        return "OMX_COLOR_Format24BitABGR6666";
    case OMX_COLOR_FormatMax:
        return "OMX_COLOR_FormatMax";
    }
    return 0;
}

OMX_STRING OMX_OSAL_TraceDirectionStr(OMX_IN OMX_DIRTYPE color)
{
    switch (color)
    {
    case OMX_DirInput:
        return "OMX_DirInput";
    case OMX_DirOutput:
        return "OMX_DirOutput";
    case OMX_DirMax:
        return "OMX_DirMax";
    }
    return 0;
}

void OMX_OSAL_TracePortSettings(OMX_IN OMX_U32 flags,
                                OMX_IN OMX_PARAM_PORTDEFINITIONTYPE * port)
{
    OMX_STRING str;

    switch (port->eDir)
    {
    case OMX_DirOutput:
        str = "Output";
        break;
    case OMX_DirInput:
        str = "Input";
        break;
    default:
        str = 0;
    }

    OMX_OSAL_Trace(flags,
                   "%s port settings:\n\n"
                   "           nBufferSize: %d\n"
                   "           nFrameWidth: %d\n"
                   "          nFrameHeight: %d\n"
                   "               nStride: %d\n"
                   "          nSliceHeight: %d\n\n"
                   "          eColorFormat: %s\n"
                   "    eCompressionFormat: %s\n"
                   "              nBitrate: %d\n"
                   "            xFramerate: %f\n"
                   " bFlagErrorConcealment: %s\n\n",
                   str,
                   (int) port->nBufferSize,
                   (int) port->format.video.nFrameWidth,
                   (int) port->format.video.nFrameHeight,
                   (int) port->format.video.nStride,
                   (int) port->format.video.nSliceHeight,
                   OMX_OSAL_TraceColorFormatStr(port->format.video.
                                                eColorFormat),
                   OMX_OSAL_TraceCodingTypeStr(port->format.video.
                                               eCompressionFormat),
                   port->format.video.nBitrate,
                   Q16_FLOAT(port->format.video.xFramerate),
                   ((port->format.video.bFlagErrorConcealment ==
                     OMX_TRUE) ? "true" : "false"));
}

/**
 *
 */
OMX_ERRORTYPE OMX_OSAL_Trace(OMX_IN OMX_U32 nTraceFlags,
                             OMX_IN OMX_STRING format, ...)
{
    va_list args;

    if(gTraceMutex)
        OSAL_MutexLock(gTraceMutex);

    if(nTraceFlags & traceLevel)
    {
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }

    if(gTraceMutex)
        OSAL_MutexUnlock(gTraceMutex);

    return OMX_ErrorNone;
}

/* ---------------- END TRACE -------------------- */

/* Callback declarations */
static OMX_ERRORTYPE omxclient_event_handler(OMX_IN OMX_HANDLETYPE hComponent,
                                             OMX_IN OMX_PTR pAppData,
                                             OMX_IN OMX_EVENTTYPE eEvent,
                                             OMX_IN OMX_U32 nData1,
                                             OMX_IN OMX_U32 nData2,
                                             OMX_IN OMX_PTR pEventData);

static OMX_ERRORTYPE omxclient_empty_buffer_done(OMX_IN OMX_HANDLETYPE
                                                 hComponent,
                                                 OMX_IN OMX_PTR pAppData,
                                                 OMX_IN OMX_BUFFERHEADERTYPE *
                                                 pBuffer);

static OMX_ERRORTYPE omxclient_buffer_fill_done(OMX_OUT OMX_HANDLETYPE
                                                hComponent,
                                                OMX_OUT OMX_PTR pAppData,
                                                OMX_OUT OMX_BUFFERHEADERTYPE *
                                                pBuffer);

/* function implementations */
OMX_ERRORTYPE omxclient_read_variance_file(OMX_U32 ** variance_array,
                                           OMX_U32 * array_len,
                                           OMX_STRING filename)
{
    FILE *file;

    OMX_STRING line;

    size_t len;

    ssize_t read;

    size_t count;

    OMX_U32 *tmp_array;

    file = fopen(filename, "r");

    if(!file)
    {
        return OMX_ErrorStreamCorrupt;
    }

    count = 0;
    line = 0;
    len = 0;
    tmp_array = 0;

    while((read = getline(&line, &len, file)) != -1)
    {
        ++count;
    }

    if(count == 0)
    {
        return OMX_ErrorStreamCorrupt;
    }

    rewind(file);

    tmp_array = (OMX_U32 *) malloc(count * sizeof(OMX_U32));
    count = 0;

    while((read = getline(&line, &len, file)) != -1)
    {
        tmp_array[count++] = atoi(line);
    }

    if(line)
    {
        free(line);
    }

    *variance_array = tmp_array;
    *array_len = count;

    return OMX_ErrorNone;
}

/**
 *
 */
static OMX_ERRORTYPE omxclient_event_handler(OMX_IN OMX_HANDLETYPE component,
                                             OMX_IN OMX_PTR pAppData,
                                             OMX_IN OMX_EVENTTYPE event,
                                             OMX_IN OMX_U32 nData1,
                                             OMX_IN OMX_U32 nData2,
                                             OMX_IN OMX_PTR pEventData)
{
    OMX_ERRORTYPE omxError;

    OMX_STATETYPE state;

    OMX_PARAM_PORTDEFINITIONTYPE port;

    OMX_STRING str;

    omxError = OMX_ErrorNone;

    OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                   "Got component event: event:%s data1:%u data2:%u eventdata 0x%x\n",
                   HantroOmx_str_omx_event(event), (unsigned) nData1,
                   (unsigned) nData2, pEventData);

    switch (event)
    {
    case OMX_EventPortSettingsChanged:
        {
            omxclient_struct_init(&port, OMX_PARAM_PORTDEFINITIONTYPE);
            port.nPortIndex = 1;

            OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                                      (component, OMX_IndexParamPortDefinition,
                                       &port), omxError);

            switch (port.eDir)
            {
            case OMX_DirOutput:
                str = "Output";
                break;
            case OMX_DirInput:
                str = "Input";
                break;
            default:
                str = 0;
            }

            OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                           "%s port settings changed\n\n"
                           "           nBufferSize: %d\n"
                           "           nFrameWidth: %d\n"
                           "          nFrameHeight: %d\n"
                           "               nStride: %d\n"
                           "          nSliceHeight: %d\n\n"
                           "          eColorFormat: %s\n"
                           "    eCompressionFormat: %s\n"
                           "              nBitrate: %d\n"
                           "            xFramerate: %.2f\n"
                           " bFlagErrorConcealment: %s\n\n",
                           str,
                           (int) port.nBufferSize,
                           (int) port.format.video.nFrameWidth,
                           (int) port.format.video.nFrameHeight,
                           (int) port.format.video.nStride,
                           (int) port.format.video.nSliceHeight,
                           OMX_OSAL_TraceColorFormatStr(port.format.video.
                                                        eColorFormat),
                           OMX_OSAL_TraceCodingTypeStr(port.format.video.
                                                       eCompressionFormat),
                           port.format.video.nBitrate,
                           Q16_FLOAT(port.format.video.xFramerate),
                           port.format.video.
                           bFlagErrorConcealment ? "true" : "false");
        }
        break;

    case OMX_EventCmdComplete:
        {
            switch ((OMX_COMMANDTYPE) (nData1))
            {
            case OMX_CommandStateSet:
                OMX_GetState(component, &state);
                OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "State: %s\n",
                               HantroOmx_str_omx_state(state));
                OSAL_EventSet(OMXCLIENT_PTR(pAppData)->state_event);
                break;

            default:
                break;
            }
        }
        break;

    case OMX_EventBufferFlag:
        {
            OMXCLIENT_PTR(pAppData)->EOS = OMX_TRUE;
        }
        break;

    case OMX_EventError:
        {
            OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Error: %s\n",
                           OMX_OSAL_TraceErrorStr((OMX_ERRORTYPE) nData1));
        }
        break;
    default:
        break;
    }

    return omxError;
}

/**
 *
 */
static OMX_ERRORTYPE omxclient_empty_buffer_done(OMX_IN OMX_HANDLETYPE
                                                 hComponent,
                                                 OMX_IN OMX_PTR pAppData,
                                                 OMX_IN OMX_BUFFERHEADERTYPE *
                                                 pBuffer)
{
    OMX_ERRORTYPE omxError = OMX_ErrorNone;

    OMXCLIENT *appdata = OMXCLIENT_PTR(pAppData);

    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Empty buffer done\n");

    OSAL_MutexLock(appdata->queue_mutex);
    {
        if(appdata->input_queue.writepos >= appdata->input_queue.capacity)
        {
            OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "No space in return queue\n");

            /* NOTE: correct return? */
            omxError = OMX_ErrorInsufficientResources;
        }
        else
        {
            list_push_header(&(appdata->input_queue), pBuffer);
        }
    }

    OSAL_MutexUnlock(appdata->queue_mutex);
    return omxError;
}

/**
 *
 */
static OMX_ERRORTYPE omxclient_buffer_fill_done(OMX_OUT OMX_HANDLETYPE
                                                component,
                                                OMX_OUT OMX_PTR pAppData,
                                                OMX_OUT OMX_BUFFERHEADERTYPE *
                                                buffer)
{
    OMXCLIENT *client = OMXCLIENT_PTR(pAppData);

    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG,
                   "Fill Buffer Done: filledLen %u, nOffset %u\n",
                   (unsigned) buffer->nFilledLen, (unsigned) buffer->nOffset);

    /* if eos has already been processed */
    if(client->EOS == OMX_TRUE)
    {
        list_push_header(&(client->output_queue), buffer);
        return OMX_ErrorNone;
    }

    size_t ret = fwrite(buffer->pBuffer, 1, buffer->nFilledLen, client->output);

    fflush(client->output);

    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "\twrote %u bytes to file\n", ret);

    /* if frames are captured */
    if(client->store_frames || client->store_buffers)
    {

        /* if frame file does not exist, create one */
        if(!client->frame_output)
        {
            char name[255];

            sprintf(name, "%s.%d", client->output_name,
                    (int) client->frame_count);
            client->frame_output = fopen(name, "wb");

            /* if file creation failed, return error */
            if(!client->frame_output)
            {
                return OMX_ErrorStreamCorrupt;
            }
        }

        /* write to file */
        size_t ret =
            fwrite(buffer->pBuffer, 1, buffer->nFilledLen,
                   client->frame_output);
        fflush(client->frame_output);

        OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "\twrote %u bytes to frame-file\n",
                       ret);
    }

    /* if at the end of frame, close file and increment counter */
    if(client->store_buffers ||
       (client->store_frames && buffer->nFlags & OMX_BUFFERFLAG_ENDOFFRAME))
    {
        ++(client->frame_count);
        fclose(client->frame_output);
        client->frame_output = NULL;
    }

    if(buffer->nFlags & OMX_BUFFERFLAG_EOS)
    {
        client->EOS = OMX_TRUE;
        list_push_header(&(client->output_queue), buffer);
        return OMX_ErrorNone;
    }

    buffer->nFilledLen = 0;
    buffer->nOffset = 0;
    buffer->nFlags = 0;

    return OMX_FillThisBuffer(component, buffer);
}

/**
 *
 */
OMX_ERRORTYPE omxclient_get_component_roles(OMX_STRING cComponentName,
                                            OMX_STRING cRole)
{
    OMX_ERRORTYPE omxError;

    OMX_U32 i, nRoles;

    OMX_STRING *sRoleArray;

    omxError = OMX_GetRolesOfComponent(cComponentName, &nRoles, (OMX_U8 **) 0);
    if(omxError == OMX_ErrorNone)
    {
        sRoleArray = (OMX_STRING *) OSAL_Malloc(nRoles * sizeof(OMX_STRING));

        /* check memory allocation */
        if(!sRoleArray)
        {
            return OMX_ErrorInsufficientResources;
        }

        for(i = 0; i < nRoles; ++i)
        {
            sRoleArray[i] =
                (OMX_STRING) OSAL_Malloc(sizeof(OMX_U8) *
                                         OMX_MAX_STRINGNAME_SIZE);
            if(!sRoleArray[i])
            {
                omxError = OMX_ErrorInsufficientResources;
            }
        }

        if(omxError == OMX_ErrorNone)
        {
            omxError =
                OMX_GetRolesOfComponent(cComponentName, &nRoles,
                                        (OMX_U8 **) sRoleArray);
            if(omxError == OMX_ErrorNone)
            {
                /*omxError = OMX_ErrorNotImplemented; */
                for(i = 0; i < nRoles; ++i)
                {
                    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG,
                                   "Role '%s' supported.\n", sRoleArray[i]);
                    if(strcmp(sRoleArray[i], cRole) == 0)
                    {
                        omxError = OMX_ErrorNone;
                    }
                }
            }
        }

        for(; i;)
        {
            OSAL_Free((OMX_PTR) sRoleArray[--i]);
        }
        OSAL_Free((OMX_PTR) sRoleArray);
    }

    return omxError;
}

/**
 *
 */
OMX_ERRORTYPE omxclient_component_create(OMXCLIENT * appdata,
                                         OMX_STRING cComponentName,
                                         OMX_STRING cRole, OMX_U32 buffer_count)
{
    OMX_ERRORTYPE omxError;

    OMX_CALLBACKTYPE oCallbacks;

    memset(appdata, 0, sizeof(OMXCLIENT));

    oCallbacks.EmptyBufferDone = omxclient_empty_buffer_done;
    oCallbacks.EventHandler = omxclient_event_handler;
    oCallbacks.FillBufferDone = omxclient_buffer_fill_done;

    /* get roles */
    omxError = omxclient_get_component_roles(cComponentName, cRole);
    if(omxError == OMX_ErrorNone)
    {
        /* create */
        omxError =
            OMX_GetHandle(&(appdata->component), cComponentName, appdata,
                          &oCallbacks);
        if(omxError == OMX_ErrorNone)
        {
             /**/ list_init(&(appdata->input_queue), buffer_count);
            list_init(&(appdata->output_queue), buffer_count);

            OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Component '%s' created.\n",
                           cComponentName);

            omxError = OSAL_MutexCreate(&(appdata->queue_mutex));
            if(omxError == OMX_ErrorNone)
            {
                omxError = OSAL_EventCreate(&(appdata->state_event));
                if(omxError != OMX_ErrorNone)
                {
                    OSAL_MutexDestroy(appdata->queue_mutex);
                    list_destroy(&(appdata->input_queue));
                    list_destroy(&(appdata->output_queue));
                }
            }
            else
            {
                list_destroy(&(appdata->input_queue));
                list_destroy(&(appdata->output_queue));
            }
        }
        /* no else, error propagated onwards */
    }
    /* no else */

    return omxError;
}

/**
 *
 */
OMX_ERRORTYPE omxclient_component_destroy(OMXCLIENT * appdata)
{
    OMX_ERRORTYPE omxError;

    OMX_STATETYPE state;

    OMX_BOOL timeout;

    timeout = OMX_FALSE;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetState(appdata->component, &state),
                              omxError);
    while(state != OMX_StateLoaded)
    {
        switch (state)
        {
        case OMX_StateWaitForResources:
            return OMX_ErrorInvalidState;

        case OMX_StateLoaded:
            /* Buffers should be deallocated during state transition from idle to loaded, however,
             * if buffer allocation has been failed, component has not been able to move into idle */
            break;

        case OMX_StateExecuting:
        case OMX_StatePause:

            OMXCLIENT_RETURN_ON_ERROR(omxclient_change_state_and_wait
                                      (appdata, OMX_StateIdle), omxError);

            state = OMX_StateIdle;
            break;

        case OMX_StateIdle:

            OSAL_EventReset(appdata->state_event);

            OMXCLIENT_RETURN_ON_ERROR(OMX_SendCommand
                                      (appdata->component, OMX_CommandStateSet,
                                       OMX_StateLoaded, NULL), omxError);

            omxclient_component_free_buffers(appdata);

            /* wait for LOADED */
            OMXCLIENT_RETURN_ON_ERROR(omxclient_wait_state
                                      (appdata, OMX_StateLoaded), omxError);

            state = OMX_StateLoaded;
            break;

        default:
            return OMX_ErrorInvalidState;
        }
    }

    list_destroy(&(appdata->input_queue));
    list_destroy(&(appdata->output_queue));

    OSAL_MutexDestroy(appdata->queue_mutex);
    appdata->queue_mutex = 0;

    OSAL_EventDestroy(appdata->state_event);
    appdata->state_event = 0;

    return OMX_FreeHandle(appdata->component);
}

OMX_ERRORTYPE omxclient_component_free_buffers(OMXCLIENT * appdata)
{
    OMX_ERRORTYPE error = OMX_ErrorNone;

    OMX_U32 i;

    OMX_BUFFERHEADERTYPE *hdr;

    for(i = list_available(&(appdata->input_queue));
        i && error == OMX_ErrorNone; --i)
    {
        list_get_header(&(appdata->input_queue), &hdr);
        error = OMX_FreeBuffer(appdata->component, 0, hdr);
    }

    for(i = list_available(&(appdata->output_queue));
        i && error == OMX_ErrorNone; --i)
    {
        list_get_header(&(appdata->output_queue), &hdr);
        error = OMX_FreeBuffer(appdata->component, 1, hdr);
    }

    return error;
}

OMX_ERRORTYPE omxclient_wait_state(OMXCLIENT * client, OMX_STATETYPE state)
{
    OMX_BOOL event_timeout;

    OMX_ERRORTYPE error;

    OMX_STATETYPE current_state;

    event_timeout = OMX_FALSE;

    /* change state to EXECUTING */
    error = OMX_GetState(client->component, &current_state);
    if(current_state != state)
    {
        OMXCLIENT_RETURN_ON_ERROR(OSAL_EventWait
                                  (client->state_event, OMXCLIENT_EVENT_TIMEOUT,
                                   &event_timeout), error);

        if(event_timeout)
            return OMX_ErrorTimeout;

        error = OMX_GetState(client->component, &current_state);
        if(error != OMX_ErrorNone)
            return error;

        if(current_state != state)
            return OMX_ErrorInvalidState;
    }

    return error;
}

OMX_ERRORTYPE omxclient_change_state_and_wait(OMXCLIENT * client,
                                              OMX_STATETYPE state)
{

    OMX_BOOL event_timeout;

    OMX_ERRORTYPE error;

    OMX_STATETYPE current_state;

    event_timeout = OMX_FALSE;

    /* change state to EXECUTING */
    error = OMX_GetState(client->component, &current_state);
    if(current_state != state)
    {
        OSAL_EventReset(client->state_event);

        OMXCLIENT_RETURN_ON_ERROR(OMX_SendCommand
                                  (client->component, OMX_CommandStateSet,
                                   state, NULL), error);

        OMXCLIENT_RETURN_ON_ERROR(OSAL_EventWait
                                  (client->state_event, OMXCLIENT_EVENT_TIMEOUT,
                                   &event_timeout), error);

        if(event_timeout)
            return OMX_ErrorTimeout;

        error = OMX_GetState(client->component, &current_state);
        if(error != OMX_ErrorNone)
            return error;

        if(current_state != state)
            return OMX_ErrorInvalidState;
    }

    return error;
}

/**
 *
 */
OMX_ERRORTYPE omxclient_execute(OMXCLIENT * appdata, OMX_STRING input_filename,
                                OMX_U32 * variance, OMX_U32 varlen,
                                OMX_STRING output_filename)
{
    OMX_ERRORTYPE omxError;

    OMX_STATETYPE state;

    OMX_U32 varpos;

    omxError = OMX_ErrorNone;
    state = OMX_StateLoaded;
    varpos = 0;

    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Using input '%s'\n", input_filename);
    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Using output '%s'\n",
                   output_filename);

    appdata->input = NULL;
    appdata->output = NULL;

    char error_string[256];

    memset(error_string, 0, sizeof(error_string));

    /* Open input file */

    appdata->input = fopen(input_filename, "rb");
    if(appdata->input == NULL)
    {
        strerror_r(errno, error_string, sizeof(error_string));
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "'%s'\n", error_string);

        return OMX_ErrorStreamCorrupt;
    }

    /* Open output file */

    appdata->output = fopen(output_filename, "wb");
    if(appdata->output == NULL)
    {
        strerror_r(errno, error_string, sizeof(error_string));
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "'%s'\n", error_string);

        return OMX_ErrorStreamCorrupt;
    }

    /* change component state */

    /* -> waiting for IDLE */
    OMXCLIENT_RETURN_ON_ERROR(omxclient_wait_state(appdata, OMX_StateIdle),
                              omxError);

    /* -> transition to EXECUTING */
    OMXCLIENT_RETURN_ON_ERROR(omxclient_change_state_and_wait
                              (appdata, OMX_StateExecuting), omxError);

    /* Tell component to ... fill these buffers */
    OMX_BUFFERHEADERTYPE *output_buffer = NULL;

    do
    {
        list_get_header(&(appdata->output_queue), &output_buffer);
        if(output_buffer)
        {
            output_buffer->nOutputPortIndex = 1;
            omxError = OMX_FillThisBuffer(appdata->component, output_buffer);

            if(omxError != OMX_ErrorNone)
            {
                return omxError;
            }
        }
    }
    while(output_buffer);

    /* run the decoder/encoder job */

    appdata->EOS = OMX_FALSE;

    OMX_BOOL eof = OMX_FALSE;

    while(eof == OMX_FALSE)
    {
        OMX_BUFFERHEADERTYPE *input_buffer = NULL;

        /* Get input (synch) >> */
        OSAL_MutexLock(appdata->queue_mutex);
        {
            list_get_header(&appdata->input_queue, &input_buffer);
        }
        OSAL_MutexUnlock(appdata->queue_mutex);
        /* << Get input (synch) */

        if(input_buffer == NULL)
        {
            /* NOTE: output buffer not available, wait -> event */
            usleep(1000 * 1000);
            continue;
        }

        if(!appdata->input)
        {
            return OMX_ErrorInsufficientResources;
        }

        input_buffer->nInputPortIndex = 0;

        size_t ret;

        if(varpos < varlen)
        {
            if(variance[varpos] > input_buffer->nAllocLen)
            {
                return OMX_ErrorBadParameter;
            }

            ret = fread(input_buffer->pBuffer, 1, variance[varpos++],
                        appdata->input);
        }
        else
        {
            ret = fread(input_buffer->pBuffer, 1, input_buffer->nAllocLen,
                        appdata->input);
        }

        input_buffer->nOffset = 0;
        input_buffer->nFilledLen = ret;

        eof = feof(appdata->input) != 0;
        if(eof)
        {
            input_buffer->nFlags |= OMX_BUFFERFLAG_EOS;
        }

        omxError = OMX_EmptyThisBuffer(appdata->component, input_buffer);
        if(omxError != OMX_ErrorNone)
        {
            return omxError;
        }

        printf("\twrote %u bytes to component\n", ret);
        if(eof)
        {
            printf("\tinput EOF reached\n");
        }

        usleep(0);
    }

    /* get stream end event */
    while(appdata->EOS == OMX_FALSE)
    {
        usleep(1000);
    }

    return omxError;
}

int read_rcv_file(FILE * strm, char *buffer, int bufflen, void *state,
                  OMX_BOOL * eof)
{
    RCVSTATE *rcv = (RCVSTATE *) state;

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
            rcv->rcV1 = !(buff[3] & 0x40);
            fseek(strm, 0, SEEK_SET);
            if(rcv->advanced)
                return fread(buffer, 1, bufflen, strm);

            if(rcv->rcV1)
                return fread(buffer, 1, 20, strm);
            else
                return fread(buffer, 1, 36, strm);
        }
        break;
    default:
        {
            if(rcv->advanced)
            {
                int ret = fread(buffer, 1, bufflen, strm);

                *eof = feof(strm);
                return ret;
            }

            int framesize = 0;

            int ret = 0;

            unsigned char buff[8];

            if(rcv->rcV1)
                fread(buff, 1, 4, strm);
            else
                fread(buff, 1, 8, strm);

            framesize |= buff[0];
            framesize |= buff[1] << 8;
            framesize |= buff[2] << 16;

            assert(framesize > 0);
            if(bufflen < framesize)
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
    return 0;   // make gcc happy
}

/**
 *
 */
OMX_ERRORTYPE omxclient_execute_rcv(OMXCLIENT * appdata,
                                    OMX_STRING input_filename,
                                    OMX_STRING output_filename)
{
    OMX_ERRORTYPE omxError;

    OMX_STATETYPE state;

    OMX_U32 varpos;

    omxError = OMX_ErrorNone;
    state = OMX_StateLoaded;
    varpos = 0;

    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Using input '%s'\n", input_filename);
    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Using output '%s'\n",
                   output_filename);

    appdata->input = NULL;
    appdata->output = NULL;

    char error_string[256];

    memset(error_string, 0, sizeof(error_string));

    /* Open input file */

    appdata->input = fopen(input_filename, "rb");
    if(appdata->input == NULL)
    {
        strerror_r(errno, error_string, sizeof(error_string));
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "'%s'\n", error_string);

        return OMX_ErrorStreamCorrupt;
    }

    /* Open output file */

    appdata->output = fopen(output_filename, "wb");
    if(appdata->output == NULL)
    {
        strerror_r(errno, error_string, sizeof(error_string));
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "'%s'\n", error_string);

        return OMX_ErrorStreamCorrupt;
    }

    /* change component state */

    /* -> waiting for IDLE */
    OMXCLIENT_RETURN_ON_ERROR(omxclient_wait_state(appdata, OMX_StateIdle),
                              omxError);

    /* -> transition to EXECUTING */
    OMXCLIENT_RETURN_ON_ERROR(omxclient_change_state_and_wait
                              (appdata, OMX_StateExecuting), omxError);

    /* Tell component to ... fill these buffers */
    OMX_BUFFERHEADERTYPE *output_buffer = NULL;

    do
    {
        list_get_header(&(appdata->output_queue), &output_buffer);
        if(output_buffer)
        {
            output_buffer->nOutputPortIndex = 1;
            omxError = OMX_FillThisBuffer(appdata->component, output_buffer);

            if(omxError != OMX_ErrorNone)
            {
                return omxError;
            }
        }
    }
    while(output_buffer);

    /* run the decoder/encoder job */

    RCVSTATE rcv;

    memset(&rcv, 0, sizeof(RCVSTATE));

    appdata->EOS = OMX_FALSE;

    OMX_BOOL eof = OMX_FALSE;

    while(eof == OMX_FALSE)
    {
        OMX_BUFFERHEADERTYPE *input_buffer = NULL;

        /* Get input (synch) >> */
        OSAL_MutexLock(appdata->queue_mutex);
        {
            list_get_header(&appdata->input_queue, &input_buffer);
        }
        OSAL_MutexUnlock(appdata->queue_mutex);
        /* << Get input (synch) */

        if(input_buffer == NULL)
        {
            /* NOTE: output buffer not available, wait -> event */
            usleep(1000 * 1000);
            continue;
        }

        if(!appdata->input)
        {
            return OMX_ErrorInsufficientResources;
        }

        input_buffer->nInputPortIndex = 0;

        size_t ret =
            read_rcv_file(appdata->input, (char *) input_buffer->pBuffer,
                          input_buffer->nAllocLen, &rcv, &eof);

        input_buffer->nOffset = 0;
        input_buffer->nFilledLen = ret;

        if(eof)
            input_buffer->nFlags |= OMX_BUFFERFLAG_EOS;

        omxError = OMX_EmptyThisBuffer(appdata->component, input_buffer);
        if(omxError != OMX_ErrorNone)
        {
            return omxError;
        }

        printf("\twrote %u bytes to component\n", ret);
        if(eof)
        {
            printf("\tinput EOF reached\n");
        }

        usleep(0);
    }

    /* get stream end event */
    while(appdata->EOS == OMX_FALSE)
    {
        usleep(1000);
    }

    return omxError;
}

/*------------------------------------------------------------------------------

    NextVop

    Function calculates next input vop depending input and output frame
    rates.

    Input   inputRateNumer  (input.yuv) frame rate numerator.
            inputRateDenom  (input.yuv) frame rate denominator
            outputRateNumer (stream.mpeg4) frame rate numerator.
            outputRateDenom (stream.mpeg4) frame rate denominator.
            frameCnt        Frame counter.
            firstVop        The first vop of input.yuv sequence.

    Return  next    The next vop of input.yuv sequence.

------------------------------------------------------------------------------*/
OMX_U32 omxclient_next_vop(OMX_U32 inputRateNumer, OMX_U32 inputRateDenom,
                           OMX_U32 outputRateNumer, OMX_U32 outputRateDenom,
                           OMX_U32 frameCnt, OMX_U32 firstVop)
{
    OMX_U32 sift;

    OMX_U32 skip;

    OMX_U32 numer;

    OMX_U32 denom;

    OMX_U32 next;

    numer = inputRateNumer * outputRateDenom;
    denom = inputRateDenom * outputRateNumer;

    if(numer >= denom)
    {
        sift = 9;
        do
        {
            sift--;
        }
        while(((numer << sift) >> sift) != numer);
    }
    else
    {
        sift = 17;
        do
        {
            sift--;
        }
        while(((numer << sift) >> sift) != numer);
    }
    skip = (numer << sift) / denom;
    next = ((frameCnt * skip) >> sift) + firstVop;

    return next;
}

/*------------------------------------------------------------------------------

    ReadVop

    Read raw YUV image data from file
    Image is divided into slices, each slice consists of equal amount of
    image rows except for the bottom slice which may be smaller than the
    others. sliceNum is the number of the slice to be read
    and sliceRows is the amount of rows in each slice (or 0 for all rows).

------------------------------------------------------------------------------*/
OMX_U32 omxclient_read_vop_sliced(OMX_U8 * image, OMX_U32 width, OMX_U32 height,
                                  OMX_U32 sliceNum, OMX_U32 sliceRows,
                                  OMX_U32 frameNum, FILE * file,
                                  OMX_COLOR_FORMATTYPE inputMode)
{
    OMX_U32 byteCount = 0;

    OMX_U32 frameSize;

    OMX_U32 frameOffset;

    OMX_U32 sliceLumOffset = 0;

    OMX_U32 sliceCbOffset = 0;

    OMX_U32 sliceCrOffset = 0;

    OMX_U32 sliceLumSize;    /* The size of one slice in bytes */

    OMX_U32 sliceCbSize;

    OMX_U32 sliceCrSize;

    OMX_U32 sliceLumSizeRead;   /* The size of the slice to be read */

    OMX_U32 sliceCbSizeRead;

    OMX_U32 sliceCrSizeRead;

    if(sliceRows == 0)
    {
        sliceRows = height;
    }

    if(inputMode == OMX_COLOR_FormatYUV420PackedPlanar)
    {
        /* YUV 4:2:0 planar */
        frameSize = width * height + (width / 2 * height / 2) * 2;
        sliceLumSizeRead = sliceLumSize = width * sliceRows;
        sliceCbSizeRead = sliceCbSize = width / 2 * sliceRows / 2;
        sliceCrSizeRead = sliceCrSize = width / 2 * sliceRows / 2;
    }
    else if(inputMode == OMX_COLOR_FormatYUV420PackedSemiPlanar)
    {
        /* YUV 4:2:0 semiplanar */
        frameSize = width * height + (width / 2 * height / 2) * 2;
        sliceLumSizeRead = sliceLumSize = width * sliceRows;
        sliceCbSizeRead = sliceCbSize = width * sliceRows / 2;
        sliceCrSizeRead = sliceCrSize = 0;
    }
    else
    {
        /* YUYV 4:2:2, this includes both luminance and chrominance */
        frameSize = width * height * 2;
        sliceLumSizeRead = sliceLumSize = width * sliceRows * 2;
        sliceCbSizeRead = sliceCbSize = 0;
        sliceCrSizeRead = sliceCrSize = 0;
    }

    /* The bottom slice may be smaller than the others */
    if(sliceRows * (sliceNum + 1) > height)
    {
        sliceRows = height - sliceRows * sliceNum;

        if(inputMode == OMX_COLOR_FormatYUV420PackedPlanar)
        {
            sliceLumSizeRead = width * sliceRows;
            sliceCbSizeRead = width / 2 * sliceRows / 2;
            sliceCrSizeRead = width / 2 * sliceRows / 2;
        }
        else if(inputMode == OMX_COLOR_FormatYUV420PackedSemiPlanar)
        {
            sliceLumSizeRead = width * sliceRows;
            sliceCbSizeRead = width * sliceRows / 2;
        }
        else
        {
            sliceLumSizeRead = width * sliceRows * 2;
        }
    }

    /* Offset for frame start from start of file */
    frameOffset = frameSize * frameNum;
    /* Offset for slice luma start from start of frame */
    sliceLumOffset = sliceLumSize * sliceNum;
    /* Offset for slice cb start from start of frame */
    if(inputMode == OMX_COLOR_FormatYUV420PackedPlanar ||
       inputMode == OMX_COLOR_FormatYUV420PackedSemiPlanar)
        sliceCbOffset = width * height + sliceCbSize * sliceNum;
    /* Offset for slice cr start from start of frame */
    if(inputMode == OMX_COLOR_FormatYUV420PackedPlanar)
        sliceCrOffset = width * height +
            width / 2 * height / 2 + sliceCrSize * sliceNum;

    /* Read input from file frame by frame */
    if(file == NULL)
    {
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "Input file not provided\n");
        return -1;
    }

    fseek(file, frameOffset + sliceLumOffset, SEEK_SET);
    byteCount += fread(image, 1, sliceLumSizeRead, file);
    if(sliceCbSizeRead)
    {
        fseek(file, frameOffset + sliceCbOffset, SEEK_SET);
        byteCount += fread(image + sliceLumSizeRead, 1, sliceCbSizeRead, file);
    }
    if(sliceCrSizeRead)
    {
        fseek(file, frameOffset + sliceCrOffset, SEEK_SET);
        byteCount +=
            fread(image + sliceLumSizeRead + sliceCbSizeRead, 1,
                  sliceCrSizeRead, file);
    }

    /* Stop if last VOP of the file */
    if(feof(file))
    {
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "Can't read VOP no: %d\n",
                       frameNum);
        return -1;
    }

    return byteCount;
}

/**
 *
 */
OMX_ERRORTYPE omxclient_execute_yuv_range(OMXCLIENT * appdata,
                                          OMX_STRING input_filename,
                                          OMX_U32 * variance, OMX_U32 varlen,
                                          OMX_STRING output_filename,
                                          OMX_U32 firstVop, OMX_U32 lastVop)
{
    OMX_ERRORTYPE omxError;

    OMX_STATETYPE state;

    OMX_U32 varpos, last_pos;

    OMX_U32 src_img_size, vop, byte_count;

    OMX_PARAM_PORTDEFINITIONTYPE input_port;

    OMX_PARAM_PORTDEFINITIONTYPE output_port;

    omxError = OMX_ErrorNone;
    state = OMX_StateLoaded;
    varpos = 0;
    byte_count = 0;

    /* get port definitions */
    omxclient_struct_init(&input_port, OMX_PARAM_PORTDEFINITIONTYPE);
    omxclient_struct_init(&output_port, OMX_PARAM_PORTDEFINITIONTYPE);

    input_port.nPortIndex = 0;
    output_port.nPortIndex = 1;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (appdata->component, OMX_IndexParamPortDefinition,
                               &input_port), omxError);

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (appdata->component, OMX_IndexParamPortDefinition,
                               &output_port), omxError);

    /* open input and output files */
    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Using input '%s'\n", input_filename);
    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Using output '%s'\n",
                   output_filename);

    appdata->input = NULL;
    appdata->output = NULL;

    char error_string[256];

    memset(error_string, 0, sizeof(error_string));

    /* Open input file */
    appdata->input = fopen(input_filename, "rb");
    if(appdata->input == NULL)
    {
        strerror_r(errno, error_string, sizeof(error_string));
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "'%s'\n", error_string);

        return OMX_ErrorStreamCorrupt;
    }

    /* Open output file */
    appdata->output = fopen(output_filename, "wb");
    if(appdata->output == NULL)
    {
        strerror_r(errno, error_string, sizeof(error_string));
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "'%s'\n", error_string);

        return OMX_ErrorStreamCorrupt;
    }

    /* change component state */

    /* -> waiting for IDLE */
    OMXCLIENT_RETURN_ON_ERROR(omxclient_wait_state(appdata, OMX_StateIdle),
                              omxError);

    /* -> transition to EXECUTING */
    OMXCLIENT_RETURN_ON_ERROR(omxclient_change_state_and_wait
                              (appdata, OMX_StateExecuting), omxError);

    /* Tell component to ... fill these buffers */
    OMX_BUFFERHEADERTYPE *output_buffer = NULL;

    do
    {
        list_get_header(&(appdata->output_queue), &output_buffer);
        if(output_buffer)
        {
            output_buffer->nOutputPortIndex = 1;
            omxError = OMX_FillThisBuffer(appdata->component, output_buffer);

            if(omxError != OMX_ErrorNone)
            {
                return omxError;
            }
        }
    }
    while(output_buffer);

    /* run the decoder/encoder job */

    /* calculate input frame size */
    switch (input_port.format.video.eColorFormat)
    {

    case OMX_COLOR_FormatYUV420PackedPlanar:
    case OMX_COLOR_FormatYUV420PackedSemiPlanar:

        src_img_size =
            input_port.format.video.nStride *
            input_port.format.video.nFrameHeight +
            2 * (((input_port.format.video.nStride + 1) >> 1) *
                 ((input_port.format.video.nFrameHeight + 1) >> 1));
        break;

    case OMX_COLOR_Format16bitARGB4444:
    case OMX_COLOR_Format16bitARGB1555:
    case OMX_COLOR_Format16bitRGB565:
    case OMX_COLOR_Format16bitBGR565:
    case OMX_COLOR_FormatYUV422Planar:
    case OMX_COLOR_FormatCbYCrY:
    case OMX_COLOR_FormatYCbYCr:

        src_img_size =
            input_port.format.video.nStride *
            input_port.format.video.nFrameHeight * 2;
        break;

    default:
        return OMX_ErrorBadParameter;
    }

    last_pos = (lastVop + 1) * src_img_size;
    vop = omxclient_next_vop(Q16_FLOAT(input_port.format.video.xFramerate), 1,
                             Q16_FLOAT(output_port.format.video.xFramerate), 1,
                             0, firstVop);

    /* set input file to correct position */
    if(fseek(appdata->input, src_img_size * vop, SEEK_SET) != 0)
    {
        strerror_r(errno, error_string, sizeof(error_string));
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "'%s'\n", error_string);

        return OMX_ErrorStreamCorrupt;
    }

    appdata->EOS = OMX_FALSE;

    OMX_BOOL eof = OMX_FALSE;

    while(eof == OMX_FALSE)
    {
        OMX_BUFFERHEADERTYPE *input_buffer = NULL;

        /* Get input (synch) >> */
        OSAL_MutexLock(appdata->queue_mutex);
        {
            list_get_header(&appdata->input_queue, &input_buffer);
        }
        OSAL_MutexUnlock(appdata->queue_mutex);
        /* << Get input (synch) */

        if(input_buffer == NULL)
        {
            /* NOTE: output buffer not available, wait -> event */
            usleep(1000 * 1000);
            continue;
        }

        if(!appdata->input)
        {
            return OMX_ErrorInsufficientResources;
        }

        input_buffer->nInputPortIndex = 0;

        size_t ret;

        if(varpos < varlen)
        {

            /* check variance value */
            if(variance[varpos] > input_buffer->nAllocLen)
            {
                return OMX_ErrorBadParameter;
            }

            /* check last vop */
            if(byte_count + variance[varpos++] >= last_pos)
            {
                /* if variance value is too big,
                 * read only remaining before the end, and set stream end */

                ret = fread(input_buffer->pBuffer, 1,
                            last_pos - byte_count, appdata->input);

                /* feof does not indicate EOF if we don't read one byte more */
                if(ret == last_pos || ret == 0)
                {
                    input_buffer->nFlags |= OMX_BUFFERFLAG_EOS;
                }
            }
            else
            {
                /* read normally according to variance file */
                ret = fread(input_buffer->pBuffer, 1, variance[varpos++],
                            appdata->input);
            }
        }
        else
        {
            /* check last vop */
            if(byte_count + input_buffer->nAllocLen >= last_pos)
            {
                /* if next block to read is too big,
                 * read only remaining before the end, and set stream end */

                ret = fread(input_buffer->pBuffer, 1,
                            last_pos - byte_count, appdata->input);

                /* feof does not indicate EOF if we don't read one byte more */
                if(ret == last_pos || ret == 0)
                {
                    input_buffer->nFlags |= OMX_BUFFERFLAG_EOS;
                }
            }
            else
            {
                ret = fread(input_buffer->pBuffer, 1, input_buffer->nAllocLen,
                            appdata->input);
            }
        }

        byte_count += ret;

        input_buffer->nOffset = 0;
        input_buffer->nFilledLen = ret;

        if(input_buffer->nFlags & OMX_BUFFERFLAG_EOS ||
           (eof = feof(appdata->input)) != 0)
        {
            eof = OMX_TRUE;
        }

        if(eof)
        {
            input_buffer->nFlags |= OMX_BUFFERFLAG_EOS;
            printf("\tinput EOF reached\n");
        }

        omxError = OMX_EmptyThisBuffer(appdata->component, input_buffer);
        if(omxError != OMX_ErrorNone)
        {
            return omxError;
        }

        printf("\twrote %u bytes to component\n", ret);
        usleep(0);
    }

    /* get stream end event */
    while(appdata->EOS == OMX_FALSE)
    {
        usleep(1000);
    }

    return omxError;
}

OMX_ERRORTYPE omxlclient_initialize_execution(OMXCLIENT * appdata,
                                              OMX_STRING input_filename,
                                              OMX_STRING output_filename)
{
    OMX_ERRORTYPE omxError;

    OMX_STATETYPE state;

    OMX_PARAM_PORTDEFINITIONTYPE input_port;

    omxError = OMX_ErrorNone;
    state = OMX_StateLoaded;

    /* get port definitions */
    omxclient_struct_init(&input_port, OMX_PARAM_PORTDEFINITIONTYPE);
    input_port.nPortIndex = 0;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (appdata->component, OMX_IndexParamPortDefinition,
                               &input_port), omxError);

    /* open input and output files */
    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Using input '%s'\n", input_filename);
    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Using output '%s'\n",
                   output_filename);

    appdata->input = NULL;
    appdata->output = NULL;

    char error_string[256];

    memset(error_string, 0, sizeof(error_string));

    /* Open input file */
    appdata->input = fopen(input_filename, "rb");
    if(appdata->input == NULL)
    {
        strerror_r(errno, error_string, sizeof(error_string));
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "'%s'\n", error_string);

        return OMX_ErrorStreamCorrupt;
    }

    /* Open output file */
    appdata->output = fopen(output_filename, "wb");
    if(appdata->output == NULL)
    {
        strerror_r(errno, error_string, sizeof(error_string));
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "'%s'\n", error_string);

        return OMX_ErrorStreamCorrupt;
    }

    /* change component state */

    /* -> waiting for IDLE */
    OMXCLIENT_RETURN_ON_ERROR(omxclient_wait_state(appdata, OMX_StateIdle),
                              omxError);

    /* -> transition to EXECUTING */
    OMXCLIENT_RETURN_ON_ERROR(omxclient_change_state_and_wait
                              (appdata, OMX_StateExecuting), omxError);

    /* Tell component to ... fill these buffers */
    OMX_BUFFERHEADERTYPE *output_buffer = NULL;

    do
    {
        list_get_header(&(appdata->output_queue), &output_buffer);
        if(output_buffer)
        {
            output_buffer->nOutputPortIndex = 1;
            omxError = OMX_FillThisBuffer(appdata->component, output_buffer);

            if(omxError != OMX_ErrorNone)
            {
                return omxError;
            }
        }
    }
    while(output_buffer);

    return omxError;
}

/**
 *
 */
OMX_ERRORTYPE omxclient_execute_yuv_sliced(OMXCLIENT * appdata,
                                           OMX_STRING input_filename,
                                           OMX_STRING output_filename,
                                           OMX_U32 firstVop, OMX_U32 lastVop)
{
    OMX_ERRORTYPE omxError;

    OMX_STATETYPE state;

    OMX_U32 vop;

    OMX_PARAM_PORTDEFINITIONTYPE input_port;

    omxError = OMX_ErrorNone;
    state = OMX_StateLoaded;

    /* get port definitions */
    omxclient_struct_init(&input_port, OMX_PARAM_PORTDEFINITIONTYPE);
    input_port.nPortIndex = 0;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (appdata->component, OMX_IndexParamPortDefinition,
                               &input_port), omxError);

    /* open input and output files */
    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Using input '%s'\n", input_filename);
    OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "Using output '%s'\n",
                   output_filename);

    appdata->input = NULL;
    appdata->output = NULL;

    char error_string[256];

    memset(error_string, 0, sizeof(error_string));

    /* Open input file */
    appdata->input = fopen(input_filename, "rb");
    if(appdata->input == NULL)
    {
        strerror_r(errno, error_string, sizeof(error_string));
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "'%s'\n", error_string);

        return OMX_ErrorStreamCorrupt;
    }

    /* Open output file */
    appdata->output = fopen(output_filename, "wb");
    if(appdata->output == NULL)
    {
        strerror_r(errno, error_string, sizeof(error_string));
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "'%s'\n", error_string);

        return OMX_ErrorStreamCorrupt;
    }

    vop = firstVop;

    /* change component state */

    /* -> waiting for IDLE */
    OMXCLIENT_RETURN_ON_ERROR(omxclient_wait_state(appdata, OMX_StateIdle),
                              omxError);

    /* -> transition to EXECUTING */
    OMXCLIENT_RETURN_ON_ERROR(omxclient_change_state_and_wait
                              (appdata, OMX_StateExecuting), omxError);

    /* Tell component to ... fill these buffers */
    OMX_BUFFERHEADERTYPE *output_buffer = NULL;

    do
    {
        list_get_header(&(appdata->output_queue), &output_buffer);
        if(output_buffer)
        {
            output_buffer->nOutputPortIndex = 1;
            omxError = OMX_FillThisBuffer(appdata->component, output_buffer);

            if(omxError != OMX_ErrorNone)
            {
                return omxError;
            }
        }
    }
    while(output_buffer);

    /* run the decoder/encoder job */

    appdata->EOS = OMX_FALSE;
    OMX_BOOL eof = OMX_FALSE;

    OMX_U32 count = 0;

    OMX_U32 slice = 0;

    while(eof == OMX_FALSE)
    {
        OMX_BUFFERHEADERTYPE *input_buffer = NULL;

        /* Get input (synch) >> */
        OSAL_MutexLock(appdata->queue_mutex);
        {
            list_get_header(&appdata->input_queue, &input_buffer);
        }
        OSAL_MutexUnlock(appdata->queue_mutex);
        /* << Get input (synch) */

        if(input_buffer == NULL)
        {
            /* NOTE: output buffer not available, wait -> event */
            usleep(1000 * 1000);
            continue;
        }

        if(!appdata->input)
        {
            return OMX_ErrorInsufficientResources;
        }

        input_buffer->nInputPortIndex = 0;

        OMX_U32 read_count = (input_port.format.image.nSliceHeight == 0)
            ? input_port.format.image.nFrameHeight
            : input_port.format.image.nSliceHeight;

        OMX_U32 ret = omxclient_read_vop_sliced(input_buffer->pBuffer,
                                                input_port.format.image.nStride,
                                                input_port.format.image.
                                                nFrameHeight,
                                                slice,
                                                read_count,
                                                vop,
                                                appdata->input,
                                                input_port.format.image.
                                                eColorFormat);

        if(ret == -1)
        {
            eof = OMX_TRUE;
            input_buffer->nFlags |= OMX_BUFFERFLAG_EOS;
            ret = 0;

            OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "\tinput EOF reached\n");
        }
        else
        {
            count += read_count;
            if(count < input_port.format.image.nFrameHeight)
            {
                ++slice;
            }
            else
            {
                slice = 0;
                count = 0;
                ++vop;
            }
        }

        if(vop == lastVop && count == 0)
        {
            eof = OMX_TRUE;
            input_buffer->nFlags |= OMX_BUFFERFLAG_EOS;
        }

        input_buffer->nFilledLen = ret;
        input_buffer->nOffset = 0;

        omxError = OMX_EmptyThisBuffer(appdata->component, input_buffer);
        if(omxError != OMX_ErrorNone)
        {
            return omxError;
        }

        OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "\twrote %u bytes to component\n",
                       ret);

        usleep(0);
    }

    /* get stream end event */
    while(appdata->EOS == OMX_FALSE)
    {
        usleep(1000);
    }

    return omxError;
}

/**
 *
 */
OMX_ERRORTYPE omxclient_execute_yuv_sliced_(OMXCLIENT * appdata,
                                            OMX_U32 firstVop, OMX_U32 lastVop)
{
    OMX_ERRORTYPE omxError;

    OMX_STATETYPE state;

    OMX_PARAM_PORTDEFINITIONTYPE input_port;

    OMX_U32 vop;

    omxError = OMX_ErrorNone;
    state = OMX_StateLoaded;

    /* get port definitions */
    omxclient_struct_init(&input_port, OMX_PARAM_PORTDEFINITIONTYPE);
    input_port.nPortIndex = 0;

    OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter
                              (appdata->component, OMX_IndexParamPortDefinition,
                               &input_port), omxError);

    vop = firstVop;

    /* run the decoder/encoder job */

    appdata->EOS = OMX_FALSE;
    OMX_BOOL eof = OMX_FALSE;

    OMX_U32 count = 0;

    OMX_U32 slice = 0;

    while(eof == OMX_FALSE)
    {
        OMX_BUFFERHEADERTYPE *input_buffer = NULL;

        /* Get input (synch) >> */
        OSAL_MutexLock(appdata->queue_mutex);
        {
            list_get_header(&appdata->input_queue, &input_buffer);
        }
        OSAL_MutexUnlock(appdata->queue_mutex);
        /* << Get input (synch) */

        if(input_buffer == NULL)
        {
            /* NOTE: output buffer not available, wait -> event */
            usleep(1000 * 1000);
            continue;
        }

        if(!appdata->input)
        {
            return OMX_ErrorInsufficientResources;
        }

        input_buffer->nInputPortIndex = 0;

        OMX_U32 ret = omxclient_read_vop_sliced(input_buffer->pBuffer,
                                                input_port.format.image.nStride,
                                                input_port.format.image.
                                                nFrameHeight,
                                                slice,
                                                input_port.format.image.
                                                nSliceHeight,
                                                vop,
                                                appdata->input,
                                                input_port.format.image.
                                                eColorFormat);

        if(count < input_port.format.image.nFrameHeight)
        {
            count += input_port.format.image.nSliceHeight;
            ++slice;
        }
        else
        {
            slice = 0;
            count = 0;
            ++vop;
        }

        if(ret == -1 || vop == lastVop)
        {
            eof = OMX_TRUE;
            input_buffer->nFlags |= OMX_BUFFERFLAG_EOS;
            ret = 0;

            OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR, "\tinput EOF reached\n");
        }

        input_buffer->nFilledLen = ret;
        input_buffer->nOffset = 0;

        omxError = OMX_EmptyThisBuffer(appdata->component, input_buffer);
        if(omxError != OMX_ErrorNone)
        {
            return omxError;
        }

        OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG, "\twrote %u bytes to component\n",
                       ret);

        usleep(0);
    }

    /* get stream end event */
    while(appdata->EOS == OMX_FALSE)
    {
        usleep(1000);
    }

    return omxError;
}

OMX_ERRORTYPE omxlclient_initialize_buffers(OMXCLIENT * client)
{
    OMX_U32 i, j;

    OMX_ERRORTYPE omxError;

    OMX_PARAM_PORTDEFINITIONTYPE port;

    OSAL_EventReset(client->state_event);

    // create the buffers
    OMXCLIENT_RETURN_ON_ERROR(OMX_SendCommand
                              (client->component, OMX_CommandStateSet,
                               OMX_StateIdle, NULL), omxError);

    for(j = 0; j < 2; ++j)
    {
        omxclient_struct_init(&port, OMX_PARAM_PORTDEFINITIONTYPE);
        port.nPortIndex = j;

        OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter(client->component,
                                                   OMX_IndexParamPortDefinition,
                                                   (OMX_PTR) & port), omxError);

        /* allocate input buffers */
        for(i = 0; i < port.nBufferCountActual; ++i)
        {
            OMX_BUFFERHEADERTYPE *header = 0;

            OMXCLIENT_RETURN_ON_ERROR(OMX_AllocateBuffer(client->component,
                                                         &header,
                                                         port.nPortIndex, 0,
                                                         port.nBufferSize),
                                      omxError);

            switch (port.eDir)
            {
            case OMX_DirInput:
                list_push_header(&(client->input_queue), header);
                break;

            case OMX_DirOutput:
                list_push_header(&(client->output_queue), header);
                break;

            case OMX_DirMax:
            default:
                assert(!"Port direction may not be 'OMX_DirMax'.");
                break;
            }
        }
    }

    return omxError;
}

OMX_ERRORTYPE omxlclient_initialize_buffers_fixed(OMXCLIENT * client,
                                                  OMX_U32 size, OMX_U32 count)
{
    OMX_U32 i, j;

    OMX_ERRORTYPE omxError;

    OSAL_EventReset(client->state_event);

    // create the buffers
    OMXCLIENT_RETURN_ON_ERROR(OMX_SendCommand
                              (client->component, OMX_CommandStateSet,
                               OMX_StateIdle, NULL), omxError);

    for(j = 0; j < 2; ++j)
    {
        /* allocate input buffers */
        for(i = 0; i < count; ++i)
        {
            OMX_BUFFERHEADERTYPE *header = 0;

            OMXCLIENT_RETURN_ON_ERROR(OMX_AllocateBuffer(client->component,
                                                         &header, j, 0, size),
                                      omxError);

            switch (j)
            {
            case 0:
                list_push_header(&(client->input_queue), header);
                break;
            case 1:
                list_push_header(&(client->output_queue), header);
                break;
            }
        }
    }

    return omxError;
}

OMX_ERRORTYPE omxclient_component_initialize(OMXCLIENT * appdata,
                                             OMX_ERRORTYPE
                                             (*port_configurator_fn) (OMXCLIENT
                                                                      * appdata,
                                                                      OMX_PARAM_PORTDEFINITIONTYPE
                                                                      * port))
{
    OMX_ERRORTYPE omxError;

    OMX_PORT_PARAM_TYPE oPortParam;

    OMX_PARAM_PORTDEFINITIONTYPE sPortDef[2];

    OMX_U32 i;

    omxclient_struct_init(&oPortParam, OMX_PORT_PARAM_TYPE);

    /* detect all video ports of component */
    omxError =
        OMX_GetParameter(appdata->component, OMX_IndexParamVideoInit,
                         &oPortParam);
    if(omxError == OMX_ErrorNone)
    {
        if(oPortParam.nPorts > 0x00)
        {
            OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG,
                           "Detected %i video ports starting at %i\n",
                           oPortParam.nPorts, oPortParam.nStartPortNumber);

            omxclient_struct_init(&(sPortDef[0]), OMX_PARAM_PORTDEFINITIONTYPE);
            omxclient_struct_init(&(sPortDef[1]), OMX_PARAM_PORTDEFINITIONTYPE);

            sPortDef[0].nPortIndex = oPortParam.nStartPortNumber + 0;
            sPortDef[1].nPortIndex = oPortParam.nStartPortNumber + 1;

            /* NOTE: should the ports be known beforehand? */

            for(i = 0; i < 2; ++i)
            {
                OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter(appdata->component,
                                                           OMX_IndexParamPortDefinition,
                                                           (OMX_PTR) &
                                                           (sPortDef[i])),
                                          omxError);

                OMXCLIENT_RETURN_ON_ERROR(port_configurator_fn
                                          (appdata, &(sPortDef[i])), omxError);

                OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter(appdata->component,
                                                           OMX_IndexParamPortDefinition,
                                                           &(sPortDef[i])),
                                          omxError);

                OMX_OSAL_TracePortSettings(OMX_OSAL_TRACE_INFO, &(sPortDef[i]));
            }

        }
        else
        {
            OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                           "Component has no video ports.\n", oPortParam.nPorts,
                           oPortParam.nStartPortNumber);
        }
    }
    else
    {
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                       "Initializing video port parameters failed: '%s'\n",
                       OMX_OSAL_TraceErrorStr(omxError));

        /* error propagated onwards */
    }

    return omxError;
}

OMX_ERRORTYPE omxclient_component_initialize_jpeg(OMXCLIENT * appdata,
                                                  OMX_ERRORTYPE
                                                  (*port_configurator_fn)
                                                  (OMXCLIENT * appdata,
                                                   OMX_PARAM_PORTDEFINITIONTYPE
                                                   * port))
{
    OMX_ERRORTYPE omxError;

    OMX_PORT_PARAM_TYPE oPortParam;

    OMX_PARAM_PORTDEFINITIONTYPE sPortDef[2];

    OMX_U32 i;

    omxclient_struct_init(&oPortParam, OMX_PORT_PARAM_TYPE);

    /* detect all video ports of component */
    omxError =
        OMX_GetParameter(appdata->component, OMX_IndexParamImageInit,
                         &oPortParam);
    if(omxError == OMX_ErrorNone)
    {
        if(oPortParam.nPorts > 0x00)
        {
            OMX_OSAL_Trace(OMX_OSAL_TRACE_DEBUG,
                           "Detected %i video ports starting at %i\n",
                           oPortParam.nPorts, oPortParam.nStartPortNumber);

            omxclient_struct_init(&(sPortDef[0]), OMX_PARAM_PORTDEFINITIONTYPE);
            omxclient_struct_init(&(sPortDef[1]), OMX_PARAM_PORTDEFINITIONTYPE);

            sPortDef[0].nPortIndex = oPortParam.nStartPortNumber + 0;
            sPortDef[1].nPortIndex = oPortParam.nStartPortNumber + 1;

            /* NOTE: should the ports be known beforehand? */

            for(i = 0; i < 2; ++i)
            {
                OMXCLIENT_RETURN_ON_ERROR(OMX_GetParameter(appdata->component,
                                                           OMX_IndexParamPortDefinition,
                                                           (OMX_PTR) &
                                                           (sPortDef[i])),
                                          omxError);

                port_configurator_fn(appdata, &(sPortDef[i]));

                OMXCLIENT_RETURN_ON_ERROR(OMX_SetParameter(appdata->component,
                                                           OMX_IndexParamPortDefinition,
                                                           &(sPortDef[i])),
                                          omxError);

                OMX_OSAL_TracePortSettings(OMX_OSAL_TRACE_INFO, &(sPortDef[i]));
            }

        }
        else
        {
            OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                           "Component has no image ports.\n", oPortParam.nPorts,
                           oPortParam.nStartPortNumber);
        }
    }
    else
    {
        OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                       "Initializing video port parameters failed: '%s'\n",
                       OMX_OSAL_TraceErrorStr(omxError));

        /* error propagated onwards */
    }

    return omxError;
}

/**
 *
 */
OMX_ERRORTYPE omxclient_check_component_version(OMX_IN OMX_HANDLETYPE
                                                hComponent)
{
    OMX_ERRORTYPE omxError;

    OMX_STRING componentName;

    OMX_VERSIONTYPE componentVersion;

    OMX_VERSIONTYPE specVersion;

    OMX_UUIDTYPE uuid;

    componentName = (OMX_STRING) OSAL_Malloc(OMX_MAX_STRINGNAME_SIZE);

    if(componentName)
    {
        omxError = OMX_GetComponentVersion(hComponent, componentName,
                                           &componentVersion, &specVersion,
                                           &uuid);

        if(omxError == OMX_ErrorNone)
        {
            OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                           "Tester IL-Specification version: %d.%d.%d.%d\n",
                           OMX_VERSION_MAJOR, OMX_VERSION_MINOR,
                           OMX_VERSION_REVISION, OMX_VERSION_STEP);

            OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                           "Component IL-Specification version: %d.%d.%d.%d\n",
                           specVersion.s.nVersionMajor,
                           specVersion.s.nVersionMinor, specVersion.s.nRevision,
                           specVersion.s.nStep);

            OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO,
                           "Component version: %d.%d.%d.%d\n",
                           componentVersion.s.nVersionMajor,
                           componentVersion.s.nVersionMinor,
                           componentVersion.s.nRevision,
                           componentVersion.s.nStep);

            /* OMX_OSAL_Trace(OMX_OSAL_TRACE_INFO, "Component uuid: %s\n", uuid); */

            if(specVersion.s.nVersionMajor != OMX_VERSION_MAJOR ||
               specVersion.s.nVersionMinor != OMX_VERSION_MINOR ||
               specVersion.s.nRevision != OMX_VERSION_REVISION ||
               specVersion.s.nStep != OMX_VERSION_STEP)
            {
                OMX_OSAL_Trace(OMX_OSAL_TRACE_ERROR,
                               "Used OMX IL-specification versions differ\n");
                omxError = OMX_ErrorVersionMismatch;
            }
        }

        OSAL_Free((OMX_PTR) componentName);
    }

    return omxError;
}
