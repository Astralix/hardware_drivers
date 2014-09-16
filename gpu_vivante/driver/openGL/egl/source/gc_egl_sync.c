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

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcdZONE_EGL_SYNC

EGLAPI EGLSyncKHR EGLAPIENTRY
eglCreateSyncKHR(
    EGLDisplay Dpy,
    EGLenum type,
    const EGLint *attrib_list
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSync sync;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Dpy=0x%x type=%d attrib_list=0x%x", Dpy, type, attrib_list);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=0x%x", EGL_NO_SYNC_KHR);
        return EGL_NO_SYNC_KHR;
    }

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    if ( (dpy == gcvNULL) ||
         (dpy->signature != EGL_DISPLAY_SIGNATURE) )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SYNC_KHR);
        return EGL_NO_SYNC_KHR;
    }

    if ((type != EGL_SYNC_REUSABLE_KHR) && (type != EGL_SYNC_FENCE_KHR))
    {
        thread->error = EGL_BAD_ATTRIBUTE;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SYNC_KHR);
        return EGL_NO_SYNC_KHR;
    }

    if (attrib_list != gcvNULL && attrib_list[0] != EGL_NONE)
    {
        thread->error = EGL_BAD_ATTRIBUTE;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SYNC_KHR);
        return EGL_NO_SYNC_KHR;
    }

    /* Create new sync. */
    status = gcoOS_Allocate(gcvNULL,
                           sizeof(struct eglSync),
                           &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        thread->error = EGL_BAD_ALLOC;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SYNC_KHR);
        return EGL_NO_SYNC_KHR;
    }

    sync = pointer;

    /* Initialize context. */
    sync->signature         = EGL_SYNC_SIGNATURE;
    sync->type              = type;
    sync->condition         = EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR;

    status = gcoOS_CreateSignal(gcvNULL,
                                gcvTRUE,
                                &sync->signal);

    if (gcmIS_ERROR(status))
    {
        /* Roll back. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, sync));

        thread->error = EGL_BAD_ALLOC;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SYNC_KHR);
        return EGL_NO_SYNC_KHR;
    }

    switch (type)
    {
    case EGL_SYNC_REUSABLE_KHR:
        break;
    case EGL_SYNC_FENCE_KHR:
        iface.command            = gcvHAL_SIGNAL;
        iface.u.Signal.signal    = sync->signal;
	    iface.u.Signal.auxSignal = gcvNULL;
        iface.u.Signal.process   = dpy->process;
	    iface.u.Signal.fromWhere = gcvKERNEL_PIXEL;
        /* Send event. */
        gcoHAL_ScheduleEvent(gcvNULL, &iface);
        gcoHAL_Commit(gcvNULL, gcvFALSE);
        break;
    default:
        thread->error = EGL_BAD_ATTRIBUTE;
        /* Roll back. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, sync));
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SYNC_KHR);
        return EGL_NO_SYNC_KHR;
    }

    gcmTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_SIGNAL,
        "%s(%d): sync signal created 0x%08X",
        __FUNCTION__, __LINE__,
        sync->signal
        );

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglCreateSyncKHR 0x%08X 0x%08X (0x%08X) := 0x%08X",
                Dpy, type, attrib_list, sync);
    gcmDUMP_API_ARRAY_TOKEN(attrib_list, EGL_NONE);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("return=0x%x", sync);
    return sync;
}


EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySyncKHR(
    EGLDisplay Dpy,
    EGLSyncKHR Sync
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSync sync;
    gceSTATUS status;

    gcmHEADER_ARG("Dpy=0x%x Sync=0x%x", Dpy, Sync);
    gcmDUMP_API("${EGL eglDestroySyncKHR 0x%08X 0x%08X}", Dpy, Sync);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    if ( (dpy == gcvNULL) ||
         (dpy->signature != EGL_DISPLAY_SIGNATURE) )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the sync. */
    sync = VEGL_SYNC(Sync);

    /* Test for valid EGLSync structure. */
    if ( (sync == gcvNULL) ||
         (sync->signature != EGL_SYNC_SIGNATURE) )
    {
        /* Bad display. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Signal before destroying. */
    status = gcoOS_Signal(gcvNULL,
                            sync->signal,
                            gcvTRUE);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Destroy the signal. */
    status = gcoOS_DestroySignal(gcvNULL,
                                sync->signal);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Free the eglSync structure. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, sync));

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;
}

