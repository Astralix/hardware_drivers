/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/




#include "gc_egl_precomp.h"
#if defined(ANDROID)
#include <ui/android_native_buffer.h>
#include "gc_gralloc_priv.h"
#endif

#ifdef __QNXNTO__
#include "gc_hal_user.h"
#endif

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcdZONE_EGL_SWAP

#if gcdDUMP_FRAME_TGA

#ifdef ANDROID
#    include <cutils/properties.h>
#endif

#ifdef UNDERCE
#    include <winbase.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONF_MAX_LEN 256
#define STRING_LEN   1024

static void
_SaveFrameTga(
    VEGLThreadData thread,
    VEGLSurface draw,
    gcsPOINT_PTR RectOrigin,
    gcsPOINT_PTR RectSize
    )
{
    do
    {
        gcoSURF resolve;
        gctUINT8_PTR frameBGR, frameARGB;
        gctUINT8 tgaHeader[18];
        gctPOINTER logical = gcvNULL, frameMemory;
        gceSTATUS status;
        gctINT32 resolveStride, x, y;
        gctFILE file = 0;
        static gctUINT32 frameCount = 0;
        gctSTRING fileName, out_dir,l,p;
        gctUINT fileNameOffset = 0, k = 0, i = 0, a = 0, o = 0, flag = 0;
        gctINT32 frame_start = 0, frame_end = 0;
        char array_string[STRING_LEN]="\0";
        char array_temp[20] = "\0";
        static gctINT32 frame_array[STRING_LEN]={0};

#ifdef UNDER_CE
        TCHAR moduleName[CONF_MAX_LEN];
        char cur[CONF_MAX_LEN],conf[CONF_MAX_LEN], *p1;
#endif

        frameCount++;
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, CONF_MAX_LEN, (gctPOINTER *)&out_dir));

#ifdef UNDER_CE
        GetModuleFileName(NULL, moduleName, CONF_MAX_LEN);
        wcstombs(cur, moduleName, CONF_MAX_LEN);
        strcpy(conf,cur);
        p1 = strrchr(conf, '\\');
        strcpy(p1 + 1, "drv_config.ini");
        gcmERR_BREAK(gcoOS_Open(gcvNULL, conf, gcvFILE_READ, &file));
#else
        gctSTRING conf;
#if defined(ANDROID)
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, PROPERTY_VALUE_MAX, (gctPOINTER *)&conf));
        if(0 == property_get("drv.config",conf,NULL))
        {
            gcoOS_Open(gcvNULL, "/sdcard/drv_config.ini", gcvFILE_READ, &file);
        }
        else
        {
            gcoOS_Open(gcvNULL, conf, gcvFILE_READ, &file);
        }
        gcmERR_BREAK(gcmOS_SAFE_FREE(gcvNULL, conf));
#else
        conf = getenv("DRV_CONFIG");
        if(!conf)
        {
            gcoOS_Open(gcvNULL, "./drv_config.ini", gcvFILE_READ, &file);
        }
        else
        {
            gcoOS_Open(gcvNULL, conf, gcvFILE_READ, &file);
        }
#endif
#endif

        if(file)
        {
            a = fscanf(file, "FRAME_ARRAY = %s\n", array_string);
            o = fscanf(file, "OUT_DIR = %s\n", out_dir);
            fscanf(file, "FRAME_START = %d\n", &frame_start);
            fscanf(file, "FRAME_END = %d\n", &frame_end);
            gcmERR_BREAK(gcoOS_Close(gcvNULL, file));
        }
        else
        {
            a = 0;
            o = 0;
            frame_start = 0;
            frame_end = 1000;
        }

        if (a != 0)
        {
            p = array_string;
            do
            {
                l=strstr(p,",");
                strncpy(array_temp,p,(l-p));
                frame_array[i++] = atoi(array_temp);
                p=l+1;
            }
            while (l < (p + strlen(p) - 1));

            flag = 0;
            for (k = 0; k < i; k++)
            {
                if((frame_array[k] >= 0)
                && (frameCount == (gctUINT32)frame_array[k]))
                {
                    frame_array[k] = -1;
                    flag = 1;
                }
            }
            if (flag == 0)
            {
                break;
            }

        }
        else
        {
           if ((frameCount < (gctUINT32)frame_start) && (frame_start > 0))
           {
                break;
           }

           if ((frameCount > (gctUINT32)frame_end) && (frame_end > 0))
           {
                break;
           }
        }

        if(o == 0)
        {
#ifdef UNDER_CE
            gcmVERIFY_OK(
            gcoOS_PrintStrSafe(out_dir, 10, &fileNameOffset,
                               cur));
#else
#if defined(ANDROID)
            gcmVERIFY_OK(
            gcoOS_PrintStrSafe(out_dir, 10, &fileNameOffset,
                               "/sdcard/"));
#else
            gcmVERIFY_OK(
            gcoOS_PrintStrSafe(out_dir, 10, &fileNameOffset,
                               "./"));
#endif
#endif
        }

        fileNameOffset = 0;

        gcmERR_BREAK(gcoSURF_Construct(
                    gcvNULL,
                    RectSize->x,
                    RectSize->y,
                    1,
                    gcvSURF_BITMAP,
                    gcvSURF_A8R8G8B8,
                    gcvPOOL_DEFAULT,
                    &resolve));
        gcmERR_BREAK(gcoSURF_ResolveRect(
                    draw->renderTarget,
                    resolve,
                    RectOrigin,
                    RectOrigin,
                    RectSize
                    ));

        gcmERR_BREAK(gcoHAL_Commit(gcvNULL, gcvTRUE));
        gcmERR_BREAK(gcoSURF_Lock(resolve, gcvNULL, &logical));
        gcmERR_BREAK(gcoSURF_GetAlignedSize(resolve, gcvNULL, gcvNULL, &resolveStride));
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, RectSize->x * RectSize->y * 3, &frameMemory));

        frameBGR = (gctUINT8_PTR)frameMemory;
        for(y = 0; y < RectSize->y; ++y)
        {
            frameARGB = (gctUINT8_PTR)((gctUINT32)logical + y * resolveStride);
            for(x = 0; x < RectSize->x; ++x)
            {
                frameBGR[0] = frameARGB[0];
                frameBGR[1] = frameARGB[1];
                frameBGR[2] = frameARGB[2];
                frameARGB += 4;
                frameBGR  += 3;
            }
        }

        gcmERR_BREAK(gcoSURF_Unlock(resolve, &logical));
        gcmERR_BREAK(gcoSURF_Destroy(resolve));

        tgaHeader[ 0] = 0;
        tgaHeader[ 1] = 0;
        tgaHeader[ 2] = 2;
        tgaHeader[ 3] = 0;
        tgaHeader[ 4] = 0;
        tgaHeader[ 5] = 0;
        tgaHeader[ 6] = 0;
        tgaHeader[ 7] = 0;
        tgaHeader[ 8] = 0;
        tgaHeader[ 9] = 0;
        tgaHeader[10] = 0;
        tgaHeader[11] = 0;
        tgaHeader[12] = (RectSize->x  & 0x00ff);
        tgaHeader[13] = (RectSize->x  & 0xff00) >> 8;
        tgaHeader[14] = (RectSize->y & 0x00ff);
        tgaHeader[15] = (RectSize->y & 0xff00) >> 8;
        tgaHeader[16] = 24;
        tgaHeader[17] = 0;
        if ((thread->api == EGL_OPENGL_ES_API)
         && (thread->context->client == 1))
        {
            tgaHeader[17] = 0x1 << 5;
        }
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, CONF_MAX_LEN, (gctPOINTER *)&fileName));

        gcmVERIFY_OK(
            gcoOS_PrintStrSafe(fileName, CONF_MAX_LEN, &fileNameOffset,
                              out_dir));

        gcmVERIFY_OK(
            gcoOS_PrintStrSafe(fileName, CONF_MAX_LEN, &fileNameOffset,
                               "frame_%d.tga", frameCount));

        gcmERR_BREAK(gcoOS_Open(gcvNULL, fileName, gcvFILE_CREATE, &file));
        gcmERR_BREAK(gcoOS_Write(gcvNULL, file, 18, tgaHeader));
        gcmERR_BREAK(gcoOS_Write(gcvNULL, file, RectSize->x * RectSize->y * 3, frameMemory));
        gcmERR_BREAK(gcoOS_Close(gcvNULL, file));
        gcmERR_BREAK(gcmOS_SAFE_FREE(gcvNULL, frameMemory));
        gcmERR_BREAK(gcmOS_SAFE_FREE(gcvNULL, fileName));
        gcmERR_BREAK(gcmOS_SAFE_FREE(gcvNULL, out_dir));
    }
    while(gcvFALSE);
}
#endif

static void
_DumpSurface(
    gcoDUMP Dump,
    gcoSURF Surface,
    gceDUMP_TAG Type
    )
{
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gctUINT width, height;
    gctINT stride;
    gceSTATUS status;

    if (Surface != gcvNULL)
    {
        gcmONERROR(gcoSURF_Lock(
            Surface, address, memory
            ));

        gcmONERROR(gcoSURF_GetAlignedSize(
            Surface, &width, &height, &stride
            ));

        gcmONERROR(gcoDUMP_DumpData(
            Dump, Type, address[0], height * stride, memory[0]
            ));

        gcmONERROR(gcoSURF_Unlock(
            Surface, memory[0]
            ));
        memory[0] = gcvNULL;
    }

OnError:
    if (memory[0] != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(Surface, memory[0]));
    }

    return;
}

