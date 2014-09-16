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

#include <windows.h>
#include <ddraw.h>


#ifdef USE_CE_DIRECTDRAW

// Set WRAP_FULLSCREEN_WINDOW to 1 if there is no GC2D accelerated GDI
#define WRAP_FULLSCREEN_WINDOW 0

typedef struct __BITFIELDINFO{
    BITMAPINFO    bmi;
    RGBQUAD       bmiColors[2];
} BITFIELDINFO;

typedef struct __veglLocalDDSurf
{
    struct eglBackBuffer eglBackBuf;
    gctPOINTER           ddSurface;
    struct __veglLocalDDSurf *next;
} veglLocalDDSurf, *veglLocalDDSurf_PTR;

typedef struct __veglLocalDisplay
{
    IDirectDraw *pDD;
    IDirectDrawSurface *pDDS;
    gctPOINTER  logical;
    gctUINT     physical;
    gctINT      stride;
    gctINT      width;
    gctINT      height;
    RECT        rect;
    veglLocalDDSurf_PTR localBackSurf;
} veglLocalDisplay;

static gctBOOL
GetDDSFormat(LPDDPIXELFORMAT DdpFormat, gceSURF_FORMAT *Format, gctUINT32 *Bpp)
{
    gceSURF_FORMAT format;

    if (DdpFormat->dwFlags != DDPF_RGB)
    {
        return gcvFALSE;
    }

    switch (DdpFormat->dwRGBBitCount)
    {
    case 16:
        format = gcvSURF_R5G6B5;
        break;

    case 32:
        format = gcvSURF_X8R8G8B8;
        break;

    default:
        return gcvFALSE;
    }

    if (Format)
    {
        *Format = format;
    }

    if (Bpp)
    {
        *Bpp = DdpFormat->dwRGBBitCount;
    }

    return gcvTRUE;
}

static veglLocalDDSurf*
CreateBackSurface(IDirectDrawSurface* pDDSurface)
{
    gcoSURF surf = gcvNULL;
    veglLocalDDSurf *lsurf = gcvNULL;

    do {
        DDSURFACEDESC dds;
        gceSTATUS status;

        gcoOS_ZeroMemory(&dds, sizeof(dds));

        dds.dwSize = sizeof(dds);

        if (FAILED(pDDSurface->Lock(NULL,
                &dds,
                DDLOCK_WAITNOTBUSY | DDLOCK_WRITEONLY,
                NULL)))
        {
            break;
        }

        pDDSurface->Unlock(NULL);

        if (!dds.lpSurface)
        {
            break;
        }

        gceSURF_FORMAT format;
        if (!GetDDSFormat(&dds.ddpfPixelFormat, &format, gcvNULL))
        {
            break;
        }

        gcmERR_BREAK(gcoSURF_ConstructWrapper(
            gcvNULL,
            &surf
            ));

        /* Set the underlying frame buffer surface. */
        gcmERR_BREAK(gcoSURF_SetBuffer(
            surf,
            gcvSURF_BITMAP,
            format,
            dds.lPitch,
            dds.lpSurface,
            ~0U
            ));

        /* Set the window. */
        gcmERR_BREAK(gcoSURF_SetWindow(
            surf,
            0,
            0,
            dds.dwWidth,
            dds.dwHeight
            ));

        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, sizeof(veglLocalDDSurf), (gctPOINTER*)&lsurf));

        gcoOS_ZeroMemory(lsurf, sizeof(veglLocalDDSurf));

        lsurf->ddSurface          = (gctPOINTER)pDDSurface;
        lsurf->eglBackBuf.surface = surf;
        lsurf->eglBackBuf.context = dds.lpSurface;

        return lsurf;

    } while (gcvFALSE);

    if (surf)
    {
        gcoSURF_Destroy(surf);
    }

    return gcvNULL;
}

static gctBOOL
DestroyBackSufaces(veglLocalDDSurf* lSurf)
{

    while (lSurf)
    {
        veglLocalDDSurf *next;

        next = lSurf->next;
        if (lSurf->eglBackBuf.surface)
        {
            gcoSURF_Destroy(lSurf->eglBackBuf.surface);
        }

        gcoOS_Free(gcvNULL, lSurf);
        lSurf = next;
    }

    return gcvTRUE;
}

