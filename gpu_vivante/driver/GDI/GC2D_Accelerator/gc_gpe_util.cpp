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

gceSURF_FORMAT EGPEFormatToGALFormat(EGPEFormat format)
{
        switch (format)
        {
        case gpe8Bpp:
            return gcvSURF_INDEX8;

        case gpe16Bpp:
            return gcvSURF_R5G6B5;

        case gpe32Bpp:
            return gcvSURF_A8R8G8B8;

        default:
            return gcvSURF_UNKNOWN;
        }
}

gceSTATUS QueryAlignment(
    OUT gctUINT32 * Width,
    OUT gctUINT32 * Height)
{
    if (Width)
        *Width = 16;

    if (Height)
        *Height = 4;

    return gcvSTATUS_OK;
}

gceSTATUS AlignWidthHeight(
    IN OUT gctUINT32 * Width,
    IN OUT gctUINT32 * Height)
{
    gctUINT AlingedWidth, AlingedHeight, tmp;
    gceSTATUS status = gcvSTATUS_OK;

    QueryAlignment(&AlingedWidth, &AlingedHeight);

    if (Width != gcvNULL)
    {
        tmp = *Width;

        /* Align the width. */
        *Width = gcmALIGN(tmp, AlingedWidth);
        if (tmp != *Width)
            status = gcvSTATUS_MISMATCH;
    }

    if (Height != gcvNULL)
    {
        tmp = *Height;

        /* Align the height. */
        *Height = gcmALIGN(tmp, AlingedHeight);
        if (tmp != *Height)
            status = gcvSTATUS_MISMATCH;
    }

    /* Success. */
    return status;
}

gceSURF_FORMAT GPEFormatToGALFormat(EGPEFormat format, GPEFormat * gpeFormatPtr)
{
    if (!gpeFormatPtr)
    {
        return gcvSURF_UNKNOWN;
    }

    gctUINT32 RedMask, GreenMask, BlueMask, AlphaMask = 0;
    gctBOOL bMask;

    switch (gpeFormatPtr->m_PaletteEntries)
    {
    case 4:
        AlphaMask = gpeFormatPtr->m_pPalette[3];
    case 3:
        RedMask   = gpeFormatPtr->m_pPalette[0];
        GreenMask = gpeFormatPtr->m_pPalette[1];
        BlueMask  = gpeFormatPtr->m_pPalette[2];
        bMask = gcvTRUE;
        break;

    default:
        RedMask = GreenMask = BlueMask = 0;
        bMask = gcvFALSE;
    }

    switch (EGPEFormatToBpp[format])
    {
    case 8:
        return gcvSURF_INDEX8;

    case 16:
        if (bMask)
        {
            if(RedMask == 0xF800 && GreenMask == 0x7E0 && BlueMask == 0x1F && AlphaMask == 0)
                   return gcvSURF_R5G6B5;
            else if(RedMask == 0x7C00 && GreenMask == 0x3E0 && BlueMask == 0x1F && AlphaMask == 0x8000)
                   return gcvSURF_A1R5G5B5;
            else if(RedMask == 0x7C00 && GreenMask == 0x3E0 && BlueMask == 0x1F && AlphaMask == 0)
                   return gcvSURF_X1R5G5B5;
            else if(RedMask == 0xF00 && GreenMask == 0xF0 && BlueMask == 0xF && AlphaMask == 0xF000)
                   return gcvSURF_A4R4G4B4;
            else if(RedMask == 0xF00 && GreenMask == 0xF0 && BlueMask == 0xF && AlphaMask == 0)
                   return gcvSURF_X4R4G4B4;
            else
                return gcvSURF_UNKNOWN;
        }
        else
        {
            return gcvSURF_R5G6B5;// default 16bit format when no mask
        }

    case 32:
        if (bMask)
        {
            if (RedMask == 0xFF0000 && GreenMask == 0xFF00 && BlueMask == 0xFF && AlphaMask == 0xFF000000)
                   return gcvSURF_A8R8G8B8;
            else if (RedMask == 0xFF0000 && GreenMask == 0xFF00 && BlueMask == 0xFF && AlphaMask == 0)
                   return gcvSURF_X8R8G8B8;
            else
                return gcvSURF_UNKNOWN;
        }
        else
        {
            return gcvSURF_A8R8G8B8;// default 32bit format when no mask
        }

    default:
        return gcvSURF_UNKNOWN;
    }
}

