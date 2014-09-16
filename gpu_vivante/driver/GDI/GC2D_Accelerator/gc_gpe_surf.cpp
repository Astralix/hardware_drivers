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



#include "gc_gpe_precomp.h"
#include <ceddk.h>

//////////////////////////////////////////////////////////////////////////////
//              GC2DSurf
//////////////////////////////////////////////////////////////////////////////

GC2DSurf::GC2DSurf()
{
    mType = GC2D_OBJ_UNKNOWN;
    mFlag = 0;
    mAcc2D = gcvNULL;

    mDMAAddr = 0;
    mPhysicalAddr = 0;
    mVirtualAddr = 0;

    mLockUserMemory = gcvFALSE;
    mMapInfo = gcvNULL;

    mGalSurf = gcvNULL;

    mStride = 0;
    mWidth = 0;
    mHeight = 0;
    mSize = 0;
    mFormat = gcvSURF_UNKNOWN;
    mRotate = gcvSURF_0_DEGREE;


    mOwnerFlag = 0;

    mUDMAAddr = 0;
    mUVirtualAddr = gcvNULL;
    mUPhysicalAddr = 0;
    mUstride = 0;
    mLockUserUMemory = gcvFALSE;
    mUMapInfo = gcvNULL;

    mVDMAAddr = 0;
    mVVirtualAddr = gcvNULL;
    mVPhysicalAddr = 0;
    mVstride = 0;
    mLockUserVMemory = gcvFALSE;
    mVMapInfo = gcvNULL;
}

GC2DSurf::~GC2DSurf()
{
    if (IsValid())
    {
        Denitialize(gcvTRUE);
    }
}

