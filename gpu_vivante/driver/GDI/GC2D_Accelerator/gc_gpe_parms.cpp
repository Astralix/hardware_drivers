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

gctBOOL GC2DBrush::Init()
{
    if (mVirtualAddr)
    {
        return gcvFALSE;
    }

    mVirtualAddr = AllocNonCacheMem(PAGE_SIZE);
    gcmASSERT(!BYTE_OFFSET(mVirtualAddr));
    if (!mVirtualAddr)
    {
        return gcvFALSE;
    }

    if (!LockPages(mVirtualAddr, PAGE_SIZE, (PDWORD)&mDMAAddr, LOCKFLAG_QUERY_ONLY))
    {
        gcmVERIFY(FreeNonCacheMem(mVirtualAddr));
        return gcvFALSE;
    }

    mDMAAddr <<= UserKInfo[KINX_PFN_SHIFT];

    return gcvTRUE;
}

gctBOOL GC2DBrush::Denit()
{
    if (!mVirtualAddr)
    {
        return gcvFALSE;
    }

    gcmVERIFY(FreeNonCacheMem(mVirtualAddr));
    mVirtualAddr = gcvNULL;
    mDMAAddr = ~0;

    return gcvTRUE;
}

GC2DParms::GC2DParms()
{
    mType = GC2D_OBJ_UNKNOWN;
    mBlit = BLIT_INVALID;
    mTrsp = gcvSURF_OPAQUE;

    mMirrorX = gcvFALSE;
    mMirrorY = gcvFALSE;

    mSrcRect.left = 0;
    mSrcRect.top = 0;
    mSrcRect.right = 0;
    mSrcRect.bottom = 0;

    mDstRect.left = 0;
    mDstRect.top = 0;
    mDstRect.right = 0;
    mDstRect.bottom = 0;

    mMonoRect.left = 0;
    mMonoRect.top = 0;
    mMonoRect.right = 0;
    mMonoRect.bottom = 0;

    mFgRop = 0;
    mBgRop = 0;

    mDstBuffer.mAuxBuffer = gcvNULL;
    mDstBuffer.mBackTable = gcvNULL;
    mDstBuffer.mOrgTable = gcvNULL;
    mDstBuffer.mMapAddr = 0;
    mDstBuffer.mBackTableSize = 0;

    mSrcBuffer.mAuxBuffer = gcvNULL;
    mSrcBuffer.mBackTable = gcvNULL;
    mSrcBuffer.mOrgTable = gcvNULL;
    mSrcBuffer.mMapAddr = 0;
    mSrcBuffer.mBackTableSize = 0;

    mMonoBuffer = gcvNULL;
    mMonoBufferSize = 0;

    mHwVersion = 0;

    mABEnable = gcvFALSE;
}

GC2DParms::~GC2DParms()
{
    gcmASSERT(mType == GC2D_OBJ_UNKNOWN);

    if (mBrush.mVirtualAddr)
    {
        gcmVERIFY(FreeNonCacheMem(mBrush.mVirtualAddr));
    }

    if (mDstSurf.mType != GC2D_OBJ_UNKNOWN)
    {
        ResetSurface(gcvFALSE);
    }

    if (mSrcSurf.mType != GC2D_OBJ_UNKNOWN)
    {
        ResetSurface(gcvTRUE);
    }

    if (mDstBuffer.mAuxBuffer)
    {
    }

    if (mDstBuffer.mBackTable)
    {
    }

    if (mDstBuffer.mOrgTable)
    {
    }

    if (mSrcBuffer.mAuxBuffer)
    {
    }

    if (mSrcBuffer.mBackTable)
    {
    }

    if (mSrcBuffer.mOrgTable)
    {
    }

    if (mMonoBuffer)
    {
    }
}

gctBOOL GC2DParms::Initialize(
            GC2D_Accelerator_PTR Acc2D,
            gctUINT32 VideoPhyBase,
            gctUINT32 VideoMemSize,
            gctUINT MaxWidth,
            gctUINT MaxHeight)
{
    GC2D_ENTER_ARG("VideoPhyBase = %u, VideoMemSize = %u, ScreenWidth = %d, ScreenHeight = %d",
        VideoPhyBase,VideoMemSize, MaxWidth, MaxHeight);

    if (IsValid())
    {
        GC2D_LEAVE();
        return gcvTRUE;
    }

    mVideoPhyBase = VideoPhyBase;
    mVideoMemSize = VideoMemSize;
    mMinPixelNum = Acc2D->mMinPixelNum;
    mMaxPixelNum = MaxWidth * MaxHeight;

    if (!mMaxPixelNum)
    {
        GC2D_LEAVE();
        return gcvFALSE;
    }

    mMonoBufferSize = gcmALIGN(mMaxPixelNum, 8) >> 3;
    mMonoBuffer = new gctUINT8[mMonoBufferSize];
    if (mMonoBuffer == gcvNULL)
    {
        GC2D_LEAVE();
        return gcvFALSE;
    }

    if (!InitTempBuffer(Acc2D, &mDstBuffer, MaxWidth, MaxHeight))
    {
        GC2D_LEAVE();
        return gcvFALSE;
    }

    if (!InitTempBuffer(Acc2D, &mSrcBuffer, MaxWidth, MaxHeight))
    {
        GC2D_LEAVE();
        return gcvFALSE;
    }

    if (!mBrush.Init())
    {
        GC2D_LEAVE();
        return gcvFALSE;
    }

    mDstSurf.mAcc2D = Acc2D;
    mSrcSurf.mAcc2D = Acc2D;

    if (gcoHAL_IsFeatureAvailable(Acc2D->mHal, gcvFEATURE_2DPE20))
    {
        mHwVersion = 1;
    }

    mType = GC2D_OBJ_PARMS;

    GC2D_LEAVE();
    return gcvTRUE;
}

