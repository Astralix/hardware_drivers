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



#ifndef __MISC_H__
#define __MISC_H__

#define ROP4_USE_SOURCE(Rop4) \
    ((( (Rop4) ^ ( (Rop4) >> 2 ) ) & 0x3333 ) ? gcvTRUE : gcvFALSE)

#define ROP4_USE_PATTERN(Rop4) \
    ((( (Rop4) ^ ( (Rop4) >> 4 ) ) & 0x0F0F ) ? gcvTRUE : gcvFALSE)

#define ROP4_USE_MASK(Rop4) \
    ((( (Rop4) ^ ( (Rop4) >> 8 ) ) & 0x00FF ) ? gcvTRUE : gcvFALSE)


/******************************************************************************\
|* The definations for class GC2DParms                                         *|
\******************************************************************************/

enum BLIT_TYPE
{
    BLIT_INVALID,
    BLIT_NORMAL,
    BLIT_MASKED_SOURCE,
    BLIT_MONO_SOURCE,
    BLIT_STRETCH,
};

class GC2DBrush
{
public:
    GC2D_OBJECT_TYPE    mType;
    gceSURF_FORMAT      mFormat;
    gctUINT32           mX;
    gctUINT32           mY;
    gctUINT32           mDMAAddr;
    gctPOINTER          mVirtualAddr;
    gctUINT32           mSolidColor;
    gctBOOL             mSolidEnable;
    gctBOOL             mConvert;

public:
    GC2DBrush ()
    {
        mType = GC2D_OBJ_UNKNOWN;
        mFormat = gcvSURF_UNKNOWN;
        mDMAAddr = ~0;
        mVirtualAddr = 0;
        mSolidEnable = gcvFALSE;
    }

    gctBOOL Init();
    gctBOOL Denit();

    gctBOOL IsValid() const { return mType == GC2D_OBJ_BRUSH; }
};

class GC2DMono
{
public:
    GC2D_OBJECT_TYPE    mType;
    gctPOINTER          mStream;
    gctINT32            mStride;
    gctUINT32           mWidth;
    gctUINT32           mHeight;
    gctUINT32           mFgColor;
    gctUINT32           mBgColor;
    gctUINT32           mTsColor;

public:
    GC2DMono ()
    {
        mType = GC2D_OBJ_UNKNOWN;
    }

    gctBOOL IsValid() const { return mType == GC2D_OBJ_MONO; }
};
typedef class GC2DSurf *GC2DSurf_PTR;

class GC2DParms
{
    friend class GC2D_Accelerator;
public:
    GC2DParms();
    ~GC2DParms();

    gctBOOL Initialize(
        GC2D_Accelerator_PTR Acc2D,
        gctUINT32 VideoPhyBase,
        gctUINT32 VideoMemSize,
        gctUINT MaxWidth,
        gctUINT MaxHeight);
    gctBOOL Denitialize(GC2D_Accelerator_PTR Acc2D);

    gctBOOL IsValid() const { return mType == GC2D_OBJ_PARMS; }

    BOOL ContainGC2DSurf(GPESurf *pGPESurf) const;
    BOOL IsGPESurfSupported(GPESurf *pGPESurf, gctBOOL bSrc);

    gctBOOL SetBrush(
        GPEBltParms * pBltParms,
        gceSURF_ROTATION DestRotate);

    gctBOOL SetMono(
        GPEBltParms * pBltParms,
        gctBOOL bSrc,
        GPESurf *Surf,
        gceSURF_ROTATION Rot,
        gctBOOL RopAAF0);

    gctBOOL SetSurface(
        GPEBltParms * pBltParms,
        gctBOOL bSrc,
        GPESurf *pGPESurf,
        gceSURF_ROTATION Rotation);

    gctBOOL SetAlphaBlend(
        GPEBltParms * pBltParms);

    gctBOOL ResetSurface(gctBOOL bSrc);

    gctBOOL Reset(gco2D Engine2D);

    gctBOOL Blit(gco2D Engine2D);

///////////////////////////////////////////////////////////////////////////
private:
    GC2DSurf                mDstSurf;
    gcsRECT                 mDstRect;
    gcsRECT                 mClipRect;

    GC2DSurf                mSrcSurf;
    gcsRECT                 mSrcRect;
    gctUINT32               mSrcTrspColor;
    gctUINT32               mPalette[256];
    gctBOOL                 mPaletteConvert;