VEGLWorkerInfo
veglGetWorker(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display,
    IN VEGLSurface Surface
    )
{
    gceSTATUS status;
    gctBOOL acquired = gcvFALSE;
    VEGLWorkerInfo worker;

    /* Get access to avaliable worker list. */
    gcmONERROR(gcoOS_AcquireMutex(gcvNULL, Surface->workerMutex, gcvINFINITE));
    acquired = gcvTRUE;

    /* Surface we have available workers? */
    if (Surface->availableWorkers != gcvNULL)
    {
        /* Yes, pick the first one and use it. */
        worker = Surface->availableWorkers;
        Surface->availableWorkers = worker->next;
    }
    else
    {
        /* No, get the last submitted worker. */
        worker = Surface->lastSubmittedWorker;

        /* Remove the worker from display worker list. */
        worker->prev->next = worker->next;
        worker->next->prev = worker->prev;
    }

    /* Set Worker as busy. */
    worker->draw = Surface;

    /* Release access mutex. */
    gcmONERROR(gcoOS_ReleaseMutex(gcvNULL, Surface->workerMutex));
    acquired = gcvFALSE;

    /* Create worker's signal. */
    if (worker->signal == gcvNULL)
    {
        gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvFALSE, &worker->signal));

        gcmTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_SIGNAL,
            "%s(%d): worker thread signal created 0x%08X",
            __FUNCTION__, __LINE__,
            worker->signal
            );
    }

    /* Return the worker. */
    return worker;

OnError:
    if (acquired)
    {
        gcmVERIFY_OK(gcoOS_ReleaseMutex(gcvNULL, Surface->workerMutex));
    }

    return gcvNULL;
}

gceSTATUS
veglReleaseWorker(
    VEGLWorkerInfo Worker
    )
{
    gceSTATUS status;
    VEGLSurface ownerSurface;
    gctBOOL acquired = gcvFALSE;

    /* Get a shortcut to the thread. */
    ownerSurface = Worker->draw;

    /* Get access to avaliable worker list. */
    gcmONERROR(gcoOS_AcquireMutex(gcvNULL, ownerSurface->workerMutex, gcvINFINITE));
    acquired = gcvTRUE;

    /* Add it back to the thread available worker list. */
    Worker->next = ownerSurface->availableWorkers;
    ownerSurface->availableWorkers = Worker;

    /* Set Worker as available. */
    Worker->draw = gcvNULL;

    /* Release access mutex. */
    gcmONERROR(gcoOS_ReleaseMutex(gcvNULL, ownerSurface->workerMutex));
    acquired = gcvFALSE;

OnError:
    if (acquired)
    {
        gcmVERIFY_OK(gcoOS_ReleaseMutex(gcvNULL, ownerSurface->workerMutex));
    }

    return status;
}

VEGLWorkerInfo
veglFreeWorker(
    VEGLWorkerInfo Worker
    )
{
    gceSTATUS status;
    VEGLSurface ownerSurface;
    gctBOOL acquired = gcvFALSE;
    VEGLWorkerInfo nextWorker;

    /* Get a shortcut to the surface. */
    ownerSurface = Worker->draw;

    /* Get the next worker. */
    nextWorker = Worker->next;

    /* Remove worker from display worker list. */
    Worker->prev->next = Worker->next;
    Worker->next->prev = Worker->prev;

    /* Release the surface. */
    if (Worker->targetSignal != gcvNULL)
    {
        gcmONERROR(gcoOS_Signal(gcvNULL, Worker->targetSignal, gcvTRUE));
    }

    /* Get access to avaliable worker list. */
    gcmONERROR(gcoOS_AcquireMutex(gcvNULL, ownerSurface->workerMutex, gcvINFINITE));
    acquired = gcvTRUE;

    /* Add it back to the thread available worker list. */
    Worker->next = ownerSurface->availableWorkers;
    ownerSurface->availableWorkers = Worker;

    /* Set Worker as available. */
    Worker->draw = gcvNULL;

    /* Release access mutex. */
    gcmONERROR(gcoOS_ReleaseMutex(gcvNULL, ownerSurface->workerMutex));
    acquired = gcvFALSE;

    return nextWorker;

OnError:
    if (acquired)
    {
        gcmVERIFY_OK(gcoOS_ReleaseMutex(gcvNULL, ownerSurface->workerMutex));
    }

    return gcvNULL;
}

#if defined(ANDROID)
/* In Android, for 3D apps we don't issue resolves during swap.
   So we don't need to schedule events, we directly raise signals
   for worker thread instead.
 */
gctBOOL
veglSubmitWorkerFromCPU(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display,
    IN VEGLWorkerInfo Worker
    )
{
    Worker->prev =  Display->workerSentinel.prev;
    Worker->next = &Display->workerSentinel;

    Display->workerSentinel.prev->next = Worker;
    Display->workerSentinel.prev       = Worker;

    Worker->draw->lastSubmittedWorker = Worker;

    if (gcmIS_ERROR(gcoOS_Signal(gcvNULL, Worker->signal, gcvTRUE)))
    {
        /* Bad surface. */
        Thread->error = EGL_BAD_SURFACE;
        return gcvFALSE;
    }

    if (gcmIS_ERROR(gcoOS_Signal(gcvNULL, Display->startSignal, gcvTRUE)))
    {
        /* Bad surface. */
        Thread->error = EGL_BAD_SURFACE;
        return gcvFALSE;
    }

    return gcvTRUE;
}
#endif

gctBOOL
veglSubmitWorker(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display,
    IN VEGLWorkerInfo Worker,
    IN gctBOOL ScheduleSignals
    )
{
    Worker->prev =  Display->workerSentinel.prev;
    Worker->next = &Display->workerSentinel;

    Display->workerSentinel.prev->next = Worker;
    Display->workerSentinel.prev       = Worker;

    Worker->draw->lastSubmittedWorker = Worker;

    if (ScheduleSignals)
    {
#if gcdENABLE_VG
        if (Thread->openVGpipe && Thread->api == EGL_OPENVG_API)
        {
            gcsTASK_SIGNAL_PTR workerSignal;
            gcsTASK_SIGNAL_PTR startSignal;

            /* Allocate a cluster event. */
            if (gcmIS_ERROR(gcoHAL_ReserveTask(gcvNULL,
                                               gcvBLOCK_PIXEL,
                                               2,
                                               gcmSIZEOF(gcsTASK_SIGNAL) * 2,
                                               (gctPOINTER *) &workerSignal)))
            {
                /* Bad surface. */
                Thread->error = EGL_BAD_SURFACE;
                return gcvFALSE;
            }

            /* Determine the start signal set task pointer. */
            startSignal = (gcsTASK_SIGNAL_PTR) (workerSignal + 1);

            /* Fill in event info. */
            workerSignal->id      = gcvTASK_SIGNAL;
            workerSignal->process = Display->process;
            workerSignal->signal  = Worker->signal;

            startSignal->id       = gcvTASK_SIGNAL;
            startSignal->process  = Display->process;
            startSignal->signal   = Display->startSignal;
        }
        else
#endif
        {
#if COMMAND_PROCESSOR_VERSION > 1
            gcsTASK_SIGNAL_PTR workerSignal;
            gcsTASK_SIGNAL_PTR startSignal;

            /* Allocate a cluster event. */
            if (gcmIS_ERROR(gcoHAL_ReserveTask(gcvNULL,
                                               gcvBLOCK_PIXEL,
                                               2,
                                               gcmSIZEOF(gcsTASK_SIGNAL) * 2,
                                               (gctPOINTER *) &workerSignal)))
            {
                /* Bad surface. */
                Thread->error = EGL_BAD_SURFACE;
                return gcvFALSE;
            }

            /* Determine the start signal set task pointer. */
            startSignal = (gcsTASK_SIGNAL_PTR) (workerSignal + 1);

            /* Fill in event info. */
            workerSignal->id      = gcvTASK_SIGNAL;
            workerSignal->process = Display->process;
            workerSignal->signal  = Worker->signal;

            startSignal->id       = gcvTASK_SIGNAL;
            startSignal->process  = Display->process;
            startSignal->signal   = Display->startSignal;
#else
            gcsHAL_INTERFACE iface;

            iface.command            = gcvHAL_SIGNAL;
            iface.u.Signal.signal    = Worker->signal;
            iface.u.Signal.auxSignal = gcvNULL;
            iface.u.Signal.process   = Display->process;
            iface.u.Signal.fromWhere = gcvKERNEL_PIXEL;

            /* Schedule the event. */
            if (gcmIS_ERROR(gcoHAL_ScheduleEvent(gcvNULL, &iface)))
            {
                /* Bad surface. */
                Thread->error = EGL_BAD_SURFACE;
                return gcvFALSE;
            }

            iface.command            = gcvHAL_SIGNAL;
            iface.u.Signal.signal    = Display->startSignal;
            iface.u.Signal.auxSignal = gcvNULL;
            iface.u.Signal.process   = Display->process;
            iface.u.Signal.fromWhere = gcvKERNEL_PIXEL;

            /* Schedule the event. */
            if (gcmIS_ERROR(gcoHAL_ScheduleEvent(gcvNULL, &iface)))
            {
                /* Bad surface. */
                Thread->error = EGL_BAD_SURFACE;
                return gcvFALSE;
            }
#endif
        }
    }

    return gcvTRUE;
}

