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
 * GDI API code.
 */

#include "gc_egl_precomp.h"

/*******************************************************************************
***** Version Signature *******************************************************/

const char * _EGL_PLATFORM = "\n\0$PLATFORM$GDI$\n";

#ifndef gcmABS
#define gcmABS(x) (((x) < 0) ? -(x) : (x))
#endif

typedef struct __BITFIELDINFO{
    BITMAPINFO    bmi;
    RGBQUAD       bmiColors[2];
} BITFIELDINFO;

NativeDisplayType
veglGetDefaultDisplay(
    void
    )
{
    /* Return device context for desktop window. */
    return GetDC(GetDesktopWindow());
}

void
veglReleaseDefaultDisplay(
    IN NativeDisplayType Display
    )
{
    /* Release device context for desktop window. */
    ReleaseDC(GetDesktopWindow(), Display);
}

gctBOOL
veglIsValidDisplay(
    IN NativeDisplayType Display
    )
{
    if (GetDeviceCaps(Display, BITSPIXEL) == 0)
    {
        return gcvFALSE;
    }

    return gcvTRUE;
}

#ifndef USE_CE_DIRECTDRAW
gctBOOL
veglInitLocalDisplayInfo(
    VEGLDisplay Display
    )
{
    return gcvTRUE;
}

gctBOOL
veglDeinitLocalDisplayInfo(
    VEGLDisplay Display
    )
{
    return gcvTRUE;
}

gctBOOL
veglGetDisplayInfo(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT VEGLDisplayInfo Info
    )
{
    return gcvFALSE;
}

gctBOOL
veglGetDisplayBackBuffer(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT VEGLBackBuffer BackBuffer,
    OUT gctBOOL_PTR Flip
    )
{
    *Flip = gcvFALSE;

    return gcvTRUE;
}

gctBOOL
veglSetDisplayFlip(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    IN VEGLBackBuffer BackBuffer
    )
{
    return gcvFALSE;
}

gctBOOL
veglSetDisplayFlipRegions(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    IN VEGLBackBuffer BackBuffer,
    IN gctINT NumRects,
    IN gctINT_PTR Rects
    )
{
    return gcvFALSE;
}

