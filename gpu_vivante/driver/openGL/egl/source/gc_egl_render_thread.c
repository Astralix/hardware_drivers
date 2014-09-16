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

#if gcdRENDER_THREADS

#define _GC_OBJ_ZONE    gcdZONE_EGL_CONTEXT


/* Entry point of each render thread. */
static gctPOINTER eglfRenderThread(IN gctPOINTER Data)
{
    gcsRENDER_THREAD *thread = (gcsRENDER_THREAD *) Data;
    VEGLThreadData threadData;
    gceSTATUS status;
    veglDISPATCH *dispatch;

    gcmHEADER_ARG("Data=%p", Data);

    /* Allocate the thread data. */
    threadData = veglGetThreadData();
    if (threadData == gcvNULL)
    {
        gcmTRACE_ZONE(gcvLEVEL_ERROR, glvZONE_EGL_RENDER_THREAD,
                      "Cannot create thread data for render tread.\n");

        goto OnError;
    }

    /* Mark that this is a render thread. */
    threadData->context      = thread->context;
    threadData->renderThread = thread;

    /* Get the dispatch table. */
    dispatch = _GetDispatch(threadData, thread->context);

    /* Check if there is a render thread handler. */
    if (dispatch->renderThread == gcvNULL)
    {
        gcmTRACE_ZONE(gcvLEVEL_ERROR, glvZONE_EGL_RENDER_THREAD,
                      "The current API doesn't support render threads.\n");

        goto OnError;
    }

    /* Copy the handler pointer. */
    thread->handler = dispatch->renderThread;

    /* Create a new API context with the owning context as a shared context. */
    thread->apiContext = _CreateApiContext(threadData,
                                           thread->context,
                                           gcvNULL, gcvNULL,
                                           ((VEGLContext) thread->context)
                                           ->context);

    /* Make sure we have a valid API context. */
    if (thread->apiContext == gcvNULL)
    {
        gcmTRACE_ZONE(gcvLEVEL_ERROR, glvZONE_EGL_RENDER_THREAD,
                      "Cannot create an API context for a render thread.\n");

        goto OnError;
    }

    /* Reset pointers. */
    thread->consumer = 0;
    thread->producer = 0;

    /* Create the signals. */
    gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvFALSE, &thread->signalDone));
    gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvFALSE, &thread->signalGo));

    /* Mark thread as running. */
    thread->status = gcvSTATUS_TRUE;
    gcmONERROR(gcoOS_Signal(gcvNULL, thread->signalHeartbeat, gcvTRUE));

    /* Loop forever. */
    for (;;)
    {
        /* Wait for go signal. */
        gcmONERROR(gcoOS_WaitSignal(gcvNULL, thread->signalGo, gcvINFINITE));

        /* Check if we are asked to quit. */
        if (thread->status == gcvSTATUS_TERMINATE)
        {
            break;
        }

        /* Loop while there are messages to process. */
        while (thread->consumer != thread->producer)
        {
            /* Call the render thread handler. */
            (*thread->handler)(thread);

            /* Signal the heartbeat. */
            gcmONERROR(gcoOS_Signal(gcvNULL, thread->signalHeartbeat, gcvTRUE));
        }

        /* Signal we are done. */
        gcmONERROR(gcoOS_Signal(gcvNULL, thread->signalDone, gcvTRUE));
    }

    /* Thread has stopped. */
    status = gcvSTATUS_FALSE;

OnError:
    /* Mark thread as stopped. */
    thread->status = status;

    /* Destroy the signals. */
    if (thread->signalDone != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, thread->signalDone));
        thread->signalDone = gcvNULL;
    }

    if (thread->signalGo != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, thread->signalGo));
        thread->signalGo = gcvNULL;
    }

    if (thread->apiContext != gcvNULL)
    {
        /* Destroy the API context. */
        _DestroyApiContext(threadData, thread->context, thread->apiContext);
        thread->apiContext = gcvNULL;
    }

    /* Return nothing. */
    gcmFOOTER_NO();
    return gcvNULL;
}

