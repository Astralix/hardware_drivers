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




#ifndef __gc_egl_h_
#define __gc_egl_h_

#include <EGL/egl.h>
#include <KHR/khrvivante.h>

#include "gc_hal.h"
#include "gc_hal_driver.h"
#include "gc_hal_engine.h"
#include "gc_egl_common.h"

#if gcdSUPPORT_SWAP_RECTANGLE
    #ifndef MAX_NUM_SURFACE_BUFFERS
    #define MAX_NUM_SURFACE_BUFFERS 8
    #endif
#endif

#define EGL_MAX_SWAP_BUFFER_REGION_RECTS 10

#ifdef __cplusplus
extern "C" {
#endif

#define gcdZONE_EGL_API             (gcvZONE_API_EGL | (1 << 0))
#define gcdZONE_EGL_SURFACE         (gcvZONE_API_EGL | (1 << 1))
#define gcdZONE_EGL_CONTEXT         (gcvZONE_API_EGL | (1 << 2))
#define gcdZONE_EGL_CONFIG          (gcvZONE_API_EGL | (1 << 3))
#define gcdZONE_EGL_OS              (gcvZONE_API_EGL | (1 << 4))
#define gcdZONE_EGL_IMAGE           (gcvZONE_API_EGL | (1 << 5))
#define gcdZONE_EGL_SWAP            (gcvZONE_API_EGL | (1 << 6))
#define gcdZONE_EGL_INIT            (gcvZONE_API_EGL | (1 << 7))
#define gcdZONE_EGL_SYNC            (gcvZONE_API_EGL | (1 << 8))
#define glvZONE_EGL_COMPOSE         (gcvZONE_API_EGL | (1 << 9))
#define glvZONE_EGL_RENDER_THREAD   (gcvZONE_API_EGL | (1 << 10))

#include "gc_hal_raster.h"

typedef struct eglDisplay       *VEGLDisplay;
typedef struct eglContext       *VEGLContext;
typedef struct eglConfig        *VEGLConfig;
typedef struct eglSurface       *VEGLSurface;
typedef struct eglImage         *VEGLImage;
typedef struct eglSync          *VEGLSync;
typedef struct eglImageRef      *VEGLImageRef;
typedef struct eglThreadData    *VEGLThreadData;
typedef struct eglWorkerInfo    *VEGLWorkerInfo;
typedef struct eglBackBuffer    *VEGLBackBuffer;
typedef struct eglDisplayInfo   *VEGLDisplayInfo;
typedef struct eglComposition   *VEGLComposition;

#define VEGL_DISPLAY(p)         ((VEGLDisplay) (p))
#define VEGL_CONTEXT(p)         ((VEGLContext) (p))
#define VEGL_CONFIG(p)          ((VEGLConfig)  (p))
#define VEGL_SURFACE(p)         ((VEGLSurface) (p))
#define VEGL_IMAGE(p)           ((VEGLImage)   (p))
#define VEGL_SYNC(p)            ((VEGLSync)    (p))

#define EGL_DISPLAY_SIGNATURE   gcmCC('E','G','L','D')
#define EGL_SURFACE_SIGNATURE   gcmCC('E','G','L','S')
#define EGL_CONTEXT_SIGNATURE   gcmCC('E','G','L','C')
#define EGL_IMAGE_SIGNATURE     gcmCC('E','G','L','I')
#define EGL_SYNC_SIGNATURE      gcmCC('E','G','L','Y')

#define veglUSE_HAL_DUMP        0
#define veglRENDER_LIST_LENGTH  5

struct eglRenderList
{
    gctSIGNAL                   signal;
    gcoSURF                     surface;
    gctPOINTER                  bits;

    struct eglRenderList *      prev;
    struct eglRenderList *      next;
};

struct eglBackBuffer
{
    gctPOINTER                  context;
    gcoSURF                     surface;
	gctUINT32                   offset;
    gcsPOINT                    origin;
    gcsSIZE                     size;
};

struct eglWorkerInfo
{
    gctSIGNAL                   signal;
    gctSIGNAL                   targetSignal;

    /* Owner of this surface. */
    VEGLSurface                 draw;
    void*                       bits;

    struct eglBackBuffer        backBuffer;

	gctINT                      numRects;
    gctINT                      rects[EGL_MAX_SWAP_BUFFER_REGION_RECTS * 4];

    /* Used by eglDisplay worker list. */
    VEGLWorkerInfo              prev;
    VEGLWorkerInfo              next;
};

#ifndef __QNXNTO__
#define EGL_WORKER_COUNT        4
#else
#define EGL_WORKER_COUNT        1
#endif

struct eglThreadData
{
    /* gcoDUMP object. */
    gcoDUMP                     dump;

    /* Last known error code. */
    EGLenum                     error;

    /* Current API. */
    EGLenum                     api;
    EGLBoolean                  es20;

    /* Hardware capabilities. */
    gceCHIPMODEL                chipModel;
    EGLint                      maxWidth;
    EGLint                      maxHeight;
    EGLint                      maxSamples;
    gctBOOL                     openVGpipe;
    gctBOOL                     vaa;

    /* Current context. */
    VEGLContext                 context;
    EGLint                      lastClient;

#if gcdENABLE_VG
    gctINT32                    chipCount;
    gcsHAL_LIMITS               chipLimits[gcdCHIP_COUNT];
#endif

    struct eglWorkerInfo *      worker;

    /* Composition. */
    struct eglComposition *     composition;

#if gcdRENDER_THREADS
    /* Pointer to render thread information for render threads. */
    gcsRENDER_THREAD            *renderThread;
#endif
};

struct eglImageRef
{
    VEGLImageRef                next;
    EGLNativePixmapType         pixmap;
    gcoSURF                     surface;
};

struct eglDisplay
{
    /* Signature. */
    gctUINT                     signature;

    /* Next EGLDisplay. */
    VEGLDisplay                 next;

    /* Handle to device context. */
    NativeDisplayType           hdc;
	gctPOINTER					localInfo;
    gctBOOL                     releaseDpy;

    /* Process handle. */
    gctHANDLE                   process;
    gctHANDLE                   ownerThread;

    /* Number of configurations. */
    EGLint                      configCount;

    /* Pointer to configurations. */
    VEGLConfig                  config;

    /* Allocated resources. */
    VEGLSurface                 surfaceStack;
    VEGLContext                 contextStack;
    VEGLImage                   imageStack;
    VEGLImageRef                imageRefStack;

    /* Reference counter. */
    gcsATOM_PTR                 referenceDpy;

    /* Worker thread for copying data. */
    gctHANDLE                   workerThread;
    gctSIGNAL                   startSignal;
    gctSIGNAL                   stopSignal;
    gctSIGNAL                   doneSignal;
    gctPOINTER                  suspendMutex;

    /* EGL Blob Cache Functions */
    EGLGetBlobFuncANDROID       blobCacheGet;
    EGLSetBlobFuncANDROID       blobCacheSet;

    /* Sentinel for a list of active workers that are queued up. */
    struct eglWorkerInfo        workerSentinel;
};

struct eglDisplayInfo
{
    gctINT32                    width;
    gctINT32                    height;
    gctINT32                    stride;

    gctBOOL                     flip;

    gctPOINTER                  logical;
    gctUINT32                   physical;

#ifndef __QNXNTO__
    /* 355_FB_MULTI_BUFFER */
    gctINT                      multiBuffer;    /* Single(1), Double(2), Triple(3).*/
    gctINT32                    backBufferY;    /* Where the current backbuffer located. */
    gctPOINTER                  logicals[2];   /* Addresses for backbuffers. */
    gctUINT32                   physicals[2];
#else
    gctPOINTER                  logical2;
    gctUINT32                   physical2;
#endif
};

struct eglConfig
{
    EGLint                      bufferSize;
    EGLint                      alphaSize;
    EGLint                      blueSize;
    EGLint                      greenSize;
    EGLint                      redSize;
    EGLint                      depthSize;
    EGLint                      stencilSize;
    EGLenum                     configCaveat;
    EGLint                      configId;
    EGLBoolean                  defaultConfig;
    EGLBoolean                  nativeRenderable;
    EGLint                      nativeVisualType;
    EGLint                      samples;
    EGLint                      sampleBuffers;
    EGLenum                     surfaceType;
    EGLBoolean                  bindToTetxureRGB;
    EGLBoolean                  bindToTetxureRGBA;
    EGLint                      luminanceSize;
    EGLint                      alphaMaskSize;
    EGLenum                     colorBufferType;
    EGLenum                     renderableType;
    EGLenum                     conformant;
    EGLenum                     matchFormat;
    EGLint                      matchNativePixmap;
    EGLint                      width;
    EGLint                      height;
#if defined(ANDROID)
    /* Special config on android to return BGRA visual. */
    EGLBoolean                  swizzleRB;
#endif
    /* EGL_ANDROID_recordable extension. */
    EGLBoolean                  recordableConfig;
};

struct eglSurface
{
    /* Signature. */
    gctUINT                     signature;

    /* Next EGLSurface. */
    VEGLSurface                 next;

    /* Set if VG pipe is present and this surface was created when the current
       API was set to OpenVG. */
    gctBOOL                     openVG;

    /* Swap surface. */
    gcoSURF                     swapSurface;
    gctUINT                     swapBitsPerPixel;

    /* Direct frame buffer access. */
    gcoSURF                     fbWrapper;

	/* 355_FB_MULTI_BUFFER */
#ifndef __QNXNTO__
    gcoSURF                     fbWrappers[2];  /* Up to Triple Buffer at most. */
#else
    gcoSURF                     fbWrapper2;
#endif
    gctBOOL                     fbDirect;
    struct eglDisplayInfo       fbInfo;

    /* Render target array. */
    gctBOOL                     renderListEnable;
    struct eglRenderList *      renderListHead;
    struct eglRenderList *      renderListCurr;
    gctUINT                     renderListCount;

    /* Render target. */
    gcoSURF                     renderTarget;
    void *                      renderTargetBits;
    gceSURF_FORMAT              renderTargetFormat;
    gcsSURF_FORMAT_INFO_PTR     renderTargetInfo[2];

    /* Depth buffer. */
    gcoSURF                     depthBuffer;
    gceSURF_FORMAT              depthFormat;

    /* Resolve surface. */
    gcoSURF                     resolve;
    gceSURF_FORMAT              resolveFormat;
    void *                      resolveBits;
    gctUINT                     resolveBitsPerPixel;

    /* For "render to texture" To bind texture. */
    gcoSURF                     texBinder;

    /* Bitmap data. */
    gctINT                      bitsPerPixel;
    gctINT                      bitsX;
    gctINT                      bitsY;
    gctINT                      bitsWidth;
    gctINT                      bitsHeight;
    gctUINT                     bitsAlignedWidth;
    gctUINT                     bitsAlignedHeight;
    gctINT                      bitsStride;

#if defined(ANDROID)
    /* Swap rectangle. */
    gcsPOINT                    swapOrigin;
    gcsPOINT                    swapSize;

#if gcdSUPPORT_SWAP_RECTANGLE
    gctUINT                     bufferCount;

    /* Swap areas. */
    gcsPOINT                    bufferSwapOrigin[MAX_NUM_SURFACE_BUFFERS];
    gcsPOINT                    bufferSwapSize[MAX_NUM_SURFACE_BUFFERS];

    gctPOINTER                  bufferHandle[MAX_NUM_SURFACE_BUFFERS];
#   endif
#endif

    /* Surface Config Data. */
    struct eglConfig            config;

    /* Reference counter. */
    gctBOOL                     created;
    gcsATOM_PTR                 reference;

    /* Surface type. */
    EGLint                      type;
    EGLenum                     buffer;
    gceSURF_COLOR_TYPE          colorType;
    EGLint                      swapBehavior;

    /* Window attributes. */
    NativeWindowType            hwnd;

    /* PBuffer attributes. */
    EGLBoolean                  largestPBuffer;
    EGLBoolean                  mipmapTexture;
    EGLint                      mipmapLevel;
    EGLenum                     textureFormat;
    EGLenum                     textureTarget;

    /* Pixmap attributes. */
    NativePixmapType            pixmap;

    gcoSURF                     pixmapSurface;

    gctINT                      pixmapStride;
    gctPOINTER                  pixmapBits;

    gctINT                      tempPixmapStride;
    gctPOINTER                  tempPixmapBits;
    gctUINT32                   tempPixmapPhys;

    /* Lock surface attributes. */
    EGLBoolean                  locked;
    gcoSURF                     lockBuffer;
    void *                      lockBits;
    gceSURF_FORMAT              lockBufferFormat;
    gctINT                      lockBufferStride;
    EGLBoolean                  lockPreserve;

    /* Image swap workers. */
    struct eglWorkerInfo        workers[EGL_WORKER_COUNT];
    VEGLWorkerInfo              availableWorkers;
    VEGLWorkerInfo              lastSubmittedWorker;
    gctPOINTER                  workerMutex;

    EGLint                      clippedRects[EGL_MAX_SWAP_BUFFER_REGION_RECTS * 4];

#if defined(ANDROID)
    /* EGL_ANDROID_get_render_buffer. */
    struct eglBackBuffer        backBuffer;

#if (ANDROID_SDK_VERSION >= 11)
    EGLBoolean                  firstFrame;
#   endif

#endif
};

struct eglImage
{
    /* Internal image struct pointer. */
    khrEGL_IMAGE                image;

    /* Signature. */
    gctUINT                     signature;

    /* Next eglImage. */
    VEGLImage                   next;
};

struct eglContext
{
    /* Signature. */
    gctUINT                     signature;

    /* Next EGLContext. */
    VEGLContext                 next;

    /* Bounded thread. */
    VEGLThreadData              thread;

    /* Bounded API. */
    EGLenum                     api;
    EGLint                      client;

    /* Context dispatch table for API. */
    veglDISPATCH *              dispatch;

    /* Attached display. */
    VEGLDisplay                 display;

    /* Shared configuration. */
    VEGLContext                 sharedContext;

    /* Current read/draw surface */
    VEGLSurface                 read;
    VEGLSurface                 draw;

    /* Context for API. */
    void *                      context;

#if gcdRENDER_THREADS
    /* Array of render threads. */
    gcsRENDER_THREAD*           renderThreads[gcdRENDER_THREADS];

    /* Number of allocated render threads. */
    gctUINT                     renderThreadCount;

    /* Index of current render thread. */
    gctUINT                     currentRenderThread;
#endif
};

struct eglSync
{
    /* Signature. */
    gctUINT                     signature;

    /* Type */
    EGLenum                     type;

    /* Type */
    EGLenum                     condition;

    /* Signal */
    gctSIGNAL                   signal;
};

typedef enum _VEGL_COLOR_FORMAT
{
    VEGL_DEFAULT    = (1 << 0),
    VEGL_ALPHA      = (1 << 1),
    VEGL_444        = (1 << 2),
    VEGL_555        = (1 << 3),
    VEGL_565        = (1 << 4),
    VEGL_888        = (1 << 5),
    VEGL_YUV        = (1 << 6),

    VEGL_4444       = VEGL_ALPHA | VEGL_444,
    VEGL_5551       = VEGL_ALPHA | VEGL_555,
    VEGL_8888       = VEGL_ALPHA | VEGL_888
}
VEGL_COLOR_FORMAT;

typedef struct EGL_CONFIG_COLOR
{
    EGLint                      bufferSize;

    EGLint                      redSize;
    EGLint                      greenSize;
    EGLint                      blueSize;
    EGLint                      alphaSize;

    VEGL_COLOR_FORMAT           formatFlags;
}
* VEGLConfigColor;

typedef struct EGL_CONFIG_DEPTH
{
    int depthSize;
    int stencilSize;
}
* VEGLConfigDepth;


/*******************************************************************************
** Thread API functions.
*/

VEGLThreadData
veglGetThreadData(
    void
    );

/*******************************************************************************
** Display API functions.
*/

EGLBoolean
veglReferenceDisplay(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display
    );

void
veglDereferenceDisplay(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display,
    EGLBoolean Always
    );


/*******************************************************************************
** Surface API functions.
*/

EGLBoolean
veglReferenceSurface(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    );

void
veglDereferenceSurface(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface,
    IN EGLBoolean Always
    );

gceSTATUS
veglAddRenderListSurface(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    );

gceSTATUS
veglFreeRenderList(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    );

gceSTATUS
veglSetDriverTarget(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    );

gceSTATUS
veglDriverSurfaceSwap(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    );

EGLint
veglResizeSurface(
    IN VEGLSurface Surface,
    IN gctUINT Width,
    IN gctUINT Height,
    gceSURF_FORMAT ResolveFormat,
    gctUINT BitsPerPixel
    );

/*******************************************************************************
** eglImage API functions.
*/
void
veglDestroyImage(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display,
    IN VEGLImage Image
    );


/*******************************************************************************
** Swap worker API functions.
*/

void
veglSuspendSwapWorker(
    VEGLDisplay Display
    );

void
veglResumeSwapWorker(
    VEGLDisplay Display
    );

VEGLWorkerInfo
veglGetWorker(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display,
    IN VEGLSurface Surface
    );

gctBOOL
veglReleaseWorker(
    VEGLWorkerInfo Worker
    );

VEGLWorkerInfo
veglFreeWorker(
    VEGLWorkerInfo Worker
    );

gctBOOL
veglSubmitWorker(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display,
    IN VEGLWorkerInfo Worker,
    IN gctBOOL ScheduleSignals
    );

#if defined(_WIN32)
#   define veglTHREAD_RETURN DWORD
#else
#   define veglTHREAD_RETURN void *
#endif

#ifndef WINAPI
#   define WINAPI
#endif

veglTHREAD_RETURN WINAPI
veglSwapWorker(
    void* Display
    );

gctHANDLE
veglGetModule(
    gcoOS Os,
    EGLBoolean Egl,
    VEGLContext Context,
    gctINT_PTR Index
    );


/*******************************************************************************
** Bridge functions.
*/

veglDISPATCH *
_GetDispatch(
    VEGLThreadData Thread,
    VEGLContext Context
    );

void *
_CreateApiContext(
    VEGLThreadData Thread,
    VEGLContext Context,
    gcoOS Os,
    gcoHAL Hal,
    void * SharedContext
    );

EGLBoolean
_DestroyApiContext(
    VEGLThreadData Thread,
    VEGLContext Context,
    void * ApiContext
    );

EGLBoolean
_FlushApiContext(
    VEGLThreadData Thread,
    VEGLContext Context,
    void * ApiContext
    );

EGLBoolean
_SetApiContext(
    VEGLThreadData Thread,
    VEGLContext Context,
    void * ApiContext,
    gcoSURF Draw,
    gcoSURF Read,
    gcoSURF Depth
    );

gceSTATUS
_SetBuffer(
    VEGLThreadData Thread,
    gcoSURF Draw
    );

EGLBoolean
_Flush(
    VEGLThreadData Thread
    );

EGLBoolean
_Finish(
    VEGLThreadData Thread
    );

EGLBoolean
_eglProfileCallback(
    VEGLThreadData Thread,
    IN gctUINT32 Enum,
    IN gctUINT32 Value
    );

gcoSURF
_GetClientBuffer(
    VEGLThreadData Thread,
    void * Context,
    EGLClientBuffer buffer
    );

EGLenum
_BindTexImage(
    VEGLThreadData Thread,
    gcoSURF Surface,
    EGLenum Format,
    EGLenum Target,
    EGLBoolean Mipmap,
    EGLint Level,
    gcoSURF *BindTo
    );

EGLenum
_CreateImageTexture(
    VEGLThreadData Thread,
    EGLenum Target,
    gctINT Texture,
    gctINT Level,
    gctINT Depth,
    gctPOINTER Image
    );

EGLenum
_CreateImageFromRenderBuffer(
    VEGLThreadData Thread,
    gctUINT Framebuffer,
    gctPOINTER Image
    );

EGLenum
_CreateImageFromVGParentImage(
    VEGLThreadData  Thread,
    unsigned int    vgimage_obj,
    VEGLImage       eglimage
    );

/* Render thread APIs. */
gctUINT eglfCreateRenderThreads(IN VEGLContext Context);
gctBOOL eglfDestroyRenderThreads(IN VEGLContext Context);
EGLBoolean eglfRenderThreadSwap(IN VEGLThreadData Thread,
                                IN EGLDisplay Display,
                                IN EGLSurface Draw);
EGLBoolean eglfRenderThreadSetContext(IN VEGLContext Context,
                                      IN gcoSURF Draw,
                                      IN gcoSURF Read,
                                      IN gcoSURF Depth);

#ifdef __cplusplus
}
#endif

#endif /* __gc_egl_h_ */
