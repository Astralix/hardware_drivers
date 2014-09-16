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



#ifndef __GC2D_ACCELERATOR_H__
#define __GC2D_ACCELERATOR_H__

#include    <gc_hal.h>
#include    <gc_hal_raster.h>
#include    <gc_hal_driver.h>

/****************************************************************************
* Option:
*        GC2D_GATHER_STATISTICS: enable/disable gathering statistics
*        GC2D_MIN_RENDERING_PIXEL: the default minimal number for the hardware
*            rendering.
*
*****************************************************************************/
#define GC2D_GATHER_STATISTICS   0
#define GC2D_MIN_RENDERING_PIXEL 0x100

/****************************************************************************/

#define GC2D_REGISTRY_PATH      TEXT("Drivers\\BuiltIn\\GCHAL")
#define GC2D_ENABLE_VALNAME     TEXT("GC2DAcceleratorEnable")
#define GC2D_SYNCMODE_VALNAME   TEXT("GC2DAcceleratorSyncMode")
#define GC2D_MIN_SIZE_VALNAME   TEXT("GC2DAcceleratorMinSize")

#define MAX_GC2D_FORMAT_EMUN 20

#define GC2D_DOMAIN(n)          (1 << (n))
#define GC2D_GCHAL              GC2D_DOMAIN(0)
#define GC2D_DISPLAY_BLIT       GC2D_DOMAIN(1)
#define GC2D_FUNCTION_TRACE     GC2D_DOMAIN(3)
#define GC2D_TEST               GC2D_DOMAIN(20)
#define GC2D_ALWAYS_OUT         GC2D_DOMAIN(29)
#define GC2D_DUMP_TRACE         GC2D_DOMAIN(29)
#define GC2D_DEBUG_TRACE        GC2D_DOMAIN(30)
#define GC2D_ERROR_TRACE        GC2D_DOMAIN(31)
#define GC2D_TRACE_DOMAIN      (GC2D_ALWAYS_OUT)

void GC2D_DebugTrace(
    IN gctUINT32 Flag,
    IN char* Message,
    ...
    );

void GC2D_DebugFatal(
    IN char* Message,
    ...
    );

#ifdef DEBUG

#define GC2D_ENTER() \
    GC2D_DebugTrace(GC2D_FUNCTION_TRACE, "Enter %s(%d)", __FUNCTION__, __LINE__)

#define GC2D_LEAVE() \
    GC2D_DebugTrace(GC2D_FUNCTION_TRACE, "Leave %s(%d)", __FUNCTION__, __LINE__)

#define GC2D_ENTER_ARG(TEXT, ...) \
    GC2D_DebugTrace(GC2D_FUNCTION_TRACE, "Enter %s(%d): "TEXT, __FUNCTION__, __LINE__, __VA_ARGS__)

#define GC2D_LEAVE_ARG(TEXT, ...) \
    GC2D_DebugTrace(GC2D_FUNCTION_TRACE, "Leave %s(%d): "TEXT, __FUNCTION__, __LINE__, __VA_ARGS__)

#else

#define GC2D_ENTER()
#define GC2D_LEAVE()
#define GC2D_ENTER_ARG(TEXT, ...)
#define GC2D_LEAVE_ARG(TEXT, ...)

#endif

enum GC2D_FEATURE_ENABLE
{
    GC2D_FEATURE_ENABLE_BLTPREPARE  = 0x00000001,
    GC2D_FEATURE_ENABLE_ALPHABLEND  = 0x00000002,
    GC2D_FEATURE_ENABLE_TRANSPARENT = 0x00000004,
    GC2D_FEATURE_ENABLE_STRETCH     = 0x00000008,
    GC2D_FEATURE_ENABLE_MASK        = 0x00000010,
};

enum GC2D_SYNC_MODE
{
    GC2D_SYNC_MODE_FORCE        = 0,
    GC2D_SYNC_MODE_ASYNC        = 1,
    GC2D_SYNC_MODE_FULL_ASYNC   = 2,
};

