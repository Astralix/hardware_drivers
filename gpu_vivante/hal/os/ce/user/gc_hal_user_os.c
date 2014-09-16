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




#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include "gc_hal_user.h"

#ifndef TLS_OUT_OF_INDEXES
#define TLS_OUT_OF_INDEXES 0xFFFFFFFF
#endif

#define MAX_PATH_LENGTH 1024

#define _GC_OBJ_ZONE    gcvZONE_OS

#ifdef GENERATE_K_HAL
HANDLE GPU_Open(
    DWORD Reference,
    DWORD access,
    DWORD shareMode
    );

gctBOOL
GPU_Close(
    DWORD Reference
    );

gctBOOL GPU_IOControl(
    DWORD Reference,
    DWORD IoControlCode,
    PBYTE InputBuffer,
    DWORD InputBufferSize,
    PBYTE OutputBuffer,
    DWORD OutputBufferSize,
    PDWORD ReturnedBytes
    );
#endif

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
static void _ReportDB(
    void
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmVERIFY_OK(gcoOS_ZeroMemory(&iface, sizeof(iface)));

    iface.command = gcvHAL_DATABASE;
    iface.u.Database.processID = (gctUINT32) gcoOS_GetCurrentProcessID();
    iface.u.Database.validProcessID = gcvTRUE;

    /* Call kernel service. */
    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        ));

    if ((iface.u.Database.vidMem.counters.bytes     != 0) ||
        (iface.u.Database.nonPaged.counters.bytes   != 0) ||
        (iface.u.Database.contiguous.counters.bytes != 0))
    {
        gcmTRACE(gcvLEVEL_ERROR, "\n");
        gcmTRACE(gcvLEVEL_ERROR, "******* MEMORY LEAKS DETECTED *******\n");
    }

    if (iface.u.Database.vidMem.counters.bytes != 0)
    {
        gcmTRACE(gcvLEVEL_ERROR, "\n");
        gcmTRACE(gcvLEVEL_ERROR, "vidMem.bytes      = %d\n", iface.u.Database.vidMem.counters.bytes);
        gcmTRACE(gcvLEVEL_ERROR, "vidMem.maxBytes   = %d\n", iface.u.Database.vidMem.counters.maxBytes);
        gcmTRACE(gcvLEVEL_ERROR, "vidMem.totalBytes = %d\n", iface.u.Database.vidMem.counters.totalBytes);
    }

    if (iface.u.Database.nonPaged.counters.bytes != 0)
    {
        gcmTRACE(gcvLEVEL_ERROR, "\n");
        gcmTRACE(gcvLEVEL_ERROR, "nonPaged.bytes      = %d\n", iface.u.Database.nonPaged.counters.bytes);
        gcmTRACE(gcvLEVEL_ERROR, "nonPaged.maxBytes   = %d\n", iface.u.Database.nonPaged.counters.maxBytes);
        gcmTRACE(gcvLEVEL_ERROR, "nonPaged.totalBytes = %d\n", iface.u.Database.nonPaged.counters.totalBytes);
    }

    if (iface.u.Database.contiguous.counters.bytes != 0)
    {
        gcmTRACE(gcvLEVEL_ERROR, "\n");
        gcmTRACE(gcvLEVEL_ERROR, "contiguous.bytes      = %d\n", iface.u.Database.contiguous.counters.bytes);
        gcmTRACE(gcvLEVEL_ERROR, "contiguous.maxBytes   = %d\n", iface.u.Database.contiguous.counters.maxBytes);
        gcmTRACE(gcvLEVEL_ERROR, "contiguous.totalBytes = %d\n", iface.u.Database.contiguous.counters.totalBytes);
    }

OnError:;
}
#endif

/******************************************************************************\
***************************** gcoOS Object Structure ****************************
\******************************************************************************/

struct _gcoOS
{
    /* Object. */
    gcsOBJECT                   object;

    /* Context. */
    gctPOINTER                  context;

    /* Heap. */
    gcoHEAP                     heap;

    /* Base address. */
    gctUINT32                   baseAddress;

#if VIVANTE_PROFILER
    gctUINT32                   allocCount;
    gctSIZE_T                   allocSize;
    gctSIZE_T                   maxAllocSize;
    gctUINT32                   freeCount;
    gctSIZE_T                   freeSize;
    LARGE_INTEGER               startTime;
    LARGE_INTEGER               endTime;
#endif
    /* Handle to the device. */
    HANDLE                      device;
    gcsATOM_PTR                 reference;

#ifdef GENERATE_K_HAL
    gcsTLS_PTR                    tls;
#endif
};

/******************************************************************************\
*********************************** Globals ************************************
\******************************************************************************/

static DWORD g_ProcessKey = TLS_OUT_OF_INDEXES;

gcsPLS gcPLS =
{
    gcvNULL,        /* gcoOS object.      */
    gcvNULL,        /* gcoHAL object.     */

    0,              /* internalSize       */
    gcvNULL,        /* internalPhysical   */
    gcvNULL,        /* internalLogical    */

    0,              /* externalSize       */
    gcvNULL,        /* externalPhysical   */
    gcvNULL,        /* externalLogical    */

    0,              /* contiguousSize     */
    gcvNULL,        /* contiguousPhysical */
    gcvNULL,        /* contiguousLogical  */

    gcvNULL         /* eglSurfaceInfo */
};

/******************************************************************************\
************************* Process/Thread Local Storage *************************
\******************************************************************************/
#ifndef GENERATE_K_HAL
static void
_TLSDestructor(
    gctPOINTER TLS
    )
{
    gcsTLS_PTR tls;

    gcmHEADER_ARG("TLS=0x%x", TLS);

    tls = (gcsTLS_PTR) TLS;
    gcmASSERT(tls != gcvNULL);

    if (tls->destructor != gcvNULL)
    {
        tls->destructor(tls);
        tls->destructor = gcvNULL;
    }

    if (tls->hardware != gcvNULL)
    {
        gceHARDWARE_TYPE type = tls->currentType;
        tls->currentType = gcvHARDWARE_3D;

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_HARDWARE,
            "%s(%d): destroying hardware object 0x%08X.",
            __FUNCTION__, __LINE__, tls->hardware
            );

        gcmVERIFY_OK(gcoHARDWARE_Destroy(tls->hardware));
        tls->hardware = gcvNULL;
        tls->currentType = type;
    }

    if (tls->engine2D != gcvNULL)
    {
        gcmVERIFY_OK(gco2D_Destroy(tls->engine2D));
        tls->engine2D = gcvNULL;
    }

    if (tls->hardware2D != gcvNULL)
    {
        gceHARDWARE_TYPE type = tls->currentType;
        tls->currentType = gcvHARDWARE_2D;

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_HARDWARE,
            "%s(%d): destroying hardware object 0x%08X.",
            __FUNCTION__, __LINE__, tls->hardware2D
            );

        gcmVERIFY_OK(gcoHARDWARE_Destroy(tls->hardware2D));
        tls->hardware2D = gcvNULL;
        tls->currentType = type;
    }

#if gcdENABLE_VG
    if (tls->engineVG != gcvNULL)
    {
        gcmVERIFY_OK(gcoVG_Destroy(tls->engineVG));
        tls->engineVG = gcvNULL;
    }

    if (tls->vg != gcvNULL)
    {
        gceHARDWARE_TYPE type = tls->currentType;
        tls->currentType = gcvHARDWARE_VG;

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcvZONE_HARDWARE,
            "%s(%d): destroying hardware object 0x%08X.",
            __FUNCTION__, __LINE__, tls->vg
            );

        gcmVERIFY_OK(gcoVGHARDWARE_Destroy(tls->vg));
        tls->vg = gcvNULL;
        tls->currentType = type;

    }
#endif

    gcmVERIFY_OK(gcoOS_FreeMemory(gcvNULL, tls));

    gcmVERIFY(TlsSetValue(g_ProcessKey, gcvNULL));

    gcmFOOTER_NO();
}
#endif

static gceSTATUS
_MapMemory(
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T NumberOfBytes,
    OUT gctPOINTER * Logical
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Physical=0x%x NumberOfBytes=%lu", Physical, NumberOfBytes);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(NumberOfBytes > 0);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    /* Call kernel API to unmap the memory. */
    iface.command              = gcvHAL_MAP_MEMORY;
    iface.u.MapMemory.physical = Physical;
    iface.u.MapMemory.bytes    = NumberOfBytes;

    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        ));

    /* Return logical address. */
    *Logical = iface.u.MapMemory.logical;

    /* Success. */
    gcmFOOTER_ARG("*Logical=0x%x", *Logical);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_UnmapMemory(
    IN gctPHYS_ADDR Physical,
    IN gctSIZE_T NumberOfBytes,
    IN gctPOINTER Logical
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Physical=0x%x NumberOfBytes=%lu Logical=0x%x",
                  Physical, NumberOfBytes, Logical);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(NumberOfBytes > 0);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    /* Call kernel API to unmap the memory. */
    iface.command                = gcvHAL_UNMAP_MEMORY;
    iface.u.UnmapMemory.physical = Physical;
    iface.u.UnmapMemory.bytes    = NumberOfBytes;
    iface.u.UnmapMemory.logical  = Logical;

    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, sizeof(iface),
        &iface, sizeof(iface)
        ));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static void _ModuleConstructor(
    void
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Each process gets its own objects. */
    gcmASSERT(gcPLS.hal == gcvNULL);

    gcmASSERT(gcPLS.internalLogical   == gcvNULL);
    gcmASSERT(gcPLS.externalLogical   == gcvNULL);
    gcmASSERT(gcPLS.contiguousLogical == gcvNULL);

#ifndef GENERATE_K_HAL
    gcmASSERT(gcPLS.os  == gcvNULL);

    /* Create key. */
    g_ProcessKey = TlsAlloc();

    if (g_ProcessKey == TLS_OUT_OF_INDEXES)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): TlsAlloc failed\n",
            __FUNCTION__, __LINE__
            );

        gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

    /* Construct OS object. */
    gcmONERROR(gcoOS_Construct(gcvNULL, gcvNULL));
#endif

    /* Construct gcoHAL object. */
    gcmONERROR(gcoHAL_Construct(gcvNULL, gcvNULL, &gcPLS.hal));

    /* Query the video memory sizes. */
    gcmONERROR(gcoOS_QueryVideoMemory(
        gcPLS.os,
        &gcPLS.internalPhysical,
        &gcPLS.internalSize,
        &gcPLS.externalPhysical,
        &gcPLS.externalSize,
        &gcPLS.contiguousPhysical,
        &gcPLS.contiguousSize
        ));

    /* Map internal video memory. */
    if (gcPLS.internalSize != 0)
    {
        gcmONERROR(_MapMemory(
             gcPLS.internalPhysical,
             gcPLS.internalSize,
            &gcPLS.internalLogical
            ));
    }

    /* Map external video memory. */
    if (gcPLS.externalSize != 0)
    {
        gcmONERROR(_MapMemory(
             gcPLS.externalPhysical,
             gcPLS.externalSize,
            &gcPLS.externalLogical
            ));
    }

    /* Map contiguous video memory. */
    if (gcPLS.contiguousSize != 0)
    {
        gcmONERROR(_MapMemory(
             gcPLS.contiguousPhysical,
             gcPLS.contiguousSize,
            &gcPLS.contiguousLogical
            ));
    }

    gcmFOOTER_ARG(
        "gcPLS.os=0x%08X, gcPLS.hal=0x%08X"
        " internal=0x%08X external=0x%08X contiguous=0x%08X",
        gcPLS.os, gcPLS.hal,
        gcPLS.internalLogical, gcPLS.externalLogical, gcPLS.contiguousLogical
        );

    return;

OnError:
    gcmFOOTER();
}

static void
_ModuleDestructor(
    void
    )
{
    gcsTLS_PTR tls;

    gcmHEADER();

#ifndef GENERATE_K_HAL
    gcoOS_FreeThreadData(gcvTRUE);

    tls = (gcsTLS_PTR) TlsGetValue(g_ProcessKey);
#else
    tls = gcPLS.os->tls;
#endif
    if (tls != gcvNULL)
    {
        if (tls->hardware != gcvNULL)
        {
            gceHARDWARE_TYPE type = tls->currentType;
            tls->currentType = gcvHARDWARE_3D;

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_HARDWARE,
                "%s(%d): destroying hardware object 0x%08X.",
                __FUNCTION__, __LINE__, tls->hardware
                );

            gcmVERIFY_OK(gcoHARDWARE_Destroy(tls->hardware));
            tls->hardware = gcvNULL;
            tls->currentType = type;
        }

        if (tls->hardware2D != gcvNULL)
        {
            gceHARDWARE_TYPE type = tls->currentType;
            tls->currentType = gcvHARDWARE_2D;

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_HARDWARE,
                "%s(%d): destroying hardware object 0x%08X.",
                __FUNCTION__, __LINE__, tls->hardware2D
                );

            gcmVERIFY_OK(gcoHARDWARE_Destroy(tls->hardware2D));
            tls->hardware2D = gcvNULL;
            tls->currentType = type;
        }

