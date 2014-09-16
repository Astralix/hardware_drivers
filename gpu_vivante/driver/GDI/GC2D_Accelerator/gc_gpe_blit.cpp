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

#define GPEBLTFUN (SCODE (GPE::*) (struct GPEBltParms*))

///////////////////////////////////////////////////////////////////////
// check the pBltParms
///////////////////////////////////////////////////////////////////////
SCODE GC2D_Accelerator::BltParmsFilter(GPEBltParms *pBltParms)
{
    GC2D_ENTER_ARG("pBltParms = 0x%08p", pBltParms);

    do {
        ///////////////////////////////////////////////////////////////////////
        // check dst parameters
        ///////////////////////////////////////////////////////////////////////
        GPESurf* pGPEDst = (GPESurf*) pBltParms->pDst;
        GPESurf* pGPESrc = (GPESurf*) pBltParms->pSrc;
        GPESurf* pGPEMask = (GPESurf*) pBltParms->pMask;
        GPESurf* pGPEBrush = (GPESurf*) pBltParms->pBrush;

#if GC2D_GATHER_STATISTICS
        if (pGPEDst)
        {
            gssDst.RecordSurf(pGPEDst);

            GC2D_DebugTrace(GC2D_DISPLAY_BLIT, ("GC2DBltParmsFilter: " \
                "Dest Width=%d Height=%d Format=%d Stride=%d GPESurf=%08X\r\n"),
                    pGPEDst->Width(), pGPEDst->Height(),
                    pGPEDst->Format(), pGPEDst->Stride(),
                    pGPEDst);
        }

        if (pGPESrc)
        {
            gssSrc.RecordSurf(pGPESrc);

            GC2D_DebugTrace(GC2D_DISPLAY_BLIT, ("GC2DBltParmsFilter: "\
                "Src Width=%d Height=%d Format=%d Stride=%d GPESurf=%08X\r\n"),
                    pGPESrc->Width(), pGPESrc->Height(),
                    pGPESrc->Format(), pGPESrc->Stride(),
                    pGPESrc);
        }

        if (pGPEMask)
        {
            gssMask.RecordSurf(pGPEMask);

            GC2D_DebugTrace(GC2D_DISPLAY_BLIT, ("GC2DBltParmsFilter: "\
                "Mask Width=%d Height=%d Format=%d Stride=%d GPESurf=%08X\r\n"),
                    pGPEMask->Width(), pGPEMask->Height(),
                    pGPEMask->Format(), pGPEMask->Stride(),
                    pGPEMask);
        }

        if (pGPEBrush)
        {
            gssBrush.RecordSurf(pGPEBrush);

            GC2D_DebugTrace(GC2D_DISPLAY_BLIT, ("GC2DBltParmsFilter: "\
                "Brush Width=%d Height=%d Format=%d Stride=%d GPESurf=%08X\r\n"),
                    pGPEBrush->Width(), pGPEBrush->Height(),
                    pGPEBrush->Format(), pGPEBrush->Stride(),
                    pGPEBrush);
        }
#endif
        // check whether dst surface is right and can be wrapped
        if (!pGPEDst)
        {
            break;
        }
        else if (!mParms->IsGPESurfSupported(pGPEDst, gcvFALSE))
        {
            break;
        }

        gceSURF_ROTATION dRot = GPERotationToGALRotation(pGPEDst->Rotate());
        if (dRot == -1)
        {
            break;
        }

        ///////////////////////////////////////////////////////////////////////
        // check src parameters
        ///////////////////////////////////////////////////////////////////////
        gctUINT32 Rop4 = pBltParms->rop4;
        gctUINT8 fRop = (gctUINT8)(Rop4 & 0x00FF);
        gctUINT8 bRop = (gctUINT8)((Rop4 >> 8) & 0x00FF);
        gctBOOL UseSource = ROP4_USE_SOURCE(Rop4);
        gceSURF_ROTATION sRot;

        // alphablend related check
        if (pBltParms->bltFlags & BLT_ALPHABLEND)
        {
            // Src will be refered anyway
            if(!pGPESrc)
            {
                break;
            }

            UseSource = gcvTRUE;

            if (!mParms->mHwVersion)
            {
                // PE1.0 format limitation
                if (Rop4 != 0xCCCC)
                {
                    break;
                }

                if (pBltParms->blendFunction.AlphaFormat &&
                    (gcvSURF_R5G6B5 == GPEFormatToGALFormat(pGPEDst->Format(), pGPEDst->FormatPtr())))
                {
                    break;
                }
            }
        }

        // check whether src surface is right and can be wrapped
        if (UseSource)
        {
            if (!pGPESrc || (pGPESrc->Stride() == 0))
            {
                break;
            }

            sRot = GPERotationToGALRotation(pGPESrc->Rotate());
            if (sRot == -1)
            {
                break;
            }

            if (pGPESrc->Format() == gpe1Bpp)
            {
                // not support mono stretch
                if (pBltParms->bltFlags & BLT_STRETCH)
                {
                    break;
                }

                if (!mParms->mHwVersion)
                {
                    gceSURF_ROTATION rot = dRot;
                    if (!GetRelativeRotation(sRot, &rot))
                    {
                        break;
                    }

                    if ((rot != gcvSURF_0_DEGREE)
                        || (rot != gcvSURF_90_DEGREE))
                    {
                        // PE 1.0 not support
                        break;
                    }

                    if (pGPEDst->Stride() <= 0)
                    {
                        // PE 1.0 not support
                        break;
                    }
                }
            }
            else if (!mParms->IsGPESurfSupported(pGPESrc, gcvTRUE))
            {
                break;
            }
        }

        ///////////////////////////////////////////////////////////////////////
        // check mask parameters
        ///////////////////////////////////////////////////////////////////////
        gctBOOL UseMask = ROP4_USE_MASK(Rop4);

        // check the mask surfce
        if (UseMask)
        {
            if (!(mEnableFlag & GC2D_FEATURE_ENABLE_MASK))
            {
                break;
            }

            if (!pGPEMask)
            {
                break;
            }

            gceSURF_ROTATION mRot = GPERotationToGALRotation(pGPEMask->Rotate());
            if (mRot == -1)
            {
                break;
            }

            if ((pGPEMask->Format() != gpe1Bpp)
                || (pBltParms->bltFlags & BLT_STRETCH))
            {
                break;
            }

            int stride = abs(pGPEMask->Stride());
            if ((stride == 0) || (stride & 3))
            {
                break;
            }

            if (UseSource)
            {
                if (pGPESrc->Format() == gpe1Bpp)
                {
                    break;
                }

                if ((pBltParms->xPositive < 1) || (pBltParms->yPositive < 1))
                {
                    break;
                }

                if (!mParms->mHwVersion && (pGPESrc->Stride() <= 0))
                {
                    // PE1.0 not support
                    break;
                }

                if (mRot != sRot)
                {
                    break;
                }
            }

            if (!mParms->mHwVersion)
            {
                gceSURF_ROTATION rot = dRot;
                if (!GetRelativeRotation(mRot, &rot))
                {
                    break;
                }

                if ((rot != gcvSURF_0_DEGREE)
                    && (rot != gcvSURF_90_DEGREE))
                {
                    // PE 1.0 not support
                    break;
                }

                if (pGPEDst->Stride() <= 0)
                {
                    // PE 1.0 not support
                    break;
                }
            }
        }

        ///////////////////////////////////////////////////////////////////////
        // check brush parameters
        ///////////////////////////////////////////////////////////////////////
        gctBOOL UsePattern = ROP4_USE_PATTERN(Rop4);
        if (UsePattern)
        {
            GPESurf *pBrush = pBltParms->pBrush;

            if(pBrush)
            {
                if ((pBrush->Format() == gpe1Bpp)
                    || (pBrush->Format() == gpe24Bpp)
                    || (pBrush->IsRotate()))
                {
                    break;
                }

                gctINT stride = abs(pBrush->Stride());
                gceSURF_FORMAT format = GPEFormatToGALFormat(pBrush->Format(), pBrush->FormatPtr());

                if ((format == gcvSURF_UNKNOWN)
                    || (format == gcvSURF_INDEX8)
                    || (stride == 0))
                {
                    break;
                }

                if ((pBrush->Width() != 8) || (pBrush->Height() != 8)
                    || (stride != (GetSurfBytesPerPixel(pBrush) * 8)))
                {
                    break;
                }
            }
        }

        GC2D_LEAVE_ARG("%d", S_OK);
        return(S_OK);

    } while (gcvFALSE);

    GC2D_LEAVE_ARG("%d", -ERROR_NOT_SUPPORTED);

    return -ERROR_NOT_SUPPORTED;
}