enum GC2D_ACCELERATOR_INIT_FLAG
{
    GC2D_ACCELERATOR_INIT_NONE          = 0,
    GC2D_ACCELERATOR_INIT_REGISTERY     = 0x00000001,
    GC2D_ACCELERATOR_INIT_2D_ENGINE     = 0x00000002,
    GC2D_ACCELERATOR_INIT_PARMS         = 0x00000004,
    GC2D_ACCELERATOR_INIT_FINISH_FLAG   = 0x00000008,

    GC2D_ACCELERATOR_INIT_DONE          = 0x0000000F,
};

enum GC2D_OBJECT_TYPE
{
    GC2D_OBJ_UNKNOWN                 = 0,
    GC2D_OBJ_2D                      = gcmCC('2','D','E','G'),
    GC2D_OBJ_SURF                    = gcmCC('S','U','R','F'),
    GC2D_OBJ_BRUSH                   = gcmCC('B','R','U','S'),
    GC2D_OBJ_MONO                    = gcmCC('M','O','N','O'),
    GC2D_OBJ_PARMS                   = gcmCC('P','A','R','M'),
};

enum GC2DSURF_PROPERTY
{
    GC2DSURF_PROPERTY_DEFAULT        = 0, /* non-cachable & non-contiguous */
    GC2DSURF_PROPERTY_CACHABLE       = 0x00000001,
    GC2DSURF_PROPERTY_CONTIGUOUS     = 0x00000002,
    GC2DSURF_PROPERTY_ALL            = GC2DSURF_PROPERTY_CACHABLE
                                     | GC2DSURF_PROPERTY_CONTIGUOUS,
};

#define DMA_PHYSICAL_BASE   0x80000000
#define DMA_PHYSICAL_OFFSET 0x7FFFFFFF
#define MAX_WIDTH_HEIGHT    0x10000
#define MAX_STRIDE          0x40000
#define MAX_LOCK_PAGES      0x400

class GC2D_Accelerator;
class GC2D_SurfWrapper;
class GC2DSurf;
class GC2DParms;

typedef class GC2D_Accelerator *GC2D_Accelerator_PTR;

// Local surface class
class GC2DSurf {
public:
    GC2DSurf();
    ~GC2DSurf();

    gceSTATUS Initialize(
        GC2D_Accelerator_PTR Acc2D,
        gctUINT flag, gctUINT PhysAddr, gctPOINTER VirtAddr,
        gctUINT width, gctUINT height, gctUINT stride,
        gceSURF_FORMAT format, gceSURF_ROTATION rotate);

    gceSTATUS InitializeYUV(
        GC2D_Accelerator_PTR Acc2D,
        gctUINT flag, gceSURF_FORMAT format,
        gctUINT YPhysAddr, gctPOINTER YVirtAddr, gctUINT YStride,
        gctUINT UPhysAddr, gctPOINTER UVirtAddr, gctUINT UStride,
        gctUINT VPhysAddr, gctPOINTER VVirtAddr, gctUINT VStride,
        gctUINT width, gctUINT height, gceSURF_ROTATION rotate
        );

    gceSTATUS InitializeGALSurface(
        GC2D_Accelerator_PTR Acc2D, gcoSURF galSurf, gceSURF_ROTATION rotate);

    gceSTATUS Denitialize(gctBOOL sync);

    gctBOOL IsValid() const;

private:
    gctBOOL LockUserMemory(gctPOINTER VirtAddr, DWORD size, gctUINT* PhysAddr);
    gctBOOL UnlockUserMemory(gctPOINTER VirtAddr, DWORD size);

public:
    GC2D_OBJECT_TYPE        mType;
    gctUINT                 mFlag;
    GC2D_Accelerator_PTR    mAcc2D;

    gctUINT             mDMAAddr;
    gctUINT             mPhysicalAddr;
    gctPOINTER          mVirtualAddr;

    gctBOOL             mLockUserMemory;
    gctPOINTER          mMapInfo;
    gcoSURF             mGalSurf;

    gctINT              mStride;
    gctUINT             mWidth;
    gctUINT             mHeight;
    gctUINT             mSize;
    gceSURF_FORMAT      mFormat;
    gceSURF_ROTATION    mRotate;