    GC2DMono                mMono;
    gcsRECT                 mMonoRect;

    GC2DBrush               mBrush;

    gctBOOL                 mMirrorX;
    gctBOOL                 mMirrorY;

    gctUINT8                mFgRop;
    gctUINT8                mBgRop;

    gceSURF_TRANSPARENCY    mTrsp;
    BLIT_TYPE               mBlit;

    //////// AlphaBlend parameters////////////
    gctBOOL                     mABEnable;
    gctUINT8                    mABSrcAg;
    gctUINT8                    mABDstAg;
    gceSURF_PIXEL_ALPHA_MODE    mABSrcAMode;
    gceSURF_PIXEL_ALPHA_MODE    mABDstAMode;
    gceSURF_GLOBAL_ALPHA_MODE   mABSrcAgMode;
    gceSURF_GLOBAL_ALPHA_MODE   mABDstAgMode;
    gceSURF_BLEND_FACTOR_MODE   mABSrcFactorMode;
    gceSURF_BLEND_FACTOR_MODE   mABDstFactorMode;
    gceSURF_PIXEL_COLOR_MODE    mABSrcColorMode;
    gceSURF_PIXEL_COLOR_MODE    mABDstColorMode;

///////////////////////////////////////////////////////////////////////////
protected:
    GC2D_OBJECT_TYPE    mType;
    gctUINT32           mHwVersion;

    struct TEMP_BUFFER
    {
    GC2DSurf_PTR    mAuxBuffer;
    gctPOINTER      mOrgTable;
    gctUINT32       mMapAddr;
    gctPOINTER      mBackTable;
    gctUINT32       mBackTableSize;
    } mDstBuffer, mSrcBuffer;

    gctUINT8_PTR    mMonoBuffer;
    gctUINT32       mMonoBufferSize;

    gctUINT32       mVideoPhyBase;
    gctUINT32       mVideoMemSize;

    gctUINT32       mMinPixelNum;
    gctUINT32       mMaxPixelNum;

protected:
    gctBOOL InitTempBuffer(
        GC2D_Accelerator_PTR Acc2D,
        TEMP_BUFFER *Buffer,
        gctUINT Width,
        gctUINT Height);
    gctBOOL DenitTempBuffer(
        GC2D_Accelerator_PTR Acc2D,
        TEMP_BUFFER *Buffer);
};

/******************************************************************************\
|*                 Local utility functions                                    *|
\******************************************************************************/

gceSURF_FORMAT    EGPEFormatToGALFormat(EGPEFormat format);
gceSURF_FORMAT    GPEFormatToGALFormat(EGPEFormat format, GPEFormat * gpeFormatPtr);
gceSURF_FORMAT    DDGPEFormatToGALFormat(EDDGPEPixelFormat format);
gceSURF_ROTATION  GPERotationToGALRotation(int iRotate);
int GetSurfBytesPerPixel(GPESurf* surf);
int GetSurfWidth(GPESurf* surf);
int GetSurfHeight(GPESurf* surf);

gctBOOL
TransBrush(
    gctPOINTER pBrushDst,
    gctPOINTER pBrushSrc,
    gctINT Bpp,
    gceSURF_ROTATION Rotate
    );

gctBOOL
GetRelativeRotation(
    IN gceSURF_ROTATION Orientation,
    IN OUT gceSURF_ROTATION *Relation);

gctBOOL
TransRect(
    IN OUT gcsRECT_PTR Rect,
    IN gceSURF_ROTATION Rotation,
    IN gceSURF_ROTATION toRotation,
    IN gctINT32 SurfaceWidth,
    IN gctINT32 SurfaceHeight
    );

gceSTATUS
AlignWidthHeight(
    IN OUT gctUINT32 * Width,
    IN OUT gctUINT32 * Height);

gceSTATUS QueryAlignment(
    OUT gctUINT32 * Width,
    OUT gctUINT32 * Height);

gctUINT32 GetStretchFactor(
    gctINT32 SrcSize,
    gctINT32 DestSize
    );

gctPOINTER AllocNonCacheMem(gctUINT32 size);
gctBOOL FreeNonCacheMem(gctPOINTER addr);
gctUINT32 GetFormatSize(gceSURF_FORMAT format);

#define gcmNO_SUCCESS(status)        (status != gcvSTATUS_OK)

#endif //__MISC_H__
//__MISC_H__
//__MISC_H__
//__MISC_H__
//__MISC_H__
//__MISC_H__
