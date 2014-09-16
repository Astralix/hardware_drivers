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


#define  WINVER			0x0600
#define _WIN32_WINNT	0x0600
#define _WIN32_WINDOWS	0x0410
#define _WIN32_IE		0x0700

#include <stdio.h>
#include <tchar.h>
#include <afx.h>
#include <math.h>
#include "Generator.h"
#include "Formats.h"

#define vgmDEFINE_COMPONENT(Width, Start) \
{ \
	Width, \
	Start, \
	(1 << Width) - 1 \
}

static vgsFORMAT_INFO _upsampledGroup[] =
{
	{
		/* Name:             */	"lL_8",
		/* Internal format:  */	"gcvSURF_L8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	8,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(0,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(0,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(8,  0)
	},
	{
		/* Name:             */	"A_4",
		/* Internal format:  */	"gcvSURF_A4",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	4,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(0,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(0,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(4,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	}
};

static vgsFORMAT_LIST _upsampledGroupList[] =
{
	{ _upsampledGroup, gcmCOUNTOF(_upsampledGroup) },
};

static vgsFORMAT_INFO _rgbaGroup[] =
{
	{
		/* Name:             */	"sRGBX_8888",
		/* Internal format:  */	"gcvSURF_R8G8B8X8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 24),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  8),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sRGBA_8888",
		/* Internal format:  */	"gcvSURF_R8G8B8A8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 24),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  8),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sRGBA_8888_PRE",
		/* Internal format:  */	"gcvSURF_R8G8B8A8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvTRUE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 24),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  8),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sRGB_565",
		/* Internal format:  */	"gcvSURF_R5G6B5",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	16,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(5, 11),
		/* Green channel:    */	vgmDEFINE_COMPONENT(6,  5),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(5,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sRGBA_5551",
		/* Internal format:  */	"gcvSURF_R5G5B5A1",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	16,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(5, 11),
		/* Green channel:    */	vgmDEFINE_COMPONENT(5,  6),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(5,  1),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(1,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sRGBA_4444",
		/* Internal format:  */	"gcvSURF_R4G4B4A4",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	16,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(4, 12),
		/* Green channel:    */	vgmDEFINE_COMPONENT(4,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(4,  4),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(4,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sL_8",
		/* Internal format:  */	"gcvSURF_L8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	8,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(0,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(0,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(8,  0)
	},
	{
		/* Name:             */	"lRGBX_8888",
		/* Internal format:  */	"gcvSURF_R8G8B8X8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 24),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  8),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"lRGBA_8888",
		/* Internal format:  */	"gcvSURF_R8G8B8A8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 24),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  8),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"lRGBA_8888_PRE",
		/* Internal format:  */	"gcvSURF_R8G8B8A8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvTRUE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 24),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  8),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"lL_8",
		/* Internal format:  */	"gcvSURF_L8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	8,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(0,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(0,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(8,  0)
	},
	{
		/* Name:             */	"A_8",
		/* Internal format:  */	"gcvSURF_A8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	8,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(0,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(0,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"BW_1",
		/* Internal format:  */	"gcvSURF_L1",
		/* Upsampled format: */	"lL_8",
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	1,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(0,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(0,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(1,  0)
	},
	{
		/* Name:             */	"A_1",
		/* Internal format:  */	"gcvSURF_A1",
		/* Upsampled format: */	"A_4",
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	1,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(0,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(0,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(1,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"A_4",
		/* Internal format:  */	"gcvSURF_A4",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	4,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(0,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(0,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(4,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"lRGB_565",
		/* Internal format:  */	"gcvSURF_R5G6B5",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvFALSE,
		/* Bits per pixel:   */	16,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(5, 11),
		/* Green channel:    */	vgmDEFINE_COMPONENT(6,  5),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(5,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	}
};

static vgsFORMAT_INFO _argbGroup[] =
{
	{
		/* Name:             */	"sXRGB_8888",
		/* Internal format:  */	"gcvSURF_X8R8G8B8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 16),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sARGB_8888",
		/* Internal format:  */	"gcvSURF_A8R8G8B8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 16),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sARGB_8888_PRE",
		/* Internal format:  */	"gcvSURF_A8R8G8B8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvTRUE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 16),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		gcvNULL
	},
	{
		/* Name:             */	"sARGB_1555",
		/* Internal format:  */	"gcvSURF_A1R5G5B5",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	16,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(5, 10),
		/* Green channel:    */	vgmDEFINE_COMPONENT(5,  5),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(5,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(1, 15),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sARGB_4444",
		/* Internal format:  */	"gcvSURF_A4R4G4B4",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	16,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(4,  8),
		/* Green channel:    */	vgmDEFINE_COMPONENT(4,  4),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(4,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(4, 12),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		gcvNULL
	},
	{
		/* Name:             */	"lXRGB_8888",
		/* Internal format:  */	"gcvSURF_X8R8G8B8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 16),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"lARGB_8888",
		/* Internal format:  */	"gcvSURF_A8R8G8B8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 16),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"lARGB_8888_PRE",
		/* Internal format:  */	"gcvSURF_A8R8G8B8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvTRUE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8, 16),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8,  0),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	}
};

static vgsFORMAT_INFO _bgraGroup[] =
{
	{
		/* Name:             */	"sBGRX_8888",
		/* Internal format:  */	"gcvSURF_B8G8R8X8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  8),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 24),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sBGRA_8888",
		/* Internal format:  */	"gcvSURF_B8G8R8A8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  8),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 24),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sBGRA_8888_PRE",
		/* Internal format:  */	"gcvSURF_B8G8R8A8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvTRUE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  8),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 24),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sBGR_565",
		/* Internal format:  */	"gcvSURF_B5G6R5",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	16,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(5,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(6,  5),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(5, 11),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sBGRA_5551",
		/* Internal format:  */	"gcvSURF_B5G5R5A1",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	16,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(5,  1),
		/* Green channel:    */	vgmDEFINE_COMPONENT(5,  6),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(5, 11),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(1,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sBGRA_4444",
		/* Internal format:  */	"gcvSURF_B4G4R4A4",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	16,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(4,  4),
		/* Green channel:    */	vgmDEFINE_COMPONENT(4,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(4, 12),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(4,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		gcvNULL
	},
	{
		/* Name:             */	"lBGRX_8888",
		/* Internal format:  */	"gcvSURF_B8G8R8X8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  8),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 24),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"lBGRA_8888",
		/* Internal format:  */	"gcvSURF_B8G8R8A8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  8),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 24),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"lBGRA_8888_PRE",
		/* Internal format:  */	"gcvSURF_B8G8R8A8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvTRUE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  8),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8, 16),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 24),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8,  0),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	}
};

static vgsFORMAT_INFO _abgrGroup[] =
{
	{
		/* Name:             */	"sXBGR_8888",
		/* Internal format:  */	"gcvSURF_X8B8G8R8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 16),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sABGR_8888",
		/* Internal format:  */	"gcvSURF_A8B8G8R8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 16),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sABGR_8888_PRE",
		/* Internal format:  */	"gcvSURF_A8B8G8R8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvTRUE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 16),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		gcvNULL
	},
	{
		/* Name:             */	"sABGR_1555",
		/* Internal format:  */	"gcvSURF_A1B5G5R5",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	16,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(5,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(5,  5),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(5, 10),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(1, 15),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"sABGR_4444",
		/* Internal format:  */	"gcvSURF_A4B4G4R4",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	16,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvFALSE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(4,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(4,  4),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(4,  8),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(4, 12),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		gcvNULL
	},
	{
		/* Name:             */	"lXBGR_8888",
		/* Internal format:  */	"gcvSURF_X8B8G8R8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 16),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(0, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"lABGR_8888",
		/* Internal format:  */	"gcvSURF_A8B8G8R8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvFALSE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 16),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		/* Name:             */	"lABGR_8888_PRE",
		/* Internal format:  */	"gcvSURF_A8B8G8R8",
		/* Upsampled format: */	gcvNULL,
		/* Native format:    */	gcvTRUE,
		/* Bits per pixel:   */	32,
		/* Premultipiled:    */	gcvTRUE,
		/* Linear:           */	gcvTRUE,
		/* Red channel:      */	vgmDEFINE_COMPONENT(8,  0),
		/* Green channel:    */	vgmDEFINE_COMPONENT(8,  8),
		/* Blue channel:     */	vgmDEFINE_COMPONENT(8, 16),
		/* Alpha channel:    */	vgmDEFINE_COMPONENT(8, 24),
		/* Luminance:        */	vgmDEFINE_COMPONENT(0,  0)
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	},
	{
		gcvNULL
	}
};

static vgsFORMAT_LIST _groupList[] =
{
	{ _rgbaGroup, gcmCOUNTOF(_rgbaGroup) },
	{ _argbGroup, gcmCOUNTOF(_argbGroup) },
	{ _bgraGroup, gcmCOUNTOF(_bgraGroup) },
	{ _abgrGroup, gcmCOUNTOF(_abgrGroup) }
};

gctFLOAT vgfGetColorGamma(
	gctFLOAT Color
	)
{
	gctFLOAT result;

	result = (Color <= 0.00304f)
		? Color * 12.92f
		: 1.0556f * ((gctFLOAT) pow(Color, 1.0f / 2.4f)) - 0.0556f;

	return result;
}

gctFLOAT vgfGetColorInverseGamma(
	gctFLOAT Color
	)
{
	if (Color <= 0.03928f)
	{
		Color /= 12.92f;
	}
	else
	{
		Color = (gctFLOAT) pow((Color + 0.0556f) / 1.0556f, 2.4f);
	}

	return Color;
}

gctFLOAT vgfGetGrayScale(
	gctFLOAT Red,
	gctFLOAT Green,
	gctFLOAT Blue
	)
{
	return 0.2126f * Red + 0.7152f * Green + 0.0722f * Blue;
}

void vgfGetFormatGroupList(
	vgsFORMAT_LIST_PTR * List,
	gctUINT_PTR Count
	)
{
	* List  = _groupList;
	* Count = gcmCOUNTOF(_groupList);
}

void vgfGetUpsampledFormatList(
	vgsFORMAT_LIST_PTR * List
	)
{
	* List = _upsampledGroupList;
}