static gctBOOL
CreateBackSufaces(veglLocalDisplay *veglLocalDisplay, IDirectDrawSurface* pDDSurface)
{
    veglLocalDDSurf * lsurf;

    lsurf = CreateBackSurface(pDDSurface);
    if (!lsurf)
    {
        return gcvFALSE;
    }

    lsurf->next = veglLocalDisplay->localBackSurf;
    veglLocalDisplay->localBackSurf = lsurf;

    return gcvTRUE;
}

static gctBOOL
IsSupportedDDSurface(void *pDDSurface, gctUINT32 *Caps)
{
    if (IsWindow((HWND)pDDSurface))
    {
        return gcvFALSE;
    }

    DDSCAPS dwsCap;
    IDirectDrawSurface *pDDS = (IDirectDrawSurface*)pDDSurface;

    if(FAILED(pDDS->GetCaps(&dwsCap)))
    {
        return gcvFALSE;
    }
    else
    {
        if (Caps)
        {
            *Caps = dwsCap.dwCaps;
        }

        return gcvTRUE;
    }
}

gctBOOL
veglInitLocalDisplayInfo(
    VEGLDisplay Display
    )
{
    veglLocalDisplay *ldisplay = (veglLocalDisplay*)Display->localInfo;

    do {
        if (ldisplay == gcvNULL)
        {
            if (gcmIS_ERROR(gcoOS_Allocate(
                    gcvNULL, gcmSIZEOF(veglLocalDisplay), (gctPOINTER*)&ldisplay)))
            {
                break;
            }

            gcoOS_ZeroMemory(ldisplay, gcmSIZEOF(veglLocalDisplay));

#if WRAP_FULLSCREEN_WINDOW
            if (FAILED(DirectDrawCreate(NULL, &ldisplay->pDD, NULL)))
            {
                break;
            }

            SystemParametersInfo(SPI_GETWORKAREA, 0, &ldisplay->rect, FALSE);
#endif
            Display->localInfo = ldisplay;
        }

        return gcvTRUE;

    } while (gcvFALSE);

    if (ldisplay)
    {
        if (ldisplay->pDD)
        {
            ldisplay->pDD->Release();
        }

        gcoOS_Free(gcvNULL, ldisplay);
    }

    Display->localInfo = gcvNULL;

    return gcvFALSE;
}

gctBOOL
veglDeinitLocalDisplayInfo(
    VEGLDisplay Display
    )
{
    veglLocalDisplay *ldisplay = (veglLocalDisplay*)Display->localInfo;

    if (ldisplay)
    {
        if (ldisplay->localBackSurf)
        {
            DestroyBackSufaces(ldisplay->localBackSurf);
            ldisplay->localBackSurf = gcvNULL;
        }
#if WRAP_FULLSCREEN_WINDOW
        if (ldisplay->pDDS)
        {
            if (ldisplay->logical)
            {
                ldisplay->pDDS->Unlock(gcvNULL);
            }

            ldisplay->pDDS->Release();
        }

        if (ldisplay->pDD)
        {
            ldisplay->pDD->Release();
        }
#endif
        gcoOS_Free(gcvNULL, ldisplay);
    }

    Display->localInfo = gcvNULL;

    return gcvTRUE;
}