gctBOOL
veglGetWindowInfo(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT gctINT_PTR X,
    OUT gctINT_PTR Y,
    OUT gctUINT_PTR Width,
    OUT gctUINT_PTR Height,
    OUT gctUINT_PTR BitsPerPixel,
    OUT gceSURF_FORMAT * Format
    )
{
    RECT rect;
    INT bitsPerPixel;
    gceSURF_FORMAT format;

    /* Get device context bit depth. */
    bitsPerPixel = GetDeviceCaps(Display->hdc, BITSPIXEL);

    /* Return format for window depth. */
    switch (bitsPerPixel)
    {
    case 16:
        /* 16-bits per pixel. */
        format = gcvSURF_R5G6B5;
        break;

    case 32:
        /* 32-bits per pixel. */
        format = gcvSURF_A8R8G8B8;
        break;

    default:
        /* Unsupported colot depth. */
        return gcvFALSE;
    }

    ShowWindow( Window, SW_SHOWNORMAL );

    /* Query window client rectangle. */
    if (!GetClientRect(Window, &rect))
    {
        /* Error. */
        return gcvFALSE;
    }

    /* Set the output window parameters. */
    if(X != gcvNULL)
        * X = rect.left;
    if(Y != gcvNULL)
        * Y = rect.top;
    if(Width != gcvNULL)
        * Width = rect.right  - rect.left;
    if(Height != gcvNULL)
        * Height = rect.bottom - rect.top;
    if(BitsPerPixel != gcvNULL)
        * BitsPerPixel = bitsPerPixel;
    if(Format != gcvNULL)
        * Format = format;

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglDrawImage(
    IN VEGLDisplay Display,
    IN VEGLSurface Surface,
    IN gctPOINTER Bits,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gctBOOL result = gcvFALSE;
    HDC context;
    int bitsPerPixel = Surface->swapBitsPerPixel;

    do
    {
        gctINT count;
        gctINT width, height;
        gceORIENTATION orientation;
        BITFIELDINFO bfi;
        PBITMAPINFOHEADER info = &bfi.bmi.bmiHeader;
        gctUINT32 *mask = (gctUINT32*)(info + 1);

        /* Get the device context of the window. */
        context = GetDC(Surface->hwnd);

        if (context == gcvNULL)
        {
            /* Error. */
            break;
        }

        width  = Surface->bitsAlignedWidth;
        height = Surface->bitsAlignedHeight;

        if (gcmIS_ERROR(gcoSURF_QueryOrientation(
            Surface->swapSurface, &orientation
            )))
        {
            break;
        }

        if (orientation == gcvORIENTATION_BOTTOM_TOP)
        {
            height = -height;
        }

        switch (Surface->resolveFormat)
		{
		case gcvSURF_A8R8G8B8:
		case gcvSURF_X8R8G8B8:
			mask[0] = 0x00FF0000;
			mask[1] = 0x0000FF00;
			mask[2] = 0x000000FF;
			break;

        case gcvSURF_R8G8B8A8:
		case gcvSURF_R8G8B8X8:
			mask[0] = 0xFF000000;
			mask[1] = 0x00FF0000;
			mask[2] = 0x0000FF00;
			break;

        case gcvSURF_X8B8G8R8:
		case gcvSURF_A8B8G8R8:
			mask[0] = 0x000000FF;
			mask[1] = 0x0000FF00;
			mask[2] = 0x00FF0000;
			break;

        case gcvSURF_B8G8R8A8:
		case gcvSURF_B8G8R8X8:
			mask[0] = 0x0000FF00;
			mask[1] = 0x00FF0000;
			mask[2] = 0xFF000000;
			break;

		case gcvSURF_X4R4G4B4:
		case gcvSURF_A4R4G4B4:
			mask[0] = 0x00000F00;
			mask[1] = 0x000000F0;
			mask[2] = 0x0000000F;
			break;

		case gcvSURF_R4G4B4X4:
		case gcvSURF_R4G4B4A4:
			mask[0] = 0x0000F000;
			mask[1] = 0x00000F00;
			mask[2] = 0x000000F0;
			break;

		case gcvSURF_X4B4G4R4:
		case gcvSURF_A4B4G4R4:
			mask[0] = 0x0000000F;
			mask[1] = 0x000000F0;
			mask[2] = 0x00000F00;
			break;

		case gcvSURF_B4G4R4X4:
		case gcvSURF_B4G4R4A4:
			mask[0] = 0x000000F0;
			mask[1] = 0x00000F00;
			mask[2] = 0x0000F000;
			break;

		case gcvSURF_R5G6B5:
			mask[0] = 0x0000F800;
			mask[1] = 0x000007E0;
			mask[2] = 0x0000001F;
			break;

		case gcvSURF_B5G6R5:
			mask[0] = 0x0000001F;
			mask[1] = 0x000007E0;
			mask[2] = 0x0000F800;
			break;

		case gcvSURF_A1R5G5B5:
		case gcvSURF_X1R5G5B5:
			mask[0] = 0x00007C00;
			mask[1] = 0x000003E0;
			mask[2] = 0x0000001F;
			break;

		case gcvSURF_R5G5B5X1:
		case gcvSURF_R5G5B5A1:
			mask[0] = 0x0000F800;
			mask[1] = 0x000007C0;
			mask[2] = 0x0000003E;
			break;

		case gcvSURF_B5G5R5X1:
		case gcvSURF_B5G5R5A1:
			mask[0] = 0x0000003E;
			mask[1] = 0x000007C0;
			mask[2] = 0x0000F800;
			break;

		case gcvSURF_X1B5G5R5:
		case gcvSURF_A1B5G5R5:
			mask[0] = 0x0000001F;
			mask[1] = 0x000003E0;
			mask[2] = 0x00007C00;
			break;

		default:
			ReleaseDC(Surface->hwnd, context);
			return gcvFALSE;
		}

        /* Fill in the bitmap information. */
        info->biSize          = sizeof(bfi.bmi.bmiHeader);
        info->biWidth         = width;
        info->biHeight        = - height;
        info->biPlanes        = 1;
        info->biBitCount      = (gctUINT16) bitsPerPixel;
        info->biCompression   = BI_BITFIELDS;
        info->biSizeImage     = ((width * bitsPerPixel / 8 + 3) & ~3) * gcmABS(height);
        info->biXPelsPerMeter = 0;
        info->biYPelsPerMeter = 0;
        info->biClrUsed       = 0;
        info->biClrImportant  = 0;

        /* Draw bitmap bits to window. */
        count = StretchDIBits(
            context,
            Left,    Top, Width, Height,
            Left, Top, Width, Height,
            Bits,
            (BITMAPINFO *) info,
            DIB_RGB_COLORS,
            SRCCOPY
            );

        if (count == 0)
        {
            /* Error. */
            break;
        }

        /* Define FRAME_DUMP to be the prefix of the filename if you
           want each frame to be saved as a bitmap, for example:
           #define FRAME_DUMP "Image_"
           Each frame will be saved as Image_0001.bmp with the "0001"
           being incremented each frame. */
//#define FRAME_DUMP
#ifdef FRAME_DUMP
        {
            static int image = 1;
            char fileName[80];
            FILE * f;

            sprintf_s(fileName, sizeof(fileName),
                      FRAME_DUMP "%04d.bmp",
                      image++);
            if (fopen_s(&f, fileName, "wb") == 0)
            {
                BITMAPFILEHEADER header;
                header.bfType = 'MB';
                header.bfSize = sizeof(header) +
                                sizeof(Surface->bitmap) +
                                Surface->bitmap.biSizeImage;
                header.bfOffBits = sizeof(header) + sizeof(Surface->bitmap);
                fwrite(&header, sizeof(header), 1, f);
                fwrite(&Surface->bitmap, sizeof(Surface->bitmap), 1, f);
                fwrite(Bits, Surface->bitmap.biSizeImage, 1, f);
                fclose(f);
            }
        }
#       endif /* FRAME_DUMP */

        /* Success. */
        result = gcvTRUE;
    }
    while (gcvFALSE);

    /* Release the device context. */
    if (context != gcvNULL)
    {
        ReleaseDC(Surface->hwnd, context);
    }

    /* Return result. */
    return result;
}
#endif

EGLint
veglGetNativeVisualId(
    IN VEGLDisplay dpy,
    IN VEGLConfig Config
    )
{
    return (EGLint) (ULONG_PTR) VEGL_DISPLAY(dpy)->hdc;
}

gctBOOL
veglDrawPixmap(
    IN VEGLDisplay Display,
    IN EGLNativePixmapType Pixmap,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Right,
    IN gctINT Bottom,
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT BitsPerPixel,
    IN gctPOINTER Bits
    )
{
    DIBSECTION dib;

    /* Get bitmap information. */
    gcoOS_ZeroMemory(&dib, sizeof(dib));
    dib.dsBmih.biSize = sizeof(dib.dsBmih);

    if (GetObject(Pixmap, sizeof(dib), &dib) == 0)
    {
        /* Invalid bitmap. */
        return gcvFALSE;
    }

    if (dib.dsBm.bmBits == gcvNULL)
    {
        gctBOOL ret = gcvFALSE;
		BITFIELDINFO bfi;
        PBITMAPINFOHEADER bm = &bfi.bmi.bmiHeader;
        gctUINT32 *mask = (gctUINT32*)(bm + 1);
        HGDIOBJ hBitmap = gcvNULL;
        HDC hdcMem = gcvNULL;

		do
		{
			hdcMem = CreateCompatibleDC(Display->hdc);
			if (hdcMem == gcvNULL)
			{
				break;
			}

			hBitmap = SelectObject(hdcMem, Pixmap);
			if (hBitmap == gcvNULL)
			{
				break;
			}

			gcoOS_ZeroMemory(bm, sizeof(BITMAPINFOHEADER));

			if (BitsPerPixel == 32)
			{
				mask[0] = 0x00FF0000;
				mask[1] = 0x0000FF00;
				mask[2] = 0x000000FF;
			}
			else if (BitsPerPixel == 16)
			{
				mask[0] = 0x0000F800;
				mask[1] = 0x000007E0;
				mask[2] = 0x0000001F;
			}
			else
			{
				break;
			}

			bm->biSize           = sizeof(bfi.bmi.bmiHeader);
			bm->biWidth          = Width;
			bm->biHeight         = -Height;
			bm->biPlanes         = 1;
			bm->biCompression    = BI_BITFIELDS;
			bm->biBitCount       = (WORD)BitsPerPixel;
			bm->biSizeImage      = (BitsPerPixel * Width * Height) << 3;
			bm->biXPelsPerMeter  = 0;
			bm->biYPelsPerMeter  = 0;
			bm->biClrUsed        = 0;
			bm->biClrImportant   = 0;

			ret = SetDIBitsToDevice(
				hdcMem,
				0, 0, Width, Height,
				0, 0, Top, Bottom - Top,
				Bits,
				(BITMAPINFO*)bm,
				DIB_RGB_COLORS
				) ? gcvTRUE : gcvFALSE;

		} while (gcvFALSE);

		if (hBitmap)
		{
			SelectObject(hdcMem, hBitmap);
		}

		if (hdcMem)
		{
			DeleteDC(hdcMem);
		}

        return ret;
    }
    else
    {
        return gcvFALSE;
    }
}

gctBOOL
veglGetPixmapInfo(
    IN NativeDisplayType Display,
    IN NativePixmapType Pixmap,
    OUT gctUINT_PTR Width,
    OUT gctUINT_PTR Height,
    OUT gctUINT_PTR BitsPerPixel,
    OUT gceSURF_FORMAT * Format
    )
{
    DIBSECTION dib;

    /* Get bitmap information. */
    gcoOS_ZeroMemory(&dib, sizeof(dib));
    dib.dsBmih.biSize = sizeof(dib.dsBmih);
    if (GetObject(Pixmap, sizeof(dib), &dib) == 0)
    {
        /* Invalid bitmap. */
        return gcvFALSE;
    }

    /* Return info to caller. */
    if(Width != gcvNULL)
        *Width = dib.dsBm.bmWidth;
    if(Height != gcvNULL)
        *Height = dib.dsBm.bmHeight;
    if(BitsPerPixel != gcvNULL)
        *BitsPerPixel = dib.dsBm.bmBitsPixel;

    if(Format != gcvNULL)
    {
        gctUINT r, g, b;

        r = dib.dsBitfields[0];
        g = dib.dsBitfields[1];
        b = dib.dsBitfields[2];

        switch (dib.dsBm.bmBitsPixel)
        {
        case 16:
            if ((r == 0x7C00) && (g == 0x03E0) && (b == 0x001F))
            {
                *Format = gcvSURF_X1R5G5B5;
            }
            else if ((r == 0x0F00) && (g == 0x00F0) && (b == 0x000F))
            {
                *Format = gcvSURF_X4R4G4B4;
            }
            else
            {
                *Format = gcvSURF_R5G6B5;
            }
            break;

        case 32:
            *Format = gcvSURF_X8R8G8B8;
            break;

        default:
            return gcvFALSE;
        }
    }

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglGetPixmapBits(
    IN NativeDisplayType Display,
    IN NativePixmapType Pixmap,
    OUT gctPOINTER * Bits,
    OUT gctINT_PTR Stride,
    OUT gctUINT32_PTR Physical
    )
{
    DIBSECTION dib;

    /* Get bitmap information. */
    gcoOS_ZeroMemory(&dib, sizeof(dib));
    dib.dsBmih.biSize = sizeof(dib.dsBmih);

    if (GetObject(Pixmap, sizeof(dib), &dib) == 0)
    {
        /* Invalid bitmap. */
        return gcvFALSE;
    }

    /* Return info to caller. */
    *Bits   = dib.dsBm.bmBits;
    *Stride = dib.dsBm.bmWidthBytes;

    if (Physical != gcvNULL)
    {
        /* No physical address for pixmaps. */
        *Physical = ~0U;
    }

#ifdef UNDER_CE
    if (dib.dsBm.bmBits)
    {
        CacheRangeFlush(dib.dsBm.bmBits, dib.dsBm.bmWidthBytes * dib.dsBm.bmHeight, CACHE_SYNC_ALL);
    }
#endif

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglCopyPixmapBits(
    IN VEGLDisplay Display,
    IN NativePixmapType Pixmap,
    IN gctUINT DstWidth,
    IN gctUINT DstHeight,
    IN gctINT DstStride,
    IN gceSURF_FORMAT DstFormat,
    OUT gctPOINTER DstBits
    )
{
    DIBSECTION dib;
    WORD bpp;

    /* Get bitmap information. */
    gcoOS_ZeroMemory(&dib, sizeof(dib));
    dib.dsBmih.biSize = sizeof(dib.dsBmih);

    if ((DstBits == gcvNULL) || (GetObject(Pixmap, sizeof(dib), &dib) == 0))
    {
        /* Invalid bitmap. */
        return gcvFALSE;
    }

    switch (DstFormat)
    {
    case gcvSURF_R5G6B5:
        bpp = 16;
        break;

    case gcvSURF_X8R8G8B8:
    case gcvSURF_A8R8G8B8:
        bpp = 32;
        break;

    default:
        return gcvFALSE;
    }

    if (dib.dsBm.bmBits == gcvNULL)
    {
        gctBOOL ret = gcvFALSE;
        HGDIOBJ hBitmap = gcvNULL;
        HGDIOBJ hBitmapBackSrc = gcvNULL;
        HGDIOBJ hBitmapBackDst = gcvNULL;
        HDC hdcSrc = gcvNULL;
        HDC hdcDst = gcvNULL;

        do {
		    BITFIELDINFO bfi;
            PBITMAPINFOHEADER bm = &bfi.bmi.bmiHeader;
            gctUINT32 *mask = (gctUINT32*)(bm + 1);
            gctPOINTER bits = gcvNULL;

            hdcSrc = CreateCompatibleDC(Display->hdc);
            if (!hdcSrc)
            {
                break;
            }

            hdcDst = CreateCompatibleDC(Display->hdc);
            if (!hdcDst)
            {
                break;
            }

            gcoOS_ZeroMemory(bm, sizeof(BITMAPINFOHEADER));

			if (bpp == 32)
			{
				mask[0] = 0x00FF0000;
				mask[1] = 0x0000FF00;
				mask[2] = 0x000000FF;
			}
			else if (bpp == 16)
			{
				mask[0] = 0x0000F800;
				mask[1] = 0x000007E0;
				mask[2] = 0x0000001F;
			}
			else
			{
				break;
			}

			bm->biSize           = sizeof(bfi.bmi.bmiHeader);
			bm->biWidth          = (gctINT)DstWidth;
			bm->biHeight         = -(gctINT)DstHeight;
			bm->biPlanes         = 1;
			bm->biCompression    = BI_BITFIELDS;
			bm->biBitCount       = bpp;
			bm->biSizeImage      = 0;
			bm->biXPelsPerMeter  = 0;
			bm->biYPelsPerMeter  = 0;
			bm->biClrUsed        = 0;
			bm->biClrImportant   = 0;

            hBitmap = CreateDIBSection(hdcDst,(BITMAPINFO*)bm, DIB_RGB_COLORS, &bits, NULL, 0);
            if (!hBitmap || !bits)
            {
                break;
            }

            hBitmapBackDst = SelectObject(hdcDst, hBitmap);
            if (!hBitmapBackDst)
            {
                break;
            }

            hBitmapBackSrc = SelectObject(hdcSrc, Pixmap);
            if (!hBitmapBackSrc)
            {
                break;
            }

            if (!BitBlt(hdcDst, 0, 0, DstWidth, DstHeight, hdcSrc, 0, 0, SRCCOPY))
            {
                break;
            }

            gcoOS_ZeroMemory(&dib, sizeof(dib));
            dib.dsBmih.biSize = sizeof(dib.dsBmih);
            if (!GetObject(hBitmap, sizeof(dib), &dib))
            {
                break;
            }

            if (dib.dsBm.bmWidthBytes == DstStride)
            {
                gcoOS_MemCopy(DstBits, bits, (bpp * DstWidth * DstHeight) >> 3);
            }
            else
            {
                gctUINT n;
                gctUINT8* src = (gctUINT8*)bits;
                gctUINT8* dst = (gctUINT8*)DstBits;
                gctINT stride = gcmMIN(DstStride, dib.dsBm.bmWidthBytes);

                for (n = 0; n < DstHeight; n++)
                {
                    gcoOS_MemCopy(dst, src, stride);

                    src += dib.dsBm.bmWidthBytes;
                    dst += DstStride;
                }
            }

            ret = gcvTRUE;

        } while (gcvFALSE);

        if (hBitmapBackDst)
        {
            SelectObject(hdcDst, hBitmapBackDst);
        }

        if (hBitmapBackSrc)
        {
            SelectObject(hdcSrc, hBitmapBackSrc);
        }

        if (hBitmap)
        {
            DeleteObject(hBitmap);
        }

        if (hdcDst)
        {
            DeleteDC(hdcDst);
        }

        if (hdcSrc)
        {
            DeleteDC(hdcSrc);
        }

        return ret;
    }
    else
    {
        if (dib.dsBm.bmWidthBytes == DstStride)
        {
            gcoOS_MemCopy(DstBits, dib.dsBm.bmBits, (bpp * DstWidth * DstHeight) >> 3);
        }
        else
        {
            gctUINT n;
            gctUINT8* src = (gctUINT8*)dib.dsBm.bmBits;
            gctUINT8* dst = (gctUINT8*)DstBits;
            gctINT stride = gcmMIN(DstStride, dib.dsBm.bmWidthBytes);

            for (n = 0; n < DstHeight; n++)
            {
                gcoOS_MemCopy(dst, src, stride);

                src += dib.dsBm.bmWidthBytes;
                dst += DstStride;
            }
        }

        return gcvTRUE;
    }
}

gctBOOL
veglGetWindowBits(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    OUT gctPOINTER * Logical,
    OUT gctUINT32_PTR Physical,
    OUT gctINT32_PTR Stride
    )
{
    return gcvFALSE;
}

gctBOOL
veglIsColorFormatSupport(
    IN NativeDisplayType Display,
    IN VEGLConfig       Config
    )
{
    return gcvTRUE;
}