gceSURF_FORMAT DDGPEFormatToGALFormat(EDDGPEPixelFormat format)
{
        switch (format)
        {
        case ddgpePixelFormat_565:
            return gcvSURF_R5G6B5;

        case ddgpePixelFormat_5551:
            return gcvSURF_A1R5G5B5;

        case ddgpePixelFormat_4444:
            return gcvSURF_A4R4G4B4;

        case ddgpePixelFormat_5550:
            return gcvSURF_X1R5G5B5;

        case ddgpePixelFormat_8888:
            return gcvSURF_A8R8G8B8;

#if UNDER_CE >= 600
        case ddgpePixelFormat_YUY2:
            return gcvSURF_YUY2;

        case ddgpePixelFormat_UYVY:
            return gcvSURF_UYVY;

        case ddgpePixelFormat_YV12:
            return gcvSURF_YV12;
#else
        case ddgpePixelFormat_YUYV422:
            return gcvSURF_YUY2;

        case ddgpePixelFormat_UYVY422:
            return gcvSURF_UYVY;
#endif

        default:
            return gcvSURF_UNKNOWN;
        }
}

gceSURF_ROTATION GPERotationToGALRotation(int iRotate)
{
    switch (iRotate)
    {
    case DMDO_0:
        return gcvSURF_0_DEGREE;

    case DMDO_90:
        return gcvSURF_270_DEGREE;

    case DMDO_180:
        return gcvSURF_180_DEGREE;

    case DMDO_270:
        return gcvSURF_90_DEGREE;

    default:
        return (gceSURF_ROTATION)-1;
    }
}


gctBOOL TransBrush(gctPOINTER pBrushDst, gctPOINTER pBrushSrc, gctINT Bpp, gceSURF_ROTATION Rotate)
{
    if (pBrushSrc == gcvNULL || pBrushDst == gcvNULL)
    {
        return gcvFALSE;
    }

    if ((Bpp != 1) && (Bpp != 2) && (Bpp != 4))
    {
        // not support
        return gcvFALSE;
    }

    gctINT stride = 8 * Bpp;

    if (Rotate == gcvSURF_0_DEGREE)
    {
        gcoOS_MemCopy(pBrushDst, pBrushSrc, stride * 8);

        return gcvTRUE;
    }

    gctINT y;
    for (y = 0; y < 8; y++)
    {
        gctINT x;
        for (x = 0; x < 8; x++)
        {
            gctUINT8* src = ((gctUINT8*)pBrushSrc)  + y * stride + x * Bpp;
            gctUINT8* dst;

            switch (Rotate)
            {
            case gcvSURF_90_DEGREE:
                dst = ((gctUINT8*)pBrushDst) + x * stride + (7 - y) * Bpp;
                break;

            case gcvSURF_180_DEGREE:
                dst = ((gctUINT8*)pBrushDst) + (7 - y) * stride + (7 - x) * Bpp;
                break;

            case gcvSURF_270_DEGREE:
                dst = ((gctUINT8*)pBrushDst) + (7 - x) * stride + y * Bpp;
                break;

            default:
                return gcvFALSE;
            }

            switch (Bpp)
            {
            case 1:
                *(gctUINT8*)(dst) = *(gctUINT8*)(src);
                break;
            case 2:
                *(gctUINT16*)(dst) = *(gctUINT16*)(src);
                break;
            case 4:
                *(gctUINT32*)(dst) = *(gctUINT32*)(src);
                break;
            default:
                return gcvFALSE;
            }
        }
    }

    return gcvTRUE;
}

int GetSurfWidth(GPESurf* surf)
{
    switch (surf->Rotate())
    {
    case DMDO_0:
    case DMDO_180:
        return surf->Width();

    case DMDO_90:
    case DMDO_270:
        return surf->Height();

    default:
        gcmASSERT(0);
        return 0;
    }
}

int GetSurfHeight(GPESurf* surf)
{
    switch (surf->Rotate())
    {
    case DMDO_0:
    case DMDO_180:
        return surf->Height();

    case DMDO_90:
    case DMDO_270:
        return surf->Width();

    default:
        gcmASSERT(0);
        return 0;
    }
}

int GetSurfBytesPerPixel(GPESurf* surf)
{
#if UNDER_CE >= 600
    return surf->BytesPerPixel();
#else
    return (EGPEFormatToBpp[surf->Format()]) >> 3;
#endif
}