/* Worker thread to copy resolve buffer to display. */
veglTHREAD_RETURN WINAPI
veglSwapWorker(
    void* Display
    )
{
    VEGLDisplay display;
    VEGLWorkerInfo displayWorker;
    VEGLWorkerInfo currWorker;
    gctINT i;

    gcmHEADER_ARG("Display=0x%x", Display);

    /* Cast the display object. */
    display = VEGL_DISPLAY(Display);

    while (gcvTRUE)
    {
        /* Wait for the start signal. */
        if gcmIS_ERROR(gcoOS_WaitSignal(gcvNULL, display->startSignal, gcvINFINITE))
        {
            break;
        }

        /* Check the thread's stop signal. */
        if (gcmIS_SUCCESS(gcoOS_WaitSignal(gcvNULL, display->stopSignal, 0)))
        {
            /* Stop had been signaled, exit. */
            break;
        }

        /* Acquire synchronization mutex. */
        veglSuspendSwapWorker(display);

        for (
            currWorker = display->workerSentinel.next;
            (currWorker != gcvNULL) && (currWorker->draw != gcvNULL);
        )
        {
#if defined(ANDROID)
            /* Wait for the worker's surface. */
            if (gcmIS_ERROR(gcoOS_WaitSignal(gcvNULL, currWorker->signal, gcvINFINITE)))
            {
                break;
            }

            /* The current worker is the one to be displayed. */
            displayWorker = currWorker;
#else
            VEGLWorkerInfo nextWorker;

            /* Is the worker's surface still being processed? */
            if (gcmIS_ERROR(gcoOS_WaitSignal(gcvNULL, currWorker->signal, 0)))
            {
                /* Yes, skip it. */
                currWorker = currWorker->next;
                continue;
            }

            /* Assume the current worker is the one to be displayed. */
            displayWorker = currWorker;

            /* Check if next frames are available. */
            for (
                nextWorker = currWorker->next;
                nextWorker->draw != gcvNULL;
                nextWorker = nextWorker->next
            )
            {
                /* Skip workers from other surfaces. */
                if (nextWorker->draw != currWorker->draw)
                {
                    continue;
                }

                /* Is the next worker ready? */
                if (gcmIS_ERROR(gcoOS_WaitSignal(gcvNULL, nextWorker->signal, 0)))
                {
                    /* Not yet. */
                    break;
                }

                /* Have we previously found a ready worker? */
                if (displayWorker != currWorker)
                {
                    /* Yes, ignore it. */
                    veglFreeWorker(displayWorker);
                }

                /* Store the current worker. */
                displayWorker = nextWorker;
            }
#endif
            gcmDUMP_FRAMERATE();

            if (displayWorker->draw->fbDirect)
            {
                if (displayWorker->numRects == 1)
                {
                    veglSetDisplayFlip(
                        display->hdc,
                        displayWorker->draw->hwnd,
                        &displayWorker->backBuffer
                        );
                }
                else
                {
                    veglSetDisplayFlipRegions(
                        display->hdc,
                        displayWorker->draw->hwnd,
                        &displayWorker->backBuffer,
                        displayWorker->numRects,
                        displayWorker->rects);
                }
            }
            else
            {
                for (i = 0; i < displayWorker->numRects; ++i)
                {
                    veglDrawImage(
                        display,
                        displayWorker->draw,
                        displayWorker->bits,
                        displayWorker->rects[4*i],
                        displayWorker->rects[4*i+1],
                        displayWorker->rects[4*i+2],
                        displayWorker->rects[4*i+3]
                        );
                }
            }

            /* Free the more recent worker. */
            if (displayWorker != currWorker)
            {
                veglFreeWorker(displayWorker);
            }

            /* Free the current worker. */
            currWorker = veglFreeWorker(currWorker);
        }

        /* Release synchronization mutex. */
        veglResumeSwapWorker(display);
    }

    gcmFOOTER_ARG("return=%ld", (veglTHREAD_RETURN) 0);
    /* Success. */
    return (veglTHREAD_RETURN) 0;
}

#ifdef __QNXNTO__
static void
_SurfaceClipAlignRect(
    const gcsSURF_INFO_PTR surfInfo,
    EGLint* rect
    )
{
    EGLint x0 = rect[0];
    EGLint y0 = rect[1];
    EGLint x1 = x0 + rect[2];
    EGLint y1 = y0 + rect[3];

    if (x0 < 0)
        x0 = 0;

    if (y0 < 0)
        y0 = 0;

    if (x1 > surfInfo->rect.right)
        x1 = surfInfo->rect.right;

    if (y1 > surfInfo->rect.bottom)
        y1 = surfInfo->rect.bottom;

    rect[0] = x0;
    rect[1] = y0;
    rect[2] = x1 - x0;
    rect[3] = y1 - y0;
}
#endif

void
veglSuspendSwapWorker(
    VEGLDisplay Display
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x", Display);

    gcmASSERT(Display != gcvNULL);
    if (Display->suspendMutex != gcvNULL)
    {
        gcmONERROR(gcoOS_AcquireMutex(gcvNULL,
                                        Display->suspendMutex,
                                        gcvINFINITE));
    }
    gcmFOOTER_NO();
    return;

OnError:
    gcmFOOTER();
    return;
}

