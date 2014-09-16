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







/*
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable
 * license fees to QNX Software Systems before you may reproduce,
 * modify or distribute this software, or any work that includes
 * all or part of this software.   Free development licenses are
 * available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email
 * licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review
 * this entire file for other proprietary rights or license notices,
 * as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

/*
 * Qnx specific functions.
 */

#define _QNX_SOURCE
#include "gc_egl_precomp.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#ifndef GAL_DEV
#define GAL_DEV	"/dev/galcore"
#endif

static gctBOOL initialized = gcvFALSE;
static pthread_key_t key;
static pthread_t worker_thread;
static VEGLThreadData _thread = NULL;

#define _GC_OBJ_ZONE		gcdZONE_EGL_OS

void
veglDestroyThread(
	void
	)
{
}

static void
veglThreadDestructor(
	void * Thread
	)
{
	gcmHEADER_ARG("Thread=0x%x", Thread);

	pthread_setspecific(key, Thread);

	if (Thread != NULL)
	{
		veglDestroyThreadData(Thread);

		gcmVERIFY_OK(gcoOS_Free(gcvNULL, Thread));

		if (_thread == Thread) _thread = NULL;
	}

	pthread_setspecific(key, NULL);

	gcmFOOTER_NO();
}

static void
signalHandler(
	int signum
	)
{
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	if (_thread != NULL && _thread->displayStack != EGL_NO_DISPLAY)
	{
		eglMakeCurrent(_thread->displayStack, NULL, NULL, NULL);

		eglTerminate(_thread->displayStack);
	}

	exit(0);
}

void
_setup(
	void
	)
{
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	{
		signal(SIGINT, signalHandler);
	}

	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
	{
		signal(SIGQUIT, signalHandler);
	}
}

void __attribute__((destructor))
__fini(
	void
    )
{
	VEGLThreadData thread;

	gcmHEADER();

	if (initialized)
	{
		/* Get a pointer to the associated object. */
		thread = (VEGLThreadData) pthread_getspecific(key);

		if (0 == access(GAL_DEV, F_OK)) {
		    veglDestroyThreadData(thread);
		}

		gcmVERIFY_OK(gcoOS_Free(gcvNULL, thread));

		pthread_setspecific(key, NULL);

		pthread_key_delete(key);

		_thread = NULL;

		veglDeinitializeGlobalData();

		initialized = gcvFALSE;
	}

	gcmFOOTER_NO();
}

gctBOOL
veglGetThreadPtr(
	VEGLThreadData * Thread,
	gctBOOL * NewThread
	)
{
	/* Has the key been created for the thread? */
	if (initialized)
	{
		/* Get a pointer to the associated object. */
		*Thread = (VEGLThreadData) pthread_getspecific(key);
	}
	else
	{
		/* Create a key. */
		if (pthread_key_create(&key, veglThreadDestructor) < 0)
		{
			return gcvFALSE;
		}

		veglInitializeGlobalData();

		/* Key has been initialized. */
		initialized = gcvTRUE;

		/* No object has been associated yet. */
		*Thread = NULL;
	}

	/* Has the object been allocated? */
	if (*Thread == NULL)
	{
		/* Allocate. */
		if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
									   gcmSIZEOF(struct _EGL_THREAD_DATA),
									   (gctPOINTER *) Thread)))
		{
			gcmTRACE_ZONE(gcvLEVEL_ERROR, gcdZONE_EGL_OS,
						  "%s(%d): malloc failed (%d bytes)",
						  __FUNCTION__, __LINE__,
						  gcmSIZEOF(struct _EGL_THREAD_DATA));

			return gcvFALSE;
		}

		/* Set thread data pointer. */
		pthread_setspecific(key, *Thread);

		/* Set the new thread flag. */
		*NewThread = gcvTRUE;
	}
	else
	{
		/* This is not a new thread. */
		*NewThread = gcvFALSE;
	}

	if (_thread == NULL) _thread = *Thread;

	/* Success. */
	return gcvTRUE;
}

gctHANDLE
veglCreateThread(
	veglTHREAD_ROUTINE Worker,
	void * Argument
	)
{
	if (pthread_create(&worker_thread, NULL, Worker, Argument) != 0)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_ERROR, gcdZONE_EGL_OS,
			"%s(%d): an error has occurred.",
			__FUNCTION__, __LINE__
			);

		return NULL;
	}

	gcmTRACE_ZONE(
		gcvLEVEL_INFO, gcdZONE_EGL_OS,
		"%s(%d): created thread 0x%x",
		__FUNCTION__, __LINE__,
		worker_thread
		);

	return (gctHANDLE) worker_thread;
}

void
veglCloseThread(
	gctHANDLE ThreadHandle
	)
{
	pthread_join((pthread_t) ThreadHandle, NULL);
	worker_thread = 0;
}

gctBOOL
veglIsOurThread(
	void
	)
{
	return (pthread_equal(pthread_self(), worker_thread));
}