gctBOOL
GetRelativeRotation(
    IN gceSURF_ROTATION Orientation,
    IN OUT gceSURF_ROTATION *Relation)
{
    switch (Orientation)
    {
    case gcvSURF_0_DEGREE:
        break;

    case gcvSURF_90_DEGREE:
        switch (*Relation)
        {
        case gcvSURF_0_DEGREE:
            *Relation = gcvSURF_270_DEGREE;
            break;

        case gcvSURF_90_DEGREE:
            *Relation = gcvSURF_0_DEGREE;
            break;

        case gcvSURF_180_DEGREE:
            *Relation = gcvSURF_90_DEGREE;
            break;

        case gcvSURF_270_DEGREE:
            *Relation = gcvSURF_180_DEGREE;
            break;

        default:
            return gcvFALSE;
        }
        break;

    case gcvSURF_180_DEGREE:
        switch (*Relation)
        {
        case gcvSURF_0_DEGREE:
            *Relation = gcvSURF_180_DEGREE;
            break;

        case gcvSURF_90_DEGREE:
            *Relation = gcvSURF_270_DEGREE;
            break;

        case gcvSURF_180_DEGREE:
            *Relation = gcvSURF_0_DEGREE;
            break;

        case gcvSURF_270_DEGREE:
            *Relation = gcvSURF_90_DEGREE;
            break;

        default:
            return gcvFALSE;
        }
        break;

    case gcvSURF_270_DEGREE:
        switch (*Relation)
        {
        case gcvSURF_0_DEGREE:
            *Relation = gcvSURF_90_DEGREE;
            break;

        case gcvSURF_90_DEGREE:
            *Relation = gcvSURF_180_DEGREE;
            break;

        case gcvSURF_180_DEGREE:
            *Relation = gcvSURF_270_DEGREE;
            break;

        case gcvSURF_270_DEGREE:
            *Relation = gcvSURF_0_DEGREE;
            break;

        default:
            return gcvFALSE;
        }
        break;

    default:
        return gcvFALSE;
    }

    return gcvTRUE;
}

gctBOOL
TransRect(
    IN OUT gcsRECT_PTR Rect,
    IN gceSURF_ROTATION Rotation,
    IN gceSURF_ROTATION toRotation,
    IN gctINT32 SurfaceWidth,
    IN gctINT32 SurfaceHeight
    )
{
    gctINT32 temp;

    /* Verify the arguments. */
    if (Rect == gcvNULL)
    {
        return gcvFALSE;
    }

    if (toRotation == gcvSURF_90_DEGREE
        || toRotation == gcvSURF_270_DEGREE)
    {
        temp = SurfaceWidth;
        SurfaceWidth = SurfaceHeight;
        SurfaceHeight = temp;
    }

    if (!GetRelativeRotation(toRotation, &Rotation))
    {
        return gcvFALSE;
    }

    switch (Rotation)
    {
    case gcvSURF_0_DEGREE:
        break;

    case gcvSURF_90_DEGREE:
        temp = Rect->left;
        Rect->left = SurfaceWidth - Rect->bottom;
        Rect->bottom = Rect->right;
        Rect->right = SurfaceWidth - Rect->top;
        Rect->top = temp;

        break;

    case gcvSURF_180_DEGREE:
        temp = Rect->left;
        Rect->left = SurfaceWidth - Rect->right;
        Rect->right = SurfaceWidth - temp;
        temp = Rect->top;
        Rect->top = SurfaceHeight - Rect->bottom;
        Rect->bottom = SurfaceHeight - temp;

        break;

    case gcvSURF_270_DEGREE:
        temp = Rect->left;
        Rect->left = Rect->top;
        Rect->top = SurfaceHeight - Rect->right;
        Rect->right = Rect->bottom;
        Rect->bottom = SurfaceHeight - temp;

        break;

    default:
        return gcvFALSE;
    }

    return gcvTRUE;
}

gctUINT32 GetStretchFactor(
    gctINT32 SrcSize,
    gctINT32 DestSize
    )
{
    gctUINT stretchFactor;

    if ( (SrcSize > 1) && (DestSize > 1) )
    {
        stretchFactor = ((SrcSize - 1) << 16) / (DestSize - 1);
    }
    else if ( (SrcSize > 0) && (DestSize > 0) )
    {
        stretchFactor = (SrcSize << 16) / DestSize;
    }
    else
    {
        stretchFactor = 0;
    }

    return stretchFactor;
}

gctPOINTER AllocNonCacheMem(gctUINT32 size)
{
    return VirtualAlloc(NULL, size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE|PAGE_NOCACHE);
}

gctBOOL FreeNonCacheMem(gctPOINTER addr)
{
    return VirtualFree(addr, 0, MEM_RELEASE);
}

gctUINT32 GetFormatSize(gceSURF_FORMAT format)
{
    switch (format)
    {
        case gcvSURF_A8R8G8B8:
        case gcvSURF_X8R8G8B8:
            return 4;

        case gcvSURF_X4R4G4B4:
        case gcvSURF_A4R4G4B4:
        case gcvSURF_R5G6B5:
        case gcvSURF_A1R5G5B5:
        case gcvSURF_X1R5G5B5:
            return 2;

        case gcvSURF_INDEX8:
            return 1;

        case gcvSURF_YUY2:
        case gcvSURF_UYVY:
            return 2;

        case gcvSURF_YV12:
            return 1;

        default:
            return 0;
    }
}