static EGLBoolean
_SwapBuffersRegion(
    VEGLThreadData Thread,
    EGLDisplay Dpy,
    EGLSurface Draw,
    EGLint NumRects,
    const EGLint* Rects
    )
{
    EGLBoolean result;
    VEGLThreadData thread;

    do
    {
        gceSTATUS status = gcvSTATUS_OK;
        VEGLDisplay dpy;
        VEGLSurface draw;
        gctPOINTER bits = gcvNULL;
        gctINT i;
        gcsPOINT srcOrigin, dstOrigin, size;

        /* Create shortcuts to objects. */
        thread = Thread;
        dpy    = VEGL_DISPLAY(Dpy);
        draw   = VEGL_SURFACE(Draw);

        /* Only window surfaces need to be swapped. */
        if ((gctPOINTER) draw->hwnd == gcvNULL)
        {
            /* Not a window surface. */
            thread->error = EGL_SUCCESS;
            break;
        }

        /* Only back buffers need to be posted. */
        if (draw->buffer != EGL_BACK_BUFFER)
        {
            /* Not a back buffer. */
            thread->error = EGL_SUCCESS;
            break;
        }

#if defined(ANDROID)
       /* EGL_ANDROID_get_render_buffer. */
       if (draw->backBuffer.context != gcvNULL)
       {
           VEGLWorkerInfo worker;

           /* Suspend the worker thread. */
           veglSuspendSwapWorker(dpy);

           /* Find an available worker. */
           worker = veglGetWorker(thread, dpy, draw);
           if (worker == gcvNULL)
           {
               /* Something horrible has happened. */
               thread->error = EGL_BAD_ACCESS;
               break;
           }

           /* Fill in the worker information. */
           worker->draw         = draw;
           worker->backBuffer   = draw->backBuffer;
           worker->targetSignal = gcvNULL;

           /* Submit the worker. */
           gcmVERIFY(veglSubmitWorker(thread, dpy, worker, gcvTRUE));

           /* Commit the command buffer. */
           if (gcmIS_ERROR(gcoHAL_Commit(gcvNULL, gcvFALSE)))
           {
               /* Something horrible has happened. */
               thread->error = EGL_BAD_SURFACE;
               break;
           }

           /* Resume the swap thread. */
           veglResumeSwapWorker(dpy);

           /* Reset backBuffer. */
           draw->backBuffer.context = gcvNULL;
           draw->backBuffer.surface = gcvNULL;

           thread->error = EGL_SUCCESS;
           break;
       }
#endif

        /* Copy the rects for clipping. */
        gcoOS_MemCopy(
            draw->clippedRects,
            Rects,
            sizeof(EGLint) * 4 * NumRects
            );

#ifdef __QNXNTO__
        /* Clip each rectangle against the source buffer dimensions. */
        for (i = 0; i < NumRects; ++i)
        {
            _SurfaceClipAlignRect(
                &draw->renderTarget->info,
                draw->clippedRects + i * 4
                );
        }
#endif

        /* Flush the pipe. */
        _Flush(thread);

        /* Do we have direct access to the frame buffer? */
        if (draw->fbDirect)
        {
            /* 3D pipe? */
            if (!draw->openVG)
            {
                struct eglBackBuffer backBuffer;
                gcoSURF resolveTarget;
                gctBOOL flip;
                gctBOOL success;

#if gcdSUPPORT_SWAP_RECTANGLE
                gceTILING      srcTiling;
                gceORIENTATION destOrient;
                gcsPOINT swapSrcOrigin;
                gcsPOINT swapDstOrigin;
                gcsPOINT swapSize;
                gctINT   resolveHeight;
                gctINT   left;
                gctINT   top;
                gctINT   right;
                gctINT   bottom;
                gctINT   i;
#endif

                success = veglGetDisplayBackBuffer(
                    dpy, draw->hwnd, &backBuffer, &flip
                    );

                if (!success)
                {
                    gcmTRACE(
                        gcvLEVEL_ERROR,
                        "%s(%d): Get back buffer failed",
                        __FUNCTION__, __LINE__
                        );

                    break;
                }

                resolveTarget = (flip && (backBuffer.surface != gcvNULL))
                    ? backBuffer.surface
                    : draw->fbWrapper;

#if gcdDUMP
                {
                    gctUINT32 physical[3] = {0};
                    gctPOINTER logical[3] = {gcvNULL};
                    gctINT stride;

                    gcoSURF_Lock(resolveTarget, physical, logical);
                    gcoSURF_Unlock(resolveTarget, logical[0]);
                    gcoSURF_GetAlignedSize(resolveTarget, gcvNULL, gcvNULL, &stride);

                    gcmDUMP(gcvNULL,
                            "@[swap 0x%08X %dx%d +%u]",
                            physical[0],
                            draw->bitsWidth, draw->bitsHeight,
                            stride);
                }
#endif

#if gcdSUPPORT_SWAP_RECTANGLE
                /* Set bounding box to swap rectangle of this buffer. */
                left   = draw->clippedRects[0];
                top    = draw->clippedRects[1];
                right  = left + draw->clippedRects[2];
                bottom = top  + draw->clippedRects[3];

                for (i = 0; i < draw->bufferCount; i++)
                {
                    if (draw->bufferHandle[i] == backBuffer.context)
                    {
                        /* Update swap rectangle. */
                        draw->bufferSwapOrigin[i].x = draw->clippedRects[0];
                        draw->bufferSwapOrigin[i].y = draw->clippedRects[1];
                        draw->bufferSwapSize[i].x   = draw->clippedRects[2];
                        draw->bufferSwapSize[i].y   = draw->clippedRects[3];
                        break;
                    }
                }

                if (i == draw->bufferCount)
                {
                    gcmASSERT(i < MAX_NUM_SURFACE_BUFFERS);

                    /* Swap rectangle for new buffer. */
                    draw->bufferSwapOrigin[i].x = draw->clippedRects[0];
                    draw->bufferSwapOrigin[i].y = draw->clippedRects[1];
                    draw->bufferSwapSize[i].x   = draw->clippedRects[2];
                    draw->bufferSwapSize[i].y   = draw->clippedRects[3];

                    draw->bufferHandle[i]       = backBuffer.context;
                    draw->bufferCount++;
                }

                for (i = 0; i < draw->bufferCount; i++)
                {
                    if (draw->bufferHandle[i] != backBuffer.context)
                    {
                        gctUINT bufferRight, bufferBottom;

                        /* Update bounding box. */
                        bufferRight  = draw->bufferSwapOrigin[i].x + draw->bufferSwapSize[i].x;
                        bufferBottom = draw->bufferSwapOrigin[i].y + draw->bufferSwapSize[i].y;

                        left   = gcmMIN(left,   draw->bufferSwapOrigin[i].x);
                        top    = gcmMIN(top,    draw->bufferSwapOrigin[i].y);
                        right  = gcmMAX(right,  bufferRight);
                        bottom = gcmMAX(bottom, bufferBottom);
                    }
                }

                if (gcmIS_ERROR(gcoSURF_GetTiling(draw->renderTarget,
                                                  &srcTiling)))
                {
                    thread->error = EGL_BAD_SURFACE;
                    break;
                }

                if (gcmIS_ERROR(gcoSURF_QueryOrientation(resolveTarget,
                                                         &destOrient)))
                {
                    thread->error = EGL_BAD_SURFACE;
                    break;
                }

                /* Compute resolve aligned height. */
                switch (srcTiling)
                {
                case gcvMULTI_SUPERTILED:
                case gcvMULTI_TILED:
                    resolveHeight = (draw->bitsHeight + 7) & ~7;
                    break;

                default:
                    resolveHeight = (draw->bitsHeight + 3) & ~3;
                    break;
                }

                if (destOrient == gcvORIENTATION_TOP_BOTTOM)
                {
                    gctINT tmp = top;
                    top    = resolveHeight - bottom;
                    bottom = resolveHeight - tmp;
                }

                /* Align the swap rectangle. */
                switch (srcTiling)
                {
                case gcvMULTI_SUPERTILED:
                    left  &= ~63;
                    top   &= ~127;
                    right  = (right + 15) & ~15;
                    bottom = (bottom + 7) & ~7;
                    break;

                case gcvSUPERTILED:
                    left  &= ~63;
                    top   &= ~63;
                    right  = (right + 15) & ~15;
                    bottom = (bottom + 3) & ~3;
                    break;

                case gcvMULTI_TILED:
                    left  &= ~3;
                    top   &= ~7;
                    right  = (right + 15) & ~15;
                    bottom = (bottom + 7) & ~7;
                    break;

                case gcvTILED:
                case gcvLINEAR:
                default:
                    left  &= ~3;
                    top   &= ~3;
                    right  = (right + 15) & ~15;
                    bottom = (bottom + 3) & ~3;
                    break;
                }

                swapSize.x   = right  - left;
                swapSize.y   = bottom - top;

                swapSrcOrigin.x = left;
                swapSrcOrigin.y = top;

                swapDstOrigin.x = left;
                swapDstOrigin.y = resolveHeight - bottom;

                backBuffer.origin.x += swapDstOrigin.x;
                backBuffer.origin.y += swapDstOrigin.y;
#endif

#if defined(ANDROID) && gcdDEFER_RESOLVES
                struct gc_private_handle_t *privHandle = (struct gc_private_handle_t *)((android_native_buffer_t *)backBuffer.context)->handle;

                /* PRIV_FLAGS_FRAMEBUFFER = 0x00000001 is used only for compositor. */
                if (!(privHandle->flags & 0x00000001))
                {
                    gctSIGNAL resolveSubmittedSignal;
                    VEGLWorkerInfo worker;
                    gcsPOINT zeroSizeRect = {0, 0};
                    gctBOOL resized = (draw->clippedRects[2] != privHandle->width)
                                   || (draw->clippedRects[3] != privHandle->height);

                    /*
                       Update render target.

                       TO DO: Combine gcoSURF_ExportRenderTargetByKey and gcoSURF_PrepareRemoteResolveRect.
                     */
                    if (gcmIS_ERROR(gcoSURF_ExportRenderTargetByKey(backBuffer.surface, draw->renderTarget)))
                    {
                        /* Do we want/need to handle this? How? */
                        gcmTRACE(
                            gcvLEVEL_ERROR,
                            "%s(%d): failed",
                            __FUNCTION__, __LINE__
                            );
                    }

                    /* Has the size changed? If so, throw away the frame.
                       Prevent compositor from resolving by installing 0x0 size resolve.
                     */
#if gcdSUPPORT_SWAP_RECTANGLE
                    if (gcmIS_ERROR(gcoSURF_PrepareRemoteResolveRect(draw->renderTarget,
                                                                    &swapSrcOrigin,
                                                                    &backBuffer.origin,
                                                                    resized ? &zeroSizeRect : &swapSize)))
#else
                    srcOrigin.x = draw->clippedRects[0];
                    srcOrigin.y = draw->clippedRects[1];
                    size.x      = draw->clippedRects[2];
                    size.y      = draw->clippedRects[3];

                    if (gcmIS_ERROR(gcoSURF_PrepareRemoteResolveRect(draw->renderTarget,
                                                                    &srcOrigin,
                                                                    &backBuffer.origin,
                                                                    resized ? &zeroSizeRect : &size)))
#endif
                    {
                        /* Do we want/need to handle this? How? */
                        gcmTRACE(
                            gcvLEVEL_ERROR,
                            "%s(%d): failed",
                            __FUNCTION__, __LINE__
                            );
                    }

                    /* Suspend the worker thread. */
                    veglSuspendSwapWorker(dpy);

                    /* Find an available worker. */
                    worker = veglGetWorker(thread, dpy, draw);
                    if (worker == gcvNULL)
                    {
                        /* Something horrible has happened. */
                        thread->error = EGL_BAD_ACCESS;
                        break;
                    }

                    /* Fill in the worker information. */
                    worker->draw         = draw;
                    worker->backBuffer   = backBuffer;
                    worker->targetSignal = gcvNULL;
                    worker->numRects     = 1;
                    worker->rects[0]     = draw->clippedRects[0];
                    worker->rects[1]     = draw->clippedRects[1];
                    worker->rects[2]     = draw->clippedRects[2];
                    worker->rects[3]     = draw->clippedRects[3];

                    /* Submit the worker. */
                    gcmVERIFY(veglSubmitWorkerFromCPU(thread, dpy, worker));

                    /* Resume the swap thread. */
                    veglResumeSwapWorker(dpy);

                    if (resized == gcvFALSE)
                    {
                        gcoSURF_GetRTSignal(draw->renderTarget, &resolveSubmittedSignal);
                        /*
                            Wait until the resolve is submitted in SF before proceeding.
                        */

                        /*
                            At times SF seems to throw away a perfectly good frame, in which
                            case we never get a signal back.

                            TO DO: Is there a way to detect when SF will discard the frame?
                         */
                        status = gcoOS_WaitSignal(gcvNULL, resolveSubmittedSignal, 500);

                        if (gcmIS_ERROR(status))
                        {
                            if (status != gcvSTATUS_TIMEOUT)
                            {
                                gcmTRACE(
                                    gcvLEVEL_ERROR,
                                    "%s(%d): gcoOS_WaitSignal failed. resolveSubmittedSignal = %d, status=%d",
                                    __FUNCTION__, __LINE__, resolveSubmittedSignal, status
                                    );
                            }
                            break;
                        }
                    }
                }
                else
                {
#endif

#ifdef __QNXNTO__
                /* Set fbWrapper to back buffer surface for now.
                 * Use backBuffer.surface later on.
                 */
                if (flip && (backBuffer.surface == gcvNULL))
                {
                    gctPOINTER bbOldLogical, bbNewLogical;
                    gctUINT32 bbOldPhysical, bbNewPhysical;
                    gctINT32 bbStride;
                    gctUINT32 baseAddress = 0;

                    /* In case we need to roll back. */
                    bbOldPhysical = resolveTarget->physical;
                    bbOldLogical = resolveTarget->logical;

                    if (gcmIS_ERROR(gcoOS_GetBaseAddress(gcvNULL, &baseAddress)))
                    {
                        thread->error = EGL_BAD_ACCESS;
                        break;
                    }

                    /* Get window buffer pointer for current frame. */
                    veglGetWindowBits(dpy->hdc,
                                      draw->hwnd,
                                      &bbNewLogical,
                                      &bbNewPhysical,
                                      &bbStride);

                    /* Use the window buffer for current frame. */
                    resolveTarget->physical = bbNewPhysical - baseAddress;
                    resolveTarget->logical = bbNewLogical;
                    resolveTarget->info.node.physical = bbNewPhysical - baseAddress;
                    resolveTarget->info.node.logical = bbNewLogical;
                }
#endif

#if gcdSUPPORT_SWAP_RECTANGLE
                /* If using cached video memory, we should clean cache before using GPU
                   to do resolve rectangle. fix Bug3508*/
                gcoSURF_CPUCacheOperation(draw->renderTarget, gcvCACHE_CLEAN);

                if (gcmIS_ERROR(gcoSURF_ResolveRect(draw->renderTarget,
                                                    resolveTarget,
                                                    &swapSrcOrigin,
                                                    flip ? &backBuffer.origin
                                                         : &swapDstOrigin,
                                                    &swapSize)))
                {
                    thread->error = EGL_BAD_SURFACE;
                    break;
                }
#else
                /* If using cached video memory, we should clean cache before using GPU
                   to do resolve rectangle. fix Bug3508*/
                gcoSURF_CPUCacheOperation(draw->renderTarget, gcvCACHE_CLEAN);

                for (i = 0; i < NumRects; i++)
                {
                    srcOrigin.x = draw->clippedRects[i * 4 + 0];
                    srcOrigin.y = draw->clippedRects[i * 4 + 1];
                    size.x      = draw->clippedRects[i * 4 + 2];
                    size.y      = draw->clippedRects[i * 4 + 3];

                    dstOrigin.x = flip ? (backBuffer.origin.x + srcOrigin.x) : srcOrigin.x;
                    dstOrigin.y = flip ? (backBuffer.origin.y + srcOrigin.y) : srcOrigin.y;

                    if (gcmIS_ERROR(gcoSURF_ResolveRect(draw->renderTarget,
                                                        resolveTarget,
                                                        &srcOrigin,
                                                        &dstOrigin,
                                                        &size)))
                    {
                        thread->error = EGL_BAD_SURFACE;
                        break;
                    }
                }

                if (thread->error == EGL_BAD_SURFACE)
                {
                    thread->error = EGL_BAD_SURFACE;
                    break;
                }
#endif

                if (flip)
                {
                    VEGLWorkerInfo worker;

#if defined(ANDROID) && (ANDROID_SDK_VERSION >= 11)
                    if (draw->firstFrame)
                    {
                        /* Stall hardware for the first frame to make sure
                         * buffer count change is applied in queueBuffer. */
                        if (gcmIS_ERROR(gcoHAL_Commit(gcvNULL, gcvTRUE)))
                        {
                            thread->error = EGL_BAD_SURFACE;
                            break;
                        }

                        /* Flip. */
                        veglSetDisplayFlip(
                            dpy->hdc,
                            draw->hwnd,
                            &backBuffer
                            );

                        /* Clear flag. */
                        draw->firstFrame = gcvFALSE;

                        /* Success. */
                        thread->error = EGL_SUCCESS;
                        break;
                    }
#endif

                    /* Suspend the worker thread. */
                    veglSuspendSwapWorker(dpy);

                    /* Find an available worker. */
                    worker = veglGetWorker(thread, dpy, draw);
                    if (worker == gcvNULL)
                    {
                        /* Something horrible has happened. */
                        thread->error = EGL_BAD_ACCESS;
                        break;
                    }

                    /* Fill in the worker information. */
                    worker->draw         = draw;
                    worker->backBuffer   = backBuffer;
                    worker->targetSignal = gcvNULL;
                    worker->numRects     = NumRects;
                    gcoOS_MemCopy(
                            worker->rects,
                            draw->clippedRects,
                            sizeof(EGLint) * 4 * NumRects);

                    /* Submit the worker. */
                    gcmVERIFY(veglSubmitWorker(thread, dpy, worker, gcvTRUE));

                    /* Commit the command buffer. */
                    if (gcmIS_ERROR(gcoHAL_Commit(gcvNULL, gcvFALSE)))
                    {
                        /* Something horrible has happened. */
                        thread->error = EGL_BAD_SURFACE;
                        break;
                    }

                    /* Resume the swap thread. */
                    veglResumeSwapWorker(dpy);
                }
#if defined(ANDROID) && gcdDEFER_RESOLVES
                }
#endif
            }
#ifdef __QNXNTO__
            else
            {
                gcoSURF pSurface;
                static gctUINT32 s_nSurfaceIndex = 0;

                gcoHAL_Commit(gcvNULL, gcvTRUE);

                veglSetDisplayFlip(dpy->hdc, draw->hwnd, gcvNULL);

                gcoSURF_Destroy(draw->renderTarget);

                pSurface = (s_nSurfaceIndex++ % 2) ? draw->fbWrapper2 : draw->fbWrapper;
                draw->renderTarget = pSurface;

                gcoSURF_ReferenceSurface(draw->renderTarget);

                _SetBuffer(thread, pSurface);
            }
#else
            /* 355_FB_MULTI_BUFFER */
            else
            {
                struct eglBackBuffer backBuffer;
                gctBOOL flip;
                gctBOOL success;

                success = veglGetDisplayBackBuffer(
                    dpy, draw->hwnd, &backBuffer, &flip
                    );
                if (!success)
                {
                    break;
                }

                if (flip)
                {
                    /* Commit the command buffer. */
                    if (gcmIS_ERROR(gcoHAL_Commit(gcvNULL, gcvTRUE)))
                    {
                        /* Something horrible has happened. */
                        thread->error = EGL_BAD_SURFACE;
                        break;
                    }

                    if (draw->fbInfo.multiBuffer > 1)
                    {
                        gcoSURF newTarget = gcvNULL;
                        gctINT rendBuf = backBuffer.origin.y * draw->fbInfo.multiBuffer / draw->fbInfo.height;
                        rendBuf++;
                        rendBuf %= draw->fbInfo.multiBuffer;

                        /* Set Displayt buffer. */
                        veglSetDisplayFlip(dpy->hdc, draw->hwnd, &backBuffer);

                        /* Set Render Buffer. */
                        switch (rendBuf)
                        {
                            case 0:
                                newTarget = draw->fbWrapper;
                            break;

                            case 1:
                                newTarget = draw->fbWrappers[0];
                            break;

                            case 2:
                                newTarget = draw->fbWrappers[1];
                            break;

                            default:
                                gcmASSERT(gcvFALSE);
                                newTarget = draw->fbWrapper;
                            break;
                        }
                        draw->renderTarget=newTarget;
                        _SetBuffer(thread, newTarget);
                    }
                }
            }
#endif

            /* Success. */
            thread->error = EGL_SUCCESS;

#if gcdDUMP_FRAME_TGA
            srcOrigin.x = 0;
            srcOrigin.y = 0;
            size.x = draw->bitsWidth;
            size.y = draw->bitsHeight;

            _SaveFrameTga(thread, draw, &srcOrigin, &size);
#endif
            break;
        }

#if gcdDUMP
        {
            gctUINT32 physical[3] = {0};
            gctPOINTER logical[3] = {gcvNULL};
            gctINT stride;

            gcoSURF_Lock(draw->resolve, physical, logical);
            gcoSURF_Unlock(draw->resolve, logical[0]);
            gcoSURF_GetAlignedSize(draw->resolve, gcvNULL, gcvNULL, &stride);

            gcmDUMP(gcvNULL,
                    "@[swap 0x%08X %dx%d +%u]",
                    physical[0],
                    draw->bitsWidth, draw->bitsHeight,
                    stride);
        }
#endif

#if gcdDUMP_FRAME_TGA
        srcOrigin.x = 0;
        srcOrigin.y = 0;
        size.x = draw->bitsWidth;
        size.y = draw->bitsHeight;

        _SaveFrameTga(thread, draw, &srcOrigin, &size);
#endif

#ifndef VIVANTE_NO_3D
        {
            gctINT32 stride;
            gctPOINTER logical;
            gctUINT32 physical;
            gcsPOINT rectOrigin;

            if (veglGetWindowBits(dpy->hdc, draw->hwnd, &logical, &physical, &stride))
            {
                gctINT32 x, y;
                gctUINT width, height, bitsPerPixel;
                gceSURF_FORMAT format;
                gctUINT32 baseAddress = 0;

                if (!veglGetWindowInfo(dpy, draw->hwnd, &x, &y, &width, &height,
                                        &bitsPerPixel, &format))
                {
                    /* Bad surface. */
                    thread->error = EGL_BAD_SURFACE;
                    break;
                }

                if (gcmIS_ERROR(gcoOS_GetBaseAddress(gcvNULL, &baseAddress)))
                {
                    thread->error = EGL_BAD_ACCESS;
                    break;
                }

                for (i = 0; i < NumRects; i++)
                {
                    /* Translate the origin. */
                    rectOrigin.x = draw->clippedRects[4 *i + 0];
                    rectOrigin.y = draw->clippedRects[4 *i + 1];
                    size.x       = draw->clippedRects[4 *i + 2];
                    size.y       = draw->clippedRects[4 *i + 3];

                    if (gcmIS_ERROR(depr_gcoSURF_ResolveRect(draw->renderTarget,
                                                        gcvNULL,
                                                        physical - baseAddress,
                                                        gcvNULL,
                                                        stride,
                                                        gcvSURF_BITMAP,
                                                        format,
                                                        width,
                                                        height,
                                                        &rectOrigin,
                                                        &rectOrigin,
                                                        &size)))
                    {
                        /* Bad surface. */
                        thread->error = EGL_BAD_SURFACE;
                        break;
                    }
                }

                if (thread->error == EGL_BAD_SURFACE)
                {
                    break;
                }

                if (gcmIS_ERROR(gcoHAL_Commit(gcvNULL, gcvTRUE)))
                {
                    thread->error = EGL_BAD_SURFACE;
                    break;
                }

                thread->error = EGL_SUCCESS;
                break;
            }
        }
#endif

        /* Resolve into the resolve surface if present. */
        if (draw->resolve != gcvNULL)
        {
            for (i = 0; i < NumRects; i++)
            {
                srcOrigin.x = draw->clippedRects[4 * i + 0];
                srcOrigin.y = draw->clippedRects[4 * i + 1];
                size.x      = draw->clippedRects[4 * i + 2];
                size.y      = draw->clippedRects[4 * i + 3];

                status = gcoSURF_ResolveRect(
                    draw->renderTarget,
                    draw->resolve,
                    &srcOrigin,
                    &srcOrigin,
                    &size
                    );

                if (gcmIS_ERROR(status))
                {
                    /* Bad surface. */
                    thread->error = EGL_BAD_SURFACE;
                    break;
                }
            }

            if (thread->error == EGL_BAD_SURFACE)
            {
                break;
            }

            /* Set resolve surface bits. */
            bits = draw->resolveBits;

            if (thread->dump != gcvNULL)
            {
                gctBOOL enabled;

                /* Query whether the dumping is enabled. */
                gcmVERIFY_OK(gcoDUMP_IsEnabled(thread->dump, &enabled));

                if (enabled)
                {
                    /* Stall the hardware. */
                    if (gcmIS_ERROR(gcoHAL_Commit(gcvNULL, gcvTRUE)))
                    {
                        thread->error = EGL_BAD_SURFACE;
                        break;
                    }

                    /* Dump render target. */
                    _DumpSurface(thread->dump,
                                 draw->renderTarget,
                                 gcvTAG_RENDER_TARGET);

                    /* Dump depth buffer. */
                    _DumpSurface(thread->dump,
                                 draw->depthBuffer,
                                 gcvTAG_DEPTH);

                    /* Dump resolve buffer. */
                    _DumpSurface(thread->dump,
                                 draw->resolve,
                                 gcvTAG_RESOLVE);

                    /* End the current frame. */
                    gcmVERIFY_OK(gcoDUMP_FrameEnd(thread->dump));

                    /* Start a new frame. */
                    gcmVERIFY_OK(gcoDUMP_FrameBegin(thread->dump));
                }
            }
        }

        /* Schedule the resolved surface to be drawn by the worker thread. */
        if (dpy->workerThread != gcvNULL)
        {
            VEGLWorkerInfo worker;
            gctBOOL partialRect;

            /* Determine whether we need to swap the whole surface
               or only a part of it. */
            partialRect
                =  (draw->clippedRects[0] != 0)
                || (draw->clippedRects[1] != 0)
                || (draw->clippedRects[2] != draw->bitsWidth)
                || (draw->clippedRects[3] != draw->bitsHeight);

            /* Stall the pipe if requested rectangle is not the same
               as the previous. */
            if (partialRect &&
                ((draw->clippedRects[0] != draw->lastSubmittedWorker->backBuffer.origin.x)   ||
                 (draw->clippedRects[1] != draw->lastSubmittedWorker->backBuffer.origin.y)   ||
                 (draw->clippedRects[2] != draw->lastSubmittedWorker->backBuffer.size.width) ||
                 (draw->clippedRects[3] != draw->lastSubmittedWorker->backBuffer.size.height)))
            {
                if (gcmIS_ERROR(gcoHAL_Commit(gcvNULL, gcvTRUE)))
                {
                    /* Bad surface. */
                    thread->error = EGL_BAD_SURFACE;
                    break;
                }
            }

            /* Suspend the worker thread. */
            veglSuspendSwapWorker(dpy);

            /* Find an available worker. */
            worker = veglGetWorker(thread, dpy, draw);
            if (worker == gcvNULL)
            {
                /* Something horrible has happened. */
                thread->error = EGL_BAD_ACCESS;
                break;
            }

            /* Multi-buffered target enabled? */
            if (draw->renderListEnable)
            {
                worker->targetSignal = draw->renderListCurr->signal;
                worker->bits         = draw->renderListCurr->bits;
            }
            else
            {
                /* Bits must be set. */
                gcmASSERT(bits != gcvNULL);

                /* Set surface attributes. */
                worker->targetSignal = gcvNULL;
                worker->bits         = bits;
            }

            worker->draw                   = Draw;
            worker->backBuffer.origin.x    = draw->clippedRects[0];
            worker->backBuffer.origin.y    = draw->clippedRects[1];
            worker->backBuffer.size.width  = draw->clippedRects[2];
            worker->backBuffer.size.height = draw->clippedRects[3];
            worker->next                   = gcvNULL;
            worker->numRects               = NumRects;
            gcoOS_MemCopy(
                    worker->rects,
                    draw->clippedRects,
                    sizeof(EGLint) * 4 * NumRects);

            /* Submit worker. */
            if (!veglSubmitWorker(thread, dpy, worker, gcvTRUE))
            {
                thread->error = EGL_BAD_SURFACE;
                break;
            }

            /* Resume the thread. */
            veglResumeSwapWorker(dpy);

            /* Commit the command buffer. */
            if (gcmIS_ERROR(gcoHAL_Commit(gcvNULL, gcvFALSE)))
            {
                /* Bad surface. */
                thread->error = EGL_BAD_SURFACE;
                break;
            }

            /* Set the target. */
            if (gcmIS_ERROR(veglDriverSurfaceSwap(thread, draw)))
            {
                thread->error = EGL_BAD_SURFACE;
                break;
            }

            /* Success. */
            thread->error = EGL_SUCCESS;
            break;
        }

        /* Stall the hardware. */
        if (gcmIS_ERROR(gcoHAL_Commit(gcvNULL, gcvTRUE)))
        {
            /* Bad surface. */
            thread->error = EGL_BAD_SURFACE;
            break;
        }

        /* Draw the image on the window. */
        for (i = 0; i < NumRects; i++)
        {
            if (!veglDrawImage(dpy, Draw, bits,
                               draw->clippedRects[i * 4 + 0],
                               draw->clippedRects[i * 4 + 1],
                               draw->clippedRects[i * 4 + 2],
                               draw->clippedRects[i * 4 + 3]))
            {
                /* Bad surface. */
                thread->error = EGL_BAD_SURFACE;
                break;
            }
        }

        if (thread->error == EGL_BAD_SURFACE)
        {
            /* Bad surface. */
            break;
        }

        /* Success. */
        thread->error = EGL_SUCCESS;
    }
    while (EGL_FALSE);

    /* Determine result. */
    result = ((thread != gcvNULL) && (thread->error == EGL_SUCCESS))
        ? EGL_TRUE
        : EGL_FALSE;

#if VIVANTE_PROFILER
    if (thread->api == EGL_OPENGL_ES_API)
    {
        if (thread->context->client == 1)
        {
            /* GL_PROFILER_FRAME_END */
            _eglProfileCallback(thread, 10, 0);
        }
        else if (thread->context->client == 2)
        {
            /* GL2_PROFILER_FRAME_END */
            _eglProfileCallback(thread, 10, 0);
        }
    }
    else if (thread->api == EGL_OPENVG_API)
    {
        _eglProfileCallback(thread, 10, 0);
    }
#endif

    /* Return result. */
    return result;
}