gctBOOL
veglGetDisplayInfo(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT VEGLDisplayInfo Info
    )
{
    veglLocalDisplay *ldisplay = (veglLocalDisplay*)Display->localInfo;

    do {
        if (!ldisplay)
        {
            break;
        }

        if (IsWindow(Window))
        {
#if WRAP_FULLSCREEN_WINDOW
            RECT rect;

            /* Check whether fullscreen. */
            if (!GetClientRect(Window, &rect))
            {
                break;
            }

            if (!ClientToScreen(Window, (LPPOINT)&rect)
                || !ClientToScreen(Window, ((LPPOINT)&rect) + 1))
            {
                break;
            }

            if ((rect.left > ldisplay->rect.left)
                || (rect.top > ldisplay->rect.top)
                || (rect.right < ldisplay->rect.right)
                || (rect.bottom < ldisplay->rect.bottom))
            {
                break;
            }

            if (FAILED(ldisplay->pDD->SetCooperativeLevel(Window, DDSCL_FULLSCREEN)))
            {
                break;
            }

            if (ldisplay->pDDS == gcvNULL)
            {
                /* create primary surface. */

                DDSURFACEDESC dds;

                gcoOS_ZeroMemory(&dds, sizeof(dds));

                dds.dwSize = sizeof(dds);

                dds.dwFlags = DDSD_CAPS;

                dds.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

                if (FAILED(ldisplay->pDD->CreateSurface(&dds,&ldisplay->pDDS, NULL)))
                {
                    break;
                }

                if (FAILED(ldisplay->pDDS->Lock(NULL,
                        &dds,
                        DDLOCK_WAITNOTBUSY | DDLOCK_WRITEONLY,
                        NULL)))
                {
                    break;
                }

                if (!dds.lpSurface)
                {
                    ldisplay->pDDS->Unlock(NULL);
                    break;
                }

                ldisplay->logical = dds.lpSurface;
                ldisplay->physical = ~0U;
                ldisplay->stride = dds.lPitch;
                ldisplay->width = dds.dwWidth;
                ldisplay->height = dds.dwHeight;
            }

            Info->logical = ldisplay->logical;
            Info->physical = ldisplay->physical;
            Info->stride = ldisplay->stride ;
            Info->flip  = gcvFALSE;
            Info->width = ldisplay->width;
            Info->height = ldisplay->height;

            return gcvTRUE;
#else
            return gcvFALSE;
#endif
        }
        else if (IsSupportedDDSurface(Window, gcvNULL))
        {
            IDirectDrawSurface *pDDS = (IDirectDrawSurface*)Window;
            DDSURFACEDESC dds;

            gcoOS_ZeroMemory(&dds, sizeof(dds));

            dds.dwSize = sizeof(dds);

            if (FAILED(pDDS->Lock(NULL,
                    &dds,
                    DDLOCK_WAITNOTBUSY | DDLOCK_WRITEONLY,
                    NULL)))
            {
                break;
            }

            pDDS->Unlock(NULL);

            if (!dds.lpSurface)
            {
                break;
            }

            Info->logical = dds.lpSurface;
            Info->physical = ~0U;
            Info->stride = dds.lPitch;
            Info->width = dds.dwWidth;
            Info->height = dds.dwHeight;

            if ((dds.ddsCaps.dwCaps & DDSCAPS_FLIP) == DDSCAPS_FLIP)
            {
                if (!CreateBackSufaces(ldisplay, pDDS))
                {
                    break;
                }

                Info->flip = gcvTRUE;
            }
            else
            {
                Info->flip = gcvFALSE;
            }

            return gcvTRUE;
        }

    } while (gcvFALSE);

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
    if (IsWindow(Window))
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
    else if (IsSupportedDDSurface(Window, gcvNULL))
    {
        IDirectDrawSurface *pDDS = (IDirectDrawSurface*)Window;
        DDSURFACEDESC		ddsd;

        gcoOS_ZeroMemory(&ddsd, sizeof(ddsd));

        ddsd.dwSize = sizeof(ddsd);

        if (FAILED(pDDS->GetSurfaceDesc(&ddsd)))
        {
            return gcvFALSE;
        }

        if (!GetDDSFormat(&ddsd.ddpfPixelFormat, Format, BitsPerPixel))
        {
            return gcvFALSE;
        }

        if(X != gcvNULL)
            * X = 0;
        if(Y != gcvNULL)
            * Y = 0;
        if(Width != gcvNULL)
            * Width = ddsd.dwWidth;
        if(Height != gcvNULL)
            * Height = ddsd.dwHeight;

        return gcvTRUE;
    }

    return gcvFALSE;
}

static HRESULT WINAPI
EnumGetBackBufferCallback(LPDIRECTDRAWSURFACE lpDDSurface,
                     LPDDSURFACEDESC lpDDSurfaceDesc,
                     LPVOID lpContext)
{
    IDirectDrawSurface **pDDSBack = (IDirectDrawSurface**)lpContext;

    if (!(*pDDSBack))
    {
        *pDDSBack = lpDDSurface;
    }

    return DDENUMRET_OK;
}