SCODE GC2D_Accelerator::GC2DBltPrepare(GPEBltParms *pBltParms)
{
    SCODE sc = -ERROR_NOT_SUPPORTED;
    GC2D_ENTER_ARG("pBltParms = 0x%08p", pBltParms);

    do {
        ///////////////////////////////////////////////////////////////////////
        // check GC2D object
        ///////////////////////////////////////////////////////////////////////
        if (!GC2DIsValid())
        {
            break;
        }

        // if disable the prepare
        if (!(mEnableFlag & GC2D_FEATURE_ENABLE_BLTPREPARE))
        {
            break;
        }

#if GC2D_GATHER_STATISTICS

        if (!(nPrepare % 400))
        {
            RETAILMSG(1, (TEXT("GC2D Stat: Prepare:(%d/%d), AnyBlt:(-%d/%d), ROP:(%d/%d), Alpha:(%d/%d), Sync:(-%d/%d)\n"),
                nPrepareSucceed,nPrepare, nAnyBlitFailed, nAnyBlit,
                nROPBlitSucceed, nROPBlit, nAlphaBlendSucceed, nAlphaBlend,
                nASyncCommitFailed, nASyncCommit));
        }

        ++nPrepare;
#endif

        ///////////////////////////////////////////////////////////////////////
        // check bltFlags
        ///////////////////////////////////////////////////////////////////////
        if (pBltParms->bltFlags & (~(BLT_WAITNOTBUSY | BLT_ALPHABLEND | BLT_STRETCH | BLT_TRANSPARENT)))
        {
            break;
        }

        if ((pBltParms->bltFlags & BLT_STRETCH)
            && (!(mEnableFlag & GC2D_FEATURE_ENABLE_STRETCH)
            || (pBltParms->iMode == BILINEAR)
            #if UNDER_CE >= 600
            || (pBltParms->iMode == HALFTONE)
            #endif
            || (pBltParms->iMode == BLACKONWHITE)
            || (pBltParms->iMode == COLORONCOLOR)))
        {
            break;
        }

        if ((pBltParms->bltFlags & BLT_TRANSPARENT)
            && (!(mEnableFlag & GC2D_FEATURE_ENABLE_TRANSPARENT)
            || (pBltParms->rop4 != 0xCCCC)))
        {
            break;
        }

        if ((pBltParms->bltFlags & BLT_ALPHABLEND)
            && (!(mEnableFlag & GC2D_FEATURE_ENABLE_ALPHABLEND)))
        {
            break;
        }

        sc = BltParmsFilter(pBltParms);

    } while (gcvFALSE);

    if (sc == S_OK)
    {
#if GC2D_GATHER_STATISTICS
        nPrepareSucceed++;
#endif
        GC2D_DebugTrace(GC2D_DISPLAY_BLIT, ("GC2DBltPrepare: AnyBlt is selected\n"));
        pBltParms->pBlt = GPEBLTFUN &GC2D_Accelerator::AnyBlt;

        GC2D_LEAVE_ARG("%d", S_OK);
        return S_OK;
    }
    else
    {
        GC2D_LEAVE_ARG("%d", sc);
        return sc;
    }
}