void
veglResumeSwapWorker(
    VEGLDisplay Display
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x", Display);

    gcmASSERT(Display != gcvNULL);
    if (Display->suspendMutex != gcvNULL)
    {
        gcmONERROR(gcoOS_ReleaseMutex(gcvNULL,
                                      Display->suspendMutex));
    }
    gcmFOOTER_NO();
    return;

OnError:
    gcmFOOTER();
    return;
}

/* Local function to combine common code between eglSwapBuffers and
 * eglSwapBuffersRegionEXT.
 *
 * NumRects = 0 signifies fullscreen swap.
 */
EGLBoolean
_eglSwapBuffersRegion(
    EGLDisplay Dpy,
    EGLSurface Draw,
    EGLint NumRects,
    const EGLint* Rects
    )
{
    EGLBoolean result;
    VEGLThreadData  thread;
    VEGLDisplay dpy;
    VEGLSurface draw;
    gctINT x,y;
    gctUINT width, height;
    gceSURF_FORMAT resolveFormat;
    gctUINT bitsPerPixel;
    gctBOOL resized;
    EGLint rects[4];

    gcmHEADER_ARG("Dpy=0x%x Draw=0x%x NumRects=0x%x Rects=0x%x", Dpy, Draw, NumRects, Rects);

    do
    {
        /* Get thread data. */
        thread = veglGetThreadData();
        if (thread == gcvNULL)
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): veglGetThreadData failed.",
                __FUNCTION__, __LINE__
                );

            result = EGL_FALSE;
            break;
        }

        /* Create shortcuts to objects. */
        dpy  = VEGL_DISPLAY(Dpy);

        /* Test for valid EGLDisplay structure. */
        if ((dpy == gcvNULL) || (dpy->signature != EGL_DISPLAY_SIGNATURE))
        {
            /* Bad display. */
            thread->error = EGL_BAD_DISPLAY;
            result = EGL_FALSE;
            break;
        }

        /* Test if EGLDisplay structure has been initialized. */
        if (dpy->referenceDpy == gcvNULL)
        {
            /* Not initialized. */
            thread->error = EGL_NOT_INITIALIZED;
            result = EGL_FALSE;
            break;
        }

        /* Test for valid EGLSurface structure. */
        for (draw = dpy->surfaceStack; draw != gcvNULL; draw = draw->next)
        {
            if (draw == VEGL_SURFACE(Draw))
            {
                break;
            }
        }

        if (draw == gcvNULL)
        {
            /* Bad surface*/
            thread->error = EGL_BAD_SURFACE;
            result = EGL_FALSE;
            break;
        }

        /* Test if surface is locked. */
        if (draw->locked)
        {
            thread->error = EGL_BAD_ACCESS;
            result = EGL_FALSE;
            break;
        }