/* Create all render threads. */
gctUINT eglfCreateRenderThreads(IN VEGLContext Context)
{
    gctUINT i;
    gceSTATUS status;
    gcsRENDER_THREAD *render;

    gcmHEADER_ARG("Context=%p", Context);

    /* Process all render threads. */
    for (i = 0; i < gcdRENDER_THREADS; i++)
    {
        /* Allocate the render thread structure. */
        render = gcvNULL;
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  gcmSIZEOF(gcsRENDER_THREAD),
                                  (gctPOINTER *) &render));

        /* Save pointer to the render thread structure. */
        Context->renderThreads[i] = render;

        /* Pass in pointer to owning context. */
        render->context = Context;

        /* Mark thread as initializating. */
        render->status = gcvSTATUS_FALSE;

        /* Create the heartbeat signal. */
        gcmONERROR(gcoOS_CreateSignal(gcvNULL,
                                      gcvFALSE,
                                      &render->signalHeartbeat));

        /* Create the thread. */
        gcmONERROR(gcoOS_CreateThread(gcvNULL,
                                      eglfRenderThread, render,
                                      &render->thread));

        /* Loop while the thread is initializing. */
        while (render->status == gcvSTATUS_FALSE)
        {
            /* Wait for the heartbeat. */
            status = gcoOS_WaitSignal(gcvNULL, render->signalHeartbeat, 1);
            if (gcmIS_ERROR(status) && (status != gcvSTATUS_TIMEOUT))
            {
                /* Error. */
                gcmONERROR(status);
            }
        }

        /* Check for thread error. */
        gcmONERROR(render->status);

        /* Increment the thread count. */
        Context->renderThreadCount = i + 1;
    }

    /* Return number of threads created. */
    gcmFOOTER_ARG("%u", Context->renderThreadCount);
    return Context->renderThreadCount;

OnError:
    if (render != gcvNULL)
    {
        /* Destroy the heartbeat for the current thread. */
        if (render->signalHeartbeat != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, render->signalHeartbeat));
            render->signalHeartbeat = gcvNULL;
        }
    }

    /* Return number of threads created. */
    gcmFOOTER_ARG("%u", Context->renderThreadCount);
    return Context->renderThreadCount;
}

/* Destroy all render threads. */
gctBOOL eglfDestroyRenderThreads(IN VEGLContext Context)
{
    gctUINT i;
    gceSTATUS status;

    gcmHEADER_ARG("Context=%p", Context);

    /* Process all render threads. */
    for (i = 0; i < gcdRENDER_THREADS; i++)
    {
        /* Get pointer to the render thread structure. */
        gcsRENDER_THREAD *render = Context->renderThreads[i];

        /* Test if render thread structre has been allocated. */
        if (render != gcvNULL)
        {
            /* Test if this is a valid thread. */
            if (render->thread != gcvNULL)
            {
                /* Wait for the render thread to be fiished. */
                eglfWaitRenderThread(render);

                /* Mark the thread to be destroyed. */
                render->status = gcvSTATUS_TERMINATE;

                /* Destroy the thread. */
                gcmONERROR(gcoOS_Signal(gcvNULL, render->signalGo, gcvTRUE));

                /* Wait for the thread to be finished. */
                gcmONERROR(gcoOS_CloseThread(gcvNULL, render->thread));
                render->thread = gcvNULL;

                /* Destroy the heartbeat. */
                gcmONERROR(gcoOS_DestroySignal(gcvNULL, render->signalHeartbeat));
                render->signalHeartbeat = gcvNULL;

                /* Mark the thread as destroyed. */
                render->thread = gcvNULL;
            }

            /* Free the render thread structure. */
            gcmONERROR(gcoOS_Free(gcvNULL, render));
        }
    }

    /* Success. */
    gcmFOOTER_ARG("%u", gcvTRUE);
    return gcvTRUE;

OnError:
    /* Error. */
    gcmFOOTER_ARG("%u", gcvFALSE);
    return gcvFALSE;
}

/* Get the pointer to the current render thread information structure. */
gcsRENDER_THREAD *eglfGetCurrentRenderThread(void)
{
    VEGLThreadData thread;
    VEGLContext context;
    gcsRENDER_THREAD * render = gcvNULL;

    gcmHEADER();

    /* Get thread information. */
    thread = veglGetThreadData();
    if (thread != gcvNULL)
    {
        /* Check if this is a render thread. */
        if (thread->renderThread != gcvNULL)
        {
            /* Simplre return the pointer to its own information. */
            render = thread->renderThread;
        }

        else
        {
            /* Get the current context. */
            context = thread->context;
            if (context != gcvNULL)
            {
                /* Get the current render thread. */
                render = context->renderThreads[context->currentRenderThread];
            }
        }
    }

    /* Return pointer to current render thread structure. */
    gcmFOOTER_ARG("%p", render);
    return render;
}