gctBOOL GC2DParms::Denitialize(GC2D_Accelerator_PTR Acc2D)
{
    Reset(Acc2D->mEngine2D);

    gcmVERIFY(DenitTempBuffer(Acc2D, &mSrcBuffer));
    gcmVERIFY(DenitTempBuffer(Acc2D, &mDstBuffer));

    if (mMonoBuffer)
    {
        delete [] mMonoBuffer;
        mMonoBuffer = gcvNULL;
    }

    gcmVERIFY(mBrush.Denit());

    mType = GC2D_OBJ_UNKNOWN;

    return gcvTRUE;
}

gctBOOL GC2DParms::InitTempBuffer(
    GC2D_Accelerator_PTR Acc2D,
    TEMP_BUFFER *Buffer,
    gctUINT Width,
    gctUINT Height)
{
    gcoSURF GALSurf = gcvNULL;

    do {
        if (gcmNO_SUCCESS(Acc2D->CreateGALSurface(
                Width, Height,
                gcvSURF_A8R8G8B8, gcvPOOL_SYSTEM,
                &GALSurf)))
        {
            break;
        }

        Buffer->mAuxBuffer = new GC2DSurf;
        if (!Buffer->mAuxBuffer)
        {
            break;
        }

        if (gcmNO_SUCCESS(Buffer->mAuxBuffer->InitializeGALSurface(
                Acc2D, GALSurf, gcvSURF_0_DEGREE)))
        {
            break;
        }

        // Clean buffer
        memset(Buffer->mAuxBuffer->mVirtualAddr, 0, Buffer->mAuxBuffer->mSize);

        gcmASSERT(!(Buffer->mAuxBuffer->mDMAAddr & DMA_PHYSICAL_BASE));
        gcmASSERT(Buffer->mAuxBuffer->mDMAAddr);
        if (gcmNO_SUCCESS(gcoOS_MapUserMemory(
            Acc2D->mOs,
            Buffer->mAuxBuffer->mVirtualAddr,
            Buffer->mAuxBuffer->mSize,
            &Buffer->mOrgTable,
            &Buffer->mMapAddr)))
        {
            break;
        }

        Buffer->mBackTableSize = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
            Buffer->mAuxBuffer->mVirtualAddr, Buffer->mAuxBuffer->mSize) * sizeof(DWORD);
        Buffer->mBackTable = new gctUINT8[Buffer->mBackTableSize];
        if (!Buffer->mBackTable)
        {
            break;
        }

        memcpy(Buffer->mBackTable, Buffer->mOrgTable, Buffer->mBackTableSize);

        return gcvTRUE;

    } while (gcvFALSE);

    if (Buffer->mMapAddr && Buffer->mOrgTable)
    {
        gcmVERIFY_OK(gcoOS_UnmapUserMemory(
            Acc2D->mOs, Buffer->mAuxBuffer->mVirtualAddr,
            Buffer->mAuxBuffer->mSize, Buffer->mOrgTable, Buffer->mMapAddr));

        Buffer->mOrgTable = gcvNULL;
        Buffer->mMapAddr = 0;
    }

    if (Buffer->mAuxBuffer)
    {
        delete Buffer->mAuxBuffer;
        Buffer->mAuxBuffer = gcvNULL;
    }

    if (GALSurf)
    {
        gcmVERIFY_OK(Acc2D->DestroyGALSurface(GALSurf));
    }

    return gcvFALSE;
}

gctBOOL GC2DParms::DenitTempBuffer(
    GC2D_Accelerator_PTR Acc2D,
    TEMP_BUFFER *Buffer)
{
    gcoSURF GALSurf = gcvNULL;

    if (Buffer->mBackTable && Buffer->mBackTableSize)
    {
        gcmASSERT(Buffer->mMapAddr && Buffer->mOrgTable);
        memcpy(Buffer->mOrgTable, Buffer->mBackTable, Buffer->mBackTableSize);

        delete [] Buffer->mBackTable;
        Buffer->mBackTable = gcvNULL;
        Buffer->mBackTableSize = 0;
    }

    if (Buffer->mMapAddr && Buffer->mOrgTable)
    {
        gcmVERIFY_OK(gcoOS_UnmapUserMemory(
            Acc2D->mOs, Buffer->mAuxBuffer->mVirtualAddr,
            Buffer->mAuxBuffer->mSize, Buffer->mOrgTable, Buffer->mMapAddr));

        Buffer->mOrgTable = gcvNULL;
        Buffer->mMapAddr = 0;
    }

    if (Buffer->mAuxBuffer)
    {
        GALSurf = Buffer->mAuxBuffer->mGalSurf;
        Buffer->mAuxBuffer->Denitialize(gcvTRUE);
        delete Buffer->mAuxBuffer;
        Buffer->mAuxBuffer = gcvNULL;
    }

    if (GALSurf)
    {
        gcmVERIFY_OK(Acc2D->DestroyGALSurface(GALSurf));
    }

    return gcvTRUE;
}

