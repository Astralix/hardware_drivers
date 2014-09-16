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




#include "gc_vg_precomp.h"

#if (defined(LINUX) || defined(__QNXNTO__))
#include <stdlib.h>
#include <pthread.h>

static pthread_key_t tlsKey = 0;

static void _GlobalDestructor(
	void *value
	)
{
	free(value);
	pthread_setspecific(tlsKey, gcvNULL);
}

vgsTHREADDATA_PTR vgfGetThreadData(
	gctBOOL Create
    )
{
	vgsTHREADDATA_PTR thread;

	do
	{
		/* Do we have a key yet for this thread? */
		if (!tlsKey)
		{
			/* Do we need to create? */
			if (!Create)
			{
				thread = gcvNULL;
				break;
			}

			/* Create a key. */
			gctINT rc = pthread_key_create(&tlsKey, _GlobalDestructor);
			if (rc < 0)
			{
				thread = gcvNULL;
				break;
			}
		}

		/* Get the thread data. */
		thread = pthread_getspecific(tlsKey);

		if ((thread == gcvNULL) && Create)
		{
			/* Allocate memory for thread. */
			thread = malloc(gcmSIZEOF(struct _vgsTHREADDATA));

			if (thread != gcvNULL)
			{
				/* Updata thread data. */
				pthread_setspecific(tlsKey, thread);

				/* Initialize thread data. */
				thread->context = gcvNULL;
			}
		}
	}
	while (gcvFALSE);

	/* Return pointer to thread data. */
	return thread;
}

#else

static DWORD TLSIndex;

vgsTHREADDATA_PTR vgfGetThreadData(
	gctBOOL Create
	)
{
	vgsTHREADDATA_PTR thread;

	/* Get current thread data. */
	thread = TlsGetValue(TLSIndex);

	if ((thread == gcvNULL) && Create)
	{
		/* Allocate memory for thread. */
		thread = LocalAlloc(LPTR, gcmSIZEOF(struct _vgsTHREADDATA));

		if (thread != gcvNULL)
		{
			/* Updata thread data. */
			TlsSetValue(TLSIndex, thread);

			/* Initialize thread data. */
			thread->context = gcvNULL;
		}
	}

	/* Return pointer to thread data. */
	return thread;
}

gctBOOL __stdcall DllMain(
	IN HINSTANCE Instance,
	IN DWORD Reason,
	IN LPVOID Reserved
)
{
	void* data;

	switch (Reason)
	{
	case DLL_PROCESS_ATTACH:
		gcmTRACE(0, "OpenVG11 DllMain(DLL_PROCESS_ATTACH)\n");

		/* Allocate a TLS index. */
		TLSIndex = TlsAlloc();
		if (TLSIndex == TLS_OUT_OF_INDEXES)
		{
			return gcvFALSE;
		}

		break;

	case DLL_THREAD_ATTACH:
		gcmTRACE(0, "OpenVG11 DllMain(DLL_THREAD_ATTACH)\n");

		/* Don't allocate thread data here, it will be done
		   by veglSetContext call. */
		break;

	case DLL_THREAD_DETACH:
		gcmTRACE(0, "OpenVG11 DllMain(DLL_THREAD_DETACH)\n");

		/* Release memory for this thread. */
		data = TlsGetValue(TLSIndex);
		if (data != gcvNULL)
		{
			LocalFree((HLOCAL) data);
		}
		break;

	case DLL_PROCESS_DETACH:
		gcmTRACE(0, "OpenVG11 DllMain(DLL_PROCESS_DETACH)\n");

		/* Release memory for this thread. */
		data = TlsGetValue(TLSIndex);
		if (data != gcvNULL)
		{
			LocalFree((HLOCAL) data);
		}

		/* Release the TLS index. */
		TlsFree(TLSIndex);
		break;
	}

	return gcvTRUE;
}

#endif