#if gcdENABLE_VG
        if (tls->engineVG != gcvNULL)
        {
            gcmVERIFY_OK(gcoVG_Destroy(tls->engineVG));
            tls->engineVG = gcvNULL;
        }

        if (tls->vg != gcvNULL)
        {
            gceHARDWARE_TYPE type = tls->currentType;
            tls->currentType = gcvHARDWARE_VG;

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_HARDWARE,
                "%s(%d): destroying hardware object 0x%08X.",
                __FUNCTION__, __LINE__, tls->vg
                );

            gcmVERIFY_OK(gcoVGHARDWARE_Destroy(tls->vg));
            tls->vg = gcvNULL;
            tls->currentType = type;

        }
#endif
    }

    if (gcPLS.contiguousLogical != gcvNULL)
    {
        gcmVERIFY_OK(_UnmapMemory(
            gcPLS.contiguousPhysical,
            gcPLS.contiguousSize,
            gcPLS.contiguousLogical
            ));

        gcPLS.contiguousLogical = gcvNULL;
    }

    if (gcPLS.externalLogical != gcvNULL)
    {
        gcmVERIFY_OK(_UnmapMemory(
            gcPLS.externalPhysical,
            gcPLS.externalSize,
            gcPLS.externalLogical
            ));

        gcPLS.externalLogical = gcvNULL;
    }

    if (gcPLS.internalLogical != gcvNULL)
    {
        gcmVERIFY_OK(_UnmapMemory(
            gcPLS.internalPhysical,
            gcPLS.internalSize,
            gcPLS.internalLogical
            ));

        gcPLS.internalLogical = gcvNULL;
    }

    if (gcPLS.hal != gcvNULL)
    {
        gcmVERIFY_OK(gcoHAL_Destroy(gcPLS.hal));
        gcPLS.hal = gcvNULL;
    }

#ifndef GENERATE_K_HAL
    gcmVERIFY_OK(gcoOS_Destroy(gcPLS.os));

    if (g_ProcessKey != TLS_OUT_OF_INDEXES)
    {
        TlsFree(g_ProcessKey);
        g_ProcessKey = TLS_OUT_OF_INDEXES;
    }
#else
    gcmVERIFY_OK(gcoOS_FreeMemory(gcvNULL, tls));

    gcPLS.os->tls = gcvNULL;
#endif

    gcmFOOTER_NO();
}

gctBOOL WINAPI
DllMain(
    IN HINSTANCE Instance,
    IN DWORD Reason,
    IN LPVOID Reserved
    )
{
#ifndef GENERATE_K_HAL
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        _ModuleConstructor();
        break;

    case DLL_PROCESS_DETACH:
        _ModuleDestructor();
        break;

    case DLL_THREAD_DETACH:
        gcoOS_FreeThreadData(gcvFALSE);
        break;
    }
#endif
    return gcvTRUE;
}

/******************************************************************************\
********************************* gcoOS API Code *******************************
\******************************************************************************/

/*******************************************************************************
 **
 ** gcoOS_GetPLSValue
 **
 ** Get value associated with the given key.
 **
 ** INPUT:
 **
 **     gcePLS_VALUE key
 **         key to look up.
 **
 ** OUTPUT:
 **
 **     None
 **
 ** RETURN:
 **
 **     gctPOINTER
 **         Pointer to object associated with key.
 */
gctPOINTER
gcoOS_GetPLSValue(
    IN gcePLS_VALUE key
    )
{
    switch (key)
    {
        case gcePLS_VALUE_EGL_DISPLAY_INFO :
            return gcPLS.eglDisplayInfo;

        case gcePLS_VALUE_EGL_SURFACE_INFO :
            return gcPLS.eglSurfaceInfo;
    }
    return gcvNULL;
}

/*******************************************************************************
 **
 ** gcoOS_SetPLSValue
 **
 ** Associated object represented by 'value' with the given key.
 **
 ** INPUT:
 **
 **     gcePLS_VALUE key
 **         key to associate.
 **
 **     gctPOINTER value
 **         value to associate with key.
 **
 ** OUTPUT:
 **
 **     None
 **
 */
void
gcoOS_SetPLSValue(
    IN gcePLS_VALUE key,
    IN gctPOINTER value
    )
{
    switch (key)
    {
        case gcePLS_VALUE_EGL_DISPLAY_INFO :
            gcPLS.eglDisplayInfo = value;
            return;

        case gcePLS_VALUE_EGL_SURFACE_INFO :
            gcPLS.eglSurfaceInfo = value;
            return;
    }
}

/*******************************************************************************
 **
 ** gcoOS_GetTLS
 **
 ** Get access to the thread local storage.
 **
 ** INPUT:
 **
 **     Nothing.
 **
 ** OUTPUT:
 **
 **     gcsTLS_PTR * TLS
 **         Pointer to a variable that will hold the pointer to the TLS.
 */
gceSTATUS
gcoOS_GetTLS(
    OUT gcsTLS_PTR * TLS
    )
{
    gceSTATUS status;
    gcsTLS_PTR tls;

    gcmHEADER_ARG("TLS=%p", TLS);

#ifndef GENERATE_K_HAL
    tls = (gcsTLS_PTR) TlsGetValue(g_ProcessKey);
#else
    tls = gcPLS.os->tls;
#endif
    if (tls == NULL)
    {
        gcmONERROR(gcoOS_AllocateMemory(
            gcvNULL, gcmSIZEOF(gcsTLS), (gctPOINTER *) &tls
            ));

        gcmONERROR(gcoOS_ZeroMemory(
            tls, gcmSIZEOF(gcsTLS)
            ));

        /* The default hardware type is 2D */
        tls->currentType = gcvHARDWARE_2D;

#ifndef GENERATE_K_HAL
        if (!TlsSetValue(g_ProcessKey, tls))
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): TlsSetValue failed.\n",
                __FUNCTION__, __LINE__
                );

            gcmONERROR(gcvSTATUS_GENERIC_IO);
        }
#else
        gcPLS.os->tls = tls;
#endif
    }

    * TLS = tls;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (tls != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_FreeMemory(gcvNULL, (gctPOINTER) tls));
    }

    * TLS = gcvNULL;

    gcmFOOTER();
    return status;
}

#ifndef GENERATE_K_HAL
/*******************************************************************************
**
**  gcoOS_FreeThreadData
**
**  Destroy the objects associated with the current thread.
**
**  INPUT:
**
**       gctBOOL ProcessExiting
**          Indicates whether process is terminating.
**
**  OUTPUT:
**
**      Nothing.
*/
void
gcoOS_FreeThreadData(
    gctBOOL ProcessExiting
    )
{
    gctPOINTER tls;

    tls = TlsGetValue(g_ProcessKey);

    if (tls != NULL)
    {
        _TLSDestructor(tls);
    }
}
#endif