gceSTATUS GC2DSurf::Initialize(
        GC2D_Accelerator_PTR pAcc2D,
        gctUINT flag, gctUINT PhysAddr, gctPOINTER VirtAddr,
        gctUINT width, gctUINT height, gctUINT stride,
        gceSURF_FORMAT format, gceSURF_ROTATION rotate)
{
    // already initialized
    if (mType != GC2D_OBJ_UNKNOWN)
    {
        return gcvSTATUS_INVALID_OBJECT;
    }

    if (pAcc2D == gcvNULL || !pAcc2D->GC2DIsValid())
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check format
    switch (format)
    {
    // list of the supported formats except planar YUV
    case    gcvSURF_X4R4G4B4:
    case    gcvSURF_A4R4G4B4:
    case    gcvSURF_X1R5G5B5:
    case    gcvSURF_A1R5G5B5:
    case    gcvSURF_R5G6B5  :
    case    gcvSURF_X8R8G8B8:
    case    gcvSURF_A8R8G8B8:
    case    gcvSURF_INDEX8  :
    case    gcvSURF_YUY2:
    case    gcvSURF_UYVY:
        break;

    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check width , height & stride
    if (width >= MAX_WIDTH_HEIGHT || height >= MAX_WIDTH_HEIGHT
        || stride >= MAX_STRIDE || width > stride)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // alignment check
    if (gcmNO_SUCCESS(AlignWidthHeight(&width, &height)))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }


    // check buffer, can not wrap without buffer
    if (PhysAddr == 0 && VirtAddr == gcvNULL)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check flags
    if (flag & (~GC2DSURF_PROPERTY_ALL))
    {
        // unknown flag
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    mFormat = format;
    mRotate = rotate;
    mWidth = width;
    mHeight = height;
    mStride = stride;
    mSize = stride * height;
    mFlag = flag;
    mPhysicalAddr = PhysAddr;
    mVirtualAddr = VirtAddr;

    // if the user specified buffer is contiguous, the MMU is not needed
    if (mFlag & GC2DSURF_PROPERTY_CONTIGUOUS)
    {
        if (mPhysicalAddr)
        {
            gcmASSERT(!(mPhysicalAddr & DMA_PHYSICAL_BASE));
            mDMAAddr = mPhysicalAddr & DMA_PHYSICAL_OFFSET;
        }
        else
        {
            gcmASSERT(mVirtualAddr);
            mLockUserMemory = LockUserMemory(mVirtualAddr, mSize, &mDMAAddr);
            if (!mLockUserMemory)
            {
                return gcvSTATUS_INVALID_ARGUMENT;
            }
        }
    }
    else if (!mVirtualAddr || gcmNO_SUCCESS(gcoOS_MapUserMemory(pAcc2D->mOs, mVirtualAddr, mSize, &mMapInfo, &mDMAAddr)))
    {
        mMapInfo = gcvNULL;
        mDMAAddr = gcvNULL;
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if ((mFlag & GC2DSURF_PROPERTY_CACHABLE) && mVirtualAddr)
    {
        CacheRangeFlush(mVirtualAddr, mSize, CACHE_SYNC_ALL);
    }

    // init the member variables
    mAcc2D = pAcc2D;
    mGalSurf = gcvNULL;

    mUDMAAddr = 0;
    mUVirtualAddr = 0;
    mUstride = 0;
    mUPhysicalAddr = 0;
    mLockUserUMemory = gcvFALSE;
    mUMapInfo = gcvNULL;

    mVDMAAddr = 0;
    mVVirtualAddr = 0;
    mVstride = 0;
    mVPhysicalAddr = 0;
    mLockUserVMemory = gcvFALSE;
    mVMapInfo = 0;

    mType = GC2D_OBJ_SURF;

    return gcvSTATUS_OK;
}

// if both UXXXXAddr and VXXXXAddr are NULL, we take the U component and V component
//  contiguous with Y component
gceSTATUS GC2DSurf::InitializeYUV(
        GC2D_Accelerator_PTR pAcc2D,
        gctUINT flag, gceSURF_FORMAT format,
        gctUINT YPhysAddr, gctPOINTER YVirtAddr, gctUINT YStride,
        gctUINT UPhysAddr, gctPOINTER UVirtAddr, gctUINT UStride,
        gctUINT VPhysAddr, gctPOINTER VVirtAddr, gctUINT VStride,
        gctUINT width, gctUINT height, gceSURF_ROTATION rotate
        )
{
    gctBOOL UVContingous = UPhysAddr == 0 && UVirtAddr == gcvNULL
                && VPhysAddr == 0 && VVirtAddr == gcvNULL;
    gctUINT32 UVSize;

    // already initialized
    if (mType != GC2D_OBJ_UNKNOWN)
    {
        return gcvSTATUS_INVALID_OBJECT;
    }

    if (pAcc2D == gcvNULL || !pAcc2D->GC2DIsValid())
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check format
    switch (format)
    {
    // list of the supported formats
    case    gcvSURF_YV12:
    case    gcvSURF_I420:
        if (!UVContingous)
        {
            if ((UPhysAddr == 0 && UVirtAddr == gcvNULL)
                || (VPhysAddr == 0 && VVirtAddr == gcvNULL))
            {
                return gcvSTATUS_INVALID_ARGUMENT;
            }
        }

        if (YPhysAddr == 0 && YVirtAddr == gcvNULL)
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }
        break;

    case    gcvSURF_YUY2:
    case    gcvSURF_UYVY:
        if (!UVContingous)
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }
        else
        {
            return Initialize(pAcc2D, flag, YPhysAddr, YVirtAddr, width, height, YStride, format, rotate);
        }

    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check rotation
    switch (rotate)
    {
    // list of the supported rotations
    case    gcvSURF_0_DEGREE:
    case    gcvSURF_90_DEGREE:
        break;

    case    gcvSURF_180_DEGREE:
    case    gcvSURF_270_DEGREE:
    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check width , height & stride
    if (width >= MAX_WIDTH_HEIGHT || height >= MAX_WIDTH_HEIGHT
        || YStride >= MAX_STRIDE || UStride >= MAX_STRIDE
        || VStride >= MAX_STRIDE)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // alignment check
    if (gcmNO_SUCCESS(AlignWidthHeight(&width, &height)))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check flags
    if (flag & (~GC2DSURF_PROPERTY_ALL))
    {
        // unknown flag
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (!(flag & GC2DSURF_PROPERTY_CONTIGUOUS) && YVirtAddr == gcvNULL)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // if the user specified buffer is contiguous, the MMU is not needed
    if (flag & GC2DSURF_PROPERTY_CONTIGUOUS)
    {
        if (YPhysAddr)
        {
            gcmASSERT(!(YPhysAddr & DMA_PHYSICAL_BASE));
            mDMAAddr = YPhysAddr & DMA_PHYSICAL_OFFSET;
        }
        else
        {
            gctSIZE_T mapSize = height*YStride;

            if (UVContingous)
                mapSize += mapSize >> 1;

            // lock the user virtual memory
            mLockUserMemory = LockUserMemory(YVirtAddr, mapSize, &mDMAAddr);
            if (!mLockUserMemory)
            {
                return gcvSTATUS_INVALID_ARGUMENT;
            }
        }
    }
    else
    {
        gctSIZE_T mapSize = height*YStride;

        if (UVContingous)
            mapSize += mapSize >> 1;

        if (gcmNO_SUCCESS(gcoOS_MapUserMemory(pAcc2D->mOs, YVirtAddr, mapSize, &mMapInfo, &mDMAAddr)))
        {
            mMapInfo = gcvNULL;
            mDMAAddr = gcvNULL;
            return gcvSTATUS_INVALID_ARGUMENT;
        }
    }

    mPhysicalAddr = YPhysAddr;
    mVirtualAddr = YVirtAddr;
    mWidth = width;
    mHeight = height;
    mStride = YStride;

    // total size
    mSize = height*YStride;
    mSize += mSize >> 1;

    if (UStride == 0)
        mUstride = mStride >> 1;

    if (VStride == 0)
        mVstride = mStride >> 1;

    UVSize = (mHeight >> 1) * mUstride;

    if (UVContingous)
    {
        if (gcvSURF_I420 == format)
        {
            mUDMAAddr = mDMAAddr + mHeight * mStride;
            mVDMAAddr = mUDMAAddr + UVSize;
        }
        else if (gcvSURF_YV12 == format)
        {
            mVDMAAddr = mDMAAddr + mHeight * mStride;
            mUDMAAddr = mVDMAAddr + UVSize;
        }
        else
        {
        }
    }
    else
    {
        if (flag & GC2DSURF_PROPERTY_CONTIGUOUS)
        {
            if (UPhysAddr)
            {
                gcmASSERT(!(UPhysAddr & DMA_PHYSICAL_BASE));
                mUDMAAddr = UPhysAddr & DMA_PHYSICAL_OFFSET;
            }
            else if (UVirtAddr)
            {
                mLockUserUMemory = LockUserMemory(UVirtAddr, UVSize, &mUDMAAddr);
                if (!mLockUserUMemory)
                {
                    goto ERROR_EXIT;
                }
            }
            else
            {
                goto ERROR_EXIT;
            }

            if (VPhysAddr)
            {
                gcmASSERT(!(VPhysAddr & DMA_PHYSICAL_BASE));
                mVDMAAddr = VPhysAddr & DMA_PHYSICAL_OFFSET;
            }
            else if (VVirtAddr)
            {
                mLockUserVMemory = LockUserMemory(VVirtAddr, UVSize, &mVDMAAddr);
                if (!mLockUserVMemory)
                {
                    goto ERROR_EXIT;
                }
            }
            else
            {
                goto ERROR_EXIT;
            }
        }
        else
        {
            if (gcmNO_SUCCESS(gcoOS_MapUserMemory(pAcc2D->mOs, UVirtAddr, UVSize, &mUMapInfo, &mUDMAAddr)))
            {
                goto ERROR_EXIT;
            }

            if (gcmNO_SUCCESS(gcoOS_MapUserMemory(pAcc2D->mOs, VVirtAddr, UVSize, &mVMapInfo, &mVDMAAddr)))
            {
                goto ERROR_EXIT;
            }
        }
    }

    mUPhysicalAddr = UPhysAddr;
    mUVirtualAddr = UVirtAddr;
    mVPhysicalAddr = VPhysAddr;
    mVVirtualAddr = VVirtAddr;

    // init the member variables
    mAcc2D = pAcc2D;
    mFormat = format;
    mRotate = rotate;

    mFlag = flag;
    mGalSurf = gcvNULL;


    mType = GC2D_OBJ_SURF;

    return gcvSTATUS_OK;

ERROR_EXIT:

    if (mMapInfo)
    {
        gctSIZE_T mapSize = mHeight*YStride;

        if (UVContingous)
            mapSize += mapSize >> 1;

        gcmVERIFY_OK(gcoOS_UnmapUserMemory(pAcc2D->mOs, YVirtAddr, mapSize, mMapInfo, mDMAAddr));
    }

    if (mUMapInfo)
    {
        gcmVERIFY_OK(gcoOS_UnmapUserMemory(pAcc2D->mOs, UVirtAddr, UVSize, mUMapInfo, mUDMAAddr));
    }

    if (mVMapInfo)
    {
        gcmVERIFY_OK(gcoOS_UnmapUserMemory(pAcc2D->mOs, VVirtAddr, UVSize, mVMapInfo, mVDMAAddr));
    }

    if (mLockUserMemory)
    {
        gctSIZE_T mapSize = mHeight*YStride;

        if (UVContingous)
            mapSize += mapSize >> 1;

        // unlock the user virtual memory
        gcmVERIFY(UnlockUserMemory(YVirtAddr, mapSize));
    }

    if (mLockUserUMemory)
    {
        // unlock the user virtual memory
        gcmVERIFY(UnlockUserMemory(UVirtAddr, UVSize));
    }

    if (mLockUserVMemory)
    {
        // unlock the user virtual memory
        gcmVERIFY(UnlockUserMemory(VVirtAddr, UVSize));
    }

    // clear this object
    mType = GC2D_OBJ_UNKNOWN;
    mFlag = 0;
    mAcc2D = gcvNULL;

    mDMAAddr = 0;
    mPhysicalAddr = 0;
    mVirtualAddr = 0;

    mLockUserMemory = gcvFALSE;
    mMapInfo = gcvNULL;

    mGalSurf = gcvNULL;

    mStride = 0;
    mWidth = 0;
    mHeight = 0;
    mSize = 0;
    mFormat = gcvSURF_UNKNOWN;
    mRotate = gcvSURF_0_DEGREE;


    mOwnerFlag = 0;

    mUDMAAddr = 0;
    mUVirtualAddr = gcvNULL;
    mUPhysicalAddr = 0;
    mUstride = 0;
    mLockUserUMemory = gcvFALSE;
    mUMapInfo = gcvNULL;

    mVDMAAddr = 0;
    mVVirtualAddr = gcvNULL;
    mVPhysicalAddr = 0;
    mVstride = 0;
    mLockUserVMemory = gcvFALSE;
    mVMapInfo = gcvNULL;

    return gcvSTATUS_INVALID_ARGUMENT;
}


gceSTATUS GC2DSurf::InitializeGALSurface(
    GC2D_Accelerator_PTR pAcc2D, gcoSURF galSurf, gceSURF_ROTATION rotate)
{
    gceSTATUS status;
    gctUINT32 width, height;
    gctINT stride;
    gceSURF_FORMAT format;

    // already initialized
    if (mType != GC2D_OBJ_UNKNOWN)
    {
        return gcvSTATUS_INVALID_OBJECT;
    }

    if (pAcc2D == gcvNULL)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    status = gcoSURF_GetAlignedSize(galSurf,
                                        &width,
                                        &height,
                                        &stride);
    if (gcvSTATUS_OK != status)
    {
        GC2D_DebugTrace(GC2D_GCHAL, "[GC2DSurf]: Get aligned size of gal surface failed: %d\r\n", status);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    status = gcoSURF_GetFormat(galSurf, gcvNULL, &format);
    if (gcvSTATUS_OK != status)
    {
        GC2D_DebugTrace(GC2D_GCHAL, "[GC2DSurf]: Get format of gal surface failed: %d\r\n", status);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check format
    switch (format)
    {
    // list of the supported formats
    case    gcvSURF_X4R4G4B4:
    case    gcvSURF_A4R4G4B4:
    case    gcvSURF_X1R5G5B5:
    case    gcvSURF_A1R5G5B5:
    case    gcvSURF_R5G6B5  :
    case    gcvSURF_X8R8G8B8:
    case    gcvSURF_A8R8G8B8:
    case    gcvSURF_INDEX8  :
    case    gcvSURF_YUY2:
    case    gcvSURF_UYVY:
    case    gcvSURF_YV12:
    case    gcvSURF_I420:
        break;

    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check rotation
    switch (rotate)
    {
    // list of the supported rotations
    case    gcvSURF_0_DEGREE:
    case    gcvSURF_90_DEGREE:
        break;

    case    gcvSURF_180_DEGREE:
    case    gcvSURF_270_DEGREE:
    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // check width , height & stride
    if (width >= MAX_WIDTH_HEIGHT || height >= MAX_WIDTH_HEIGHT
        || stride >= MAX_STRIDE)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // alignment check
    if (gcmNO_SUCCESS(AlignWidthHeight(&width, &height)))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gctUINT32 address[3];
    gctPOINTER memory[3];

    status = gcoSURF_Lock(galSurf, address, memory);
    if (gcvSTATUS_OK != status)
    {
        GC2D_DebugTrace(GC2D_GCHAL, "[GC2DSurf]: Lock gal surface failed: %d\r\n", status);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    switch (format)
    {
    // list of the supported formats
    case    gcvSURF_X4R4G4B4:
    case    gcvSURF_A4R4G4B4:
    case    gcvSURF_X1R5G5B5:
    case    gcvSURF_A1R5G5B5:
    case    gcvSURF_R5G6B5  :
    case    gcvSURF_X8R8G8B8:
    case    gcvSURF_A8R8G8B8:
    case    gcvSURF_INDEX8  :
    case    gcvSURF_YUY2:
    case    gcvSURF_UYVY:
        mDMAAddr = address[0];
        mVirtualAddr = memory[0];
        break;

    case    gcvSURF_YV12:
        mDMAAddr = address[0];
        mVirtualAddr = memory[0];
        mVDMAAddr = address[1];
        mVVirtualAddr = memory[1];
        mUDMAAddr = address[2];
        mUVirtualAddr = memory[2];
        break;

    case    gcvSURF_I420:
        mDMAAddr = address[0];
        mVirtualAddr = memory[0];
        mUDMAAddr = address[1];
        mUVirtualAddr = memory[1];
        mVDMAAddr = address[2];
        mVVirtualAddr = memory[2];
        break;

    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    // init the member variables
    mAcc2D = pAcc2D;
    mFormat = format;
    mRotate = rotate;
    mWidth = width;
    mHeight = height;
    mStride = stride;
    mSize = height * stride;
    mFlag = 0;
    mLockUserMemory = gcvFALSE;
    mMapInfo = gcvNULL;
    mGalSurf = galSurf;

    mUstride = mStride >> 1;
    mUPhysicalAddr = 0;
    mLockUserUMemory = gcvFALSE;
    mUMapInfo = gcvNULL;

    mVstride = mStride >> 1;
    mVPhysicalAddr = 0;
    mLockUserVMemory = gcvFALSE;
    mVMapInfo = gcvNULL;

    mType = GC2D_OBJ_SURF;

    return gcvSTATUS_OK;
}

gctBOOL GC2DSurf::IsValid() const
{
#ifdef DEBUG
    if (!mAcc2D || !mAcc2D->GC2DIsValid())
    {
        return gcvFALSE;
    }

    if ((mFormat == gcvSURF_UNKNOWN) || (mAcc2D == gcvNULL))
    {
        return gcvFALSE;
    }

    if (mGalSurf && (mMapInfo || mLockUserMemory
        || mUMapInfo || mLockUserUMemory
        || mVMapInfo || mLockUserVMemory))
    {
        return gcvFALSE;
    }
#endif
    return (mType == GC2D_OBJ_SURF);
}

gceSTATUS GC2DSurf::Denitialize(gctBOOL sync)
{
    gcmASSERT(IsValid());

    if (!IsValid())
    {
        return gcvSTATUS_INVALID_OBJECT;
    }

    if (mGalSurf)
    {
        gcmVERIFY_OK(gcoSURF_Unlock(mGalSurf, mVirtualAddr));
    }
    else
    {
        gctBOOL UVContingous = gcvFALSE;
        gctUINT32 UVSize = 0;

        switch (mFormat)
        {
        case    gcvSURF_YV12:
        case    gcvSURF_I420:
            gcmASSERT((mUMapInfo == gcvNULL && mVMapInfo == gcvNULL)
                || (mUMapInfo != gcvNULL && mVMapInfo != gcvNULL));
            gcmASSERT((mLockUserUMemory == gcvFALSE && mLockUserVMemory == gcvFALSE)
                || (mLockUserUMemory == gcvTRUE && mLockUserVMemory == gcvTRUE));

            if ((mUMapInfo == gcvNULL && mVMapInfo == gcvNULL)
                || (mLockUserUMemory == gcvFALSE && mLockUserVMemory == gcvFALSE))
            {
                UVContingous = gcvTRUE;
            }

            UVSize = mHeight * mUstride;

        default:
            break;
        }

        if (mMapInfo)
        {
            gctSIZE_T mapSize = mHeight * mStride;

            if (UVContingous)
                mapSize += mapSize >> 1;

            if (sync)
            {
                gcmVERIFY_OK(gcoOS_UnmapUserMemory(mAcc2D->mOs, mVirtualAddr, mapSize, mMapInfo, mDMAAddr));
            }
            else
            {
                gcmVERIFY_OK(gcoHAL_ScheduleUnmapUserMemory(mAcc2D->mHal, mMapInfo, mapSize, mDMAAddr, mVirtualAddr));
            }
        }

        if (mUMapInfo)
        {
            if (sync)
            {
                gcmVERIFY_OK(gcoOS_UnmapUserMemory(mAcc2D->mOs, mUVirtualAddr, UVSize, mUMapInfo, mUDMAAddr));
            }
            else
            {
                gcmVERIFY_OK(gcoHAL_ScheduleUnmapUserMemory(mAcc2D->mHal, mUMapInfo, UVSize, mUDMAAddr, mUVirtualAddr));
            }
        }

        if (mVMapInfo)
        {
            if (sync)
            {
                gcmVERIFY_OK(gcoOS_UnmapUserMemory(mAcc2D->mOs, mVVirtualAddr, UVSize, mVMapInfo, mVDMAAddr));
            }
            else
            {
                gcmVERIFY_OK(gcoHAL_ScheduleUnmapUserMemory(mAcc2D->mHal, mVMapInfo, UVSize, mVDMAAddr, mVVirtualAddr));
            }
        }

        if (mLockUserMemory)
        {
            gctSIZE_T mapSize = mHeight*mStride;

            if (UVContingous)
                mapSize += mapSize >> 1;

            // unlock the user virtual memory
            gcmVERIFY(UnlockUserMemory(mVirtualAddr, mapSize));
        }

        if (mLockUserUMemory)
        {
            // unlock the user virtual memory
            gcmVERIFY(UnlockUserMemory(mUVirtualAddr, UVSize));
        }

        if (mLockUserVMemory)
        {
            // unlock the user virtual memory
            gcmVERIFY(UnlockUserMemory(mVVirtualAddr, UVSize));
        }
    }
    // clear this object
    mType = GC2D_OBJ_UNKNOWN;
    mFlag = 0;
    mAcc2D = gcvNULL;

    mDMAAddr = 0;
    mPhysicalAddr = 0;
    mVirtualAddr = 0;

    mLockUserMemory = gcvFALSE;
    mMapInfo = gcvNULL;

    mGalSurf = gcvNULL;

    mStride = 0;
    mWidth = 0;
    mHeight = 0;
    mSize = 0;
    mFormat = gcvSURF_UNKNOWN;
    mRotate = gcvSURF_0_DEGREE;


    mOwnerFlag = 0;

    mUDMAAddr = 0;
    mUVirtualAddr = gcvNULL;
    mUPhysicalAddr = 0;
    mUstride = 0;
    mLockUserUMemory = gcvFALSE;
    mUMapInfo = gcvNULL;

    mVDMAAddr = 0;
    mVVirtualAddr = gcvNULL;
    mVPhysicalAddr = 0;
    mVstride = 0;
    mLockUserVMemory = gcvFALSE;
    mVMapInfo = gcvNULL;

    return gcvSTATUS_OK;
}

gctBOOL GC2DSurf::LockUserMemory(gctPOINTER VirtAddr, DWORD size, gctUINT* PhysAddr)
{
    DWORD pfn[MAX_LOCK_PAGES];

    if (!VirtAddr || !size)
    {
        return gcvFALSE;
    }

    if (BYTES_TO_PAGES(size) > MAX_LOCK_PAGES)
    {
        return gcvFALSE;
    }

    if (LockPages(VirtAddr, size, pfn, LOCKFLAG_READ | LOCKFLAG_WRITE))
    {
        *PhysAddr = ((pfn[0] << UserKInfo[KINX_PFN_SHIFT]) + BYTE_OFFSET(VirtAddr))
            & DMA_PHYSICAL_OFFSET;
        return gcvTRUE;
    }
    else
    {
        return gcvFALSE;
    }
}

gctBOOL GC2DSurf::UnlockUserMemory(gctPOINTER VirtAddr, DWORD size)
{
    if (!VirtAddr || !size)
    {
        return gcvFALSE;
    }

    if (BYTES_TO_PAGES(size) > MAX_LOCK_PAGES)
    {
        return gcvFALSE;
    }

    return UnlockPages(VirtAddr, size);
}


//////////////////////////////////////////////////////////////////////////////
//              GC2D_SurfWrapper
//////////////////////////////////////////////////////////////////////////////
GC2D_SurfWrapper::GC2D_SurfWrapper(
        GC2D_Accelerator_PTR pAcc2D,
        int                  width,
        int                  height,
        EGPEFormat           format): mAllocFlagByUser(0), mAcc2D(gcvNULL)
{
    gceSURF_FORMAT    gcFormat = EGPEFormatToGALFormat(format);
    int                bpp = GetFormatSize(gcFormat);

    GC2D_ENTER_ARG("pAcc2D = 0x%08p, width = %d, height = %d, format = %d",
        pAcc2D, width, height, format);

    if (pAcc2D && pAcc2D->GC2DIsValid() && (gcFormat != gcvSURF_UNKNOWN) && (bpp != 0))
    {
        gctUINT32   AlignedWidth = width;
        gctUINT32   AlignedHeight = height;

        AlignWidthHeight(&AlignedWidth, &AlignedHeight);
        if (AlignedWidth == width && AlignedHeight == height)
        {
            void *bits = AllocNonCacheMem(width*height*bpp);
            if (bits)
            {
                // wrapped by GC2DSurf
                mAcc2D = pAcc2D;
                if (gcmIS_SUCCESS(mGC2DSurf.Initialize(pAcc2D,
                        GC2DSURF_PROPERTY_DEFAULT, 0, bits,
                        width, height, width*GetFormatSize(gcFormat), gcFormat, gcvSURF_0_DEGREE)))
                {
                    GPESurf::Init(width, height, mGC2DSurf.mVirtualAddr, mGC2DSurf.mStride, format);
                    mAllocFlagByUser = 1;
                    m_fOwnsBuffer = GC2D_OBJ_SURF;

                    GC2D_LEAVE();
                    return;
                }
                else
                {
                    FreeNonCacheMem(bits);
                    mAcc2D = gcvNULL;
                }
            }
        }
    }

    DDGPESurf::DDGPESurf(width, height, format);

    GC2D_LEAVE();
}

#ifndef DERIVED_FROM_GPE
GC2D_SurfWrapper::GC2D_SurfWrapper(
        GC2D_Accelerator_PTR  pAcc2D,
        int                   width,
        int                   height,
        int                   stride,
        EGPEFormat            format,
        EDDGPEPixelFormat     pixelFormat): mAllocFlagByUser(0), mAcc2D(gcvNULL)
{
    gceSURF_FORMAT    gcFormat = DDGPEFormatToGALFormat(pixelFormat);
    int                bpp = GetFormatSize(gcFormat);

    GC2D_ENTER_ARG("pAcc2D = 0x%08p, width = %d, height = %d, stride = %, format = %d, pixelFormat = %d",
        pAcc2D, width, height, stride, format, pixelFormat);

    if (pAcc2D && pAcc2D->GC2DIsValid() && (gcFormat != gcvSURF_UNKNOWN) && (bpp != 0))
    {
        gctUINT32   AlignedWidth = width;
        gctUINT32   AlignedHeight = height;

        AlignWidthHeight(&AlignedWidth, &AlignedHeight);
        if (AlignedWidth == width && AlignedHeight == height)
        {
            // wrapped by GC2DSurf
            void *bits = AllocNonCacheMem(width*height*bpp);
            if (bits)
            {
                mAcc2D = pAcc2D;
                if (gcmIS_SUCCESS(mGC2DSurf.Initialize(pAcc2D,
                        GC2DSURF_PROPERTY_DEFAULT, 0, bits,
                        width, height, width*GetFormatSize(gcFormat), gcFormat, gcvSURF_0_DEGREE)))
                {
                    Init(width, height, mGC2DSurf.mVirtualAddr, mGC2DSurf.mStride, format, pixelFormat);
                    mAllocFlagByUser = 1;
                    m_fOwnsBuffer = GC2D_OBJ_SURF;

                    GC2D_LEAVE();
                    return;
                }
                else
                {
                    FreeNonCacheMem(bits);
                    mAcc2D = gcvNULL;
                }
            }
        }
    }

    DDGPESurf::DDGPESurf(width,
                         height,
                         stride,
                         format,
                         pixelFormat);
    GC2D_LEAVE();
}
#endif

GC2D_SurfWrapper::GC2D_SurfWrapper(               // Create video-memory surface
                        GC2D_Accelerator_PTR  pAcc2D,
                        int                   width,
                        int                   height,
                        void *                pBits,            // virtual address of allocated bits
                        int                   stride,
                        EGPEFormat            format): mAllocFlagByUser(0), mAcc2D(gcvNULL)
{
    gceSURF_FORMAT    gcFormat = EGPEFormatToGALFormat(format);
    EDDGPEPixelFormat    pixelFormat = EGPEFormatToEDDGPEPixelFormat[format];

    GC2D_ENTER_ARG("pAcc2D = 0x%08p, width = %d, height = %d, pBits = 0x%08p, stride = %, format = %d",
        pAcc2D, width, height, pBits, stride, format);

    if (pAcc2D && pAcc2D->GC2DIsValid() && (gcFormat != gcvSURF_UNKNOWN))
    {
        gctUINT32   AlignedWidth = width;
        gctUINT32   AlignedHeight = height;

        AlignWidthHeight(&AlignedWidth, &AlignedHeight);
        if (AlignedWidth == width && AlignedHeight == height)
        {
            // wrapped by GC2DSurf
            mAcc2D = pAcc2D;
            if (gcmIS_SUCCESS(mGC2DSurf.Initialize(pAcc2D,
                        GC2DSURF_PROPERTY_DEFAULT, 0, pBits,
                        width, height, width*GetFormatSize(gcFormat), gcFormat, gcvSURF_0_DEGREE)))
            {
            #ifdef DERIVED_FROM_GPE
                    Init(width, height, mGC2DSurf.mVirtualAddr, mGC2DSurf.mStride, format);
            #else
                Init(width, height, mGC2DSurf.mVirtualAddr, mGC2DSurf.mStride, format, pixelFormat);
            #endif
                m_fOwnsBuffer = GC2D_OBJ_SURF;

                GC2D_LEAVE();
                return;
            }
            else
            {
                mAcc2D = gcvNULL;
            }
        }
    }

    DDGPESurf::DDGPESurf(width,
                         height,
                         pBits,
                         stride,
                         format);
    GC2D_LEAVE();
}


GC2D_SurfWrapper::~GC2D_SurfWrapper()
{
    GC2D_ENTER();

    if (m_fOwnsBuffer == GC2D_OBJ_SURF)
    {
        gcmASSERT(mGC2DSurf.IsValid());

        mGC2DSurf.Denitialize(gcvTRUE);
        if (mAllocFlagByUser)
        {
            FreeNonCacheMem(mGC2DSurf.mVirtualAddr);
            mAllocFlagByUser = 0;
        }

        mAcc2D = gcvNULL;
        m_fOwnsBuffer = 0;
    }

    GC2D_LEAVE();
}
