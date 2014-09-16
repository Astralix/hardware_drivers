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
 * Vivante specific definitions and declarations for EGL library.
 */

#ifndef __gc_egl_common_h_
#define __gc_egl_common_h_

#ifdef __cplusplus
extern "C" {
#endif


/* A generic function pointer. */
typedef void (* gltRENDER_FUNCTION)(void);

/* Union of possible variables. */
typedef union gcuRENDER_VALUE
{
    gctINT              i;
    gctUINT             u;
    gctFLOAT            f;
    gctPOINTER          p;
}
gcuRENDER_VALUE;

/* One record in the message queue. */
typedef struct gcsRENDER_MESSAGE
{
    /* Function index. */
    gltRENDER_FUNCTION  function;

    /* Argument count. */
    gctUINT             argCount;

    /* Arguments. */
    gcuRENDER_VALUE     arg[9];

    /* Output. */
    gcuRENDER_VALUE     output;

    /* Memory usage. */
    gctSIZE_T           bytes[2];
}
gcsRENDER_MESSAGE;

#define gcdRENDER_MESSAGES      18000
#define gcdRENDER_MEMORY        4096

typedef struct gcsRENDER_THREAD gcsRENDER_THREAD;

/* Prototype for a render thread. */
typedef void (* veglRENDER_THREAD) (
    gcsRENDER_THREAD * ThreadInfo
    );

/* Thread information structure for a render thread. */
struct gcsRENDER_THREAD
{
    /* Thread handle. */
    gctHANDLE           thread;

    /* Owning context. */
    EGLContext          context;

    /* API context. */
    gctPOINTER          apiContext;

    /* Pointer to render thread handler. */
    veglRENDER_THREAD   handler;

    /* Thread status. */
    volatile gceSTATUS  status;

    /* Siangl to start the thread. */
    gctSIGNAL           signalGo;

    /* Signal when the thread is done. */
    gctSIGNAL           signalDone;

    /* Signal when the thread has exceuted one message. */
    gctSIGNAL           signalHeartbeat;

    /* The message queue. */
    gcsRENDER_MESSAGE   queue[gcdRENDER_MESSAGES];

    /* Producer entry. */
    volatile gctUINT    producer;

    /* Consumer entry. */
    volatile gctUINT    consumer;

    /* The render thread local memory. */
    gctUINT8            memory[gcdRENDER_MEMORY];

    /* Producer memory offset. */
    volatile gctUINT    producerOffset;

    /* Consumer memory offset. */
    volatile gctUINT    consumerOffset;
};

gcsRENDER_MESSAGE *eglfGetRenderMessage(IN gcsRENDER_THREAD *Thread);

gcsRENDER_THREAD *eglfGetCurrentRenderThread(void);

void eglfWaitRenderThread(IN gcsRENDER_THREAD *Thread);

gctPOINTER eglfCopyRenderMemory(IN gctCONST_POINTER Memory, IN gctSIZE_T Bytes);


typedef void * (* veglCREATECONTEXT) (
	gcoOS Os,
	gcoHAL Hal,
	gctPOINTER SharedContext
	);

typedef EGLBoolean (* veglDESTROYCONTEXT) (
	void * Context
	);

typedef EGLBoolean (* veglFLUSHCONTEXT) (
	void * Context
	);

typedef gctBOOL (* veglSETCONTEXT) (
	void * Context,
	gcoSURF Draw,
	gcoSURF Read,
	gcoSURF Depth
	);

typedef gceSTATUS (* veglSETBUFFER) (
	gcoSURF Draw
	);

typedef void (* veglFLUSH) (
	void
	);

typedef void (* veglFINISH) (
	void
	);

typedef gcoSURF (* veglGETCLIENTBUFFER) (
	void * Context,
	EGLClientBuffer Buffer
	);

typedef EGLenum (* veglBINDTEXIMAGE) (
	gcoSURF Surface,
	EGLenum Format,
	EGLenum Target,
	EGLBoolean Mipmap,
	EGLint Level,
	gcoSURF *Binder
	);

typedef void (* veglPROFILER) (
	void * Profiler,
	gctUINT32 Enum,
	gctUINT32 Value
	);

typedef EGLenum (* veglCREATEIMAGETEXTURE) (
	EGLenum Target,
	gctINT Texture,
	gctINT Level,
	gctINT Depth,
	gctPOINTER Image
	);

typedef EGLenum (* veglCREATEIMAGERENDERBUFFER) (
	EGLenum Renderbuffer,
	gctPOINTER Image
	);

typedef	EGLenum (* veglCREATEIMAGEVGPARENTIMAGE) (
	unsigned int vgImage,
	void ** Images,
	int * Count
	);

typedef EGLBoolean (* veglQUERYHWVG) (
    void
    );

#if !gcdSTATIC_LINK
EGLAPI
#endif
void * EGLAPIENTRY
veglGetCurrentAPIContext(
	void
	);

#if !gcdSTATIC_LINK
EGLAPI
#endif
EGLsizeiANDROID EGLAPIENTRY
veglGetBlobCache(
    const void* key,
    EGLsizeiANDROID keySize,
    void* value,
    EGLsizeiANDROID valueSize
    );

#if !gcdSTATIC_LINK
EGLAPI
#endif
void EGLAPIENTRY
veglSetBlobCache(
    const void* key,
    EGLsizeiANDROID keySize,
    const void* value,
    EGLsizeiANDROID valueSize
    );


typedef struct _veglLOOKUP
{
	const char *								name;
	__eglMustCastToProperFunctionPointerType	function;
}
veglLOOKUP;

#define eglMAKE_LOOKUP(function) \
	{ #function, (__eglMustCastToProperFunctionPointerType) function }

typedef struct _veglDISPATCH
{
	veglCREATECONTEXT				createContext;
	veglDESTROYCONTEXT				destroyContext;
	veglSETCONTEXT					setContext;
	veglFLUSHCONTEXT				flushContext;

	veglFLUSH						flush;
	veglFINISH						finish;

	veglSETBUFFER					setBuffer;
	veglGETCLIENTBUFFER				getClientBuffer;

	veglCREATEIMAGETEXTURE			createImageTexture;
	veglCREATEIMAGERENDERBUFFER		createImageRenderbuffer;
	veglCREATEIMAGEVGPARENTIMAGE	createImageParentImage;
	veglBINDTEXIMAGE				bindTexImage;

	veglPROFILER					profiler;
	veglLOOKUP *					lookup;

    veglQUERYHWVG                   queryHWVG;

    veglRENDER_THREAD               renderThread;
}
veglDISPATCH;

#ifdef __cplusplus
}
#endif

#endif /* __gc_egl_common_h_ */