/*******************************************************************************
**
**  gcoOS_Construct
**
**  Construct a new gcoOS object.
**
**  INPUT:
**
**      gctPOINTER Context
**          Pointer to an OS specific context.
**
**  OUTPUT:
**
**      gcoOS * Os
**          Pointer to a variable that will hold the pointer to the gcoOS object.
*/
gceSTATUS
gcoOS_Construct(
    IN gctPOINTER Context,
    OUT gcoOS * Os
    )
{
    gcoOS os = gcPLS.os;
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x", Context);

#ifdef HEAP_DEBUG
    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    _CrtSetDbgFlag(flags | _CRTDBG_CHECK_ALWAYS_DF);
#endif

    if (os == gcvNULL)
    {
        /* Allocate the gcoOS structure. */
        os = malloc(gcmSIZEOF(struct _gcoOS));
        if (os == gcvNULL)
        {
            gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }

        gcoOS_ZeroMemory(os, gcmSIZEOF(struct _gcoOS));

        /* Initialize the gcoOS object. */
        os->object.type = gcvOBJ_OS;
        os->context     =  Context;
        os->heap        =  gcvNULL;
        os->baseAddress =  0;
        os->device      = INVALID_HANDLE_VALUE;
        os->reference   = gcvNULL;

        /* Set the object pointer to PLS. */
        gcmASSERT(gcPLS.os == gcvNULL);
        gcPLS.os = os;

#ifdef GENERATE_K_HAL
        os->device = GPU_Open(
            IOCTL_GCHAL_KERNEL_INTERFACE,
            IOCTL_GCHAL_KERNEL_INTERFACE,
            IOCTL_GCHAL_KERNEL_INTERFACE
            );
#else
        /* Load the kernel library. */
        os->device = CreateFile(_T("GPU1:"),
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            gcvNULL,
                            OPEN_EXISTING,
                            0,
                            gcvNULL);

        if (os->device == INVALID_HANDLE_VALUE)
        {
            free(os);
            /* Generic I/O error. */
            gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
            return gcvSTATUS_GENERIC_IO;
        }
#endif

        /* Construct heap. */
        status = gcoHEAP_Construct(gcvNULL, gcdHEAP_SIZE, &os->heap);

        if (gcmIS_ERROR(status))
        {
            gcmTRACE_ZONE(
                gcvLEVEL_WARNING, gcvZONE_OS,
                "%s(%d): Could not construct gcoHEAP (%d).",
                __FUNCTION__, __LINE__, status
                );

            os->heap = gcvNULL;
        }

        /* Query base address. */
        iface.command = gcvHAL_GET_BASE_ADDRESS;

        /* Call kernel driver. */
        status = gcoOS_DeviceControl(
            gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            &iface, gcmSIZEOF(iface),
            &iface, gcmSIZEOF(iface)
            );

        if (gcmIS_SUCCESS(status) && gcmIS_SUCCESS(iface.status))
        {
            os->baseAddress = iface.u.GetBaseAddress.baseAddress;

            gcmTRACE_ZONE(
                gcvLEVEL_INFO, gcvZONE_OS,
                "%s(%d): baseAddress is 0x%08X.",
                __FUNCTION__, __LINE__, os->baseAddress
                );
        }
        else
        {
            gcmTRACE_ZONE(
                gcvLEVEL_WARNING, gcvZONE_OS,
                "%s(%d): Setting default baseAddress of 0.",
                __FUNCTION__, __LINE__
                );
        }

        /* Construct the reference atom. */
        gcmONERROR(gcoOS_AtomConstruct(os, &os->reference));

#if VIVANTE_PROFILER
        /* Start profiler. */
        gcoOS_ProfileStart(os);
#endif

#ifdef GENERATE_K_HAL
        _ModuleConstructor();
#endif
    }

    /* Increment the reference. */
    gcmONERROR(gcoOS_AtomIncrement(os, os->reference, gcvNULL));

    /* Return pointer to the gcoOS object. */
    if (Os != gcvNULL)
    {
        *Os = os;
    }

    /* Success. */
    gcmFOOTER_ARG("*Os=0x%x", os);
    return gcvSTATUS_OK;

OnError:
    /* Roll back. */
    gcmVERIFY_OK(gcoOS_Destroy(gcvNULL));
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_Destroy
**
**  Destroys an gcoOS object.
**
**  ARGUMENTS:
**
**      gcoOS Os
**          Pointer to the gcoOS object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
**
*/
gceSTATUS
gcoOS_Destroy(
    IN gcoOS Os
    )
{
    gctINT reference = 0;
    gceSTATUS status;

    gcmHEADER();

    if (gcPLS.os != gcvNULL)
    {
        if (gcPLS.os->reference != gcvNULL)
        {
            /* Decrement the reference. */
            gcmONERROR(gcoOS_AtomDecrement(gcPLS.os,
                                           gcPLS.os->reference,
                                           &reference));

            /* Check if there are still more references. */
            if (reference > 1)
            {
                /* Success. */
                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
        }

#ifdef GENERATE_K_HAL
        _ModuleDestructor();
#endif
        if (gcPLS.os->reference != gcvNULL)
        {
            /* Destroy the reference counter. */
            gcmONERROR(gcoOS_AtomDestroy(gcPLS.os, gcPLS.os->reference));
            gcPLS.os->reference = gcvNULL;
        }

        if (gcPLS.os->heap != gcvNULL)
        {
            gcoHEAP heap = gcPLS.os->heap;

#if VIVANTE_PROFILER
            /* End profiler. */
            gcoHEAP_ProfileEnd(heap, "gcoOS_HEAP");
#endif

            /* Mark the heap as gone. */
            gcPLS.os->heap = gcvNULL;

            /* Destroy the heap. */
            gcmONERROR(gcoHEAP_Destroy(heap));
        }

        /* Close the handle to the kernel service. */
        if (gcPLS.os->device != INVALID_HANDLE_VALUE)
        {
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
            _ReportDB();
#endif

#ifdef GENERATE_K_HAL
            GPU_Close(
                IOCTL_GCHAL_KERNEL_INTERFACE
                );
#else
            CloseHandle(gcPLS.os->device);
#endif
            gcPLS.os->device = gcvNULL;
        }

#if VIVANTE_PROFILER /*gcdENABLE_PROFILING*/
        /* End profiler. */
        gcoOS_ProfileEnd(gcPLS.os, "gcoOS_Destroy");
#endif

        /* Mark the gcoOS object as unknown. */
        gcPLS.os->object.type = gcvOBJ_UNKNOWN;

        /* Free the gcoOS structure. */
        free(gcPLS.os);

        /* Reset PLS object. */
        gcPLS.os = gcvNULL;
    }

    /* Success. */
    gcmFOOTER_KILL();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_QueryVideoMemory
**
**  Query the amount of video memory.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to a gcoOS object.
**
**  OUTPUT:
**
**      gctPHYS_ADDR * InternalAddress
**          Pointer to a variable that will hold the physical address of the
**          internal memory.  If 'InternalAddress' is gcvNULL, no information
**          about the internal memory will be returned.
**
**      gctSIZE_T * InternalSize
**          Pointer to a variable that will hold the size of the internal
**          memory.  'InternalSize' cannot be gcvNULL if 'InternalAddress' is
**          not gcvNULL.
**
**      gctPHYS_ADDR * ExternalAddress
**          Pointer to a variable that will hold the physical address of the
**          external memory.  If 'ExternalAddress' is gcvNULL, no information
**          about the external memory will be returned.
**
**      gctSIZE_T * ExternalSize
**          Pointer to a variable that will hold the size of the external
**          memory.  'ExternalSize' cannot be gcvNULL if 'ExternalAddress' is
**          not gcvNULL.
**
**      gctPHYS_ADDR * ContiguousAddress
**          Pointer to a variable that will hold the physical address of the
**          contiguous memory.  If 'ContiguousAddress' is gcvNULL, no
**          information about the contiguous memory will be returned.
**
**      gctSIZE_T * ContiguousSize
**          Pointer to a variable that will hold the size of the contiguous
**          memory.  'ContiguousSize' cannot be gcvNULL if 'ContiguousAddress'
**          is not gcvNULL.
*/
gceSTATUS
gcoOS_QueryVideoMemory(
    IN gcoOS Os,
    OUT gctPHYS_ADDR * InternalAddress,
    OUT gctSIZE_T * InternalSize,
    OUT gctPHYS_ADDR * ExternalAddress,
    OUT gctSIZE_T * ExternalSize,
    OUT gctPHYS_ADDR * ContiguousAddress,
    OUT gctSIZE_T * ContiguousSize
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER();

        /* Call kernel HAL to query video memory. */
    iface.command = gcvHAL_QUERY_VIDEO_MEMORY;

        /* Call kernel service. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
            IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

        if (InternalAddress != gcvNULL)
        {
            /* Verify arguments. */
            gcmVERIFY_ARGUMENT(InternalSize != gcvNULL);

            /* Save internal memory size. */
        *InternalAddress = iface.u.QueryVideoMemory.internalPhysical;
        *InternalSize    = iface.u.QueryVideoMemory.internalSize;
        }

        if (ExternalAddress != gcvNULL)
        {
            /* Verify arguments. */
            gcmVERIFY_ARGUMENT(ExternalSize != gcvNULL);

            /* Save external memory size. */
        *ExternalAddress = iface.u.QueryVideoMemory.externalPhysical;
        *ExternalSize    = iface.u.QueryVideoMemory.externalSize;
        }

        if (ContiguousAddress != gcvNULL)
        {
            /* Verify arguments. */
            gcmVERIFY_ARGUMENT(ContiguousSize != gcvNULL);

            /* Save contiguous memory size. */
        *ContiguousAddress = iface.u.QueryVideoMemory.contiguousPhysical;
        *ContiguousSize    = iface.u.QueryVideoMemory.contiguousSize;
        }

    /* Success. */
    gcmFOOTER_ARG("*InternalAddress=0x%08x *InternalSize=%lu "
                  "*ExternalAddress=0x%08x *ExternalSize=%lu "
                  "*ContiguousAddress=0x%08x *ContiguousSize=%lu",
                  gcmOPT_VALUE(InternalAddress), gcmOPT_VALUE(InternalSize),
                  gcmOPT_VALUE(ExternalAddress), gcmOPT_VALUE(ExternalSize),
                  gcmOPT_VALUE(ContiguousAddress),
                  gcmOPT_VALUE(ContiguousSize));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_GetBaseAddress
**
**  Get the base address for the physical memory.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to the gcoOS object.
**
**  OUTPUT:
**
**      gctUINT32_PTR BaseAddress
**          Pointer to a variable that will receive the base address.
*/
gceSTATUS
gcoOS_GetBaseAddress(
    IN gcoOS Os,
    OUT gctUINT32_PTR BaseAddress
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(BaseAddress != gcvNULL);

    /* Return base address. */
    *BaseAddress = gcPLS.os->baseAddress;

    /* Success. */
    gcmFOOTER_ARG("*BaseAddress=0x%08x", *BaseAddress);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Allocate
**
**  Allocate memory from the user heap.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctSIZE_T Bytes
**          Number of bytes to allocate.
**
**  OUTPUT:
**
**      gctPOINTER * Memory
**          Pointer to a variable that will hold the pointer to the memory
**          allocation.
*/
gceSTATUS
gcoOS_Allocate(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Bytes=%lu", Bytes);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Bytes > 0);
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);

    if ((gcPLS.os != gcvNULL) && (gcPLS.os->heap != gcvNULL))
    {
        gcmONERROR(gcoHEAP_Allocate(gcPLS.os->heap, Bytes, Memory));
    }
    else
    {
        gcmONERROR(gcoOS_AllocateMemory(gcPLS.os, Bytes, Memory));
    }

    /* Success. */
    gcmFOOTER_ARG("*Memory=0x%x", *Memory);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_GetMemorySize
**
**  Get allocated memory from the user heap.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER  Memory
**          Pointer to the memory
**          allocation.
**
**  OUTPUT:
**
**      gctPOINTER MemorySize
**          Pointer to a variable that will hold the pointer to the memory
**          size.
*/
gceSTATUS
gcoOS_GetMemorySize(
    IN gcoOS Os,
    IN gctPOINTER Memory,
    OUT gctSIZE_T_PTR MemorySize
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Memory=0x%x", Memory);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);
    gcmVERIFY_ARGUMENT(MemorySize != gcvNULL);

    /* Free the memory. */
    if ((gcPLS.os != gcvNULL) && (gcPLS.os->heap != gcvNULL))
    {
        gcmONERROR(gcoHEAP_GetMemorySize(gcPLS.os->heap, Memory, MemorySize));
    }
    else
    {
        *MemorySize = 0;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
 **
**  gcoOS_Free
**
**  Free allocated memory from the user heap.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER Memory
**          Pointer to the memory allocation that needs to be freed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_Free(
    IN gcoOS Os,
    IN gctPOINTER Memory
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Memory=0x%x", Memory);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);

    /* Free the memory. */
    if ((gcPLS.os != gcvNULL) && (gcPLS.os->heap != gcvNULL))
    {
        gcmONERROR(gcoHEAP_Free(gcPLS.os->heap, Memory));
    }
    else
    {
        gcmONERROR(gcoOS_FreeMemory(gcPLS.os, Memory));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
 ** gcoOS_AllocateMemory
 **
 ** Allocate memory from the user heap.
 **
 ** INPUT:
 **
 **     gcoOS Os
 **         Pointer to an gcoOS object.
 **
 **     gctSIZE_T Bytes
 **         Number of bytes to allocate.
 **
 ** OUTPUT:
 **
 **     gctPOINTER * Memory
 **         Pointer to a variable that will hold the pointer to the memory
 **         allocation.
 */
gceSTATUS
gcoOS_AllocateMemory(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;
    gctPOINTER memory;

    gcmHEADER_ARG("Bytes=%lu", Bytes);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Bytes > 0);
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);

    /* Allocate the memory. */
#if VIVANTE_PROFILER
    memory = malloc(Bytes + gcmSIZEOF(gctSIZE_T));
#else
    memory = malloc(Bytes);
#endif

    if (memory == gcvNULL)
    {
        /* Out of memory. */
        gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
    }

#if VIVANTE_PROFILER
    {
        gcoOS os = (gcPLS.os != gcvNULL) ? gcPLS.os : Os;

        ++ (os->allocCount);
        os->allocSize += Bytes;
        if (os->allocSize > os->maxAllocSize)
        {
            os->maxAllocSize = os->allocSize;
        }

        /* Return pointer to the memory allocation. */
        *(gctSIZE_T *) memory = Bytes;
        *Memory = (gctPOINTER) ((gctSIZE_T *) memory + 1);
    }
#else
    /* Return pointer to the memory allocation. */
    *Memory = memory;
#endif

    /* Success. */
    gcmFOOTER_ARG("*Memory=0x%x", *Memory);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
 **
 ** gcoOS_FreeMemory
 **
 ** Free allocated memory from the user heap.
 **
 ** INPUT:
 **
 **     gcoOS Os
 **         Pointer to an gcoOS object.
 **
 **     gctPOINTER Memory
 **         Pointer to the memory allocation that needs to be freed.
 **
 ** OUTPUT:
 **
 **     Nothing.
 */
gceSTATUS
gcoOS_FreeMemory(
    IN gcoOS Os,
    IN gctPOINTER Memory
    )
{
    gcmHEADER_ARG("Memory=0x%x", Memory);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);

    /* Free the memory allocation. */
#if VIVANTE_PROFILER
    {
        gcoOS os = (gcPLS.os != gcvNULL) ? gcPLS.os : Os;

        os->freeSize += *((gctSIZE_T *) Memory - 1);
        free((gctSIZE_T *) Memory - 1);
        ++ (os->freeCount);
    }
#else
    free(Memory);
#endif

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
 **
**  gcoOS_DeviceControl
**
**  Perform a device I/O control call to the kernel API.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctUINT32 IoControlCode
**          I/O control code to execute.
**
**      gctPOINTER InputBuffer
**          Pointer to the input buffer.
**
**      gctSIZE_T InputBufferSize
**          Size of the input buffer in bytes.
**
**      gctSIZE_T outputBufferSize
**          Size of the output buffer in bytes.
**
**  OUTPUT:
**
**      gctPOINTER OutputBuffer
**          Output buffer is filled with the data returned from the kernel HAL
**          layer.
*/
gceSTATUS
gcoOS_DeviceControl(
    IN gcoOS Os,
    IN gctUINT32 IoControlCode,
    IN gctPOINTER InputBuffer,
    IN gctSIZE_T InputBufferSize,
    OUT gctPOINTER OutputBuffer,
    IN gctSIZE_T OutputBufferSize
    )
{
    gctUINT32 returned;
    gceSTATUS status;
    gcsTLS_PTR tls;

	gcsHAL_INTERFACE_PTR inputBuffer = (gcsHAL_INTERFACE_PTR) InputBuffer;
    gcsHAL_INTERFACE_PTR outputBuffer = (gcsHAL_INTERFACE_PTR) OutputBuffer;

    /* Set current hardware type */
    switch (inputBuffer->command)
    {
    case gcvHAL_GET_BASE_ADDRESS:
    case gcvHAL_VERSION:
    case gcvHAL_CHIP_INFO:
        inputBuffer->hardwareType = gcvHARDWARE_3D;
        break;

    default:
        status = gcoOS_GetTLS(&tls);
		if (gcmIS_ERROR(status)) return status;
        inputBuffer->hardwareType = tls->currentType;
        break;
    }

#ifdef GENERATE_K_HAL
    /* Convert the IOCTL_GCHAL_INTERFACE to IOCTL_GCHAL_KERNEL_INTERFACE*/
    if (!GPU_IOControl(  0,
                         IOCTL_GCHAL_KERNEL_INTERFACE,
                         InputBuffer,
                         InputBufferSize,
                         OutputBuffer,
                         OutputBufferSize,
                         &returned))
    {
        /* Generic I/O error. */
        gcmONERROR(gcvSTATUS_GENERIC_IO);
    }
#else
    /* Perform the device I/O control call. */
    if (!DeviceIoControl(gcPLS.os->device,
                         IoControlCode,
                         InputBuffer,
                         InputBufferSize,
                         OutputBuffer,
                         OutputBufferSize,
                         &returned,
                         gcvNULL))
    {
        /* Generic I/O error. */
        gcmONERROR(gcvSTATUS_GENERIC_IO);
    }
#endif

    /* Test for API error. */
    gcmONERROR(outputBuffer->status);

OnError:
    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gcoOS_AllocateNonPagedMemory
**
**  Allocate non-paged memory from the kernel.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctBOOL InUserSpace
**          gcvTRUE to mape the memory into the user space.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes to allocate.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the aligned number of bytes
**          allocated.
**
**      gctPHYS_ADDR * Physical
**          Pointer to a variable that will receive the physical addresses of
**          the allocated pages.
**
**      gctPOINTER * Logical
**          Pointer to a variable that will receive the logical address of the
**          allocation.
*/
gceSTATUS
gcoOS_AllocateNonPagedMemory(
    IN gcoOS Os,
    IN gctBOOL InUserSpace,
    IN OUT gctSIZE_T * Bytes,
    OUT gctPHYS_ADDR * Physical,
    OUT gctPOINTER * Logical
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("InUserSpace=%d *Bytes=%lu",
                  InUserSpace, gcmOPT_VALUE(Bytes));

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Bytes != gcvNULL);
    gcmVERIFY_ARGUMENT(Physical != gcvNULL);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.command = gcvHAL_ALLOCATE_NON_PAGED_MEMORY;
    iface.u.AllocateNonPagedMemory.bytes = *Bytes;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        ));

    /* Return allocated memory. */
    *Bytes = iface.u.AllocateNonPagedMemory.bytes;
    *Physical = iface.u.AllocateNonPagedMemory.physical;
    *Logical = iface.u.AllocateNonPagedMemory.logical;

    /* Success. */
        gcmFOOTER_ARG("*Bytes=%lu *Physical=0x%x *Logical=0x%x",
                      *Bytes, *Physical, *Logical);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
        gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_FreeNonPagedMemory
**
**  Free non-paged memory from the kernel.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctBOOL InUserSpace
**          gcvTRUE to mape the memory into the user space.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes to allocate.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the aligned number of bytes
**          allocated.
**
**      gctPHYS_ADDR * Physical
**          Pointer to a variable that will receive the physical addresses of
**          the allocated pages.
**
**      gctPOINTER * Logical
**          Pointer to a variable that will receive the logical address of the
**          allocation.
*/
gceSTATUS
gcoOS_FreeNonPagedMemory(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    IN gctPHYS_ADDR Physical,
    IN gctPOINTER Logical
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Bytes=%lu Physical=0x%x Logical=0x%x",
                  Bytes, Physical, Logical);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.command = gcvHAL_FREE_NON_PAGED_MEMORY;
    iface.u.FreeNonPagedMemory.bytes    = Bytes;
    iface.u.FreeNonPagedMemory.physical = Physical;
    iface.u.FreeNonPagedMemory.logical  = Logical;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, sizeof(iface),
        &iface, sizeof(iface)
        ));

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_AllocateContiguous
**
**  Allocate contiguous memory from the kernel.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctBOOL InUserSpace
**          gcvTRUE to map the memory into the user space.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes to allocate.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the aligned number of bytes
**          allocated.
**
**      gctPHYS_ADDR * Physical
**          Pointer to a variable that will receive the physical addresses of
**          the allocated memory.
**
**      gctPOINTER * Logical
**          Pointer to a variable that will receive the logical address of the
**          allocation.
*/
gceSTATUS
gcoOS_AllocateContiguous(
    IN gcoOS Os,
    IN gctBOOL InUserSpace,
    IN OUT gctSIZE_T * Bytes,
    OUT gctPHYS_ADDR * Physical,
    OUT gctPOINTER * Logical
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("InUserSpace=%d *Bytes=%lu",
                  InUserSpace, gcmOPT_VALUE(Bytes));

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Bytes != gcvNULL);
    gcmVERIFY_ARGUMENT(Physical != gcvNULL);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.command = gcvHAL_ALLOCATE_CONTIGUOUS_MEMORY;
    iface.u.AllocateContiguousMemory.bytes = *Bytes;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
            ));

    /* Return allocated number of bytes. */
    *Bytes = iface.u.AllocateContiguousMemory.bytes;

    /* Return physical address. */
    *Physical = iface.u.AllocateContiguousMemory.physical;

    /* Return logical address. */
    *Logical = iface.u.AllocateContiguousMemory.logical;

    /* Success. */
    gcmFOOTER_ARG("*Bytes=%lu *Physical=0x%x *Logical=0x%x",
                  *Bytes, *Physical, *Logical);
    return gcvSTATUS_OK;

OnError:
    gcmTRACE(
        gcvLEVEL_ERROR,
        "%s(%d): failed to allocate %lu bytes",
        __FUNCTION__, __LINE__, *Bytes
        );

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_FreeContiguous
**
**  Free contiguous memory from the kernel.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPHYS_ADDR Physical
**          The physical addresses of the allocated pages.
**
**      gctPOINTER Logical
**          The logical address of the allocation.
**
**      gctSIZE_T Bytes
**          Number of bytes allocated.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_FreeContiguous(
    IN gcoOS Os,
    IN gctPHYS_ADDR Physical,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Physical=0x%x Logical=0x%x Bytes=%lu",
                  Physical, Logical, Bytes);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.command = gcvHAL_FREE_CONTIGUOUS_MEMORY;
    iface.u.FreeContiguousMemory.bytes    = Bytes;
    iface.u.FreeContiguousMemory.physical = Physical;
    iface.u.FreeContiguousMemory.logical  = Logical;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        ));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_Open