#if gcdRENDER_THREADS
        /* Check for main thread. */
        if (thread->renderThread == gcvNULL)
        {
            /* Inform the render thread to do the swap. */
            result = eglfRenderThreadSwap(thread, Dpy, Draw);

            if (result)
            {
                /* Render thread is going to perform the swap - return. */
                break;
            }
        }
#endif

#if gcdFRAME_DB
        /* Add (previous) frame to the database. */
        gcoHAL_AddFrameDB();
#endif

        if (NumRects == 0)
        {
            /* Set the coordinates. */
#if gcdSUPPORT_SWAP_RECTANGLE
            rects[0] = draw->swapOrigin.x;
            rects[1] = draw->swapOrigin.y;
            rects[2] = draw->swapSize.x;
            rects[3] = draw->swapSize.y;
#else
            rects[0] = 0;
            rects[1] = 0;
            rects[2] = draw->bitsWidth;
            rects[3] = draw->bitsHeight;
#endif

            result = _SwapBuffersRegion(thread, Dpy, Draw, 1, rects);
        }
        else
        {
            /* Call generic function. */
            result = _SwapBuffersRegion(thread, Dpy, Draw, NumRects, Rects);
        }

        if (draw->hwnd == (NativeWindowType) gcvNULL)
        {
            break;
        }

        /* Query window parameters. */
        result = veglGetWindowInfo(Dpy, draw->hwnd,
                                   &x, &y,
                                   &width, &height,
                                   &bitsPerPixel,
                                   &resolveFormat);

        if (!result)
        {
            /* Window is gone. */
            break;
        }

        resized = gcvFALSE;

        if ((draw->bitsWidth != (gctINT) width)
            ||  (draw->bitsHeight != (gctINT) height)
            )
        {
            resized = gcvTRUE;
        }

        if (!resized)
        {
            break;
        }

        /* Make sure all workers have been processed. */
        if (gcmIS_ERROR(gcoOS_WaitSignal(gcvNULL,
                                         dpy->doneSignal,
                                         gcvINFINITE)))
        {
            result = gcvFALSE;
            break;
        }

        if (EGL_SUCCESS != veglResizeSurface(draw,
                                             width, height,
                                             resolveFormat,
                                             bitsPerPixel))
        {
            result = gcvFALSE;
            break;
        }

        if (gcmIS_ERROR(gcoHAL_Commit(gcvNULL, gcvTRUE)))
        {
            /* Bad surface. */
            thread->error = EGL_BAD_SURFACE;

            result = gcvFALSE;
            break;
        }

        result = gcvTRUE;
    }
    while(gcvFALSE);

    gcmFOOTER_ARG("%d", result);
    return result;
}