SCODE GC2D_Accelerator::GC2DBltComplete(GPEBltParms *blitParameters)
{
    GC2D_ENTER_ARG("blitParameters = 0x%08p", blitParameters);

    if (mSyncMode == GC2D_SYNC_MODE_ASYNC)
    {
        if ((blitParameters->pBlt == &GC2D_Accelerator::AnyBlt) && GC2DIsValid())
        {
            GC2DWaitForNotBusy();
        }
    }

    GC2D_LEAVE_ARG("%d", S_OK);
    return S_OK;
}

SCODE GC2D_Accelerator::AnyBlt(GPEBltParms * pBltParms)
{
    SCODE sc = -ERROR_NOT_SUPPORTED;

    GC2D_ENTER_ARG("pBltParms = 0x%08p", pBltParms);

#if GC2D_GATHER_STATISTICS
    ++nAnyBlit;
#endif

    sc = Blit(pBltParms);
    if (sc != S_OK)
    {
#if GC2D_GATHER_STATISTICS
        ++nAnyBlitFailed;
#endif
        GC2D_DebugTrace(GC2D_DISPLAY_BLIT, ("GC2DAnyBlt: call EmulatedBlt\r\n"));
        pBltParms->pBlt = &GPE::EmulatedBlt;
        sc = EmulatedBlt(pBltParms);
    }

    GC2D_LEAVE_ARG("%d", sc);

    return sc;
}