**
**  Open or create a file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctCONST_STRING FileName
**          File name of file to open or create.
**
**      gceFILE_MODE Mode
**          Mode to open file with:
**
**              gcvFILE_CREATE      - Overwite any existing file.
**              gcvFILE_APPEND      - Append to an exisiting file or create a
**                                    new file if there is no exisiting file.
**              gcvFILE_READ        - Open an existing file for read only.
**              gcvFILE_CREATETEXT  - Overwite any existing text file.
**              gcvFILE_APPENDTEXT  - Append to an exisiting text file or create
**                                    a new text file if there is no exisiting
**                                    file.
**              gcvFILE_READTEXT    - Open an existing text file fir read only.
**
**  OUTPUT:
**
**      gctFILE * File
**          Pointer to a variable receivig the handle to the opened file.
*/
gceSTATUS
gcoOS_Open(
    IN gcoOS Os,
    IN gctCONST_STRING FileName,
    IN gceFILE_MODE Mode,
    OUT gctFILE * File
    )
{
    static gctCONST_STRING modes[] =
    {
        "wb",
        "ab",
        "rb",
        "w",
        "a",
        "r",
    };
    FILE * file;
    char fullpath[MAX_PATH_LENGTH];
    gctCONST_STRING fullname = gcvNULL;

    gcmHEADER_ARG("FileName=%s Mode=%d",
                  gcmOPT_STRING(FileName), Mode);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(File != gcvNULL);

    if (FileName != gcvNULL && FileName[0] != '\\' && FileName[0] != '/')
    {
        TCHAR dir[MAX_PATH_LENGTH];
        int i, len;

        GetModuleFileName(NULL, dir, MAX_PATH_LENGTH);

        memset(fullpath, 0, MAX_PATH_LENGTH);
        wcstombs(fullpath, dir, MAX_PATH_LENGTH);

        len = strlen(fullpath);
        for(i = len ; i > 0 ; i--){
            if(fullpath[i-1] == '\\'){
                fullpath[i] = '\0';
                break;
            }
        }

        if (i != 0)
        {
            strcat(fullpath, FileName);
        }
        else
        {
            /* Use default path. */
            sprintf(fullpath, "\\release\\%s", FileName);
        }

        fullpath[MAX_PATH_LENGTH - 1] = '\0';

        fullname = fullpath;
    }
    else
    {
        fullname = FileName;
    }

    /* Open the file. */
    file = fopen(fullname, modes[Mode]);
    if (file == gcvNULL)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }

    /* Return handle to file. */
    *File = (gctFILE) file;

    /* Success. */
    gcmFOOTER_ARG("*File=0x%x", *File);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Close