EGLAPI EGLint EGLAPIENTRY
eglClientWaitSyncKHR(
    EGLDisplay Dpy,
    EGLSyncKHR Sync,
    EGLint flags,
    EGLTimeKHR timeout
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSync sync;
    gceSTATUS status;
    gctUINT32 wait;
    EGLint result;

    gcmHEADER_ARG("Dpy=0x%x Sync=0x%x flags=%d timeout=%lld",
                  Dpy, Sync, flags, timeout);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    if ((dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the sync. */
    sync = VEGL_SYNC(Sync);

    /* Test for valid EGLSync structure. */
    if ((sync == gcvNULL)
    ||  (sync->signature != EGL_SYNC_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Check flags */
    if (flags & EGL_SYNC_FLUSH_COMMANDS_BIT_KHR)
    {
        /* Check if the sync is unsignaled */
        status = gcoOS_WaitSignal(gcvNULL,
                                  sync->signal,
                                  0);

        if (status == gcvSTATUS_TIMEOUT)
        {
            /* Flush */
            _Flush(thread);
        }
        else if (gcmIS_ERROR(status))
        {
            /* Error. */
            thread->error = EGL_BAD_ACCESS;
            gcmFOOTER_ARG("%d", EGL_FALSE);
            return EGL_FALSE;
        }
    }

    /* Wait the signal */
    wait = (timeout == EGL_FOREVER_KHR)
         ? gcvINFINITE
         : (gctUINT32) gcoMATH_DivideUInt64(timeout, 1000000ull);

    status = gcoOS_WaitSignal(gcvNULL,
                              sync->signal,
                              wait);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Success. */
    thread->error = EGL_SUCCESS;
    result = (status == gcvSTATUS_OK) ? EGL_CONDITION_SATISFIED_KHR
                                      : EGL_TIMEOUT_EXPIRED_KHR;

    gcmDUMP_API("${EGL eglClientWaitSyncKHR 0x%08X 0x%08X 0x%08X 0x%016llX := "
                "0x%08X}",
                Dpy, Sync, flags, timeout, result);
    gcmFOOTER_ARG("%d", result);
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSignalSyncKHR(
    EGLDisplay Dpy,
    EGLSyncKHR Sync,
    EGLenum mode
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSync sync;
    gceSTATUS status;

    gcmHEADER_ARG("Dpy=0x%x Sync=0x%x mode=0x%04x", Dpy, Sync, mode);
    gcmDUMP_API("${EGL eglSignalSyncKHR 0x%08X 0x%08X 0x%08X}",
                Dpy, Sync, mode);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    if ( (dpy == gcvNULL) ||
         (dpy->signature != EGL_DISPLAY_SIGNATURE) )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the sync. */
    sync = VEGL_SYNC(Sync);

    /* Test for valid EGLSync structure. */
    if ( (sync == gcvNULL) ||
         (sync->signature != EGL_SYNC_SIGNATURE) )
    {
        /* Bad display. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Check mode */
    switch (mode)
    {
    case EGL_SIGNALED_KHR:
    case EGL_UNSIGNALED_KHR:
        break;

    default:
        /* Bad attribute. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Signal. */
    status = gcoOS_Signal(gcvNULL,
                            sync->signal,
                            (mode == EGL_SIGNALED_KHR));

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetSyncAttribKHR(
    EGLDisplay Dpy,
    EGLSyncKHR Sync,
    EGLint attribute,
    EGLint *value)
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSync sync;
    gceSTATUS status;

    gcmHEADER_ARG("Dpy=0x%x Sync=0x%x attribute=%d", Dpy, Sync, attribute);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    if ( (dpy == gcvNULL) ||
         (dpy->signature != EGL_DISPLAY_SIGNATURE) )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the sync. */
    sync = VEGL_SYNC(Sync);

    /* Test for valid EGLSync structure. */
    if ( (sync == gcvNULL) ||
         (sync->signature != EGL_SYNC_SIGNATURE) )
    {
        /* Bad display. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    switch (attribute)
    {
    case EGL_SYNC_TYPE_KHR:
        *value = sync->type;
        break;

    case EGL_SYNC_STATUS_KHR:
        status = gcoOS_WaitSignal(gcvNULL,
                                    sync->signal,
                                    0);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            thread->error = EGL_BAD_ACCESS;
            gcmFOOTER_ARG("return=%d", EGL_FALSE);
            return EGL_FALSE;
        }

        *value = ((status == gcvSTATUS_OK)? EGL_SIGNALED_KHR : EGL_UNSIGNALED_KHR);
        break;

    case EGL_SYNC_CONDITION_KHR:
        if (sync->type != EGL_SYNC_FENCE_KHR)
        {
            thread->error = EGL_BAD_ATTRIBUTE;
            gcmFOOTER_ARG("return=%d", EGL_FALSE);
            return EGL_FALSE;
        }
        *value = sync->condition;
        break;

	default:
		/* Bad attribute. */
		thread->error = EGL_BAD_ATTRIBUTE;
		gcmFOOTER_ARG("return=%d", EGL_FALSE);
		return EGL_FALSE;
	}

	/* Success. */
	thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglGetSyncAttribKHR 0x%08X 0x%08X 0x%08X := 0x%08X}",
                Dpy, Sync, attribute, *value);
    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;
}