gctBOOL GC2DParms::SetBrush(
        GPEBltParms * pBltParms,
        gceSURF_ROTATION DestRotate)
{
    gceSTATUS status;

    gcmASSERT(pBltParms);
    gcmASSERT(mBrush.mType == GC2D_OBJ_UNKNOWN);

    GPESurf* pBrush = (GPESurf*)pBltParms->pBrush;
    if(pBrush)
    {
        gcmASSERT(pBrush->Format() != gpe1Bpp);
        gcmASSERT(pBrush->Format() != gpe24Bpp);
        gcmASSERT(pBrush->Width() == 8);
        gcmASSERT(pBrush->Height() == 8);
        gcmASSERT(!pBrush->IsRotate());

        gctINT stride = pBrush->Stride();
        gceSURF_FORMAT format = GPEFormatToGALFormat(pBrush->Format(), pBrush->FormatPtr());

        gcmASSERT(format != gcvSURF_UNKNOWN);
        gcmASSERT(format != gcvSURF_INDEX8);
        gcmASSERT(stride != 0);

        LONG x;
        LONG y;
        if (pBltParms->pptlBrush)
        {
            x = pBltParms->pptlBrush->x;
            y = pBltParms->pptlBrush->y;
        }
        else
        {
            x = y = 0;
        }

        if (x < 0)
        {
            x = 8 - ((-x)%8);
        }
        else
        {
            x %= 8;
        }

        if (y < 0)
        {
            y = 8 - ((-y)%8);
        }
        else
        {
            y %= 8;
        }

        gceSURF_ROTATION dRot = GPERotationToGALRotation(pBltParms->pDst->Rotate());

        if ((mHwVersion == 0) && (DestRotate == gcvSURF_90_DEGREE))
        {
            gcmVERIFY(GetRelativeRotation(DestRotate, &dRot));
        }

        switch (dRot)
        {
        case gcvSURF_0_DEGREE:
            mBrush.mX = x;
            mBrush.mY = y;
            break;

        case gcvSURF_90_DEGREE:
            mBrush.mX = (8 - y) % 8;
            mBrush.mY = x;
            break;

        case gcvSURF_180_DEGREE:
            mBrush.mX = (8 - x) % 8;
            mBrush.mY = (8 - y) % 8;
            break;

        case gcvSURF_270_DEGREE:
            mBrush.mX = y;
            mBrush.mY = (8 - x) % 8;
            break;

        default:
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        gctUINT8 *pBrushBits = (gctUINT8*)pBrush->Buffer();
        if (stride < 0)
        {
            gctUINT8 *dst = (gctUINT8*)mBrush.mVirtualAddr + 0x100;
            gctINT n;

            stride = -stride;
            for (n = 0; n < 8; n++)
            {
                memcpy(dst + n * stride, pBrushBits - n * stride, stride);
            }

            pBrushBits = dst;
        }

        if (!TransBrush(mBrush.mVirtualAddr, pBrushBits, GetSurfBytesPerPixel(pBrush), dRot))
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        mBrush.mFormat = format;
        mBrush.mSolidEnable = gcvFALSE;
        mBrush.mType = GC2D_OBJ_BRUSH;
    }
    else
    {
        // color should be mask with the dst color mask, but it was not needed in our hardware
        mBrush.mSolidColor = pBltParms->solidColor;
        mBrush.mSolidEnable = gcvTRUE;
        mBrush.mFormat = mDstSurf.mFormat;
        mBrush.mConvert = gcvFALSE;
        mBrush.mType = GC2D_OBJ_BRUSH;
    }

    return gcvTRUE;

OnError:

    return gcvFALSE;
}

gctBOOL GC2DParms::SetMono(
        GPEBltParms * pBltParms,
        gctBOOL bSrc,
        GPESurf *Surf,
        gceSURF_ROTATION Rot,
        gctBOOL RopAAF0)
{
    if ((Surf->Format() != gpe1Bpp)
        || (Rot != gcvSURF_0_DEGREE))
    {
        return gcvFALSE;
    }

    // Reset Mono before using.
    gcmASSERT(mMono.mType == GC2D_OBJ_UNKNOWN);

    gcsRECT *rect = bSrc ? &mSrcRect : &mMonoRect;

    gcmASSERT((rect->bottom - rect->top)
           == (mDstRect.bottom - mDstRect.top));
    gcmASSERT((rect->right - rect->left)
           == (mDstRect.right - mDstRect.left));

    mMono.mWidth = GetSurfWidth(Surf);
    mMono.mHeight = GetSurfHeight(Surf);
    if (mMono.mWidth == 0)
    {
        mMono.mWidth = rect->right;
    }

    if (mMono.mHeight == 0)
    {
        mMono.mHeight = rect->bottom;
    }

    mMono.mStream = Surf->Buffer();
    mMono.mStride = Surf->Stride();
    if (mMono.mStride < 0)
    {
        gctUINT y, off = 0;
        gctUINT size;
        gctUINT8 *dst;
        gctUINT8 *src = (gctUINT8*)mMono.mStream;

        mMono.mStride = -mMono.mStride;

        size = mMono.mStride * mMono.mHeight;

        if (mMonoBufferSize < size)
        {
            if (mMonoBuffer)
            {
                delete [] mMonoBuffer;
            }

            mMonoBuffer = new UINT8[size];
            mMonoBufferSize = size;
        }

        dst = (gctUINT8*)mMonoBuffer;

        for (y = 0; y < mMono.mHeight; y++)
        {
            gcoOS_MemCopy(
                dst + off,
                src - off,
                mMono.mStride);

            off += mMono.mStride;
        }

        mMono.mStream = mMonoBuffer;
    }

    gcmASSERT(mMono.mWidth <= ((gctUINT32)mMono.mStride << 3));
    mMono.mWidth = mMono.mStride << 3;

    if (bSrc)
    {
        if (RopAAF0)
        {
            mTrsp = gcvSURF_SOURCE_MATCH;
            mMono.mTsColor = 0;
            mMono.mBgColor = 0;
            mMono.mFgColor = pBltParms->solidColor;
        }
        else
        {
            mTrsp = gcvSURF_OPAQUE;
            if (pBltParms->pLookup)
            {
                mMono.mBgColor = (pBltParms->pLookup)[0];
                mMono.mFgColor = (pBltParms->pLookup)[1];
            }
            else
            {
                return gcvFALSE;
            }

            if (pBltParms->pConvert)
            {
                mMono.mBgColor = (pBltParms->pColorConverter->*(pBltParms->pConvert))(mMono.mBgColor);
                mMono.mFgColor = (pBltParms->pColorConverter->*(pBltParms->pConvert))(mMono.mFgColor);
            }
        }
    }
    else
    {
        mTrsp = gcvSURF_SOURCE_MASK;
    }

    mMono.mType = GC2D_OBJ_MONO;

    return gcvTRUE;
}


gctBOOL GC2DParms::SetSurface(
        GPEBltParms * pBltParms,
        gctBOOL bSrc,
        GPESurf *pGPESurf,
        gceSURF_ROTATION Rotation)
{
    GC2D_ENTER_ARG("pBltParms = 0x%08p, bSrc = %d, pGPESurf = 0x%08p", pBltParms, bSrc, pGPESurf);

    if (pGPESurf == NULL)
    {
        GC2D_LEAVE();
        return gcvFALSE;
    }

    GC2DSurf *surf = bSrc ? &mSrcSurf : &mDstSurf;
    gcsRECT  *rect = bSrc ? &mSrcRect : &mDstRect;

    // Reset surfaces before using.
    gcmASSERT(surf->mType == GC2D_OBJ_UNKNOWN);

    do {
        if (ContainGC2DSurf(pGPESurf))
        {
            GC2DSurf *tmp = &((GC2D_SurfWrapper*)(pGPESurf))->mGC2DSurf;
            gcmASSERT(tmp->IsValid());

            surf->mFormat = tmp->mFormat;
            surf->mRotate = tmp->mRotate;
            surf->mWidth = tmp->mWidth;
            surf->mHeight = tmp->mHeight;
            surf->mStride = tmp->mStride;
            surf->mSize = tmp->mSize;

            surf->mVirtualAddr = tmp->mVirtualAddr;
            surf->mPhysicalAddr = tmp->mPhysicalAddr;
            surf->mDMAAddr = tmp->mDMAAddr;

            surf->mType = GC2D_OBJ_SURF;
            break;
        }

        gceSURF_FORMAT format = GPEFormatToGALFormat(pGPESurf->Format(), pGPESurf->FormatPtr());
        if (format == gcvSURF_UNKNOWN)
        {
            break;
        }

        gctUINT8 *bits = (gctUINT8*)pGPESurf->Buffer();
        gctINT32 stride = pGPESurf->Stride();
        gctINT32 width = GetSurfWidth(pGPESurf);
        gctINT32 height = GetSurfHeight(pGPESurf);

        gcmASSERT(bits);
        gcmASSERT(stride);

        if (stride < 0)
        {
            gctINT tmp;

            bits += stride * (height - 1);
            stride =- stride;

            switch (Rotation)
            {
            case gcvSURF_0_DEGREE:
            case gcvSURF_180_DEGREE:

                mMirrorY = !mMirrorY;

                tmp = rect->bottom;
                rect->bottom = height - rect->top;
                rect->top = height - tmp;

                if (!bSrc)
                {
                    tmp = mClipRect.bottom;
                    mClipRect.bottom = height - mClipRect.top;
                    mClipRect.top = height - tmp;
                }
                break;

            case gcvSURF_90_DEGREE:
            case gcvSURF_270_DEGREE:

                mMirrorX = !mMirrorX;

                tmp = rect->right;
                rect->right = height - rect->left;
                rect->left = height - tmp;

                if (!bSrc)
                {
                    tmp = mClipRect.right;
                    mClipRect.right = height - mClipRect.left;
                    mClipRect.left = height - tmp;
                }
                break;

            default:
                gcmASSERT(0);
            }
        }

        surf->mFormat = format;
        surf->mRotate = Rotation;
        surf->mWidth = width;
        surf->mHeight = height;
        surf->mStride = stride;
        surf->mSize = stride * height;
        surf->mVirtualAddr = bits;

        if (pGPESurf->InVideoMemory())
        {
            gctUINT32 phys = pGPESurf->OffsetInVideoMemory() + mVideoPhyBase;

            if (!(phys & DMA_PHYSICAL_BASE))
            {
                surf->mFlag = GC2DSURF_PROPERTY_CONTIGUOUS;
                surf->mDMAAddr = surf->mPhysicalAddr = phys;
                surf->mType = GC2D_OBJ_SURF;
                break;
            }
        }

        TEMP_BUFFER *temp = bSrc ? &mSrcBuffer : &mDstBuffer;

        gcmASSERT(surf->mSize <= temp->mAuxBuffer->mSize);

        gctUINT32 pages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(bits, surf->mSize);
        gcmASSERT(pages * sizeof(DWORD) < temp->mBackTableSize);

        if (LockPages(bits, surf->mSize, (PDWORD)temp->mOrgTable, LOCKFLAG_QUERY_ONLY))
        {
            gctUINT32 n;
            DWORD *p = (DWORD*)temp->mOrgTable;

            for (n = 0; n < pages; n++)
            {
                p[n] <<= UserKInfo[KINX_PFN_SHIFT];
            }

            surf->mDMAAddr = (gctUINT)PAGE_ALIGN(temp->mMapAddr);
            surf->mDMAAddr += BYTE_OFFSET(bits);
            surf->mPhysicalAddr = ~0;

            CacheRangeFlush(bits, surf->mSize, CACHE_SYNC_ALL);

            surf->mType = GC2D_OBJ_SURF;
            break;
        }
    } while (gcvFALSE);

    if (surf->mType == GC2D_OBJ_SURF)
    {
        if (bSrc && (surf->mFormat == gcvSURF_INDEX8))
        {
            GPEFormat *gpeFormatPtr = pGPESurf->FormatPtr();

            if (!gpeFormatPtr && (gpeFormatPtr->m_PaletteEntries == 256) && gpeFormatPtr->m_pPalette)
            {
                memcpy(mPalette, gpeFormatPtr->m_pPalette, 1024);

                mPaletteConvert = gcvTRUE;
            }
            else if (pBltParms->pLookup)
            {
                gctINT i;

                for (i = 0; i < 256; i++)
                {
                    if (pBltParms->pConvert)
                        mPalette[i] = (pBltParms->pColorConverter->*(pBltParms->pConvert))(pBltParms->pLookup[i]);
                    else
                        mPalette[i] = pBltParms->pLookup[i];
                }

                mPaletteConvert = gcvFALSE;
            }
            else
            {
                gcmASSERT(0);
                GC2D_LEAVE();
                return gcvFALSE;
            }
        }

        GC2D_LEAVE();
        return gcvTRUE;
    }
    else
    {
        GC2D_LEAVE();
        return gcvFALSE;
    }
}

gctBOOL GC2DParms::ResetSurface(gctBOOL bSrc)
{
    GC2DSurf *surf = bSrc ? &mSrcSurf : &mDstSurf;
    gcsRECT  *rect = bSrc ? &mSrcRect : &mDstRect;

    surf->mFormat = gcvSURF_UNKNOWN;
    surf->mRotate = gcvSURF_0_DEGREE;
    surf->mWidth = 0;
    surf->mHeight = 0;
    surf->mStride = 0;
    surf->mSize = 0;
    surf->mVirtualAddr = gcvNULL;
    surf->mPhysicalAddr = ~0;
    surf->mDMAAddr = ~0;
    surf->mType = GC2D_OBJ_UNKNOWN;

    return gcvTRUE;
}

gctBOOL GC2DParms::Reset(gco2D Engine2D)
{
    if (mBrush.mType != GC2D_OBJ_UNKNOWN)
    {
        mBrush.mType = GC2D_OBJ_UNKNOWN;
        mBrush.mConvert = gcvFALSE;
    }

    if (mDstSurf.mType != GC2D_OBJ_UNKNOWN)
    {
        ResetSurface(gcvFALSE);
    }

    if (mSrcSurf.mType != GC2D_OBJ_UNKNOWN)
    {
        ResetSurface(gcvTRUE);
    }

    mMirrorX = gcvFALSE;
    mMirrorY = gcvFALSE;

    mSrcRect.left = 0;
    mSrcRect.top = 0;
    mSrcRect.right = 0;
    mSrcRect.bottom = 0;

    mDstRect.left = 0;
    mDstRect.top = 0;
    mDstRect.right = 0;
    mDstRect.bottom = 0;

    mMonoRect.left = 0;
    mMonoRect.top = 0;
    mMonoRect.right = 0;
    mMonoRect.bottom = 0;

    mFgRop = 0;
    mBgRop = 0;

    mBlit = BLIT_INVALID;

    mMono.mType = GC2D_OBJ_UNKNOWN;

    mTrsp = gcvSURF_OPAQUE;

    if (mABEnable)
    {
        gco2D_DisableAlphaBlend(Engine2D);
        gco2D_SetPixelMultiplyModeAdvanced(
            Engine2D,
            gcv2D_COLOR_MULTIPLY_DISABLE,
            gcv2D_COLOR_MULTIPLY_DISABLE,
            gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE,
            gcv2D_COLOR_MULTIPLY_DISABLE);

        mABEnable = gcvFALSE;
    }

    return gcvTRUE;
}

gctBOOL GC2DParms::Blit(gco2D Engine2D)
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT16 rop4 = mFgRop | (mBgRop << 8);
    gctBOOL setSrc = ROP4_USE_SOURCE(rop4);
    gctBOOL setBrh = ROP4_USE_PATTERN(rop4);

    if (!mDstSurf.IsValid())
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if (mFgRop == mBgRop)
    {
        mTrsp = gcvSURF_OPAQUE;
    }

    switch (mTrsp)
    {
    case gcvSURF_OPAQUE:
        break;

    case gcvSURF_SOURCE_MATCH:
    case gcvSURF_SOURCE_MASK:
        if (mMono.mStride & 3)
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        setSrc = gcvTRUE;
        break;

    case gcvSURF_PATTERN_MASK:
        setBrh = gcvTRUE;
        break;

    default:
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    switch (mBlit)
    {
    case BLIT_NORMAL:
        break;

    case BLIT_MASKED_SOURCE:
    case BLIT_MONO_SOURCE:
    case BLIT_STRETCH:
        setSrc = gcvTRUE;
        break;

    default:
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if (mABEnable)
    {
        gcmONERROR(gco2D_EnableAlphaBlend(
            Engine2D,
            mABSrcAg,
            mABDstAg,
            mABSrcAMode,
            mABDstAMode,
            mABSrcAgMode,
            mABDstAgMode,
            mABSrcFactorMode,
            mABDstFactorMode,
            mABSrcColorMode,
            mABDstColorMode
            ));
    }

    if (setSrc)
    {
        if ((mBlit == BLIT_NORMAL)
            || (mBlit == BLIT_STRETCH))
        {
            if (!mSrcSurf.IsValid())
            {
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            /* Configure color source. */
            gcmONERROR(gco2D_SetColorSourceEx(
                Engine2D,
                mSrcSurf.mDMAAddr,
                mSrcSurf.mStride,
                mSrcSurf.mFormat,
                mSrcSurf.mRotate,
                mSrcSurf.mWidth,
                mSrcSurf.mHeight,
                gcvFALSE,
                mTrsp,
                mSrcTrspColor
                ));

            /* Set source rectangle size. */
            gcmONERROR(gco2D_SetSource(
                Engine2D,
                &mSrcRect
                ));

            if (mSrcSurf.mFormat == gcvSURF_INDEX8)
            {
                gcmONERROR(gco2D_LoadPalette(
                    Engine2D, 0, 256, mPalette, mPaletteConvert));
            }

            if (mBlit == BLIT_STRETCH)
            {
                /* Calculate the stretch factors. */
                gctUINT32 horFactor =
                    GetStretchFactor(mSrcRect.right - mSrcRect.left, mDstRect.right - mDstRect.left);
                gctUINT32 verFactor =
                    GetStretchFactor(mSrcRect.bottom - mSrcRect.top, mDstRect.bottom - mDstRect.top);

                if (!horFactor || !verFactor)
                {
                    gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
                }

                /* Program the stretch factors. */
                gcmONERROR((gco2D_SetStretchFactors(
                    Engine2D,
                    horFactor,
                    verFactor
                    )));
            }
        }
        else if ((mBlit == BLIT_MASKED_SOURCE)
            || (mBlit == BLIT_MONO_SOURCE))
        {
            if (!mMono.IsValid()
                || ((mMonoRect.right - mMonoRect.left) != (mDstRect.right - mDstRect.left))
                || ((mMonoRect.bottom - mMonoRect.top) != (mDstRect.bottom - mDstRect.top)))
            {
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            gctINT32 tx = mMonoRect.left & 0x1F;
            gctINT32 ty = 0;

            gctINT32 dx = mMonoRect.left - tx;
            gctINT32 dy = mMonoRect.top  - ty;

            mMono.mStream = (gctUINT8*)mMono.mStream + dy * mMono.mStride + (dx >> 3);
            mMonoRect.left = tx;
            mMonoRect.right -= dx;
            mMonoRect.top  = ty;
            mMonoRect.bottom -= dy;

            if (mBlit == BLIT_MASKED_SOURCE)
            {
                if (((mSrcRect.right - mSrcRect.left) != (mDstRect.right - mDstRect.left))
                    || ((mSrcRect.bottom - mSrcRect.top) != (mDstRect.bottom - mDstRect.top)))
                {
                    gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
                }

                dx = mSrcRect.left - tx;
                dy = mSrcRect.top  - ty;

                gctUINT off = dy * mSrcSurf.mStride + (dx * GetFormatSize(mSrcSurf.mFormat));

                mSrcSurf.mVirtualAddr = (gctUINT8*)mSrcSurf.mVirtualAddr + off;
                mSrcSurf.mPhysicalAddr += off;
                mSrcSurf.mDMAAddr += off;
                mSrcRect.left = tx;
                mSrcRect.right -= dx;
                mSrcRect.top  = ty;
                mSrcRect.bottom -= dy;
                gcmASSERT(!(mSrcSurf.mDMAAddr & 0xF));

                if (mSrcSurf.mFormat == gcvSURF_INDEX8)
                {
                    gcmONERROR(gco2D_LoadPalette(
                        Engine2D, 0, 256, mPalette, mPaletteConvert));
                }
            }
        }
        else
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
    }
    else
    {
        /* Set dummy source. */
        gcmONERROR(gco2D_SetColorSourceEx(
            Engine2D,
            mSrcBuffer.mAuxBuffer->mDMAAddr,
            mSrcBuffer.mAuxBuffer->mStride,
            mSrcBuffer.mAuxBuffer->mFormat,
            mSrcBuffer.mAuxBuffer->mRotate,
            mSrcBuffer.mAuxBuffer->mWidth,
            mSrcBuffer.mAuxBuffer->mHeight,
            gcvFALSE,
            mTrsp,
            mSrcTrspColor
            ));

        /* Set dummy source rectangle. */
        mSrcRect.left = mSrcRect.top
            = mSrcRect.right = mSrcRect.bottom = 0;
        gcmONERROR(gco2D_SetSource(
            Engine2D,
            &mSrcRect
            ));
    }
        /* Set clipping rectangle. */
    gcmONERROR(gco2D_SetClipping(Engine2D, &mClipRect));

    /* Set destination. */
    gcmONERROR(gco2D_SetTargetEx(
        Engine2D,
        mDstSurf.mDMAAddr,
        mDstSurf.mStride,
        mDstSurf.mRotate,
        mDstSurf.mWidth,
        mDstSurf.mHeight
        ));

    /* Set mirrors. */
    gcmONERROR(gco2D_SetBitBlitMirror(
        Engine2D,
        mMirrorX,
        mMirrorY
        ));

    /* Setup the brush if necessary. */
    if (setBrh)
    {
        /* Flush the brush. */
        if (mBrush.IsValid())
        {
            if (mBrush.mSolidEnable)
            {
                gcmONERROR(gco2D_LoadSolidBrush(
                    Engine2D,
                    mBrush.mFormat,
                    mBrush.mConvert,
                    mBrush.mSolidColor,
                    0xFFFFFFFFFFFFFFFF));
            }
            else
            {
                gcmONERROR(gco2D_LoadColorBrush(
                    Engine2D,
                    mBrush.mX,
                    mBrush.mY,
                    mBrush.mDMAAddr,
                    mBrush.mFormat,
                    0xFFFFFFFFFFFFFFFF));
            }
        }
        else
        {
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
    }

    if (mBlit == BLIT_NORMAL)
    {
        gcmONERROR(gco2D_Blit(
            Engine2D,
            1,
            &mDstRect,
            mFgRop,
            mBgRop,
            mDstSurf.mFormat
            ));
    }
    else if (mBlit == BLIT_STRETCH)
    {
        gcmONERROR(gco2D_StretchBlit(
            Engine2D,
            1,
            &mDstRect,
            mFgRop,
            mBgRop,
            mDstSurf.mFormat
            ));
    }
    else if ((mBlit == BLIT_MASKED_SOURCE) || (mBlit == BLIT_MONO_SOURCE))
    {
        gcsRECT destSubRect = mDstRect;
        gcsRECT monoRect = mMonoRect;

        gctINT32 maxHeight = (gco2D_GetMaximumDataCount() << 5) / gcmALIGN(monoRect.right, 32);
        gcsPOINT monoSize = {mMono.mWidth, mMono.mHeight};
        gctUINT8 *monoBase = (gctUINT8 *)mMono.mStream;
        gctUINT32 srcBase;

        /* Set source rectangle size. */
        gcmONERROR(gco2D_SetSource(
                    Engine2D,
                    &monoRect
                    ));

        if (mBlit == BLIT_MASKED_SOURCE)
        {
            srcBase = mSrcSurf.mDMAAddr;
        }
        else
        {
            /* Program the source. */
            gcmONERROR(gco2D_SetMonochromeSource(
                Engine2D,
                gcvFALSE,
                mMono.mTsColor,
                gcvSURF_UNPACKED,
                gcvFALSE,
                mTrsp,
                mMono.mFgColor,
                mMono.mBgColor
                ));
        }

        gcmASSERT(monoRect.top == 0);

        monoRect.left &= ~0x1F;
        monoRect.right = gcmALIGN(monoRect.right, 32);

        gctUINT32 line2render = monoRect.bottom;
        do {
            gcsRECT_PTR destMonoRect;
            gcsRECT     mirrorRect;

            if (monoRect.bottom > maxHeight)
            {
                monoRect.bottom = maxHeight;
            }

            destSubRect.bottom = destSubRect.top + monoRect.bottom;

            if (mBlit == BLIT_MASKED_SOURCE)
            {
                /* Configure masked source. */
                gcmONERROR(gco2D_SetMaskedSourceEx(
                    Engine2D,
                    srcBase,
                    mSrcSurf.mStride,
                    mSrcSurf.mFormat,
                    gcvFALSE,
                    gcvSURF_UNPACKED,
                    mSrcSurf.mRotate,
                    mSrcSurf.mWidth,
                    mSrcSurf.mHeight
                    ));
            }

            if (mMirrorY)
            {
                mirrorRect.left = destSubRect.left;
                mirrorRect.right = destSubRect.right;
                mirrorRect.bottom = mDstRect.bottom - destSubRect.top + mDstRect.top;
                mirrorRect.top = mDstRect.bottom - destSubRect.bottom + mDstRect.top;
                destMonoRect = &mirrorRect;
            }
            else
            {
                destMonoRect = &destSubRect;
            }

            /* Do the blit. */
            gcmONERROR(gco2D_MonoBlit(
                Engine2D,
                monoBase,
                &monoSize,
                &monoRect,
                gcvSURF_UNPACKED,
                gcvSURF_UNPACKED,
                destMonoRect,
                mFgRop, mBgRop,
                mDstSurf.mFormat
                ));

            line2render -= monoRect.bottom;

            if (line2render)
            {
                monoBase += monoRect.bottom * mMono.mStride;

                if (mBlit == BLIT_MASKED_SOURCE)
                {
                    srcBase += monoRect.bottom * mSrcSurf.mStride;
                }

                destSubRect.top = destSubRect.bottom;
                monoRect.bottom = line2render;
            }
        } while (line2render);
    }
    else
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    return gcvTRUE;

OnError:

    return gcvFALSE;
}

BOOL GC2DParms::ContainGC2DSurf(GPESurf *pGPESurf) const
{
    BOOL ret;

    GC2D_ENTER_ARG("pGPESurf = 0x%08p", pGPESurf);

    ret = ((GC2D_SurfWrapper*)(pGPESurf))->IsWrapped();

    GC2D_LEAVE_ARG("%d", ret);

    return ret;
}

BOOL GC2DParms::IsGPESurfSupported(GPESurf *pGPESurf, gctBOOL bSrc)
{
    GC2D_ENTER_ARG("pGPESurf = 0x%08p, bSrc = %d", pGPESurf, bSrc);

    do {
        if (!pGPESurf)
        {
            // Wrong parameter
            break;
        }

        // 1. rotation check
        int rotate = pGPESurf->Rotate();
        if ((rotate != DMDO_0) && (rotate != DMDO_90)
            && (rotate != DMDO_180) && (rotate != DMDO_270))
        {
            break;
        }

        // 2. width & height
        gctINT width  = GetSurfWidth(pGPESurf);
        gctINT height = GetSurfHeight(pGPESurf);
        if (width <= 0 || height <= 0)
        {
            break;
        }

        // too small?
        gctUINT32 pixel = width * height;
        if (pixel < mMinPixelNum)
        {
            break;
        }

        // too large?
        if (pixel > mMaxPixelNum)
        {
            break;
        }

        // 3. GC2DSurf wrapped ?
        if (ContainGC2DSurf(pGPESurf))
        {
            break;
        }

        // 4. format check
        EGPEFormat format = pGPESurf->Format();
        GPEFormat *gpeFormatPtr = pGPESurf->FormatPtr();
        if ((format == gpe16Bpp) || (format == gpe32Bpp))
        {
            if (GPEFormatToGALFormat(format, gpeFormatPtr) == gcvSURF_UNKNOWN)
            {
                break;
            }
        }
        else if (!bSrc || (format != gpe8Bpp))
        {
            break;
        }

        gcmASSERT(format == gpe8Bpp || format == gpe16Bpp || format == gpe32Bpp);

        // 5. Aligned?
        gctUINT8 *bits = (gctUINT8*)pGPESurf->Buffer();
        gctINT32 stride = pGPESurf->Stride();

        if (stride == 0)
        {
            break;
        }
        else if (stride < 0)
        {
            bits += stride * (height - 1);
            stride =- stride;
        }

        gctUINT32 aMask = 0xF;
        if (stride & aMask)
        {
            break;
        }

        if (((gctUINT32)bits) & aMask)
        {
            break;
        }

        GC2D_LEAVE_ARG("%d", gcvTRUE);
        return gcvTRUE;
    } while (gcvFALSE);

    GC2D_LEAVE_ARG("%d", gcvFALSE);
    return gcvFALSE;
}

gctBOOL GC2DParms::SetAlphaBlend(
        GPEBltParms * pBltParms)
{
        mABSrcAg = pBltParms->blendFunction.SourceConstantAlpha;
        mABDstAg = 0;
        mABSrcAMode = gcvSURF_PIXEL_ALPHA_STRAIGHT;
        mABDstAMode = gcvSURF_PIXEL_ALPHA_STRAIGHT;

        if (pBltParms->bltFlags & BLT_ALPHABLEND)
        {
            // Enable alphablend
            mABEnable = gcvTRUE;
        }
        else
        {
            // Disable alphablend
            mABEnable = gcvFALSE;

            return gcvTRUE;
        }

        // Negate source alpha if ALPHASRCNEG was requested
        if (pBltParms->blendFunction.BlendFlags & BLT_ALPHASRCNEG)
        {
            mABSrcAMode = gcvSURF_PIXEL_ALPHA_INVERSED;
            mABSrcAg = 0xFF - mABSrcAg;
        }
        // Negate destination alpha if ALPHADESTNEG was requested
        if (pBltParms->blendFunction.BlendFlags & BLT_ALPHADESTNEG)
        {
            mABDstAMode = gcvSURF_PIXEL_ALPHA_INVERSED;
        }

        if (pBltParms->blendFunction.AlphaFormat)
        {
            mABSrcAgMode = gcvSURF_GLOBAL_ALPHA_SCALE;
            mABDstAgMode = gcvSURF_GLOBAL_ALPHA_OFF;
#ifdef AC_SRC_ALPHA_NONPREMULT
            if (pBltParms->iMode == 1)
            {
                mABSrcColorMode = gcvSURF_COLOR_STRAIGHT;
            }
            else
            {
                mABSrcColorMode =
                    (pBltParms->blendFunction.AlphaFormat == AC_SRC_ALPHA_NONPREMULT) ?
                    gcvSURF_COLOR_STRAIGHT : gcvSURF_COLOR_MULTIPLY;
            }
#else
            mABSrcColorMode = gcvSURF_COLOR_MULTIPLY;
#endif
            mABDstColorMode = gcvSURF_COLOR_STRAIGHT;
            mABSrcFactorMode = gcvSURF_BLEND_ONE;
            mABDstFactorMode = gcvSURF_BLEND_INVERSED;
        }
        else
        {
            mABSrcAgMode = gcvSURF_GLOBAL_ALPHA_ON;
            mABDstAgMode = gcvSURF_GLOBAL_ALPHA_OFF;
            mABSrcColorMode = gcvSURF_COLOR_MULTIPLY;
            mABDstColorMode = gcvSURF_COLOR_STRAIGHT;
            mABSrcFactorMode = gcvSURF_BLEND_ONE;
            mABDstFactorMode = gcvSURF_BLEND_INVERSED;
        }

        return gcvTRUE;
}