gctBOOL
veglGetDisplayBackBuffer(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT VEGLBackBuffer BackBuffer,
    OUT gctBOOL_PTR Flip
    )
{
    gctUINT32 caps;

    do {
        if (!IsSupportedDDSurface(Window, &caps))
        {
            break;
        }

        if ((caps & DDSCAPS_FLIP) != DDSCAPS_FLIP)
        {
            break;
        }

        veglLocalDisplay *ldisplay = (veglLocalDisplay*)Display->localInfo;
        veglLocalDDSurf *lSurf = ldisplay->localBackSurf;

        while (lSurf)
        {
            if (lSurf->ddSurface == Window)
            {
                break;
            }

            lSurf = lSurf->next;
        }

        if (!lSurf)
        {
            break;
        }

        IDirectDrawSurface *pDDS = (IDirectDrawSurface*)Window;
        IDirectDrawSurface *pDDSBack = gcvNULL;
        if (FAILED(pDDS->EnumAttachedSurfaces((LPVOID)&pDDSBack,EnumGetBackBufferCallback)))
        {
            break;
        }

        if (!pDDSBack)
        {
            break;
        }

        DDSURFACEDESC dds;
        gceSTATUS status;

        gcoOS_ZeroMemory(&dds, sizeof(dds));

        dds.dwSize = sizeof(dds);

        if (FAILED(pDDSBack->Lock(NULL,
                &dds,
                DDLOCK_WAITNOTBUSY | DDLOCK_WRITEONLY,
                NULL)))
        {
            break;
        }

        pDDSBack->Unlock(NULL);

        if (!dds.lpSurface)
        {
            break;
        }

        if (dds.lpSurface != lSurf->eglBackBuf.context)
        {
            gceSURF_FORMAT format;
            if (!GetDDSFormat(&dds.ddpfPixelFormat, &format, gcvNULL))
            {
                break;
            }

            /* update the underlying frame buffer surface. */
            gcmERR_BREAK(gcoSURF_SetBuffer(
                lSurf->eglBackBuf.surface,
                gcvSURF_BITMAP,
                format,
                dds.lPitch,
                dds.lpSurface,
                ~0U
                ));

            /* Set the window. */
            gcmERR_BREAK(gcoSURF_SetWindow(
                lSurf->eglBackBuf.surface,
                0,
                0,
                dds.dwWidth,
                dds.dwHeight
                ));

            lSurf->eglBackBuf.context = dds.lpSurface;
        }

        *BackBuffer = lSurf->eglBackBuf;
        *Flip = gcvTRUE;
        return gcvTRUE;

    } while (gcvFALSE);

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
    gctUINT32 caps;

    if (!IsSupportedDDSurface(Window, &caps))
    {
        return gcvFALSE;
    }

    if ((caps & DDSCAPS_FLIP) != DDSCAPS_FLIP)
    {
        return gcvFALSE;
    }

    IDirectDrawSurface *pDDS = (IDirectDrawSurface*)Window;
    pDDS->Flip(NULL, 0);

    return gcvTRUE;
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
    IDirectDrawSurface *pDDS = gcvNULL;

    do
    {
        gctINT count;
        gctINT width, height;
        gceORIENTATION orientation;
        BITFIELDINFO bfi;
        PBITMAPINFOHEADER info = &bfi.bmi.bmiHeader;
        gctUINT32 *mask = (gctUINT32*)(info + 1);

        /* Get the device context of the window. */
        if (IsWindow(Surface->hwnd))
        {
            context = GetDC(Surface->hwnd);
        }
        else if (IsSupportedDDSurface(Surface->hwnd, gcvNULL))
        {
            pDDS = (IDirectDrawSurface*)Surface->hwnd;
            if (FAILED(pDDS->GetDC(&context)))
                break;
        }

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
        if (pDDS)
        {
            pDDS->ReleaseDC(context);
        }
        else
        {
            ReleaseDC(Surface->hwnd, context);
        }
    }

    /* Return result. */
    return result;
}
#endif