    gctUINT             mOwnerFlag;

    // for YUV Format
    gctUINT             mUDMAAddr;
    gctPOINTER          mUVirtualAddr;
    gctUINT             mUPhysicalAddr;
    gctINT              mUstride;
    gctBOOL             mLockUserUMemory;
    gctPOINTER          mUMapInfo;

    gctUINT             mVDMAAddr;
    gctPOINTER          mVVirtualAddr;
    gctUINT             mVPhysicalAddr;
    gctINT              mVstride;
    gctBOOL             mLockUserVMemory;
    gctPOINTER          mVMapInfo;
};

// GC2D_SurfWrapper can derived from GPESurf or DDGPESurf
class GC2D_SurfWrapper :
#ifdef DERIVED_FROM_GPE
    public GPESurf
#else
    public DDGPESurf
#endif
{
public:
    GC2D_SurfWrapper(
        GC2D_Accelerator_PTR Acc2D,
        int                  width,
        int                  height,
        EGPEFormat           format);

#ifndef DERIVED_FROM_GPE
    GC2D_SurfWrapper(
        GC2D_Accelerator_PTR Acc2D,
        int                  width,
        int                  height,
        int                  stride,
        EGPEFormat           format,
        EDDGPEPixelFormat    pixelFormat);
#endif

    GC2D_SurfWrapper(
        GC2D_Accelerator_PTR Acc2D,
        int                  width,
        int                  height,
        void *               pBits,            // virtual address of allocated bits
        int                  stride,
        EGPEFormat           format);

    ~GC2D_SurfWrapper();
    BOOL IsWrapped() const {return m_fOwnsBuffer == GC2D_OBJ_SURF;}

    UINT                    mAllocFlagByUser;
    GC2D_Accelerator_PTR    mAcc2D;
    GC2DSurf                mGC2DSurf;

private:
    GC2D_SurfWrapper();
};

// GC2D_Accelerator can derived from GPE or DDGPE
class GC2D_Accelerator:
#ifdef DERIVED_FROM_GPE
    public GPE
#else
    public DDGPE