EGLAPI EGLBoolean EGLAPIENTRY
eglSwapBuffers(
    EGLDisplay Dpy,
    EGLSurface Draw
    )
{
    EGLBoolean result;

    gcmHEADER_ARG("Dpy=0x%x Draw=0x%x", Dpy, Draw);
    gcmDUMP_API("${EGL eglSwapBuffers 0x%08X 0x%08X}", Dpy, Draw);

    result = _eglSwapBuffersRegion(Dpy, Draw, 0, gcvNULL);

    gcmFOOTER_ARG("%d", result);
    return result;
}


EGLAPI EGLBoolean EGLAPIENTRY
eglSwapBuffersRegionEXT(
    EGLDisplay Dpy,
    EGLSurface Draw,
    EGLint NumRects,
    const EGLint* Rects
    )
{
    EGLBoolean result;

    gcmHEADER_ARG("Dpy=0x%x Draw=0x%x NumRects=0x%x Rects=0x%x", Dpy, Draw, NumRects, Rects);
    gcmDUMP_API("${EGL eglSwapBuffersRegionEXT 0x%08X 0x%08X %d 0x%x}", Dpy, Draw, NumRects, Rects);

    if (NumRects < 0)
    {
        gcmFOOTER_ARG("return=%d", EGL_BAD_PARAMETER);
        return EGL_BAD_PARAMETER;
    }

    if (NumRects > 0 && Rects == gcvNULL)
    {
        gcmFOOTER_ARG("return=%d", EGL_BAD_PARAMETER);
        return EGL_BAD_PARAMETER;
    }

    if (NumRects > EGL_MAX_SWAP_BUFFER_REGION_RECTS)
    {
        /* Just resolve entire surface. */
        NumRects = 0;
    }

    /* TODO: Resolve regions can result in bad image.
     * Falling to fullscreen resolve until fixed.
    */
    result = _eglSwapBuffersRegion(Dpy, Draw, 0, gcvNULL);

    gcmFOOTER_ARG("%d", result);
    return result;
}


EGLAPI EGLBoolean EGLAPIENTRY
eglSwapRectangleANDROID(
    EGLDisplay Dpy,
    EGLSurface Draw,
    EGLint Left,
    EGLint Top,
    EGLint Width,
    EGLint Height
    )
{
    VEGLThreadData  thread;
    VEGLDisplay dpy;
    VEGLSurface draw;
    EGLBoolean result;
    EGLint rect[4];

    gcmHEADER_ARG("Dpy=0x%x Draw=0x%x Left=%d Top=%d Width=%d Height=%d",
                  Dpy, Draw, Left, Top, Width, Height);
    gcmDUMP_API("${EGL eglSwapRectangleANDROID 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X}",
                Dpy, Draw, Left, Top, Width, Height);

    do
    {
        /* Get thread data. */
        thread = veglGetThreadData();
        if (thread == gcvNULL)
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): veglGetThreadData failed.",
                __FUNCTION__, __LINE__
                );

            result = EGL_FALSE;
            break;
        }

        /* Create shortcuts to objects. */
        dpy  = VEGL_DISPLAY(Dpy);

        /* Test for valid EGLDisplay structure. */
        if ((dpy == gcvNULL) || (dpy->signature != EGL_DISPLAY_SIGNATURE))
        {
            /* Bad display. */
            thread->error = EGL_BAD_DISPLAY;
            result = EGL_FALSE;
            break;
        }

        /* Test if EGLDisplay structure has been initialized. */
        if (dpy->referenceDpy == gcvNULL)
        {
            /* Not initialized. */
            thread->error = EGL_NOT_INITIALIZED;
            result = EGL_FALSE;
            break;
        }

        /* Test for valid EGLSurface structure. */
        for (draw = dpy->surfaceStack; draw != gcvNULL; draw = draw->next)
        {
            if (draw == VEGL_SURFACE(Draw))
            {
                break;
            }
        }

        if (draw == gcvNULL)
        {
            /* Bad surface*/
            thread->error = EGL_BAD_SURFACE;
            result = EGL_FALSE;
            break;
        }

        /* Test if surface is locked. */
        if (draw->locked)
        {
            thread->error = EGL_BAD_ACCESS;
            result = EGL_FALSE;
            break;
        }

        /* Fill in the coordinates. */
        rect[0] = Left;
        rect[1] = Top;

        rect[2] = Width;
        rect[3] = Height;

        /* Call generic function. */
        result = _SwapBuffersRegion(thread, Dpy, Draw, 1, rect);
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("%d", result);
    return result;
}


#if defined(ANDROID)
EGLAPI EGLBoolean EGLAPIENTRY
eglSetSwapRectangleANDROID(
    EGLDisplay Dpy,
    EGLSurface Draw,
    EGLint Left,
    EGLint Top,
    EGLint Width,
    EGLint Height
    )
{
    EGLBoolean result;
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSurface draw;

    gcmHEADER_ARG("Dpy=0x%x Draw=0x%x Left=%d Top=%d Width=%d Height=%d",
                  Dpy, Draw, Left, Top, Width, Height);
    gcmDUMP_API("${EGL eglSetSwapRectangleANDROID 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X}",
                Dpy, Draw, Left, Top, Width, Height);

    do
    {
        /* Get thread data. */
        thread = veglGetThreadData();
        if (thread == gcvNULL)
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): eglSetSwapRectangleANDROID failed.",
                __FUNCTION__, __LINE__
                );

            result = EGL_FALSE;
            break;
        }

        /* Create shortcuts to objects. */
        dpy  = VEGL_DISPLAY(Dpy);

        /* Test for valid EGLDisplay structure. */
        if ((dpy == gcvNULL) || (dpy->signature != EGL_DISPLAY_SIGNATURE))
        {
            /* Bad display. */
            thread->error = EGL_BAD_DISPLAY;
            result = EGL_FALSE;
            break;
        }

        /* Test if EGLDisplay structure has been initialized. */
        if (dpy->referenceDpy == gcvNULL)
        {
            /* Not initialized. */
            thread->error = EGL_NOT_INITIALIZED;
            result = EGL_FALSE;
            break;
        }

        /* Test for valid EGLSurface structure. */
        for (draw = dpy->surfaceStack; draw != gcvNULL; draw = draw->next)
        {
            if (draw == VEGL_SURFACE(Draw))
            {
                break;
            }
        }

        if (draw == gcvNULL)
        {
            /* Bad surface*/
            thread->error = EGL_BAD_SURFACE;
            result = EGL_FALSE;
            break;
        }

        draw->swapOrigin.x = Left;
        draw->swapOrigin.y = Top;
        draw->swapSize.x   = Width;
        draw->swapSize.y   = Height;

        result = EGL_TRUE;
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("%d", result);
    return result;
}
#endif


