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



#ifndef VIVANTE_NO_3D
#include "gc_hal_user_precomp.h"
#include "gc_hal_eglplatform.h"

void gcoOS_Linkage(void)
{
	gcoOS_GetDisplay(gcvNULL);
    gcoOS_GetDisplayByIndex(0, gcvNULL);
	gcoOS_GetDisplayInfo(gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL);
	gcoOS_GetDisplayInfoEx(gcvNULL, (HALNativeWindowType)gcvNULL, 0, gcvNULL);
	gcoOS_GetDisplayVirtual(gcvNULL, gcvNULL, gcvNULL);
	gcoOS_GetDisplayBackbuffer(gcvNULL, (HALNativeWindowType)gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL);
    gcoOS_SetDisplayVirtual(gcvNULL, (HALNativeWindowType)gcvNULL, 0, 0, 0);
	gcoOS_DestroyDisplay(gcvNULL);

	gcoOS_CreateWindow(gcvNULL, 0, 0, 0, 0, gcvNULL);
	gcoOS_GetWindowInfo(gcvNULL, (HALNativeWindowType)gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL);
	gcoOS_DestroyWindow(gcvNULL, (HALNativeWindowType)gcvNULL);

	gcoOS_DrawImage(gcvNULL, (HALNativeWindowType)gcvNULL, 0, 0, 0, 0, 0, 0, 0, gcvNULL);
    gcoOS_GetImage((HALNativeWindowType)gcvNULL, 0, 0, 0, 0, gcvNULL, gcvNULL);

	gcoOS_CreatePixmap(gcvNULL, 0, 0, 0, gcvNULL);
	gcoOS_GetPixmapInfo(gcvNULL, (HALNativePixmapType)gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL);
	gcoOS_DrawPixmap(gcvNULL, (HALNativePixmapType)gcvNULL, 0, 0, 0, 0, 0, 0, 0, gcvNULL);
	gcoOS_DestroyPixmap(gcvNULL, (HALNativePixmapType)gcvNULL);

    gcoOS_LoadEGLLibrary(gcvNULL);
    gcoOS_FreeEGLLibrary(gcvNULL);
    gcoOS_ShowWindow(gcvNULL, (HALNativeWindowType)gcvNULL);
    gcoOS_HideWindow(gcvNULL, (HALNativeWindowType)gcvNULL);
    gcoOS_SetWindowTitle(gcvNULL, (HALNativeWindowType)gcvNULL, gcvNULL);
    gcoOS_CapturePointer(gcvNULL, (HALNativeWindowType)gcvNULL);
    gcoOS_GetEvent(gcvNULL, (HALNativeWindowType)gcvNULL, gcvNULL);
    gcoOS_CreateClientBuffer(0, 0, 0, 0, gcvNULL);
    gcoOS_GetClientBufferInfo(gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL);
    gcoOS_DestroyClientBuffer(gcvNULL);
}
#endif