#endif
{
    friend class GC2DSurf;
    friend class GC2DParms;

public:
    GC2D_Accelerator ()
      : mEngine2D(gcvNULL), mOs(gcvNULL), mHal(gcvNULL), mEnableFlag(0),
        mSyncMode(GC2D_SYNC_MODE_ASYNC), mCommitCounter(0),
        mFinishFlag(gcvNULL), mMinPixelNum(GC2D_MIN_RENDERING_PIXEL),
        mInitFlag(GC2D_ACCELERATOR_INIT_NONE), mParms(gcvNULL)
    {
        GC2D_ENTER();

#if GC2D_GATHER_STATISTICS
        nPrepare = 0;
        nPrepareSucceed = 0;
        nAnyBlit = 0;
        nAnyBlitFailed = 0;
        nROPBlit = 0;
        nROPBlitSucceed = 0;
        nAlphaBlend = 0;
        nAlphaBlendSucceed = 0;
        nASyncCommit = 0;
        nASyncCommitFailed = 0;
#endif
        GC2D_LEAVE();
    }

    ~GC2D_Accelerator ()
    {
        GC2D_ENTER();
        mInitFlag = 0;
        GC2D_LEAVE();
    }

    gctBOOL GC2DIsValid() const
    {
        return (mEnableFlag) && (mInitFlag == GC2D_ACCELERATOR_INIT_DONE);
    }

    gctBOOL     GC2DInitialize();
    gctBOOL     GC2DDeinitialize();
    SCODE       GC2DBltPrepare(GPEBltParms *pBltParms);
    SCODE       GC2DBltComplete(GPEBltParms *blitParameters);
    int         GC2DIsBusy();
    void        GC2DWaitForNotBusy();

    gctUINT     GC2DSetEnable(gctUINT* const pEnable = gcvNULL)
    {
        gctUINT ret = mEnableFlag;

        GC2D_ENTER_ARG("pEnable = 0x%08p", pEnable);

        if (pEnable)
        {
            mEnableFlag = *pEnable;
        }

        GC2D_LEAVE_ARG("ret = %d", ret);
        return ret;
    }

    gctUINT     GC2DChangeSyncMode(gctUINT* const pMode = gcvNULL);

    gctUINT     GC2DPowerHandler(BOOL bOff);

protected:
    gctBOOL ReadRegistry();

    gceSTATUS InitEngine2D();
    gceSTATUS DenitEngine2D();

    gceSTATUS CreateGALSurface(
        IN gctUINT Width,
        IN gctUINT Height,
        IN gceSURF_FORMAT Format,
        IN gcePOOL Pool,
        OUT gcoSURF * Surface
        );
    gceSTATUS DestroyGALSurface(
        IN gcoSURF Surface
        );

    gceSTATUS   FlushAndCommit(gctBOOL bFlush = gcvFALSE, gctBOOL bWait = gcvTRUE);
    int         UpdateBusyStatus(gctBOOL clear);

    SCODE    BltParmsFilter(GPEBltParms *pBltParms);
    SCODE    AnyBlt(GPEBltParms * pBltParms);
    SCODE    Blit(GPEBltParms * pBltParms);

    gctBOOL DumpGPEBltParms(GPEBltParms * pBltParms);
    gctBOOL DumpGPESurf(GPESurf* surf);

    gcoOS           mOs;
    gcoHAL          mHal;
    gco2D           mEngine2D;

    gctUINT         mEnableFlag;
    gctUINT         mDumpFlag;

    GC2DParms       *mParms;

    gctUINT32       mSyncMode;
    gctUINT32       mCommitCounter;
    gctUINT32_PTR   mFinishFlag;
    gctUINT32       mFinishFlagPhys;

    gctINT32        mMinPixelNum;

    gctUINT32       mInitFlag;
    gctUINT         mPowerHandle;

#if GC2D_GATHER_STATISTICS
protected:
    gctUINT nPrepare;
    gctUINT nPrepareSucceed;
    gctUINT nAnyBlit;
    gctUINT nAnyBlitFailed;
    gctUINT nROPBlit;
    gctUINT nROPBlitSucceed;
    gctUINT nAlphaBlend;
    gctUINT nAlphaBlendSucceed;
    gctUINT nASyncCommit;
    gctUINT nASyncCommitFailed;

    class GATHER_STATISTICS_SURF {
    public:
        GATHER_STATISTICS_SURF () {
            memset(nMemoryPool, 0, sizeof(nMemoryPool));
            memset(nRotate, 0, sizeof(nRotate));
            memset(nFormat, 0, sizeof(nFormat));
        }

        void RecordSurf(GPESurf* pSurf)
        {
            gcmASSERT(pSurf->Format() < MAX_GC2D_FORMAT_EMUN);
            ++nFormat[pSurf->Format()];

            switch (pSurf->Rotate())
            {
            case DMDO_0:
                ++nRotate[0];
                break;

            case DMDO_90:
                ++nRotate[1];
                break;

            case DMDO_180:
                ++nRotate[2];
                break;

            case DMDO_270:
                ++nRotate[3];
                break;

            default:
                break;
            }

            if (pSurf->InVideoMemory())
                ++nMemoryPool[0];
            else
                ++nMemoryPool[1];
        }

    protected:
        gctUINT        nMemoryPool[2];
        gctUINT        nRotate[4];
        gctUINT        nFormat[MAX_GC2D_FORMAT_EMUN];
    };
    GATHER_STATISTICS_SURF gssDst;
    GATHER_STATISTICS_SURF gssSrc;
    GATHER_STATISTICS_SURF gssMask;
    GATHER_STATISTICS_SURF gssBrush;
#endif
};

#endif //__GC2D_ACCELERATOR_H__
//__GC2D_ACCELERATOR_H__
//__GC2D_ACCELERATOR_H__
//__GC2D_ACCELERATOR_H__
//__GC2D_ACCELERATOR_H__
//__GC2D_ACCELERATOR_H__