SCODE GC2D_Accelerator::Blit(GPEBltParms * pBltParms)
{
    gctBOOL bSucceed = gcvFALSE;
    gctBOOL bForce = gcvTRUE;

    GC2D_ENTER_ARG("pBltParms = 0x%08p", pBltParms);

    GC2DWaitForNotBusy();
    mParms->Reset(mEngine2D);

    do {
        GPESurf* pGPESrc = (GPESurf*) pBltParms->pSrc;
        GPESurf* pGPEDst = (GPESurf*) pBltParms->pDst;
        GPESurf* pGPEMask = (GPESurf*) pBltParms->pMask;
        // NOTE: the struct RECTL should be the same with gcsRECT
        gcsRECT_PTR pDstRect = (gcsRECT_PTR)pBltParms->prclDst;
        gcsRECT_PTR pSrcRect = (gcsRECT_PTR)pBltParms->prclSrc;
        gcsRECT_PTR pMskRect = (gcsRECT_PTR)pBltParms->prclMask;
        gcsRECT_PTR pClpRect = (gcsRECT_PTR)pBltParms->prclClip;

        gceSURF_ROTATION dRot, sRot, mRot;
        dRot = sRot = mRot = (gceSURF_ROTATION) -1;

        gctUINT32 Rop4 = pBltParms->rop4;
        mParms->mFgRop = (gctUINT8)(Rop4 & 0xFF);
        mParms->mBgRop = (gctUINT8)((Rop4 >> 8) & 0xFF);

        gctBOOL UseSource  = ROP4_USE_SOURCE(Rop4);
        gctBOOL UsePattern = ROP4_USE_PATTERN(Rop4);
        gctBOOL UseMask    = ROP4_USE_MASK(Rop4);

        gctBOOL OptMask = (!pBltParms->pBrush)
            && ((Rop4 == 0xAAF0) || (Rop4 == 0x55F0)
             || (Rop4 == 0x00F0) || (Rop4 == 0xFFF0));
        if (OptMask)
        {
            Rop4 = 0xAACC;
            mParms->mFgRop = 0xCC;
            UsePattern = UseMask = gcvFALSE;
            UseSource = gcvTRUE;
            pGPESrc = pGPEMask;
            pGPEMask = gcvNULL;
            pSrcRect = pMskRect;
            pMskRect = gcvNULL;
            mParms->mBlit = BLIT_MONO_SOURCE;
        }

        gcmASSERT(pGPEDst);

        // 1. Set Rects & blit type
        if (pDstRect)
        {
            if (pDstRect->left < 0 || pDstRect->top < 0
                || pDstRect->right <= 0 || pDstRect->bottom <= 0)
            {
                break;
            }

            mParms->mDstRect = *pDstRect;
        }
        else
        {
            mParms->mDstRect.left = 0;
            mParms->mDstRect.top = 0;
            mParms->mDstRect.right = pGPEDst->Width();
            mParms->mDstRect.bottom = pGPEDst->Height();
        }

        gctINT32 dwRect = mParms->mDstRect.right - mParms->mDstRect.left;
        gctINT32 dhRect = mParms->mDstRect.bottom - mParms->mDstRect.top;
        if ((dwRect <= 0) || (dhRect <= 0) || (dwRect * dhRect < mMinPixelNum))
        {
            break;
        }

        if (pClpRect)
        {
            if (pClpRect->left < 0 || pClpRect->top < 0
                || pClpRect->right <= 0 || pClpRect->bottom <= 0)
            {
                break;
            }

            mParms->mClipRect = *pClpRect;
        }
        else
        {
            mParms->mClipRect = mParms->mDstRect;
        }

        gctINT32 swRect, shRect;
        if (UseSource)
        {
            gcmASSERT(pGPESrc);

            if (pSrcRect)
            {
                if (pSrcRect->left < 0 || pSrcRect->top < 0
                    || pSrcRect->right <= 0 || pSrcRect->bottom <= 0)
                {
                    break;
                }

                mParms->mSrcRect = *pSrcRect;
            }
            else
            {
                mParms->mSrcRect.left = 0;
                mParms->mSrcRect.top = 0;
                mParms->mSrcRect.right = pGPESrc->Width();
                mParms->mSrcRect.bottom = pGPESrc->Height();
            }

            swRect = mParms->mSrcRect.right - mParms->mSrcRect.left;
            shRect = mParms->mSrcRect.bottom - mParms->mSrcRect.top;
            if ((swRect <= 0) || (shRect <= 0))
            {
                break;
            }

            if (pGPESrc->Format() == gpe1Bpp)
            {
                gcmASSERT(!UseMask);

                if ((pBltParms->bltFlags & BLT_STRETCH)
                    || (swRect != dwRect)
                    || (shRect != dhRect))
                {
                    break;
                }

                mParms->mBlit = BLIT_MONO_SOURCE;
            }
            else
            {
                if ((swRect != dwRect)
                    || (shRect != dhRect))
                {
                    mParms->mBlit = BLIT_STRETCH;
                }
                else
                {
                    mParms->mBlit = BLIT_NORMAL;
                }
            }

            sRot = GPERotationToGALRotation(pGPESrc->Rotate());
        }
        else
        {
            swRect = shRect = -1;
            mParms->mBlit = BLIT_NORMAL;
        }

        gctINT32 mwRect = -1;
        gctINT32 mhRect = -1;
        if (UseMask)
        {
            gcmASSERT(pGPEMask);

            if (!pMskRect)
            {
                break;
            }
            else if (pMskRect->left < 0 || pMskRect->top < 0
            || pMskRect->right <= 0 || pMskRect->bottom <= 0)
            {
                break;
            }

            mwRect = pMskRect->right - pMskRect->left;
            mhRect = pMskRect->bottom - pMskRect->top;
            if ((mwRect <= 0) || (mhRect <= 0))
            {
                break;
            }

            if ((pGPEMask->Format() != gpe1Bpp)
                || (pBltParms->bltFlags & BLT_STRETCH)
                || (mwRect != dwRect)
                || (mhRect != dhRect))
            {
                break;
            }

            int stride = abs(pGPEMask->Stride());
            if ((stride == 0) || (stride & 3))
            {
                break;
            }

            int MaskWidth = pGPEMask->Width();
            int MaskHeight = pGPEMask->Height();

            if (MaskWidth == 0)
            {
                MaskWidth = pMskRect->right;
            }

            if (MaskHeight == 0)
            {
                MaskHeight = pMskRect->bottom;
            }

            if (MaskWidth > (stride << 3))
            {
                break;
            }

            MaskWidth = stride << 3;
            AlignWidthHeight((gctUINT32*)&MaskWidth, (gctUINT32*)&MaskHeight);
            if ((gcmALIGN(MaskWidth * MaskHeight, 8) >> 3) == 0)
            {
                break;
            }

            mParms->mBlit = BLIT_MASKED_SOURCE;
            mParms->mMonoRect = *pMskRect;

            mRot = GPERotationToGALRotation(pGPEMask->Rotate());
            if (UseSource)
            {
                gcmASSERT((mParms->mHwVersion > 0 || pGPESrc->Stride() > 0));

                if ((pGPESrc->Format() == gpe1Bpp)
                    || (mRot != sRot)
                    || (mhRect != shRect)
                    || (mwRect != swRect))
                {
                    break;
                }
            }
            else
            {
                sRot = mRot;
            }
        }

        // 2. Set rotations
        dRot = GPERotationToGALRotation(pGPEDst->Rotate());

        if (UseSource || UseMask)
        {
            gceSURF_ROTATION dRot2 = dRot;

            gcmVERIFY((GetRelativeRotation(sRot, &dRot2)));

            if ((mParms->mBlit == BLIT_MONO_SOURCE)
                || (mParms->mBlit == BLIT_MASKED_SOURCE))
            {
                if (!mParms->mHwVersion
                    && (dRot2 != gcvSURF_0_DEGREE)
                    && (dRot2 != gcvSURF_90_DEGREE))
                {
                    break;
                }

                if (UseSource)
                {
                    gcmASSERT(!UseMask || (mRot == sRot));
                    gcmVERIFY(TransRect(
                        &mParms->mSrcRect,
                        sRot, gcvSURF_0_DEGREE,
                        GetSurfWidth(pGPESrc),
                        GetSurfHeight(pGPESrc)));

                    sRot = gcvSURF_0_DEGREE;
                }

                if (UseMask)
                {
                    gcmVERIFY(TransRect(
                        &mParms->mMonoRect,
                        mRot, gcvSURF_0_DEGREE,
                        GetSurfWidth(pGPEMask),
                        GetSurfHeight(pGPEMask)));

                    mRot = gcvSURF_0_DEGREE;
                }
            }
            else
            {
                gcmASSERT(!UseMask);
                gcmASSERT(UseSource);

                gceSURF_ROTATION sRot2;

                switch (dRot2)
                {
                case gcvSURF_0_DEGREE:
                    sRot2 = dRot2 = gcvSURF_0_DEGREE;
                    break;

                case gcvSURF_90_DEGREE:
                    sRot2 = gcvSURF_0_DEGREE;
                    dRot2 = gcvSURF_90_DEGREE;
                    break;

                case gcvSURF_180_DEGREE:
                    sRot2 = gcvSURF_0_DEGREE;
                    dRot2 = gcvSURF_180_DEGREE;
                    break;

                case gcvSURF_270_DEGREE:
                    sRot2 = gcvSURF_90_DEGREE;
                    dRot2 = gcvSURF_0_DEGREE;
                    break;

                default:
                    break;
                }

                gcmVERIFY(TransRect(
                    &mParms->mSrcRect,
                    sRot, sRot2,
                    GetSurfWidth(pGPESrc),
                    GetSurfHeight(pGPESrc)));

                sRot = sRot2;
            }

            if (!mParms->mHwVersion && (dRot2 == gcvSURF_180_DEGREE))
            {
                dRot2 = gcvSURF_0_DEGREE;
                mParms->mMirrorX = !mParms->mMirrorX;
                mParms->mMirrorY = !mParms->mMirrorY;
            }

            gcmVERIFY(TransRect(
                    &mParms->mDstRect,
                    dRot, dRot2,
                    GetSurfWidth(pGPEDst),
                    GetSurfHeight(pGPEDst)));

            gcmVERIFY(TransRect(
                    &mParms->mClipRect,
                    dRot, dRot2,
                    GetSurfWidth(pGPEDst),
                    GetSurfHeight(pGPEDst)));

            dRot = dRot2;
        }
        else
        {
            gcmVERIFY(TransRect(
                    &mParms->mDstRect,
                    dRot, gcvSURF_0_DEGREE,
                    GetSurfWidth(pGPEDst),
                    GetSurfHeight(pGPEDst)));

            gcmVERIFY(TransRect(
                    &mParms->mClipRect,
                    dRot, gcvSURF_0_DEGREE,
                    GetSurfWidth(pGPEDst),
                    GetSurfHeight(pGPEDst)));

            dRot = gcvSURF_0_DEGREE;
        }

        // 3. Set src if necessary
        if (UseSource)
        {
            if (pGPESrc->Format() == gpe1Bpp)
            {
                gcmASSERT(!UseMask);

                if (!mParms->SetMono(
                        pBltParms,
                        gcvTRUE,
                        pGPESrc,
                        sRot,
                        OptMask))
                {
                    break;
                }

                mParms->mMonoRect = mParms->mSrcRect;
            }
            else if (!mParms->SetSurface(pBltParms, gcvTRUE, pGPESrc, sRot))
            {
                break;
            }

            if(pBltParms->bltFlags & BLT_TRANSPARENT)
            {
                gcmASSERT(!UseMask);

                // BUGBUG: BLT_TRANSPARENT with a wrong ROP4 = 0xCCCC by GDI
                //            Change the bRop from 0xCC to 0xAA to fix it up
                Rop4 = 0xAACC;
                mParms->mBgRop = 0xAA;
                mParms->mTrsp = gcvSURF_SOURCE_MATCH;
                mParms->mMono.mTsColor =
                mParms->mSrcTrspColor = pBltParms->solidColor;
            }
        }

        // 3. Set mask if necessary
        if (UseMask)
        {
            if (!mParms->SetMono(
                    pBltParms,
                    gcvFALSE,
                    pGPEMask,
                    mRot,
                    OptMask))
            {
                break;
            }

            if (!UseSource)
            {
                gcmASSERT(mParms->mSrcSurf.mType == GC2D_OBJ_UNKNOWN);

                mParms->mSrcRect = mParms->mMonoRect;

                // suppose the foreground pixels were less
                mParms->mFgRop |= 0xCC;
                if (!((mParms->mFgRop ^ (mParms->mFgRop >> 2)) & 0x33))
                {
                    mParms->mBgRop |= 0xCC;
                    gcmASSERT((mParms->mBgRop ^ ( mParms->mBgRop >> 2)) & 0x33);
                }

                // src must have alpha channel and its rotate must be 0 degree
                mParms->mSrcSurf.mFormat = gcvSURF_A1R5G5B5;
                mParms->mSrcSurf.mRotate = gcvSURF_0_DEGREE;
                mParms->mSrcSurf.mWidth = mParms->mMono.mWidth;
                mParms->mSrcSurf.mHeight = mParms->mMono.mHeight;
                mParms->mSrcSurf.mStride = mParms->mMono.mWidth * 2;
                mParms->mSrcSurf.mSize = mParms->mSrcSurf.mStride * mParms->mSrcSurf.mHeight;
                gcmASSERT(mParms->mSrcSurf.mSize <= mParms->mSrcBuffer.mAuxBuffer->mSize);
                mParms->mSrcSurf.mVirtualAddr = mParms->mSrcBuffer.mAuxBuffer->mVirtualAddr;
                mParms->mSrcSurf.mPhysicalAddr = mParms->mSrcBuffer.mAuxBuffer->mPhysicalAddr;
                mParms->mSrcSurf.mDMAAddr = mParms->mSrcBuffer.mAuxBuffer->mDMAAddr;
                mParms->mSrcSurf.mType = GC2D_OBJ_SURF;
            }

            if (mParms->mBlit == BLIT_MASKED_SOURCE)
            {

                if (abs(mParms->mSrcRect.left - mParms->mMonoRect.left)
                    % (16 / GetFormatSize(mParms->mSrcSurf.mFormat)))
                {
                    break;
                }
            }
        }

        // 4. Set dst
        if (!mParms->SetSurface(pBltParms, gcvFALSE, pGPEDst, dRot))
        {
            break;
        }

        // 5. Set brush if necessary
        if (UsePattern)
        {
            if (!mParms->SetBrush(pBltParms, dRot))
            {
                break;
            }
        }

        // 6. Set Alphablend
        if (!mParms->SetAlphaBlend(pBltParms))
        {
            break;
        }

#if GC2D_GATHER_STATISTICS
        if (mParms->mABEnable)
        {
            ++nAlphaBlend;
        }
        else
        {
            ++nROPBlit;
        }
#endif

        // 7. Do blit
        if (!mParms->Blit(mEngine2D))
        {
            GC2D_DebugTrace(GC2D_ERROR_TRACE, ("GC2DROPBlt failed"));
            break;
        }

        switch (mSyncMode)
        {
        case GC2D_SYNC_MODE_ASYNC:
        case GC2D_SYNC_MODE_FULL_ASYNC:
                // commit command buffer without wait
                gcmVERIFY_OK(FlushAndCommit(gcvTRUE, gcvFALSE));
                bForce = gcvFALSE;
                break;

        case GC2D_SYNC_MODE_FORCE:
        default:
            gcmVERIFY_OK(FlushAndCommit(gcvTRUE, gcvTRUE));
            break;
        }

        bSucceed = gcvTRUE;

    } while (gcvFALSE);

    if (bSucceed)
    {
        UpdateBusyStatus(bForce);

#if GC2D_GATHER_STATISTICS
        if (mParms->mABEnable)
        {
            ++nAlphaBlendSucceed;
        }
        else
        {
            ++nROPBlitSucceed;
        }
#endif
        return S_OK;
    }
    else
    {
        GC2D_LEAVE_ARG("%d", -ERROR_NOT_SUPPORTED);
        return -ERROR_NOT_SUPPORTED;
    }
}