EGLAPI EGLBoolean EGLAPIENTRY
eglCopyBuffers(
    EGLDisplay Dpy,
    EGLSurface Surface,
    EGLNativePixmapType target
    )
{
    EGLBoolean result = EGL_FALSE;
    gcoSURF tempSurface = gcvNULL;
    gctPOINTER tempSurfaceBits[3] = {gcvNULL};

    gcmHEADER_ARG("Dpy=0x%x Surface=0x%x target=%d", Dpy, Surface, target);
    gcmDUMP_API("${EGL eglCopyBuffers 0x%08X 0x%08X 0x%08X}",
                Dpy, Surface, target);

    do
    {
        gceSTATUS status;
        VEGLThreadData thread;
        VEGLDisplay dpy;
        VEGLSurface surface;
        gctUINT width, height, bitsPerPixel;
        gceSURF_FORMAT format = gcvSURF_UNKNOWN;
        gctPOINTER bits;
        gctINT stride;
        gctUINT32 alignment;

        /* Get thread data. */
        thread = veglGetThreadData();
        if (thread == gcvNULL)
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): veglGetThreadData failed.",
                __FUNCTION__, __LINE__
                );

            result = EGL_FALSE;
            break;
        }

        /* Create shortcuts to objects. */
        dpy     = VEGL_DISPLAY(Dpy);

        /* Test for valid EGLDisplay structure. */
        if ((dpy == gcvNULL) || (dpy->signature != EGL_DISPLAY_SIGNATURE))
        {
            /* Bad display. */
            thread->error = EGL_BAD_DISPLAY;
            result = EGL_FALSE;
            break;
        }

        /* Test if EGLDisplay structure has been initialized. */
        if (dpy->referenceDpy == gcvNULL)
        {
            /* Not initialized. */
            thread->error = EGL_NOT_INITIALIZED;
            result = EGL_FALSE;
            break;
        }

        /* Test for valid EGLSurface structure. */
        for (surface = dpy->surfaceStack; surface != gcvNULL; surface = surface->next)
        {
            if (surface == VEGL_SURFACE(Surface))
            {
                break;
            }
        }

        if (surface == gcvNULL)
        {
            /* Bad surface*/
            thread->error = EGL_BAD_SURFACE;
            result = EGL_FALSE;
            break;
        }

        /* Test if surface is locked. */
        if (surface->locked)
        {
            thread->error = EGL_BAD_ACCESS;
            break;
        }

        /* Flush the API. */
        _Flush(thread);

        if (!veglGetPixmapInfo(
                dpy->hdc, target, &width, &height, &bitsPerPixel, &format))
        {
            thread->error = EGL_BAD_NATIVE_PIXMAP;
            result = EGL_FALSE;
            break;
        }

        if (!veglGetPixmapBits(
                dpy->hdc, target, &bits, &stride, gcvNULL))
        {
            bits = gcvNULL;
            stride = width * bitsPerPixel;
        }

        /* Determine the required alignment. */
        gcmVERIFY_OK(gcoSURF_GetAlignment(gcvSURF_RENDER_TARGET, format, &alignment, gcvNULL, gcvNULL));

        /* Is the pixmap properly aligned? */
        if ((bits != gcvNULL) && ((gcmPTR2INT(bits) & (alignment - 1)) == 0))
        {
            /* The pixmap is properly aligned, create a wrapper surface. */
            status = gcoSURF_Construct(
                gcvNULL,
                width, height, 1,
                gcvSURF_BITMAP,
                format,
                gcvPOOL_USER,
                &tempSurface
                );

            if (gcmIS_ERROR(status))
            {
                thread->error = EGL_BAD_ALLOC;
                result = EGL_FALSE;
                break;
            }

            /* Map the surface. */
            status = gcoSURF_MapUserSurface(
                tempSurface, 0, bits, ~0U
                );

            if (gcmIS_ERROR(status))
            {
                thread->error = EGL_BAD_ALLOC;
                result = EGL_FALSE;
                break;
            }
        }
        else
        if ((surface->resolve == gcvNULL)
        ||  (surface->resolveFormat != format)
        )
        {
            /* The surface is not properly aligned, create a temporary surface
               to resolve to. */
            status = gcoSURF_Construct(
                gcvNULL,
                width, height, 1,
                gcvSURF_BITMAP,
                format,
                gcvPOOL_SYSTEM,
                &tempSurface
                );

            if (gcmIS_ERROR(status))
            {
                thread->error = EGL_BAD_ALLOC;
                result = EGL_FALSE;
                break;
            }

            /* Lock the surface. */
            status = gcoSURF_Lock(
                tempSurface, gcvNULL, tempSurfaceBits
                );

            if (gcmIS_ERROR(status))
            {
                thread->error = EGL_BAD_ALLOC;
                result = EGL_FALSE;
                break;
            }
        }
        else
        {
            tempSurfaceBits[0] = surface->resolveBits;
        }

        /* Resolve the render target. */
        status = gcoSURF_Resolve(surface->renderTarget,
                                 (tempSurface == gcvNULL) ? surface->resolve
                                                          : tempSurface);

        if (gcmIS_ERROR(status))
        {
            /* Bad surface. */
            thread->error = EGL_BAD_SURFACE;
            result = EGL_FALSE;
            break;
        }

        /* Stall the hardware. */
        status = gcoHAL_Commit(gcvNULL, gcvTRUE);

        if (gcmIS_ERROR(status))
        {
            /* Bad surface. */
            thread->error = EGL_BAD_ACCESS;
            result = EGL_FALSE;
            break;
        }

        /* Copy the resolved bits. */
        if (tempSurfaceBits[0] != gcvNULL)
        {
            gctUINT srcWidth;
            gctUINT srcHeight;
            gctINT srcStride;

            status = gcoSURF_GetAlignedSize(
                (tempSurface == gcvNULL) ? surface->resolve : tempSurface,
                &srcWidth,
                &srcHeight,
                &srcStride
                );

            if (gcmIS_ERROR(status))
            {
                /* Bad surface. */
                thread->error = EGL_BAD_ACCESS;
                result = EGL_FALSE;
                break;
            }

            if (bits != gcvNULL)
            {
                if (stride == srcStride)
                {
                    gcmVERIFY_OK(
                        gcoOS_MemCopy(bits, tempSurfaceBits[0], stride * height));
                }
                else
                {
                    gctUINT line;

                    for (line = 0; line < height; line++)
                    {
                        /* Copy a scanline. */
                        gcmVERIFY_OK(gcoOS_MemCopy(
                            (gctUINT8_PTR) bits + stride * line,
                            (gctUINT8_PTR) tempSurfaceBits[0] + srcStride * line,
                            stride
                            ));
                    }
                }
            }
            else
            {
                result = veglDrawPixmap(
                    Dpy, target,
                    0, 0, width, height,
                    srcWidth, srcHeight,
                    bitsPerPixel, tempSurfaceBits[0]
                    );
            }
        }

        /* Success. */
        result = EGL_TRUE;
    }
    while (gcvFALSE);

    /* Free resources. */
    if (tempSurface != gcvNULL)
    {
        if (tempSurfaceBits[0] != gcvNULL)
        {
            /* Unlock the surface. */
            gcmVERIFY_OK(gcoSURF_Unlock(tempSurface, tempSurfaceBits[0]));
        }

        gcmVERIFY_OK(gcoSURF_Destroy(tempSurface));
    }

    /* Success. */
    gcmFOOTER_ARG("%d", result);
    return result;
}

#if defined(ANDROID)
/* EGL_ANDROID_get_render_buffer. */

EGLAPI EGLClientBuffer EGLAPIENTRY
eglGetRenderBufferANDROID(
    EGLDisplay Dpy,
    EGLSurface Draw
    )
{
    VEGLSurface draw;
    VEGLThreadData thread;
    VEGLDisplay dpy;
    EGLClientBuffer buffer = gcvNULL;

    gcmHEADER_ARG("Dpy=0x%x Draw=0x%x", Dpy, Draw);
    gcmDUMP_API("${EGL eglSwapBuffers 0x%08X 0x%08X}", Dpy, Draw);

    do
    {
        gctBOOL flip;
        gctBOOL success;

        /* Get thread data. */
        thread = veglGetThreadData();
        if (thread == gcvNULL)
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): veglGetThreadData failed.",
                __FUNCTION__, __LINE__
                );

            break;
        }

        /* Create shortcuts to objects. */
        dpy = VEGL_DISPLAY(Dpy);

        if ((dpy == gcvNULL) || (dpy->signature != EGL_DISPLAY_SIGNATURE))
        {
            thread->error = EGL_BAD_DISPLAY;
            break;
        }

        /* Test if EGLDisplay structure has been initialized. */
        if (dpy->referenceDpy == gcvNULL)
        {
            /* Not initialized. */
            thread->error = EGL_NOT_INITIALIZED;
            break;
        }

        /* Test for valid EGLSurface structure. */
        for (draw = dpy->surfaceStack; draw != gcvNULL; draw = draw->next)
        {
            if (draw == VEGL_SURFACE(Draw))
            {
                break;
            }
        }

        if (draw == gcvNULL)
        {
            thread->error = EGL_BAD_SURFACE;
            break;
        }

        if (draw->backBuffer.context != gcvNULL)
        {
            buffer = (EGLClientBuffer) draw->backBuffer.context;
            break;
        }

        success = veglGetDisplayBackBuffer(
            dpy, draw->hwnd, &draw->backBuffer, &flip
            );

        if (!success)
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): Get back buffer failed",
                __FUNCTION__, __LINE__
                );

            thread->error = EGL_BAD_DISPLAY;
            break;
        }

        buffer = (EGLClientBuffer) draw->backBuffer.context;

        if (buffer != gcvNULL)
        {
            /* Return back swap rectangle. */
            android_native_buffer_t * nativeBuffer =
                (android_native_buffer_t *) buffer;

            /* TODO: Save to a more proper place. */
            *((gctINT_PTR) &nativeBuffer->common.reserved[0]) =
                (draw->swapOrigin.x << 16) | draw->swapOrigin.y;

            *((gctINT_PTR) &nativeBuffer->common.reserved[1]) =
                (draw->swapSize.x << 16) | draw->swapSize.y;

#if gcdSUPPORT_SWAP_RECTANGLE
            draw->swapOrigin.x = 0;
            draw->swapOrigin.y = 0;
            draw->swapSize.x   = draw->bitsWidth;
            draw->swapSize.y   = draw->bitsHeight;
#   endif
        }
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("0x%.08X", buffer);
    return buffer;
}
#endif