**
**  Close a file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_Close(
    IN gcoOS Os,
    IN gctFILE File
    )
{
    gcmHEADER_ARG("File=0x%x", File);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(File != gcvNULL);

    /* Close the file. */
    fclose((FILE *) File);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Read
**
**  Read data from an open file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**      gctSIZE_T ByteCount
**          Number of bytes to read from the file.
**
**      gctCONST_POINTER Data
**          Pointer to the data to read from the file.
**
**  OUTPUT:
**
**      gctSIZE_T * ByteRead
**          Pointer to a variable receiving the number of bytes read from the
**          file.
*/
gceSTATUS
gcoOS_Read(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctSIZE_T ByteCount,
    IN gctPOINTER Data,
    OUT gctSIZE_T * ByteRead
    )
{
    size_t byteRead;

    gcmHEADER_ARG("File=0x%x ByteCount=%lu Data=0x%x",
                  File, ByteCount, Data);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(File != gcvNULL);
    gcmVERIFY_ARGUMENT(ByteCount > 0);
    gcmVERIFY_ARGUMENT(Data != gcvNULL);

    /* Read the data from the file. */
    byteRead = fread(Data, 1, ByteCount, (FILE *) File);

    if (ByteRead != gcvNULL)
    {
        *ByteRead = (gctSIZE_T) byteRead;
    }

    /* Success. */
    gcmFOOTER_ARG("*ByteRead=%lu", gcmOPT_VALUE(ByteRead));
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Write
**
**  Write data to an open file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**      gctSIZE_T ByteCount
**          Number of bytes to write to the file.
**
**      gctCONST_POINTER Data
**          Pointer to the data to write to the file.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_Write(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctSIZE_T ByteCount,
    IN gctCONST_POINTER Data
    )
{
    size_t byteWritten;

    gcmHEADER_ARG("File=0x%x ByteCount=%lu Data=0x%x",
                  File, ByteCount, Data);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(File != gcvNULL);
    gcmVERIFY_ARGUMENT(ByteCount > 0);
    gcmVERIFY_ARGUMENT(Data != gcvNULL);

    /* Write the data to the file. */
    byteWritten = fwrite(Data, 1, ByteCount, (FILE *) File);

    if (byteWritten == ByteCount)
    {
        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    else
    {
        /* Error */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }
}

/* Flush data to a file. */
gceSTATUS
gcoOS_Flush(
    IN gcoOS Os,
    IN gctFILE File
    )
{
    gcmHEADER_ARG("File=0x%x", File);

    fflush((FILE *) File);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Create an endpoint for communication. */
gceSTATUS
gcoOS_Socket(
    IN gcoOS Os,
    IN gctINT Domain,
    IN gctINT Type,
    IN gctINT Protocol,
    OUT gctINT * SockFd
    )
{
    gctINT fd;

    gcmHEADER_ARG("Domain=%d Type=%d Protocol=%d",
                  Domain, Type, Protocol);

    /* Create a socket. */
    fd = socket(Domain, Type, Protocol);

    if (fd >= 0)
    {
        /* Return socket descriptor. */
        *SockFd = fd;

        /* Success. */
        gcmFOOTER_ARG("*SockFd=%d", *SockFd);
        return gcvSTATUS_OK;
    }
    else
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }
}

/* Close a socket. */
gceSTATUS
gcoOS_CloseSocket(
    IN gcoOS Os,
    IN gctINT SockFd
    )
{
    gcmHEADER_ARG("SockFd=%d", SockFd);

    closesocket((SOCKET) SockFd);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Initiate a connection on a socket. */
gceSTATUS
gcoOS_Connect(
    IN gcoOS Os,
    IN gctINT SockFd,
    IN gctCONST_POINTER HostName,
    IN gctUINT Port
    )
{
    gctINT rc;
    gctINT addrLen;
    struct sockaddr sockAddr;
    struct sockaddr_in *sockAddrIn;
    struct in_addr *inAddr;

    gcmHEADER_ARG("SockFd=0x%x HostName=0x%x Port=%d",
                  SockFd, HostName, Port);

    /* Get server address. */
    sockAddrIn = (struct sockaddr_in *) &sockAddr;
    inAddr = &sockAddrIn->sin_addr;
    inAddr->s_addr = inet_addr(HostName);

    /* If it is a numeric host name, convert it now */
    if (inAddr->s_addr == INADDR_NONE)
    {
        struct hostent *hostEnt;
        struct in_addr *arrayAddr;

        /* It is a real name, we solve it */
        if ((hostEnt = gethostbyname(HostName)) == NULL)
        {
            /* Error */
            gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
            return gcvSTATUS_GENERIC_IO;
        }
        arrayAddr = (struct in_addr *) *(hostEnt->h_addr_list);
        inAddr->s_addr = arrayAddr[0].s_addr;
    }

    sockAddrIn->sin_port = htons((gctUINT16) Port);

    /* Currently, for INET only. */
    addrLen = 16;

    /* Connect to server. */
    rc = connect((SOCKET) SockFd, &sockAddr, addrLen);

    if (rc)
    {
        /* Error */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Shut down part of connection on a socket. */
gceSTATUS
gcoOS_Shutdown(
    IN gcoOS Os,
    IN gctINT SockFd,
    IN gctINT How
    )
{
    gcmHEADER_ARG("SockFd=%d How=%d", SockFd, How);

    /* Shut down connection. */
    shutdown((SOCKET) SockFd, How);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Send a message on a socket. */
gceSTATUS
gcoOS_Send(
    IN gcoOS Os,
    IN gctINT SockFd,
    IN gctSIZE_T ByteCount,
    IN gctCONST_POINTER Data,
    IN gctINT Flags
    )
{
    gctINT byteSent;

    gcmHEADER_ARG("SockFd=0x%x ByteCount=%lu Data=0x%x Flags=%d",
                  SockFd, ByteCount, Data, Flags);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(ByteCount > 0);
    gcmVERIFY_ARGUMENT(Data != gcvNULL);

    /* Write the data to the file. */
    byteSent = send((SOCKET) SockFd, Data, ByteCount, Flags);

    if (byteSent == (gctINT) ByteCount)
    {
        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    else
    {
        /* Error */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }
}

/* Get environment variable value. */
gceSTATUS
gcoOS_GetEnv(
    IN gcoOS Os,
    IN gctCONST_STRING VarName,
    OUT gctSTRING * Value
    )
{
    gcmHEADER_ARG("VarName=%s", gcmOPT_STRING(VarName));

    *Value = gcvNULL;

    gcmFOOTER_ARG("status=%d", gcvSTATUS_NOT_SUPPORTED);
    return gcvSTATUS_NOT_SUPPORTED;
}

/* Get current working directory. */
gceSTATUS
gcoOS_GetCwd(
    IN gcoOS Os,
    IN gctINT SizeInBytes,
    OUT gctSTRING Buffer
    )
{
    TCHAR dir[MAX_PATH_LENGTH];
    int i, len, minSize;

    gcmHEADER_ARG("SizeInBytes=%d", SizeInBytes);

    GetModuleFileName(NULL, dir, MAX_PATH_LENGTH);

    minSize = min(SizeInBytes, MAX_PATH_LENGTH);
    memset(Buffer, 0, SizeInBytes);
    wcstombs(Buffer, dir, minSize);

    len = strlen(Buffer);
    for (i = len ; i > 0 ; i--){
        if (Buffer[i-1] == '\\'){
            Buffer[i] = '\0';
            break;
        }
    }

    if (i == 0)
    {
        /* Use default path. */
        sprintf(Buffer, "\\");
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Get file status info. */
gceSTATUS
gcoOS_Stat(
    IN gcoOS Os,
    IN gctCONST_STRING FileName,
    OUT gctPOINTER Buffer
    )
{
    struct stat
    {
        gctUINT dev;
        gctUINT16 ino;
        gctUINT16 mode;
        gctINT16 link;
        gctINT16 uid;
        gctINT16 gid;
        gctUINT rdev;
        gctINT size;
        gctINT atime;
        gctINT mtime;
        gctINT ctime;
    };

    gctUINT i;
    WIN32_FILE_ATTRIBUTE_DATA fi;
    TCHAR wPath[MAX_PATH];
    struct stat *status = (struct stat*)Buffer;

    gcmHEADER_ARG("FileName=%s", gcmOPT_STRING(FileName));

    MultiByteToWideChar(CP_ACP, 0, FileName, -1, wPath, MAX_PATH);
    if (!GetFileAttributesEx(wPath, GetFileExInfoStandard, &fi))
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    status->size = fi.nFileSizeHigh;
    status->size <<= sizeof(fi.nFileSizeHigh)*8;
    status->size += fi.nFileSizeLow;

    status->mode = ((fi.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? 0555 : 0777);
    status->mode |= ((fi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 0x4000 : 0xb000);

    status->uid = 9999;
    status->gid = 9999;

    status->ino = 0;
    for (i = 0; i < strlen(FileName); ++i)
      status->ino += FileName[i];

    status->mtime = *(gctINT*)&fi.ftLastWriteTime;

    /* Done */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_GetPos
**
**  Get the current position of a file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**  OUTPUT:
**
**      gctUINT32 * Position
**          Pointer to a variable receiving the current position of the file.
*/
gceSTATUS
gcoOS_GetPos(
    IN gcoOS Os,
    IN gctFILE File,
    OUT gctUINT32 * Position
    )
{
    gcmHEADER_ARG("File=0x%x", File);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(File != gcvNULL);
    gcmVERIFY_ARGUMENT(Position != gcvNULL);

    /* Get the current file position. */
    *Position = ftell((FILE *) File);

    /* Success. */
    gcmFOOTER_ARG("*Position=%u", *Position);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_SetPos
**
**  Set position for a file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**      gctUINT32 Position
**          Absolute position of the file to set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_SetPos(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctUINT32 Position
    )
{
    gcmHEADER_ARG("File=0x%x Position=%u", File, Position);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(File != gcvNULL);

    /* Set file position. */
    fseek((FILE *) File, Position, SEEK_SET);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Seek
**
**  Set position for a file.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctFILE File
**          Pointer to an open file object.
**
**      gctUINT32 Offset
**          Offset added to the position specified by Whence.
**
**      gceFILE_WHENCE Whence
**          Mode that specify how to add the offset to the position:
**
**              gcvFILE_SEEK_SET    - Relative to the start of the file.
**              gcvFILE_SEEK_CUR    - Relative to the current position.
**              gcvFILE_SEEK_END    - Relative to the end of the file.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_Seek(
    IN gcoOS Os,
    IN gctFILE File,
    IN gctUINT32 Offset,
    IN gceFILE_WHENCE Whence
    )
{
    gctINT result = 0;

    gcmHEADER_ARG("File=0x%x Offset=%u Whence=%d",
                  File, Offset, Whence);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(File != gcvNULL);

    /* Set file position. */
    switch (Whence)
    {
    case gcvFILE_SEEK_SET:
        result = fseek((FILE *) File, Offset, SEEK_SET);
        break;

    case gcvFILE_SEEK_CUR:
        result = fseek((FILE *) File, Offset, SEEK_CUR);
        break;

    case gcvFILE_SEEK_END:
        result = fseek((FILE *) File, Offset, SEEK_END);
        break;
    }

    if (result == 0)
    {
        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    else
    {
        /* Error */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GENERIC_IO);
        return gcvSTATUS_GENERIC_IO;
    }
}

/*******************************************************************************
**
**  gcoOS_CreateThread
**
**  Create a new thread.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**  OUTPUT:
**
**      gctPOINTER * Thread
**          Pointer to a variable that will hold a pointer to the thread.
*/
gceSTATUS
gcoOS_CreateThread(
    IN gcoOS Os,
    IN gcTHREAD_ROUTINE Worker,
    IN gctPOINTER Argument,
    OUT gctPOINTER * Thread
    )
{
    HANDLE handle;

    gcmHEADER_ARG("Worker=0x%x Argument=0x%x", Worker, Argument);

    /* Validate the arguments. */
    gcmVERIFY_ARGUMENT(Thread != gcvNULL);

    handle = CreateThread(gcvNULL, 0, Worker, Argument, 0, gcvNULL);

    if (handle == gcvNULL)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_RESOURCES);
        return gcvSTATUS_OUT_OF_RESOURCES;
    }

    *Thread = handle;

    /* Success. */
    gcmFOOTER_ARG("*Thread=0x%x", *Thread);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_CloseThread
**
**  Close a thread.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER Thread
**          Pointer to the thread to be deleted.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_CloseThread(
    IN gcoOS Os,
    IN gctPOINTER Thread
    )
{
    gcmHEADER_ARG("Thread=0x%x", Thread);

    /* Validate the arguments. */
    gcmVERIFY_ARGUMENT(Thread != gcvNULL);

    WaitForSingleObject((HANDLE) Thread, INFINITE);
    gcmVERIFY(CloseHandle((HANDLE) Thread));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_CreateMutex
**
**  Create a new mutex.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**  OUTPUT:
**
**      gctPOINTER * Mutex
**          Pointer to a variable that will hold a pointer to the mutex.
*/
gceSTATUS
gcoOS_CreateMutex(
    IN gcoOS Os,
    OUT gctPOINTER * Mutex
    )
{
    HANDLE handle;

    gcmHEADER();

    /* Validate the arguments. */
    gcmVERIFY_ARGUMENT(Mutex != gcvNULL);

    handle = CreateMutex(gcvNULL, gcvFALSE, gcvNULL);

    if (handle == gcvNULL)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_RESOURCES);
        return gcvSTATUS_OUT_OF_RESOURCES;
    }

    *Mutex = handle;

    /* Success. */
    gcmFOOTER_ARG("*Mutex=0x%x", *Mutex);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_DeleteMutex
**
**  Delete a mutex.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER Mutex
**          Pointer to the mutex to be deleted.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_DeleteMutex(
    IN gcoOS Os,
    IN gctPOINTER Mutex
    )
{
    gcmHEADER_ARG("Mutex=0x%x", Mutex);

    /* Validate the arguments. */
    gcmVERIFY_ARGUMENT(Mutex != gcvNULL);

    gcmVERIFY(CloseHandle((HANDLE) Mutex));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_AcquireMutex
**
**  Acquire a mutex.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER Mutex
**          Pointer to the mutex to be acquired.
**
**      gctUINT32 Timeout
**          Timeout value specified in milliseconds.
**          Specify the value of gcvINFINITE to keep the thread suspended
**          until the mutex has been acquired.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_AcquireMutex(
    IN gcoOS Os,
    IN gctPOINTER Mutex,
    IN gctUINT32 Timeout
    )
{
    DWORD milliSeconds;

    gcmHEADER_ARG("Mutex=0x%x Timeout=%u", Mutex, Timeout);

    /* Validate the arguments. */
    gcmVERIFY_ARGUMENT(Mutex != gcvNULL);

    milliSeconds = (Timeout == gcvINFINITE)
        ? INFINITE
        : Timeout;

    if (WaitForSingleObject((HANDLE) Mutex,
                            milliSeconds) == WAIT_TIMEOUT)
    {
        /* Timeout. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_TIMEOUT);
        return gcvSTATUS_TIMEOUT;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_ReleaseMutex
**
**  Release an acquired mutex.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctPOINTER Mutex
**          Pointer to the mutex to be released.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_ReleaseMutex(
    IN gcoOS Os,
    IN gctPOINTER Mutex
    )
{
    gcmHEADER_ARG("Mutex=0x%x", Mutex);

    /* Validate the arguments. */
    gcmVERIFY_ARGUMENT(Mutex != gcvNULL);

    /* Release the fast mutex. */
    gcmVERIFY(ReleaseMutex((HANDLE) Mutex));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Delay
**
**  Delay execution of the current thread for a number of milliseconds.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctUINT32 Delay
**          Delay to sleep, specified in milliseconds.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_Delay(
    IN gcoOS Os,
    IN gctUINT32 Delay
    )
{
    gcmHEADER_ARG("Delay=%u", Delay);

    /* Sleep for a while. */
    Sleep(Delay);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_MemCopy
**
**  Copy memory from one location to another location.  The memory cannot be
**  overlapped.
**
**  INPUT:
**
**      gctPOINTER Destination
**          Pointer to destination memory.
**
**      gctCONST_POINTER Source
**          Pointer to source memory.
**
**      gctSIZE_T Bytes
**          Number of bytes to copy.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_MemCopy(
    IN gctPOINTER Destination,
    IN gctCONST_POINTER Source,
    IN gctSIZE_T Bytes
    )
{
    gcmHEADER_ARG("Destination=0x%x Source=0x%x Bytes=%lu",
                  Destination, Source, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Destination != gcvNULL);
    gcmVERIFY_ARGUMENT(Source != gcvNULL);
    gcmVERIFY_ARGUMENT(Bytes > 0);

    /* Copy the memory. */
    memcpy(Destination, Source, Bytes);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_ZeroMemory
**
**  Fill the specified memory with 0's.
**
**  INPUT:
**
**      gctPOINTER Memory
**          Pointer to memory to fill.
**
**      gctSIZE_T Bytes
**          Number of bytes to fill.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_ZeroMemory(
    IN gctPOINTER Memory,
    IN gctSIZE_T Bytes
    )
{
    gcmHEADER_ARG("Memory=0x%x Bytes=%lu", Memory, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);
    gcmVERIFY_ARGUMENT(Bytes > 0);

    /* Zero the memory. */
    memset(Memory, 0, Bytes);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_MemFill
**
**  Fill the specified memory with a specified filler.
**
**  INPUT:
**
**      gctPOINTER Memory
**          Pointer to memory to fill.
**
**      gctUINT8 Filler
**          Value to fill the memory width.
**
**      gctSIZE_T Bytes
**          Number of bytes to fill.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_MemFill(
    IN gctPOINTER Memory,
    IN gctUINT8 Filler,
    IN gctSIZE_T Bytes
    )
{
    gcmHEADER_ARG("Memory=0x%x Filler=%u Bytes=%lu", Memory, Filler, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);
    gcmVERIFY_ARGUMENT(Bytes > 0);

    /* Fill the memory. */
    memset(Memory, Filler, Bytes);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_StrLen
**
**  Get the length of the specified string.
**
**  INPUT:
**
**      gctCONST_STRING String
**          Pointer to string to get size off.
**
**  OUTPUT:
**
**      gctSIZE_T * Length
**          Pointer to a variable that will receive the length of the string.
*/
gceSTATUS
gcoOS_StrLen(
    IN gctCONST_STRING String,
    OUT gctSIZE_T * Length
    )
{
    gcmHEADER_ARG("String=0x%x", String);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(String != gcvNULL);
    gcmVERIFY_ARGUMENT(Length != gcvNULL);

    /* Get the length of the string. */
    *Length = (gctSIZE_T) strlen(String);

    /* Success. */
    gcmFOOTER_ARG("*Length=%lu", *Length);
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_StrStr(
    IN gctCONST_STRING String,
    IN gctCONST_STRING SubString,
    OUT gctSTRING * Output
    )
{
    gcmHEADER_ARG("String=0x%x SubString=0x%x", String, SubString);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(String != gcvNULL);
    gcmVERIFY_ARGUMENT(SubString != gcvNULL);

    /* Call C. */
    *Output = strstr(String, SubString);

    /* Success. */
    gcmFOOTER_ARG("*Output=0x%x", *Output);
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_StrFindReverse(
    IN gctCONST_STRING String,
    IN gctINT8 Character,
    OUT gctSTRING * Output
    )
{
    gcmHEADER_ARG("String=0x%x Character=%d", String, Character);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(String != gcvNULL);
    gcmVERIFY_ARGUMENT(Output != gcvNULL);

    /* Call C. */
    *Output = strrchr(String, Character);

    /* Success. */
    gcmFOOTER_ARG("*Output=0x%x", *Output);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_StrCopy
**
**  Copy a string.
**
**  INPUT:
**
**      gctSTRING Destination
**          Pointer to the destination string.
**
**      gctCONST_STRING Source
**          Pointer to the source string.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_StrCopySafe(
    IN gctSTRING Destination,
    IN gctSIZE_T DestinationSize,
    IN gctCONST_STRING Source
    )
{
    gcmHEADER_ARG("Destination=0x%x DestinationSize=%lu Source=0x%x",
                  Destination, DestinationSize, Source);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Destination != gcvNULL);
    gcmVERIFY_ARGUMENT(Source != gcvNULL);

    /* Copy a string. */
#if UNDER_CE >= 600
    strcpy_s(Destination, DestinationSize, Source);
#else
    strcpy(Destination, Source);
#endif

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_StrCat
**
**  Append a string.
**
**  INPUT:
**
**      gctSTRING Destination
**          Pointer to the destination string.
**
**      gctCONST_STRING Source
**          Pointer to the source string.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_StrCatSafe(
    IN gctSTRING Destination,
    IN gctSIZE_T DestinationSize,
    IN gctCONST_STRING Source
    )
{
    gcmHEADER_ARG("Destination=0x%x DestinationSize=%lu Source=0x%x",
                  Destination, DestinationSize, Source);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Destination != gcvNULL);
    gcmVERIFY_ARGUMENT(Source != gcvNULL);

    /* Append a string. */
#if UNDER_CE >= 600
    strcat_s(Destination, DestinationSize, Source);
#else
    strcat(Destination, Source);
#endif

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_StrCmp
**
**  Compare two strings and return whether they match or not.
**
**  INPUT:
**
**      gctCONST_STRING String1
**          Pointer to the first string to compare.
**
**      gctCONST_STRING String2
**          Pointer to the second string to compare.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK if the strings match
**      gcvSTATUS_LARGER if String1 > String2
**      gcvSTATUS_SMALLER if String1 < String2
*/
gceSTATUS
gcoOS_StrCmp(
    IN gctCONST_STRING String1,
    IN gctCONST_STRING String2
    )
{
    int result;
    gceSTATUS status;

    gcmHEADER_ARG("String1=0x%x String2=0x%x", String1, String2);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(String1 != gcvNULL);
    gcmVERIFY_ARGUMENT(String2 != gcvNULL);

    /* Compare the strings and return proper status. */
    result = strcmp(String1, String2);

    status = (result == 0) ? gcvSTATUS_OK
           : (result >  0) ? gcvSTATUS_LARGER
                           : gcvSTATUS_SMALLER;

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_StrNCmp
**
**  Compare characters of two strings and return whether they match or not.
**
**  INPUT:
**
**      gctCONST_STRING String1
**          Pointer to the first string to compare.
**
**      gctCONST_STRING String2
**          Pointer to the second string to compare.
**
**      gctSIZE_T Count
**          Number of characters to compare.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK if the strings match
**      gcvSTATUS_LARGER if String1 > String2
**      gcvSTATUS_SMALLER if String1 < String2
*/
gceSTATUS
gcoOS_StrNCmp(
    IN gctCONST_STRING String1,
    IN gctCONST_STRING String2,
    IN gctSIZE_T Count
    )
{
    int result;
    gceSTATUS status;

    gcmHEADER_ARG("String1=0x%x String2=0x%x Count=%lu",
                  String1, String2, Count);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(String1 != gcvNULL);
    gcmVERIFY_ARGUMENT(String2 != gcvNULL);

    /* Compare the strings and return proper status. */
    result = strncmp(String1, String2, Count);

    status = (result == 0)
            ? gcvSTATUS_OK
            : ((result > 0) ? gcvSTATUS_LARGER : gcvSTATUS_SMALLER);
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_StrToFloat
**
**  Convert string to float.
**
**  INPUT:
**
**      gctCONST_STRING String
**          Pointer to the string to be converted.
**
**
**  OUTPUT:
**
**      gctFLOAT * Float
**          Pointer to a variable that will receive the float.
**
*/
gceSTATUS
gcoOS_StrToFloat(
    IN gctCONST_STRING String,
    OUT gctFLOAT * Float
    )
{
    gcmHEADER_ARG("String=%s", gcmOPT_STRING(String));

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(String != gcvNULL);

    *Float = (gctFLOAT) atof(String);

    gcmFOOTER_ARG("*Float=%f", *Float);
    return gcvSTATUS_OK;
}

/* Converts a hex string to 32-bit integer. */
gceSTATUS gcoOS_HexStrToInt(IN gctCONST_STRING String,
               OUT gctINT * Int)
{
    gcmHEADER_ARG("String=%s", gcmOPT_STRING(String));
    gcmVERIFY_ARGUMENT(String != gcvNULL);
    gcmVERIFY_ARGUMENT(Int != gcvNULL);

    sscanf(String, "%x", Int);

    gcmFOOTER_ARG("*Int=%d", *Int);
    return gcvSTATUS_OK;
}

/* Converts a hex string to float. */
gceSTATUS gcoOS_HexStrToFloat(IN gctCONST_STRING String,
               OUT gctFLOAT * Float)
{
    gctSTRING pch = gcvNULL;
    gctSTRING context = gcvNULL;
    gctCONST_STRING delim = "x.p";
    gctFLOAT b, exp;
    gctINT s;

    gcmHEADER_ARG("String=%s", gcmOPT_STRING(String));
    gcmVERIFY_ARGUMENT(String != gcvNULL);
    gcmVERIFY_ARGUMENT(Float != gcvNULL);

    pch = strtok_s((gctSTRING)String, delim, &context);
    pch = strtok_s(NULL, delim, &context);
    gcmVERIFY_OK(gcoOS_StrToFloat(pch, &b));
    pch = strtok_s(NULL, delim, &context);
    gcmVERIFY_OK(gcoOS_HexStrToInt(pch, &s));
    pch = strtok_s(NULL, delim, &context);
    gcmVERIFY_OK(gcoOS_StrToFloat(pch, &exp));

    *Float = (float)(b + s / (float)(1 << 24)) * (float)pow(2.0, exp);

    gcmFOOTER_ARG("*Float=%d", *Float);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_StrToInt
**
**  Convert string to integer.
**
**  INPUT:
**
**      gctCONST_STRING String
**          Pointer to the string to be converted.
**
**
**  OUTPUT:
**
**      gctINT * Int
**          Pointer to a variable that will receive the integer.
**
*/
gceSTATUS
gcoOS_StrToInt(
    IN gctCONST_STRING String,
    OUT gctINT * Int
    )
{
    gcmHEADER_ARG("String=%s", gcmOPT_STRING(String));

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(String != gcvNULL);

    *Int = (gctINT) atoi(String);

    gcmFOOTER_ARG("*Int=%d", *Int);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_MemCmp
**
**  Compare two memory regions and return whether they match or not.
**
**  INPUT:
**
**      gctCONST_POINTER Memory1
**          Pointer to the first memory region to compare.
**
**      gctCONST_POINTER Memory2
**          Pointer to the second memory region to compare.
**
**      gctSIZE_T Bytes
**          Number of bytes to compare.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK if the memory regions match or gcvSTATUS_MISMATCH if the
**      memory regions don't match.
*/
gceSTATUS
gcoOS_MemCmp(
    IN gctCONST_POINTER Memory1,
    IN gctCONST_POINTER Memory2,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Memory1=0x%x Memory2=0x%x Bytes=%lu",
                  Memory1, Memory2, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Memory1 != gcvNULL);
    gcmVERIFY_ARGUMENT(Memory2 != gcvNULL);
    gcmVERIFY_ARGUMENT(Bytes > 0);

    /* Compare the memory rregions and return proper status. */
    status = (memcmp(Memory1, Memory2, Bytes) == 0)
               ? gcvSTATUS_OK
               : gcvSTATUS_MISMATCH;
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_PrintStr
**
**  Append a "printf" formatted string to a string buffer and adjust the offset
**  into the string buffer.  There is no checking for a buffer overflow, so make
**  sure the string buffer is large enough.
**
**  INPUT:
**
**      gctSTRING String
**          Pointer to the string buffer.
**
**      gctUINT_PTR Offset
**          Pointer to a variable that holds the current offset into the string
**          buffer.
**
**      gctCONST_STRING Format
**          Pointer to a "printf" style format to append to the string buffer
**          pointet to by <String> at the offset specified by <*Offset>.
**
**      ...
**          Variable number of arguments that will be used by <Format>.
**
**  OUTPUT:
**
**      gctUINT_PTR Offset
**          Pointer to a variable that receives the new offset into the string
**          buffer pointed to by <String> after the formatted string pointed to
**          by <Formnat> has been appended to it.
*/
gceSTATUS
gcoOS_PrintStrSafe(
    IN gctSTRING String,
    IN gctSIZE_T StringSize,
    IN OUT gctUINT_PTR Offset,
    IN gctCONST_STRING Format,
    ...
    )
{
    gceSTATUS status;
    gctARGUMENTS args;

    gcmHEADER_ARG("String=0x%x StringSize=%lu *Offset=%u Format=0x%x",
                  String, StringSize, gcmOPT_VALUE(Offset), Format);

    /* Pass through to the V case. */
    gcmARGUMENTS_START(args, Format);
    status = gcoOS_PrintStrVSafe(String, StringSize, Offset, Format, args);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_PrintStrV
**
**  Append a "vprintf" formatted string to a string buffer and adjust the offset
**  into the string buffer.  There is no checking for a buffer overflow, so make
**  sure the string buffer is large enough.
**
**  INPUT:
**
**      gctSTRING String
**          Pointer to the string buffer.
**
**      gctUINT_PTR Offset
**          Pointer to a variable that holds the current offset into the string
**          buffer.
**
**      gctCONST_STRING Format
**          Pointer to a "printf" style format to append to the string buffer
**          pointet to by <String> at the offset specified by <*Offset>.
**
**      gctPOINTER ArgPtr
**          Pointer to list of arguments.
**
**  OUTPUT:
**
**      gctUINT_PTR Offset
**          Pointer to a variable that receives the new offset into the string
**          buffer pointed to by <String> after the formatted string pointed to
**          by <Formnat> has been appended to it.
*/
gceSTATUS
gcoOS_PrintStrVSafe(
    OUT gctSTRING String,
    IN gctSIZE_T StringSize,
    IN OUT gctUINT_PTR Offset,
    IN gctCONST_STRING Format,
    IN gctARGUMENTS Arguments
    )
{
    gcmHEADER_ARG("String=0x%x StringSize=%lu *Offset=%u Format=0x%x "
                  "Arguments=0x%x",
                  String, StringSize, gcmOPT_VALUE(Offset), Format, Arguments);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(String != gcvNULL);
    gcmVERIFY_ARGUMENT(Offset != gcvNULL);
    gcmVERIFY_ARGUMENT(Format != gcvNULL);

    if (*Offset < StringSize)
    {
        /* Format the string. */
        gctINT n = _vsnprintf(String + *Offset,
                             StringSize - *Offset,
                             Format,
                             Arguments);

        if (n > 0)
        {
            *Offset += n;
        }
    }

    /* Success. */
    gcmFOOTER_ARG("*Offset=%u", *Offset);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_MapUserMemory
**
**  Lock down a user buffer and return an DMA'able address to be used by the
**  hardware to access it.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctPOINTER Memory
**          Pointer to memory to lock down.
**
**      gctSIZE_T Size
**          Size in bytes of the memory to lock down.
**
**  OUTPUT:
**
**      gctPOINTER * Info
**          Pointer to variable receiving the information record required by
**          gcoOS_UnmapUserMemory.
**
**      gctUINT32_PTR Address
**          Pointer to a variable that will receive the address DMA'able by the
**          hardware.
*/
gceSTATUS
gcoOS_MapUserMemory(
    IN gcoOS Os,
    IN gctPOINTER Memory,
    IN gctSIZE_T Size,
    OUT gctPOINTER * Info,
    OUT gctUINT32_PTR Address
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Memory=0x%x Size=%lu", Memory, Size);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);
    gcmVERIFY_ARGUMENT(Size > 0);
    gcmVERIFY_ARGUMENT(Info != gcvNULL);
    gcmVERIFY_ARGUMENT(Address != gcvNULL);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.command = gcvHAL_MAP_USER_MEMORY;
    iface.u.MapUserMemory.memory = Memory;
    iface.u.MapUserMemory.size   = Size;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    /* Return the info on success. */
    *Info    = iface.u.MapUserMemory.info;
    *Address = iface.u.MapUserMemory.address;

    /* Success. */
    gcmFOOTER_ARG("*Info=0x%x *Address=0x%08x", *Info, *Address);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_UnmapUserMemory
**
**  Unlock a user buffer and that was previously locked down by
**  gcoOS_MapUserMemory.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctPOINTER Memory
**          Pointer to memory to unlock.
**
**      gctSIZE_T Size
**          Size in bytes of the memory to unlock.
**
**      gctPOINTER Info
**          Information record returned by gcoOS_MapUserMemory.
**
**      gctUINT32_PTR Address
**          The address returned by gcoOS_MapUserMemory.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_UnmapUserMemory(
    IN gcoOS Os,
    IN gctPOINTER Memory,
    IN gctSIZE_T Size,
    IN gctPOINTER Info,
    IN gctUINT32 Address
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Memory=0x%x Size=%lu Info=0x%x Address=0x%08x",
                  Memory, Size, Info, Address);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);
    gcmVERIFY_ARGUMENT(Size > 0);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.command = gcvHAL_UNMAP_USER_MEMORY;
    iface.u.UnmapUserMemory.memory  = Memory;
    iface.u.UnmapUserMemory.size    = Size;
    iface.u.UnmapUserMemory.info    = Info;
    iface.u.UnmapUserMemory.address = Address;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                 IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_StrDup
**
**  Duplicate the given string by copying it into newly allocated memory.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctCONST_STRING String
**          Pointer to string to duplicate.
**
**  OUTPUT:
**
**      gctSTRING * Target
**          Pointer to variable holding the duplicated string address.
*/
gceSTATUS
gcoOS_StrDup(
    IN gcoOS Os,
    IN gctCONST_STRING String,
    OUT gctSTRING * Target
    )
{
    gctSIZE_T bytes;
    gctSTRING string;
    gceSTATUS status;

    gcmHEADER_ARG("String=0x%x", String);

    gcmVERIFY_ARGUMENT(String != gcvNULL);
    gcmVERIFY_ARGUMENT(Target != gcvNULL);

    gcmONERROR(gcoOS_StrLen(String, &bytes));

    gcmONERROR(gcoOS_Allocate(gcvNULL, bytes + 1, (gctPOINTER *) &string));

    gcmONERROR(gcoOS_StrCopySafe(string, bytes + 1, String));

    *Target = string;

    /* Success. */
    gcmFOOTER_ARG("*Target=0x%x", gcmOPT_VALUE(Target));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_LoadLibrary
**
**  Load a library.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctCONST_STRING Library
**          Name of library to load.
**
**  OUTPUT:
**
**      gctHANDLE * Handle
**          Pointer to variable receiving the library handle.
*/
gceSTATUS
gcoOS_LoadLibrary(
    IN gcoOS Os,
    IN gctCONST_STRING Library,
    OUT gctHANDLE * Handle
    )
{
    gceSTATUS status;
    TCHAR name[MAX_PATH_LENGTH];
    gctINT i;

    gcmHEADER_ARG("Library=%s", gcmOPT_STRING(Library));

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Handle != gcvNULL);

    /* Convert the name to wide characters. */
    for (i = 0; (Library[i] != '\0') && (i < MAX_PATH_LENGTH - 1); ++i)
    {
        name[i] = Library[i];
    }
    name[i] = '\0';

    /* Load the library. */
    *Handle = LoadLibrary(name);

    /* Return error if library could not be loaded. */
    status = (*Handle == gcvNULL)
        ? gcvSTATUS_NOT_FOUND
        : gcvSTATUS_OK;

    if (gcmIS_SUCCESS(status))
    {
        gcmFOOTER_ARG("*Handle=0x%x", *Handle);
    }
    else
    {
        gcmFOOTER();
    }
    return status;
}

/*******************************************************************************
**
**  gcoOS_FreeLibrary
**
**  Unload a loaded library.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctHANDLE Handle
**          Handle of a loaded libarry.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_FreeLibrary(
    IN gcoOS Os,
    IN gctHANDLE Handle
    )
{
    gcmHEADER_ARG("Handle=0x%x", Handle);

    /* Free the library. */
    FreeLibrary(Handle);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_GetProcAddress
**
**  Get the address of a function inside a loaded library.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gctHANDLE Handle
**          Handle of a loaded libarry.
**
**      gctCONST_STRING Name
**          Name of function to get the address of.
**
**  OUTPUT:
**
**      gctPOINTER * Function
**          Pointer to variable receiving the function pointer.
*/
gceSTATUS
gcoOS_GetProcAddress(
    IN gcoOS Os,
    IN gctHANDLE Handle,
    IN gctCONST_STRING Name,
    OUT gctPOINTER * Function
    )
{
    gceSTATUS status;
    TCHAR name[MAX_PATH_LENGTH];
    gctINT i;

    gcmHEADER_ARG("Handle=0x%x Name=%s",
                  Handle, gcmOPT_STRING(Name));

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Name != gcvNULL);
    gcmVERIFY_ARGUMENT(Function != gcvNULL);

    /* Convert name to wide characters. */
    for (i = 0; (Name[i] != '\0') && (i < MAX_PATH_LENGTH - 1); ++i)
    {
        name[i] = Name[i];
    }
    name[i] = '\0';

    /* Get the address of the function. */
    *Function = GetProcAddress(Handle, name);

    /* Return error if function could not be found. */
    status = (*Function == gcvNULL)
        ? gcvSTATUS_NOT_FOUND
        : gcvSTATUS_OK;
    gcmFOOTER();
    return status;
}

#if VIVANTE_PROFILER
gceSTATUS
gcoOS_ProfileStart(
    IN gcoOS Os
    )
{
    gcPLS.os->allocCount   = 0;
    gcPLS.os->allocSize    = 0;
    gcPLS.os->maxAllocSize = 0;
    gcPLS.os->freeCount    = 0;
    gcPLS.os->freeSize     = 0;

    QueryPerformanceCounter(&gcPLS.os->startTime);

    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_ProfileEnd(
    IN gcoOS Os,
    IN gctCONST_STRING Title
    )
{
    LARGE_INTEGER frequency;
    double time;

    QueryPerformanceCounter(&gcPLS.os->endTime);
    QueryPerformanceFrequency(&frequency);

    time = (double) (gcPLS.os->endTime.QuadPart - gcPLS.os->startTime.QuadPart)
         / (double) frequency.QuadPart;

    gcmPRINT("-----[ EXECUTION STATISTICS for %s ]-----",
             Title);
    gcmPRINT("Time : %lf ms",
             time * 1000.0);

    gcmPRINT("-----[ MEMORY STATISTICS for %s ]-----",
             Title);
    gcmPRINT("Allocation count   : %u for %u bytes",
             gcPLS.os->allocCount, gcPLS.os->allocSize);
    gcmPRINT("Maximum allocation : %u bytes",
             gcPLS.os->maxAllocSize);
    gcmPRINT("Free count         : %u for %u bytes",
             gcPLS.os->freeCount,
             gcPLS.os->freeSize);

    return gcvSTATUS_OK;
}
#endif

gceSTATUS
gcoOS_Compact(
    IN gcoOS Os
    )
{
    return gcvSTATUS_OK;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------- Atoms ----------------------------------*/

struct gcsATOM
{
    /* Counter. */
    gctINT32 counter;
};

/* Create an atom. */
gceSTATUS
gcoOS_AtomConstruct(
    IN gcoOS Os,
    OUT gcsATOM_PTR * Atom
    )
{
    gceSTATUS status;
    gcsATOM_PTR atom = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Atom != gcvNULL);

    do
    {
        /* Allocate memory for the atom. */
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
                                    gcmSIZEOF(struct gcsATOM),
                                    (gctPOINTER *) &atom));

        /* Initialize the atom to 0. */
        atom->counter = 0;

        /* Return pointer to atom. */
        *Atom = atom;

        /* Success. */
        gcmFOOTER_ARG("*Atom=%p", *Atom);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Free the atom. */
    if (atom != gcvNULL)
    {
        gcmOS_SAFE_FREE(gcvNULL, atom);
    }

    /* Return error status. */
    gcmFOOTER();
    return status;
}

/* Destroy an atom. */
gceSTATUS
gcoOS_AtomDestroy(
    IN gcoOS Os,
    IN gcsATOM_PTR Atom
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Atom=0x%x", Atom);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Atom != gcvNULL);

    /* Free the atom. */
    status = gcmOS_SAFE_FREE(gcvNULL, Atom);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/* Increment an atom. */
gceSTATUS
gcoOS_AtomIncrement(
    IN gcoOS Os,
    IN gcsATOM_PTR Atom,
    OUT gctINT32_PTR OldValue OPTIONAL
    )
{
    gctINT32 value;

    gcmHEADER_ARG("Atom=0x%x", Atom);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Atom != gcvNULL);

    /* Increment the atom's counter. */
    value = InterlockedIncrement((volatile LONG *) &Atom->counter) - 1;

    if (OldValue != gcvNULL)
    {
        /* Return the original value to the caller. */
        *OldValue = value;
    }

    /* Success. */
    gcmFOOTER_ARG("*OldValue=%d", gcmOPT_VALUE(OldValue));
    return gcvSTATUS_OK;
}

/* Decrement an atom. */
gceSTATUS
gcoOS_AtomDecrement(
    IN gcoOS Os,
    IN gcsATOM_PTR Atom,
    OUT gctINT32_PTR OldValue OPTIONAL
    )
{
    gctINT32 value;

    gcmHEADER_ARG("Atom=0x%x", Atom);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Atom != gcvNULL);

    /* Decrement the atom's counter. */
    value = InterlockedDecrement((volatile LONG *) &Atom->counter) + 1;

    if (OldValue != gcvNULL)
    {
        /* Return the original value to the caller. */
        *OldValue = value;
    }

    /* Success. */
    gcmFOOTER_ARG("*OldValue=%d", gcmOPT_VALUE(OldValue));
    return gcvSTATUS_OK;
}


gctHANDLE
gcoOS_GetCurrentProcessID(
    void
    )
{
#if UNDER_CE >= 600
    return (gctHANDLE)GetCurrentProcessId();
#else
    /* Get current process handle. */
    gctHANDLE result = OpenProcess(PROCESS_ALL_ACCESS,
                                gcvFALSE,
                                GetCurrentProcessId());

    /* Close the handle. */
    CloseHandle(result);

    /* Return result. */
    return result;
#endif
}

gctHANDLE
gcoOS_GetCurrentThreadID(
    void
    )
{
    return (gctHANDLE)GetCurrentThreadId();
}

/*----------------------------------------------------------------------------*/
/*----------------------------------- Time -----------------------------------*/


/*******************************************************************************
**
**  gcoOS_GetTicks
**
**  Get the number of milliseconds since the system started.
**
**  INPUT:
**
**  OUTPUT:
**
*/
gctUINT32
gcoOS_GetTicks(
    void
    )
{
    /* Return the OS tick count. */
    return GetTickCount();
}

/*******************************************************************************
**
**  gcoOS_GetTime
**
**  Get the number of microseconds since the system started.
**
**  INPUT:
**
**  OUTPUT:
**
**      gctUINT64_PTR Time
**          Pointer to a variable to get time.
**
*/
gceSTATUS
gcoOS_GetTime(
    OUT gctUINT64_PTR Time
    )
{
    /* Return time in microseconds. */
    *Time = GetTickCount() * 1000;
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_GetCPUTime
**
**  Get CPU time usage in microseconds.
**
**  INPUT:
**
**  OUTPUT:
**
**      gctUINT64_PTR CPUTime
**          Pointer to a variable to get CPU time usage.
**
*/
gceSTATUS
gcoOS_GetCPUTime(
    OUT gctUINT64_PTR CPUTime
    )
{
    /* Return CPU time in microseconds. */
    *CPUTime = 0;
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoOS_GetMemoryUsage
**
**  Get current processes resource usage.
**
**  INPUT:
**
**  OUTPUT:
**
**      gctUINT32_PTR MaxRSS
**          Total amount of resident set memory used.
**          The value will be in terms of memory pages used.
**
**      gctUINT32_PTR IxRSS
**          Total amount of memory used by the text segment
**          in kilobytes multiplied by the execution-ticks.
**
**      gctUINT32_PTR IdRSS
**          Total amount of private memory used by a process
**          in kilobytes multiplied by execution-ticks.
**
**      gctUINT32_PTR IsRSS
**          Total amount of memory used by the stack in
**          kilobytes multiplied by execution-ticks.
**
*/
gceSTATUS
gcoOS_GetMemoryUsage(
    OUT gctUINT32_PTR MaxRSS,
    OUT gctUINT32_PTR IxRSS,
    OUT gctUINT32_PTR IdRSS,
    OUT gctUINT32_PTR IsRSS
    )
{
    /* Return memory usage. */
    *MaxRSS = 0;
    *IxRSS  = 0;
    *IdRSS  = 0;
    *IsRSS  = 0;
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoOS_ReadRegister
**
**  Read data from a register.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctUINT32 Address
**          Address of register.
**
**  OUTPUT:
**
**      gctUINT32 * Data
**          Pointer to a variable that receives the data read from the register.
*/
gceSTATUS
gcoOS_ReadRegister(
    IN gcoOS Os,
    IN gctUINT32 Address,
    OUT gctUINT32 * Data
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Address=0x%x", Address);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Data != gcvNULL);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.command = gcvHAL_READ_REGISTER;
    iface.u.ReadRegisterData.address = Address;
    iface.u.ReadRegisterData.data    = 0xDEADDEAD;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                 IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    /* Return the Data on success. */
    *Data = iface.u.ReadRegisterData.data;

    /* Success. */
    gcmFOOTER_ARG("*Data=0x%08x", *Data);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoOS_WriteRegister
**
**  Write data to a register.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctUINT32 Address
**          Address of register.
**
**      gctUINT32 Data
**          Data for register.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_WriteRegister(
    IN gcoOS Os,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER_ARG("Address=0x%x Data=0x%08x", Address, Data);

    /* Initialize the gcsHAL_INTERFACE structure. */
    iface.command = gcvHAL_WRITE_REGISTER;
    iface.u.WriteRegisterData.address = Address;
    iface.u.WriteRegisterData.data   = Data;

    /* Call kernel driver. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &iface, gcmSIZEOF(iface),
                                   &iface, gcmSIZEOF(iface)));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
gcoOS_Cache(
    IN gcuVIDMEM_NODE_PTR Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes,
    IN gceCACHEOPERATION Operation
    )
{
    gcsHAL_INTERFACE ioctl;
    gceSTATUS status;

    gcmHEADER_ARG("Node=0x%x Logical=0x%x Bytes=%u Operation=%d",
                  Node, Logical, Bytes, Operation);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);
    gcmVERIFY_ARGUMENT(Bytes > 0);

    /* Set up the cache. */
    ioctl.command            = gcvHAL_CACHE;
    ioctl.u.Cache.operation  = Operation;
    ioctl.u.Cache.node       = Node;
    ioctl.u.Cache.logical    = Logical;
    ioctl.u.Cache.bytes      = Bytes;

    /* Call the kernel. */
    gcmONERROR(gcoOS_DeviceControl(gcvNULL,
                                   IOCTL_GCHAL_INTERFACE,
                                   &ioctl, gcmSIZEOF(ioctl),
                                   &ioctl, gcmSIZEOF(ioctl)));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcoOS_CacheClean
**
**  Flush the cache for the specified addresses.  The GPU is going to need the
**  data.  If the system is allocating memory as non-cachable, this function can
**  be ignored.
**
**  ARGUMENTS:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gcuVIDMEM_NODE_PTR Node
**          Pointer to the video memory node that needs to be flushed.
**
**      gctPOINTER Logical
**          Logical address to flush.
**
**      gctSIZE_T Bytes
**          Size of the address range in bytes to flush.
*/
gceSTATUS
gcoOS_CacheClean(
    IN gcoOS Os,
    IN gcuVIDMEM_NODE_PTR Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Node=0x%x Logical=0x%x Bytes=%u",
                  Node, Logical, Bytes);

    /* Call common code. */
    gcmONERROR(gcoOS_Cache(Node, Logical, Bytes, gcvCACHE_CLEAN));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcoOS_CacheFlush
**
**  Flush the cache for the specified addresses and invalidate the lines as
**  well.  The GPU is going to need and modify the data.  If the system is
**  allocating memory as non-cachable, this function can be ignored.
**
**  ARGUMENTS:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gcuVIDMEM_NODE_PTR Node
**          Pointer to the video memory node that needs to be flushed.
**
**      gctPOINTER Logical
**          Logical address to flush.
**
**      gctSIZE_T Bytes
**          Size of the address range in bytes to flush.
*/
gceSTATUS
gcoOS_CacheFlush(
    IN gcoOS Os,
    IN gcuVIDMEM_NODE_PTR Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Node=0x%x Logical=0x%x Bytes=%u",
                  Node, Logical, Bytes);

    /* Call common code. */
    gcmONERROR(gcoOS_Cache(Node, Logical, Bytes, gcvCACHE_FLUSH));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcoOS_CacheInvalidate
**
**  Invalidate the lines. The GPU is going modify the data.  If the system is
**  allocating memory as non-cachable, this function can be ignored.
**
**  ARGUMENTS:
**
**      gcoOS Os
**          Pointer to gcoOS object.
**
**      gcuVIDMEM_NODE_PTR Node
**          Pointer to the video memory node that needs to be invalidated.
**
**      gctPOINTER Logical
**          Logical address to flush.
**
**      gctSIZE_T Bytes
**          Size of the address range in bytes to invalidated.
*/
gceSTATUS
gcoOS_CacheInvalidate(
    IN gcoOS Os,
    IN gcuVIDMEM_NODE_PTR Node,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Node=0x%x Logical=0x%x Bytes=%u",
                  Node, Logical, Bytes);

    /* Call common code. */
    gcmONERROR(gcoOS_Cache(Node, Logical, Bytes, gcvCACHE_INVALIDATE));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*----------------------------------------------------------------------------*/
/*----- Profiling ------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

gceSTATUS
gcoOS_GetProfileTick(
    OUT gctUINT64_PTR Tick
    )
{
    LARGE_INTEGER q;

    QueryPerformanceCounter(&q);
    *Tick = q.QuadPart;

    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_QueryProfileTickRate(
    OUT gctUINT64_PTR TickRate
    )
{
    LARGE_INTEGER q;

    QueryPerformanceFrequency(&q);
    *TickRate = q.QuadPart;

    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcoOS_ProfileDB
**
**  Manage the profile database.
**
**  The database layout is very simple:
**
**      <RecordID> (1 byte) <record data>
**
**  The <RecordID> can be one of the following values:
**
**      1       Initialize a new function to be profiled. Followed by the NULL-
**              terminated name of the function, 4 bytes of the function ID and
**              8 bytes of the profile tick.
**      2       Enter a function to be profiled. Followed by 4 bytes of function
**              ID and 8 bytes of the profile tick.
**      3       Exit a function to be profiled. Followed by 8 bytes of the
**              profile tick.
**
**  There are three options to manage the profile database. One would be to
**  enter a function that needs to be profiled. This is specified with both
**  <Function> and <Initialized> pointers initialized to some value. Here
**  <Function> is pointing to a string with the function name and <Initialized>
**  is pointing to a boolean value that tells the profiler whether this function
**  has been initialized or not.
**
**  The second option would be to exit a function that was being profiled. This
**  is specified by <Function> pointing to a string with the function name and
**  <Initialized> set to gcvNULL.
**
**  The third and last option is to flush the profile database. This is
**  specified with <Function> set to gcvNULL.
**
***** PARAMETERS
**
**  Function
**
**      Pointer to a string with the function name being profiled or gcvNULL to
**      flush the profile database.
**
**  Initialized
**
**      Pointer to a boolean variable that informs the profiler if the entry of
**      a function has been initialized or not, or gcvNULL to mark the exit of a
**      function being profiled.
*/
void
gcoOS_ProfileDB(
    IN gctCONST_STRING Function,
    IN OUT gctBOOL_PTR Initialized
    )
{
}

/******************************************************************************\
******************************* Signal Management ******************************
\******************************************************************************/

#undef _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    gcvZONE_SIGNAL

/*******************************************************************************
**
**  gcoOS_CreateSignal
**
**  Create a new signal.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctBOOL ManualReset
**          If set to gcvTRUE, gcoOS_Signal with gcvFALSE must be called in
**          order to set the signal to nonsignaled state.
**          If set to gcvFALSE, the signal will automatically be set to
**          nonsignaled state by gcoOS_WaitSignal function.
**
**  OUTPUT:
**
**      gctSIGNAL * Signal
**          Pointer to a variable receiving the created gctSIGNAL.
*/
gceSTATUS
gcoOS_CreateSignal(
    IN gcoOS Os,
    IN gctBOOL ManualReset,
    OUT gctSIGNAL * Signal
    )
{
    HANDLE handle;

    gcmHEADER_ARG("ManualReset=%d", ManualReset);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Signal != gcvNULL);

    /* Create an event. */
    handle = CreateEvent(gcvNULL, ManualReset, gcvFALSE, gcvNULL);

    if (handle == gcvNULL)
    {
        /* Out of resources. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OUT_OF_RESOURCES);
        return gcvSTATUS_OUT_OF_RESOURCES;
    }

    /* Return handle of event. */
    *Signal = (gctSIGNAL) handle;

    /* Success. */
    gcmFOOTER_ARG("*Signal=0x%x", *Signal);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_DestroySignal
**
**  Destroy a signal.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_DestroySignal(
    IN gcoOS Os,
    IN gctSIGNAL Signal
    )
{
    gcmHEADER_ARG("Signal=0x%x", Signal);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Signal != gcvNULL);

    /* Close the event. */
    gcmVERIFY(CloseHandle((HANDLE) Signal));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_Signal
**
**  Set a state of the specified signal.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**      gctBOOL State
**          If gcvTRUE, the signal will be set to signaled state.
**          If gcvFALSE, the signal will be set to nonsignaled state.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_Signal(
    IN gcoOS Os,
    IN gctSIGNAL Signal,
    IN gctBOOL State
    )
{
    gcmHEADER_ARG("Signal=0x%x State=%d", Signal, State);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Signal != gcvNULL);

    /* Set the new state of the event. */
    if (State)
    {
        /* Set the event to a signaled state. */
        gcmVERIFY(SetEvent((HANDLE) Signal));
    }
    else
    {
        /* Set the event to a signaled state. */
        gcmVERIFY(ResetEvent((HANDLE) Signal));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoOS_WaitSignal
**
**  Wait for a signal to become signaled.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**      gctUINT32 Wait
**          Number of milliseconds to wait.
**          Pass the value of gcvINFINITE for an infinite wait.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_WaitSignal(
    IN gcoOS Os,
    IN gctSIGNAL Signal,
    IN gctUINT32 Wait
    )
{
#if gcdNULL_DRIVER
    return gcvSTATUS_OK;
#else
    DWORD milliseconds;
    gctBOOL loop = gcvTRUE;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Signal=0x%x Wait=%u", Signal, Wait);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Signal != gcvNULL);

    /* Convert wait to milliseconds. */
    milliseconds = (Wait == gcvINFINITE) ? 10000 : Wait;

    /* Wait for event to become signaled. */
    while (loop)
    {
        switch (WaitForSingleObject((HANDLE) Signal, milliseconds))
        {
        case WAIT_OBJECT_0:
            /* Success. */
            loop = gcvFALSE;
            break;

        case WAIT_TIMEOUT:
            if (Wait != gcvINFINITE)
            {
                status = gcvSTATUS_TIMEOUT;
                loop = gcvFALSE;
                break;
            }
            break;

        default:
            status = gcvSTATUS_GENERIC_IO;
            loop = gcvFALSE;
        }
    }

    /* Return the status. */
    gcmFOOTER_ARG("Signal=0x%x status=%d", Signal, status);
    return status;
#endif
}

/*******************************************************************************
**
**  gcoOS_MapSignal
**
**  Map a signal from another process.
**
**  INPUT:
**
**      gctSIGNAL  RemoteSignal
**
**  OUTPUT:
**
**      gctSIGNAL * LocalSignal
**          Pointer to a variable receiving the created gctSIGNAL.
*/
gceSTATUS
gcoOS_MapSignal(
    IN gctSIGNAL  RemoteSignal,
    OUT gctSIGNAL * LocalSignal
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoOS_UnmapSignal
**
**  Unmap a signal mapped from another process.
**
**  INPUT:
**
**      gctSIGNAL Signal
**          Pointer to the gctSIGNAL.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_UnmapSignal(
    IN gctSIGNAL Signal
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoOS_AddSignalHandler
**
**  Adds Signal handler depending on Signal Handler Type
**
**  INPUT:
**
**      gceSignalHandlerType SignalHandlerType
**          Type of handler to be added
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoOS_AddSignalHandler (
    IN gceSignalHandlerType SignalHandlerType
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_DetectProcessByName(
    IN gctCONST_STRING Name
    )
{
    /* Unimplemented. See linux implementation. */
    return gcvSTATUS_FALSE;
}