gctBOOL GC2D_Accelerator::DumpGPESurf(GPESurf* surf)
{
    GC2D_ENTER_ARG("surf = 0x%08p", surf);

    GC2D_DebugTrace(GC2D_DUMP_TRACE,
        ("DumpGPESurf: InVideoMemory=%d, Buffer=0x%08x, Format=%d,"\
         " BPP=%d, Rotate=%d, Width=%d, Height=%d, Stride=%d\r\n"),
         surf->InVideoMemory(), surf->Buffer(), surf->Format(),
         GetSurfBytesPerPixel(surf), surf->Rotate(), surf->Width(),
         surf->Height(), surf->Stride());

    GC2D_LEAVE();
    return gcvTRUE;
}

gctBOOL GC2D_Accelerator::DumpGPEBltParms(GPEBltParms * pBltParms)
{
    GPESurf* pGPESrc = (GPESurf*) pBltParms->pSrc;
    GPESurf* pGPEDst = (GPESurf*) pBltParms->pDst;
    GPESurf* pGPEMask = (GPESurf*) pBltParms->pMask;
    GPESurf* pGPEBrush = (GPESurf*) pBltParms->pBrush;
    gcsRECT_PTR pDstRect = (gcsRECT_PTR)pBltParms->prclDst;
    gcsRECT_PTR pSrcRect = (gcsRECT_PTR)pBltParms->prclSrc;
    gcsRECT_PTR pClipRect = (gcsRECT_PTR)pBltParms->prclClip;
    gcsRECT_PTR pMaskRect = (gcsRECT_PTR)pBltParms->prclMask;

    GC2D_ENTER_ARG("pBltParms = 0x%08p", pBltParms);

    GC2D_DebugTrace(GC2D_DUMP_TRACE,
        ("DumpBltPara: ***********************************************\r\n"));
    GC2D_DebugTrace(GC2D_DUMP_TRACE,
        ("DumpBltPara: ROP4=0x%08x, solid color=0x%08x, bltFlags=0x%08x\r\n"),
        pBltParms->rop4, pBltParms->solidColor, pBltParms->bltFlags);

    GC2D_DebugTrace(GC2D_DUMP_TRACE,
        ("DumpBltPara: xPos=%d, yPos=%d, iMode=%d\r\n"),
        pBltParms->xPositive, pBltParms->yPositive, pBltParms->iMode);

    GC2D_DebugTrace(GC2D_DUMP_TRACE,
        ("DumpBltPara: Lookup=0x%08x, Convert=0x%08x, ColorConverter=0x%08x,"\
         " blendFunction=0x%08x, bltSig=0x%08x\r\n"),
         pBltParms->pLookup, pBltParms->pConvert, pBltParms->pColorConverter,
         pBltParms->blendFunction, pBltParms->bltSig);

    if (pGPEDst)
    {
        GC2D_DebugTrace(GC2D_DUMP_TRACE,
            ("DumpBltPara: Dst Surf:\r\n"));
        DumpGPESurf(pGPEDst);

        if (pDstRect)
        {
            GC2D_DebugTrace(GC2D_DUMP_TRACE,
                ("DumpBltPara: DstRect(%d,%d, %d,%d)\r\n"),
                pDstRect->left, pDstRect->top, pDstRect->right, pDstRect->bottom);
        }

        if (pClipRect)
        {
            GC2D_DebugTrace(GC2D_DUMP_TRACE,
                ("DumpBltPara: ClpRect(%d,%d, %d,%d)\r\n"),
                pClipRect->left, pClipRect->top, pClipRect->right, pClipRect->bottom);
        }
    }

    if (pGPESrc)
    {
        GC2D_DebugTrace(GC2D_DUMP_TRACE,
            ("DumpBltPara: Src Surf:\r\n"));
        DumpGPESurf(pGPESrc);

        if (pSrcRect)
        {
            GC2D_DebugTrace(GC2D_DUMP_TRACE,
                ("DumpBltPara: SrcRect(%d,%d, %d,%d)\r\n"),
                pSrcRect->left, pSrcRect->top, pSrcRect->right, pSrcRect->bottom);
        }
    }

    if (pGPEMask)
    {
        GC2D_DebugTrace(GC2D_DUMP_TRACE,
            ("DumpBltPara: Mask Surf:\r\n"));
        DumpGPESurf(pGPEMask);

        if (pMaskRect)
        {
            GC2D_DebugTrace(GC2D_DUMP_TRACE,
                ("DumpBltPara: MskRect(%d,%d, %d,%d)\r\n"),
                pMaskRect->left, pMaskRect->top, pMaskRect->right, pMaskRect->bottom);
        }
    }

    if (pGPEBrush)
    {
        GC2D_DebugTrace(GC2D_DUMP_TRACE,
            ("DumpBltPara: Brush Surf:\r\n"));
        DumpGPESurf(pGPEBrush);

        if (pBltParms->pptlBrush)
        {
            GC2D_DebugTrace(GC2D_DUMP_TRACE,
                ("DumpBltPara: BrushPoint(%d, %d)\r\n"),
                pBltParms->pptlBrush->x, pBltParms->pptlBrush->y);
        }
    }

    GC2D_LEAVE();
    return gcvTRUE;
}