/* Wait for a render thread to be finished or wait for all render threads (when
 * Thread is gcvNULL) to be finished. */
void eglfWaitRenderThread(IN gcsRENDER_THREAD *Thread)
{
    VEGLThreadData thread;
    VEGLContext context;
    gctUINT i;
    gceSTATUS status;

    gcmHEADER_ARG("Thread=%x", Thread);

    do
    {
        /* Do we need to wait for all threads? */
        if (Thread == gcvNULL)
        {
            /* Get thread information. */
            thread = veglGetThreadData();
            if (thread == gcvNULL)
            {
                break;
            }

            /* This can only be called from an app's thread. */
            gcmASSERT(thread->renderThread == gcvNULL);

            /* Get the current context. */
            context = thread->context;
            if (context == gcvNULL)
            {
                break;
            }

            /* Process all render threads. */
            for (i = 0; i < context->renderThreadCount; i++)
            {
                /* Wait for the render thread. */
                gcmASSERT(context->renderThreads[i] != gcvNULL);
                eglfWaitRenderThread(context->renderThreads[i]);
            }
        }

        /* Check if this is actually a separate thread. */
        else if (Thread->signalDone == gcvNULL)
        {
            /* Dispatch the renderer. */
            (*Thread->handler)(Thread);
        }

        else
        {
            /* Wait until the render thread has caught up with the producer. */
            while (Thread->consumer != Thread->producer)
            {
                /* Kick off the thread. */
                gcmERR_BREAK(gcoOS_Signal(gcvNULL, Thread->signalGo, gcvTRUE));

                /* Wait for the tread. */
                gcmERR_BREAK(gcoOS_WaitSignal(gcvNULL,
                                              Thread->signalDone,
                                              gcvINFINITE));
            }
        }
    }
    while (0);

    /* Done. */
    gcmFOOTER_NO();
    return;
}

/* Get the pointer to the next available message for the current render
 * thread. */
gcsRENDER_MESSAGE *eglfGetRenderMessage(IN gcsRENDER_THREAD *Thread)
{
    gcsRENDER_THREAD *thread;
    gctUINT producer;
    gcsRENDER_MESSAGE *message = gcvNULL;
    gceSTATUS status;

    gcmHEADER();

    /* Get the current render thread. */
    thread = (Thread == gcvNULL) ? eglfGetCurrentRenderThread() : Thread;
    if (thread != gcvNULL)
    {
        /* Get the producer index. */
        producer = thread->producer;

        /* Get a pointre to the next message. */
        message = &thread->queue[producer];

        /* Increment the producer index. */
        producer = (producer + 1) % gcdRENDER_MESSAGES;

        /* Wait until there is no collision with the consumer index. */
        while (producer == thread->consumer)
        {
            /* Wait for a heartbeat. */
            gcmERR_BREAK(gcoOS_WaitSignal(gcvNULL,
                                          thread->signalHeartbeat,
                                          gcvINFINITE));
        }

        /* Reset number of bytes used. */
        message->bytes[0] = 0;
        message->bytes[1] = 0;
    }

    /* Return the message pointer. */
    gcmFOOTER_ARG("%p", message);
    return message;
}

/* Let the current render thread perform a swap. */
EGLBoolean eglfRenderThreadSwap(IN VEGLThreadData Thread,
                                IN EGLDisplay Display,
                                IN EGLSurface Draw)
{
    gcsRENDER_THREAD *render;
    gcsRENDER_MESSAGE *msg;
    gceSTATUS status;
    VEGLContext context;

    gcmHEADER_ARG("Thread=%x Display=%p Draw=%p", Thread, Display, Draw);

    /* Get the current render thread. */
    render = eglfGetCurrentRenderThread();

    /* Check for a valid render thread. */
    if ((render == gcvNULL) || (render->thread == gcvNULL))
    {
        goto OnError;
    }

    /* Get pointer to the message. */
    msg = eglfGetRenderMessage(render);

    /* Initialize the message. */
    msg->function = (gltRENDER_FUNCTION) eglSwapBuffers;
    msg->argCount = 2;
    msg->arg[0].p = (gctPOINTER) Display;
    msg->arg[1].p = (gctPOINTER) Draw;

    /* Increment the producer index. */
    render->producer = (render->producer + 1) % gcdRENDER_MESSAGES;

    /* Wake up the render thread. */
    gcmONERROR(gcoOS_Signal(gcvNULL, render->signalGo, gcvTRUE));

    /* Get pointer to current context. */
    context = Thread->context;

    /* Set next render thread. */
    context->currentRenderThread = ((context->currentRenderThread + 1)
                                    % context->renderThreadCount);

    /* Get the next render thread. */
    render = eglfGetCurrentRenderThread();

    /* Wait until this next thread is finished. */
    eglfWaitRenderThread(render);

    /* Success. */
    gcmFOOTER_ARG("%u", EGL_TRUE);
    return EGL_TRUE;

OnError:
    /* Error. */
    gcmFOOTER_ARG("%u", EGL_FALSE);
    return EGL_FALSE;
}

/* Set all render threads to the new context. */
EGLBoolean eglfRenderThreadSetContext(IN VEGLContext Context,
                                      IN gcoSURF Draw,
                                      IN gcoSURF Read,
                                      IN gcoSURF Depth)
{
    gctUINT i;
    gcsRENDER_THREAD *render;
    gcsRENDER_MESSAGE *msg;
    gceSTATUS status;

    gcmHEADER_ARG("Context=%p Draw=%p Read=%p Depth=%p",
                  Context, Draw, Read, Depth);

    /* Process all render threads. */
    for (i = 0; i < Context->renderThreadCount; i++)
    {
        /* Get a pointer to the render thread. */
        render = Context->renderThreads[i];
        gcmASSERT(render != gcvNULL);

        /* Get pointer to the next available message. */
        msg = eglfGetRenderMessage(render);

        /* Initialize the message. */
        msg->function = gcvNULL;
        msg->argCount = 4;
        msg->arg[0].p = (Draw == gcvNULL) ? gcvNULL : render->apiContext;
        msg->arg[1].p = (gctPOINTER) Draw;
        msg->arg[2].p = (gctPOINTER) Read;
        msg->arg[3].p = (gctPOINTER) Depth;

        /* Increment the producer index. */
        render->producer = (render->producer + 1) % gcdRENDER_MESSAGES;

        /* Wake up the render thread. */
        gcmONERROR(gcoOS_Signal(gcvNULL, render->signalGo, gcvTRUE));
    }

    /* Success. */
    gcmFOOTER_ARG("%u", EGL_TRUE);
    return EGL_TRUE;

OnError:
    /* Success. */
    gcmFOOTER_ARG("%u", EGL_FALSE);
    return EGL_FALSE;
}

/* Copy memory into the render thread memory. */
gctPOINTER eglfCopyRenderMemory(IN gctCONST_POINTER Memory, IN gctSIZE_T Bytes)
{
    gctPOINTER ptr;
    gcsRENDER_THREAD *render;
    gceSTATUS status;

    /* Get the current render thread. */
    render = eglfGetCurrentRenderThread();
    if (render == gcvNULL)
    {
        /* No render thread, return NULL. */
        return gcvNULL;
    }

    /* Make sure we have enough bytes. */
    gcmASSERT(Bytes < gcdRENDER_MEMORY);

    /* Test if we still fit in the local memory. */
    if (render->producerOffset + Bytes <= gcdRENDER_MEMORY)
    {
        /* Loop while we would overrun the current consumer offset. */
        while ((render->producerOffset < render->consumerOffset)
            && (render->producerOffset + Bytes >= render->consumerOffset)
            )
        {
            /* Wait for a heartbeat. */
            gcmERR_BREAK(gcoOS_WaitSignal(gcvNULL,
                                          render->signalHeartbeat,
                                          gcvINFINITE));
        }

        /* Point to the memory. */
        ptr = &render->memory[render->producerOffset];

        /* Adjust producer offset. */
        render->producerOffset += Bytes;
    }
    else
    {
        /* Loop while we would overrun the current consumer offset. */
        while (Bytes >= render->consumerOffset)
        {
            /* Wait for a heartbeat. */
            gcmERR_BREAK(gcoOS_WaitSignal(gcvNULL,
                                          render->signalHeartbeat,
                                          gcvINFINITE));
        }

        /* Point to the memory. */
        ptr = render->memory;

        /* Set new producer offset. */
        render->producerOffset = Bytes;
    }

    /* Copy the memory. */
    gcoOS_MemCopy(ptr, Memory, Bytes);

    /* Return the poiter as an argument for the render thread. */
    return ptr;
}

#endif /* gcdRENDER_THREADS */
