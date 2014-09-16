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




#include "gc_hal_user_hardware_precomp.h"

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE	gcvZONE_HARDWARE

/******************************************************************************\
****************************** gcoHARDWARE API Code *****************************
\******************************************************************************/
#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**	gcoHARDWARE_QueryStreamCaps
**
**	Query the stream capabilities of the hardware.
**
**	INPUT:
**
**	OUTPUT:
**
**		gctUINT * MaxAttributes
**			Pointer to a variable that will hold the maximum number of
**			atrtributes for a primitive on success.
**
**		gctUINT * MaxStreamSize
**			Pointer to a variable that will hold the maximum number of bytes of
**			a stream on success.
**
**		gctUINT * NumberOfStreams
**			Pointer to a variable that will hold the number of streams on
**			success.
**
**		gctUINT * Alignment
**			Pointer to a variable that will receive the stream alignment
**			requirement.
*/
gceSTATUS gcoHARDWARE_QueryStreamCaps(
	OUT gctUINT32 * MaxAttributes,
	OUT gctUINT32 * MaxStreamSize,
	OUT gctUINT32 * NumberOfStreams,
	OUT gctUINT32 * Alignment
	)
{
    gceSTATUS status;
	gcoHARDWARE hardware;

    gcmHEADER_ARG("MaxAttributes=0x%x MaxStreamSize=0x%x "
			      "NumberOfStreams=0x%x Alignment=0x%x",
				  MaxAttributes, MaxStreamSize,
				  NumberOfStreams, Alignment);

    gcmGETHARDWARE(hardware);

	if (MaxAttributes != gcvNULL)
	{
		/* Return number of attributes per vertex for XAQ2. */
        if (((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 23:23) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))))
        {
		    *MaxAttributes = 16;
        }
        else
        {
		    *MaxAttributes = 10;
        }
	}

	if (MaxStreamSize != gcvNULL)
	{
		/* Return maximum number of bytes per vertex for XAQ2. */
		*MaxStreamSize = 256;
	}

	if (NumberOfStreams != gcvNULL)
	{
		/* Return number of streams for XAQ2. */
		*NumberOfStreams = hardware->streamCount;
	}

	if (Alignment != gcvNULL)
	{
		/* Return alignment. */
		*Alignment = (hardware->chipModel == gcv700) ? 128 : 8;
	}

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**	gcoHARDWARE_ConvertFormat
**
**	Convert an API format to hardware parameters.
**
**	INPUT:
**
**		gceSURF_FORMAT Format
**			API format to convert.
**
**	OUTPUT:
**
**		gctUINT32_PTR BitsPerPixel
**			Pointer to a variable that will hold the number of bits per pixel.
**
**		gctUINT32_PTR BytesPerTile
**			Pointer to a variable that will hold the number of bytes per tile.
*/
gceSTATUS gcoHARDWARE_ConvertFormat(
	IN gceSURF_FORMAT Format,
	OUT gctUINT32_PTR BitsPerPixel,
	OUT gctUINT32_PTR BytesPerTile
	)
{
	gctUINT32 bitsPerPixel = 0;
	gctUINT32 bytesPerTile = 0;
    gceSTATUS status;
	gcoHARDWARE hardware;

	gcmHEADER_ARG("Format=%d BitsPerPixel=0x%x BytesPerTile=0x%x",
				  Format, BitsPerPixel, BytesPerTile);

    gcmGETHARDWARE(hardware);

	/* Dispatch on format. */
	switch (Format)
	{
	case gcvSURF_INDEX8:
	case gcvSURF_A8:
	case gcvSURF_L8:
	case gcvSURF_R8:
		/* 8-bpp format. */
		bitsPerPixel  = 8;
		bytesPerTile  = (8 * 4 * 4) / 8;
		break;

	case gcvSURF_YV12:
	case gcvSURF_I420:
	case gcvSURF_NV12:
	case gcvSURF_NV21:
		/* 12-bpp planar YUV formats. */
		bitsPerPixel  = 12;
		bytesPerTile  = (12 * 4 * 4) / 8;
		break;

	case gcvSURF_A8L8:

    case gcvSURF_X8R8:
	case gcvSURF_G8R8:
	case gcvSURF_A8R8:

	case gcvSURF_X4R4G4B4:
	case gcvSURF_A4R4G4B4:

    case gcvSURF_R4G4B4X4:
    case gcvSURF_R4G4B4A4:

    case gcvSURF_X4B4G4R4:
    case gcvSURF_A4B4G4R4:

    case gcvSURF_B4G4R4X4:
    case gcvSURF_B4G4R4A4:

    case gcvSURF_X1R5G5B5:
	case gcvSURF_A1R5G5B5:

    case gcvSURF_R5G5B5X1:
    case gcvSURF_R5G5B5A1:

    case gcvSURF_B5G5R5X1:
    case gcvSURF_B5G5R5A1:

    case gcvSURF_X1B5G5R5:
    case gcvSURF_A1B5G5R5:

    case gcvSURF_R5G6B5:
    case gcvSURF_B5G6R5:

	case gcvSURF_YUY2:
	case gcvSURF_UYVY:
	case gcvSURF_YVYU:
	case gcvSURF_VYUY:
	case gcvSURF_NV16:
	case gcvSURF_NV61:
	case gcvSURF_D16:

	case gcvSURF_R16:
	case gcvSURF_A16:
	case gcvSURF_L16:

	case gcvSURF_R16F:
	case gcvSURF_A16F:
	case gcvSURF_L16F:

		/* 16-bpp format. */
		bitsPerPixel  = 16;
		bytesPerTile  = (16 * 4 * 4) / 8;
		break;

	case gcvSURF_X8R8G8B8:
	case gcvSURF_A8R8G8B8:
	case gcvSURF_X8B8G8R8:
	case gcvSURF_A8B8G8R8:
    case gcvSURF_R8G8B8X8:

    case gcvSURF_R8G8B8A8:

    case gcvSURF_B8G8R8X8:

    case gcvSURF_B8G8R8A8:

    case gcvSURF_X2B10G10R10:
    case gcvSURF_A2B10G10R10:

	case gcvSURF_D24X8:
	case gcvSURF_D24S8:
	case gcvSURF_D32:

	case gcvSURF_X16R16F:
	case gcvSURF_G16R16F:
	case gcvSURF_A16R16F:
	case gcvSURF_A16L16F:

    case gcvSURF_R32:
    case gcvSURF_A32:
	case gcvSURF_L32:

    case gcvSURF_R32F:
    case gcvSURF_A32F:
	case gcvSURF_L32F:

		/* 32-bpp format. */
		bitsPerPixel  = 32;
		bytesPerTile  = (32 * 4 * 4) / 8;
		break;

    case gcvSURF_X8G8R8:
    case gcvSURF_B8G8R8:
#ifndef VIVANTE_NO_3D
        if (hardware->api == gcvAPI_OPENCL)
        {
            /* 24-bpp format. */
            bitsPerPixel = 24;
		    bytesPerTile  = (24 * 4 * 4) / 8;
        }
        else
#endif
        {
		    /* Invalid format. */
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
        break;

    case gcvSURF_X16G16R16:
    case gcvSURF_B16G16R16:
    case gcvSURF_X16G16R16F:
    case gcvSURF_B16G16R16F:
#ifndef VIVANTE_NO_3D
        if (hardware->api == gcvAPI_OPENCL)
        {
            /* 48-bpp format. */
            bitsPerPixel = 48;
		    bytesPerTile  = (48 * 4 * 4) / 8;
        }
        else
#endif
        {
		    /* Invalid format. */
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
        break;

    case gcvSURF_X16B16G16R16:
    case gcvSURF_A16B16G16R16:
    case gcvSURF_X16B16G16R16F:
    case gcvSURF_A16B16G16R16F:

    case gcvSURF_X32R32:
    case gcvSURF_G32R32:
    case gcvSURF_A32R32:
    case gcvSURF_X32R32F:
    case gcvSURF_G32R32F:
    case gcvSURF_A32R32F:
#ifndef VIVANTE_NO_3D
        if (hardware->api == gcvAPI_OPENCL)
        {
            /* 64-bpp format. */
            bitsPerPixel = 64;
		    bytesPerTile  = (64 * 4 * 4) / 8;
        }
        else
#endif
        {
		    /* Invalid format. */
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
        break;

    case gcvSURF_X32G32R32:
    case gcvSURF_B32G32R32:
    case gcvSURF_X32G32R32F:
    case gcvSURF_B32G32R32F:
#ifndef VIVANTE_NO_3D
        if (hardware->api == gcvAPI_OPENCL)
        {
            /* 96-bpp format. */
            bitsPerPixel = 96;
		    bytesPerTile  = (96 * 4 * 4) / 8;
        }
        else
#endif
        {
		    /* Invalid format. */
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
        break;

    case gcvSURF_X32B32G32R32:
    case gcvSURF_A32B32G32R32:
    case gcvSURF_X32B32G32R32F:
    case gcvSURF_A32B32G32R32F:
#ifndef VIVANTE_NO_3D
        if (hardware->api == gcvAPI_OPENCL)
        {
            /* 128-bpp format. */
            bitsPerPixel = 128;
		    bytesPerTile  = (128 * 4 * 4) / 8;
        }
        else
#endif
        {
		    /* Invalid format. */
            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
        break;

	case gcvSURF_DXT1:
	case gcvSURF_ETC1:
		bitsPerPixel = 4;
		bytesPerTile = (4 * 4 * 4) / 8;
		break;

	case gcvSURF_DXT2:
	case gcvSURF_DXT3:
	case gcvSURF_DXT4:
	case gcvSURF_DXT5:
		bitsPerPixel = 8;
		bytesPerTile = (8 * 4 * 4) / 8;
		break;

	default:
		/* Invalid format. */
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
	}

	/* Set the result. */
	if (BitsPerPixel != gcvNULL)
	{
		* BitsPerPixel = bitsPerPixel;
	}

	if (BytesPerTile != gcvNULL)
	{
		* BytesPerTile = bytesPerTile;
	}

	/* Success. */
	gcmFOOTER_NO();
	return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_QueryChipIdentity
**
**  Query the identity of the hardware.
**
**  INPUT:
**
**  OUTPUT:
**
**      gceCHIPMODEL* ChipModel
**          If 'ChipModel' is not gcvNULL, the variable it points to will
**          receive the model of the chip.
**
**      gctUINT32* ChipRevision
**          If 'ChipRevision' is not gcvNULL, the variable it points to will
**          receive the revision of the chip.
**
**      gctUINT32* ChipFeatures
**          If 'ChipFeatures' is not gcvNULL, the variable it points to will
**          receive the feature set of the chip.
**
**      gctUINT32 * ChipMinorFeatures
**          If 'ChipMinorFeatures' is not gcvNULL, the variable it points to
**          will receive the minor feature set of the chip.
**
**      gctUINT32 * ChipMinorFeatures1
**          If 'ChipMinorFeatures1' is not gcvNULL, the variable it points to
**          will receive the minor feature set 1 of the chip.
**
*/
gceSTATUS gcoHARDWARE_QueryChipIdentity(
    OUT gceCHIPMODEL* ChipModel,
    OUT gctUINT32* ChipRevision,
    OUT gctUINT32* ChipFeatures,
    OUT gctUINT32* ChipMinorFeatures,
    OUT gctUINT32* ChipMinorFeatures1,
    OUT gctUINT32* ChipMinorFeatures2
    )
{
    gceSTATUS status = gcvSTATUS_OK;
	gcoHARDWARE hardware;

    gcmHEADER_ARG("ChipModel=0x%x ChipRevision=0x%x "
                    "ChipFeatures=0x%x ChipMinorFeatures=0x%x ChipMinorFeatures1=0x%x",
                    ChipModel, ChipRevision,
                    ChipFeatures, ChipMinorFeatures, ChipMinorFeatures1);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Return chip model. */
    if (ChipModel != gcvNULL)
    {
        *ChipModel = hardware->chipModel;
    }

    /* Return revision number. */
    if (ChipRevision != gcvNULL)
    {
        *ChipRevision = hardware->chipRevision;
    }

    /* Return feature set. */
    if (ChipFeatures != gcvNULL)
    {
        *ChipFeatures = hardware->chipFeatures;
    }

    /* Return minor feature set. */
    if (ChipMinorFeatures != gcvNULL)
    {
        *ChipMinorFeatures = hardware->chipMinorFeatures;
    }

    /* Return minor feature set 1. */
    if (ChipMinorFeatures1 != gcvNULL)
    {
        *ChipMinorFeatures1 = hardware->chipMinorFeatures1;
    }

    /* Return minor feature set 1. */
    if (ChipMinorFeatures2 != gcvNULL)
    {
        *ChipMinorFeatures2 = hardware->chipMinorFeatures2;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**	gcoHARDWARE_QueryCommandBuffer
**
**	Query the command buffer alignment and number of reserved bytes.
**
**	INPUT:
**
**		gcoHARDWARE Harwdare
**			Pointer to an gcoHARDWARE object.
**
**	OUTPUT:
**
**		gctSIZE_T * Alignment
**			Pointer to a variable receiving the alignment for each command.
**
**		gctSIZE_T * ReservedHead
**			Pointer to a variable receiving the number of reserved bytes at the
**          head of each command buffer.
**
**		gctSIZE_T * ReservedTail
**			Pointer to a variable receiving the number of bytes reserved at the
**          tail of each command buffer.
*/
gceSTATUS gcoHARDWARE_QueryCommandBuffer(
    OUT gctSIZE_T * Alignment,
    OUT gctSIZE_T * ReservedHead,
    OUT gctSIZE_T * ReservedTail
    )
{
	gcmHEADER_ARG("Alignment=0x%x ReservedHead=0x%x ReservedTail=0x%x",
				  Alignment, ReservedHead, ReservedTail);

    if (Alignment != gcvNULL)
    {
        /* Align every 8 bytes. */
        *Alignment = 8;
    }

    if (ReservedHead != gcvNULL)
    {
        /* Reserve space for SelectPipe. */
        *ReservedHead = 32;
    }

    if (ReservedTail != gcvNULL)
    {
        /* Reserve space for Link(). */
        *ReservedTail = 8;
    }

    /* Success. */
	gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_AlignToTile
**
**  Align the specified width and height to tile boundaries.
**
**  INPUT:
**
**      gceSURF_TYPE Type
**          Type of alignment.
**
**      gctUINT32 * Width
**          Pointer to the width to be aligned.  If 'Width' is gcvNULL, no width
**          will be aligned.
**
**      gctUINT32 * Height
**          Pointer to the height to be aligned.  If 'Height' is gcvNULL, no height
**          will be aligned.
**
**  OUTPUT:
**
**      gctUINT32 * Width
**          Pointer to a variable that will receive the aligned width.
**
**      gctUINT32 * Height
**          Pointer to a variable that will receive the aligned height.
**
**      gctBOOL_PTR SuperTiled
**          Pointer to a variable that receives the super-tiling flag for the
**          surface.
*/
gceSTATUS
gcoHARDWARE_AlignToTile(
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN OUT gctUINT32_PTR Width,
    IN OUT gctUINT32_PTR Height,
    OUT gctBOOL_PTR SuperTiled
    )
{
    gceSTATUS status;
    gctBOOL superTiled = gcvFALSE;
    gctUINT32 xAlignment, yAlignment;
    gctBOOL hAlignmentAvailable = gcvFALSE;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Type=%d Format=0x%x Width=0x%x Height=0x%x",
                  Type, Format, Width, Height);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

#ifndef VIVANTE_NO_3D
    /* Super tiling can be enabled for render targets and depth buffers. */
    superTiled =
        ((Type == gcvSURF_RENDER_TARGET)
        || (Type == gcvSURF_RENDER_TARGET_NO_TILE_STATUS)
        || (Type == gcvSURF_DEPTH)
        || (Type == gcvSURF_DEPTH_NO_TILE_STATUS)
        )
        &&
        /* Of course, hardware needs to support super tiles. */
        ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 12:12) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))));

    /* Textures can be better aligned. */
    hAlignmentAvailable = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 20:20) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))));

    if ((Format == gcvSURF_DXT1)
    || (Format == gcvSURF_DXT2)
    || (Format == gcvSURF_DXT3)
    || (Format == gcvSURF_DXT4)
    || (Format == gcvSURF_DXT5)
    || (Format == gcvSURF_CXV8U8)
    || (Format == gcvSURF_ETC1))
    {
        /* hAlignment only applies to non-compressed textures. */
        hAlignmentAvailable = gcvFALSE;
    }
#endif

    /* Compute alignment factors. */
    /* Change gcoHARDWARE_QueryTileAlignment, if this is changed. */
    xAlignment = superTiled ? 64
               : ((Type == gcvSURF_TEXTURE) && !hAlignmentAvailable) ? 4
               : 16;

    yAlignment = superTiled ? (64 * hardware->pixelPipes)
               : (4 * hardware->pixelPipes);

    if (Width != gcvNULL)
    {
        /* Align the width. */
        *Width = gcmALIGN(*Width, xAlignment);
    }

    if (Height != gcvNULL)
    {
        /* Align the height. */
        *Height = gcmALIGN(*Height, yAlignment);
    }

    if (SuperTiled != gcvNULL)
    {
        /* Copy the super tiling. */
        *SuperTiled = superTiled;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_AlignToTileCompatible
**
**  Align the specified width and height to compatible size for all cores exist
**  in this hardware.
**
**  INPUT:
**
**      gceSURF_TYPE Type
**          Type of alignment.
**
**      gctUINT32 * Width
**          Pointer to the width to be aligned.  If 'Width' is gcvNULL, no width
**          will be aligned.
**
**      gctUINT32 * Height
**          Pointer to the height to be aligned.  If 'Height' is gcvNULL, no height
**          will be aligned.
**
**  OUTPUT:
**
**      gctUINT32 * Width
**          Pointer to a variable that will receive the aligned width.
**
**      gctUINT32 * Height
**          Pointer to a variable that will receive the aligned height.
**
**      gceTILING * Tiling
**          Pointer to a variable that receives the detailed tiling info.
**
**      gctBOOL_PTR SuperTiled
**          Pointer to a variable that receives the super-tiling flag for the
**          surface.
*/
gceSTATUS
gcoHARDWARE_AlignToTileCompatible(
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN OUT gctUINT32_PTR Width,
    IN OUT gctUINT32_PTR Height,
    OUT gceTILING * Tiling,
    OUT gctBOOL_PTR SuperTiled
    )
{
    gceSTATUS status;
    gctBOOL superTiled = gcvFALSE;
    gceTILING tiling;
    gctUINT32 xAlignment, yAlignment;
    gctBOOL hAlignmentAvailable = gcvFALSE;
    gcsTLS_PTR tls;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Type=%d Format=0x%x Width=0x%x Height=0x%x",
                  Type, Format, Width, Height);

    gcmONERROR(
        gcoOS_GetTLS(&tls));

    /* Always take 3D hardware. */
    hardware = tls->hardware;

    if (hardware == gcvNULL)
    {
        /* Save previous hardware type. */
        gceHARDWARE_TYPE prevType = tls->currentType;

        /* Set to 3D hardwawre. */
        tls->currentType = gcvHARDWARE_3D;

        /* Construct hardware. */
        status = gcoHARDWARE_Construct(gcPLS.hal, &tls->hardware);

        /* Set back to previous type. */
        tls->currentType = prevType;

        /* Break if error. */
        gcmONERROR(status);

        /* Get hardware. */
        hardware = tls->hardware;
    }

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    switch (Type)
    {
    case gcvSURF_RENDER_TARGET:
    case gcvSURF_RENDER_TARGET_NO_TILE_STATUS:
    case gcvSURF_DEPTH:
    case gcvSURF_DEPTH_NO_TILE_STATUS:
        /* gcv860 and gcv1000 doesn't support supertile texture */
        if ((Type == gcvSURF_RENDER_TARGET_NO_TILE_STATUS)
            && ((hardware->chipModel == gcv860) || (hardware->chipModel == gcv1000)))
        {
            xAlignment = 16;
            yAlignment = 4 * hardware->pixelPipes;
            tiling     = gcvLINEAR;
        }
        else
        {
#ifndef VIVANTE_NO_3D
            /* Get super tiling feature. */
            superTiled = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 12:12) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))));
#endif

            /* Compute alignment factors. */
            xAlignment = superTiled ? 64 : 16;
            yAlignment = superTiled ? (64 * hardware->pixelPipes)
                : (4 * hardware->pixelPipes);

            tiling = superTiled ? (hardware->pixelPipes > 1 ? gcvMULTI_SUPERTILED
                    : gcvSUPERTILED)
                : (hardware->pixelPipes > 1 ? gcvMULTI_TILED : gcvTILED);
        }

        break;

    case gcvSURF_TEXTURE:
        /* Only 4x4 tiled texture here. */
        /* Textures can be better aligned. */
#ifndef VIVANTE_NO_3D
        hAlignmentAvailable = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 20:20) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))));

        if ((Format == gcvSURF_DXT1)
        || (Format == gcvSURF_DXT2)
        || (Format == gcvSURF_DXT3)
        || (Format == gcvSURF_DXT4)
        || (Format == gcvSURF_DXT5)
        || (Format == gcvSURF_CXV8U8)
        || (Format == gcvSURF_ETC1))
        {
            /* hAlignment only applies to non-compressed textures. */
            hAlignmentAvailable = gcvFALSE;
        }
#endif

        xAlignment = hAlignmentAvailable ? 16 : 4;
        yAlignment = 4 * hardware->pixelPipes;
        tiling     = gcvTILED;

        break;

    default:
        xAlignment = 16;
        yAlignment = 4 * hardware->pixelPipes;
        tiling     = gcvLINEAR;
    }

    if (Width != gcvNULL)
    {
        /* Align the width. */
        *Width = gcmALIGN(*Width, xAlignment);
    }

    if (Height != gcvNULL)
    {
        /* Align the height. */
        *Height = gcmALIGN(*Height, yAlignment);
    }

    if (Tiling != gcvNULL)
    {
        /* Copy the tiling. */
        *Tiling = tiling;
    }

    if (SuperTiled != gcvNULL)
    {
        /* Copy the super tiling. */
        *SuperTiled = superTiled;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**  gcoHARDWARE_QueryTileAlignment
**
**  Return the Tile Alignments in pixels.
**
**  INPUT:
**
**      gceSURF_TYPE Type
**          Type of alignment.
**
**  OUTPUT:
**      gctUINT32 * HAlignment
**          Pointer to the variable that receives Horizontal Alignment.
**
**      gctUINT32 * VAlignment
**          Pointer to the variable that receives Vertical Alignment.
**
*/
gceSTATUS
gcoHARDWARE_QueryTileAlignment(
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    OUT gceSURF_ALIGNMENT * HAlignment,
    OUT gceSURF_ALIGNMENT * VAlignment
    )
{
	gcoHARDWARE hardware;
    gceSTATUS status;
    gctBOOL superTiled = gcvFALSE;

    gcmHEADER_ARG("Type=%d HAlignment=0x%x VAlignment=0x%x",
                  Type, HAlignment, VAlignment);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Super tiling can be enabled for render targets and depth buffers. */
    superTiled =
        ((Type == gcvSURF_RENDER_TARGET)
        || (Type == gcvSURF_RENDER_TARGET_NO_TILE_STATUS)
        || (Type == gcvSURF_DEPTH)
        || (Type == gcvSURF_DEPTH_NO_TILE_STATUS)
        )
        &&
        /* Of course, hardware needs to support super tiles. */
        ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 12:12) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))));

    if (HAlignment != gcvNULL)
    {
        gctBOOL hAlignmentAvailable = gcvFALSE;

        hAlignmentAvailable = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 20:20) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))));

        if ((Format == gcvSURF_DXT1)
        || (Format == gcvSURF_DXT2)
        || (Format == gcvSURF_DXT3)
        || (Format == gcvSURF_DXT4)
        || (Format == gcvSURF_DXT5)
        || (Format == gcvSURF_CXV8U8)
        || (Format == gcvSURF_ETC1))
        {
            /* hAlignment only applies to non-compressed textures. */
            hAlignmentAvailable = gcvFALSE;
        }

        /* Return Horizontal Alignment. */
        *HAlignment = superTiled ? gcvSURF_SUPER_TILED
               : ((Type == gcvSURF_TEXTURE) && !hAlignmentAvailable) ? gcvSURF_FOUR
               : gcvSURF_SIXTEEN;
    }

    if (VAlignment != gcvNULL)
    {
        /* Return Vertical Alignment. */
        *VAlignment = superTiled ? gcvSURF_SUPER_TILED
               : gcvSURF_FOUR;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**	gcoHARDWARE_GetSurfaceTileSize
**
**	Query the tile size of the given surface.
**
**	INPUT:
**
**		Nothing.
**
**	OUTPUT:
**
**		gctINT32 * TileWidth
**			Pointer to a variable receiving the width in pixels per tile.
**
**		gctINT32 * TileHeight
**			Pointer to a variable receiving the height in pixels per tile.
*/
gceSTATUS gcoHARDWARE_GetSurfaceTileSize(
	IN gcsSURF_INFO_PTR Surface,
	OUT gctINT32 * TileWidth,
	OUT gctINT32 * TileHeight
	)
{
	gcmHEADER_ARG("Surface=0x%x TileWidth=0x%x TileHeight=0x%x",
					Surface, TileWidth, TileHeight);

	if (Surface->type == gcvSURF_BITMAP)
	{
		if (TileWidth != gcvNULL)
		{
			/* 1 pixel per 2D tile (linear). */
			*TileWidth = 1;
		}

		if (TileHeight != gcvNULL)
		{
			/* 1 pixel per 2D tile (linear). */
			*TileHeight = 1;
		}
	}
	else
	{
		if (TileWidth != gcvNULL)
		{
			/* 4 pixels per 3D tile for now. */
			*TileWidth = 4;
		}

		if (TileHeight != gcvNULL)
		{
			/* 4 lines per 3D tile. */
			*TileHeight = 4;
		}
	}

	/* Success. */
	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}
#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**	gcoHARDWARE_QueryTileSize
**
**	Query the tile sizes.
**
**	INPUT:
**
**		Nothing.
**
**	OUTPUT:
**
**		gctINT32 * TileWidth2D
**			Pointer to a variable receiving the width in pixels per 2D tile.  If
**			the 2D is working in linear space, the width will be 1.  If there is
**			no 2D, the width will be 0.
**
**		gctINT32 * TileHeight2D
**			Pointer to a variable receiving the height in pixels per 2D tile.
**			If the 2D is working in linear space, the height will be 1.  If
**			there is no 2D, the height will be 0.
**
**		gctINT32 * TileWidth3D
**			Pointer to a variable receiving the width in pixels per 3D tile.  If
**			the 3D is working in linear space, the width will be 1.  If there is
**			no 3D, the width will be 0.
**
**		gctINT32 * TileHeight3D
**			Pointer to a variable receiving the height in pixels per 3D tile.
**			If the 3D is working in linear space, the height will be 1.  If
**			there is no 3D, the height will be 0.
**
**		gctUINT32 * Stride
**			Pointer to  variable receiving the stride requirement for all modes.
*/
gceSTATUS gcoHARDWARE_QueryTileSize(
	OUT gctINT32 * TileWidth2D,
	OUT gctINT32 * TileHeight2D,
	OUT gctINT32 * TileWidth3D,
	OUT gctINT32 * TileHeight3D,
	OUT gctUINT32 * Stride
	)
{
	gcmHEADER_ARG("TileWidth2D=0x%x TileHeight2D=0x%x TileWidth3D=0x%x "
					"TileHeight3D=0x%x Stride=0x%x",
					TileWidth2D, TileHeight2D, TileWidth3D,
					TileHeight3D, Stride);

	if (TileWidth2D != gcvNULL)
	{
		/* 1 pixel per 2D tile (linear). */
		*TileWidth2D = 1;
	}

	if (TileHeight2D != gcvNULL)
	{
		/* 1 pixel per 2D tile (linear). */
		*TileHeight2D = 1;
	}

	if (TileWidth3D != gcvNULL)
	{
		/* 4 pixels per 3D tile for now. */
		*TileWidth3D = 4;
	}

	if (TileHeight3D != gcvNULL)
	{
		/* 4 lines per 3D tile. */
		*TileHeight3D = 4;
	}

	if (Stride != gcvNULL)
	{
		/* 64-byte stride requirement. */
		*Stride = 64;
	}

	/* Success. */
	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**	gcoHARDWARE_QueryTextureCaps
**
**	Query the texture capabilities.
**
**	INPUT:
**
**		Nothing.
**
**	OUTPUT:
**
**		gctUINT * MaxWidth
**			Pointer to a variable receiving the maximum width of a texture.
**
**		gctUINT * MaxHeight
**			Pointer to a variable receiving the maximum height of a texture.
**
**		gctUINT * MaxDepth
**			Pointer to a variable receiving the maximum depth of a texture.  If
**			volume textures are not supported, 0 will be returned.
**
**		gctBOOL * Cubic
**			Pointer to a variable receiving a flag whether the hardware supports
**			cubic textures or not.
**
**		gctBOOL * NonPowerOfTwo
**			Pointer to a variable receiving a flag whether the hardware supports
**			non-power-of-two textures or not.
**
**		gctUINT * VertexSamplers
**			Pointer to a variable receiving the number of sampler units in the
**			vertex shader.
**
**		gctUINT * PixelSamplers
**			Pointer to a variable receiving the number of sampler units in the
**			pixel shader.
**
**      gctUINT * MaxAnisoValue
**          Pointer to a variable receiving the maximum parameter value of
**          anisotropic filter.
*/
gceSTATUS gcoHARDWARE_QueryTextureCaps(
	OUT gctUINT * MaxWidth,
	OUT gctUINT * MaxHeight,
	OUT gctUINT * MaxDepth,
	OUT gctBOOL * Cubic,
	OUT gctBOOL * NonPowerOfTwo,
	OUT gctUINT * VertexSamplers,
	OUT gctUINT * PixelSamplers,
    OUT gctUINT * MaxAnisoValue
	)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("MaxWidth=0x%x MaxHeight=0x%x MaxDepth=0x%x "
					"Cubic=0x%x NonPowerOfTwo=0x%x VertexSamplers=0x%x "
					"PixelSamplers=0x%x MaxAnisoValue=0x%x",
					MaxWidth, MaxHeight, MaxDepth,
					Cubic, NonPowerOfTwo, VertexSamplers,
					PixelSamplers, MaxAnisoValue);


	if (MaxWidth != gcvNULL)
	{
		/* Maximum texture width for XAQ2. */
		*MaxWidth = gcoHARDWARE_IsFeatureAvailable(
			gcvFEATURE_TEXTURE_8K) ? 8192 : 2048;
	}

	if (MaxHeight != gcvNULL)
	{
		/* Maximum texture height for XAQ2. */
		*MaxHeight = gcoHARDWARE_IsFeatureAvailable(
			gcvFEATURE_TEXTURE_8K) ? 8192 : 2048;
	}

	if (MaxDepth != gcvNULL)
	{
		/* Maximum texture depth for XAQ2. */
		*MaxDepth = 1;
	}

	if (Cubic != gcvNULL)
	{
		/* XAQ2 supports cube maps. */
		*Cubic = gcvTRUE;
	}

	if (NonPowerOfTwo != gcvNULL)
	{
		/* XAQ2 does not support non-power-of-two texture maps. */
		*NonPowerOfTwo = gcvFALSE;
	}

	if ((VertexSamplers != gcvNULL) || (PixelSamplers != gcvNULL))
	{
        gcmONERROR(
            gcoHARDWARE_QuerySamplerBase((gctSIZE_T *)VertexSamplers,
                                         gcvNULL,
                                         (gctSIZE_T *)PixelSamplers,
                                         gcvNULL));
    }

	if (MaxAnisoValue != gcvNULL)
	{
		/* Return maximum value of anisotropy supported by XAQ2. */
		if(gcoHARDWARE_IsFeatureAvailable(
               gcvFEATURE_TEXTURE_ANISOTROPIC_FILTERING) == gcvSTATUS_TRUE)
        {
            /* Anisotropic. */
            *MaxAnisoValue = 16;
        }
        else
        {
            /* Isotropic. */
            *MaxAnisoValue = 1;
        }
	}

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**	gcoHARDWARE_QueryTargetCaps
**
**	Query the render target capabilities.
**
**	INPUT:
**
**	OUTPUT:
**
**		gctUINT * MaxWidth
**			Pointer to a variable receiving the maximum width of a render
**			target.
**
**		gctUINT * MaxHeight
**			Pointer to a variable receiving the maximum height of a render
**			target.
**
**		gctUINT * MultiTargetCount
**			Pointer to a variable receiving the number of render targets.
**
**		gctUINT * MaxSamples
**			Pointer to a variable receiving the maximum number of samples per
**			pixel for MSAA.
*/
gceSTATUS
gcoHARDWARE_QueryTargetCaps(
	OUT gctUINT * MaxWidth,
	OUT gctUINT * MaxHeight,
	OUT gctUINT * MultiTargetCount,
	OUT gctUINT * MaxSamples
	)
{
    gceSTATUS status = gcvSTATUS_OK;
	gcoHARDWARE hardware;

	gcmHEADER_ARG("MaxWidth=0x%x MaxHeight=0x%x "
					"MultiTargetCount=0x%x MaxSamples=0x%x",
					MaxWidth, MaxHeight,
					MultiTargetCount, MaxSamples);

    gcmGETHARDWARE(hardware);

	if (MaxWidth != gcvNULL)
	{
		/* Return maximum width of render target for XAQ2. */
        if (((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 9:9) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))))
		{
	    	*MaxWidth = 8192;
		}
        else
		{
	    	*MaxWidth = 2048;
		}
	}

	if (MaxHeight != gcvNULL)
	{
		/* Return maximum height of render target for XAQ2. */
        if (((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 9:9) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))))
		{
	    	*MaxHeight = 8192;
		}
        else
		{
	    	*MaxHeight = 2048;
		}
	}

	if (MultiTargetCount != gcvNULL)
	{
		/* Return number of render targets for XAQ2. */
		*MultiTargetCount = 1;
	}

	if (MaxSamples != gcvNULL)
	{
		/* Return number of samples per pixel for XAQ2. */
		if (((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 7:7) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))))
		{
			*MaxSamples = 4;
		}
		else
		{
			*MaxSamples = 0;
		}
#if USE_SUPER_SAMPLING
		if (*MaxSamples == 0)
		{
			*MaxSamples = 4;
		}
#endif
	}

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**	gcoHARDWARE_QueryIndexCaps
**
**	Query the index capabilities.
**
**	INPUT:
**
**		Nothing.
**
**	OUTPUT:
**
**		gctBOOL * Index8
**			Pointer to a variable receiving the capability for 8-bit indices.
**
**		gctBOOL * Index16
**			Pointer to a variable receiving the capability for 16-bit indices.
**			target.
**
**		gctBOOL * Index32
**			Pointer to a variable receiving the capability for 32-bit indices.
**
**		gctUINT * MaxIndex
**			Maximum number of indices.
*/
gceSTATUS
gcoHARDWARE_QueryIndexCaps(
	OUT gctBOOL * Index8,
	OUT gctBOOL * Index16,
	OUT gctBOOL * Index32,
	OUT gctUINT * MaxIndex
	)
{
    gcoHARDWARE hardware;
    gceSTATUS status = gcvSTATUS_OK;

	gcmHEADER_ARG("Index8=0x%x Index16=0x%x Index32=0x%x MaxIndex=0x%x",
					Index8, Index16, Index32, MaxIndex);

    gcmGETHARDWARE(hardware);

	if (Index8 != gcvNULL)
	{
		/* XAQ2 supports 8-bit indices. */
		*Index8 = gcvTRUE;
	}

	if (Index16 != gcvNULL)
	{
		/* XAQ2 supports 16-bit indices. */
		*Index16 = gcvTRUE;
	}

	if (Index32 != gcvNULL)
	{
		/* Return 32-bit indices support. */
        if (((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 31:31) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))))
        {
		    *Index32 = gcvTRUE;
        }
        else
        {
		    *Index32 = gcvFALSE;
        }
	}

	if (MaxIndex != gcvNULL)
	{
		/* Return MaxIndex supported. */
        if (((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 31:31) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))))
        {
		    /* XAQ2 supports up to 2**20 indices. */
		    *MaxIndex = 1 << 20;
        }
        else
        {
		    /* XAQ2 supports up to 2**16 indices. */
		    *MaxIndex = 1 << 16;
        }
	}
OnError:

	/* Success. */
	gcmFOOTER_NO();
	return status;
}

/*******************************************************************************
**
**	gcoHARDWARE_QueryShaderCaps
**
**	Query the shader capabilities.
**
**	INPUT:
**
**		Nothing.
**
**	OUTPUT:
**
**		gctUINT * VertexUniforms
**			Pointer to a variable receiving the number of uniforms in the vertex
**			shader.
**
**		gctUINT * FragmentUniforms
**			Pointer to a variable receiving the number of uniforms in the
**			fragment shader.
**
**		gctUINT * Varyings
**			Pointer to a variable receiving the maimum number of varyings.
**
**		gctUINT * ShaderCoreCount
**			Pointer to a variable receiving the number of shader cores.
**
**		gctUINT * ThreadCount
**			Pointer to a variable receiving the thread count.
*/
gceSTATUS
gcoHARDWARE_QueryShaderCaps(
	OUT gctUINT * VertexUniforms,
	OUT gctUINT * FragmentUniforms,
	OUT gctUINT * Varyings,
	OUT gctUINT * ShaderCoreCount,
	OUT gctUINT * ThreadCount,
	OUT gctUINT * VertexInstructionCount,
	OUT gctUINT * FragmentInstructionCount
	)
{
    gcoHARDWARE hardware;
    gceSTATUS status = gcvSTATUS_OK;

	gcmHEADER_ARG("VertexUniforms=0x%x FragmentUniforms=0x%x Varyings=0x%x",
					VertexUniforms, FragmentUniforms, Varyings);

    gcmGETHARDWARE(hardware);

	if (VertexUniforms != gcvNULL)
	{
		/* Return the vs shader const count. */
        *VertexUniforms = hardware->vsConstMax;
	}

	if (FragmentUniforms != gcvNULL)
	{
		/* Return the ps shader const count. */
		*FragmentUniforms = hardware->psConstMax;
	}

	if (Varyings != gcvNULL)
	{
		/* Return the shader varyings count. */
        if (((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 23:23) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))))
        {
		    *Varyings = 12;
        }
        else
        {
		    *Varyings = 8;
        }
	}

	if (ShaderCoreCount != gcvNULL)
	{
		/* Return the shader core count. */
        *ShaderCoreCount = hardware->shaderCoreCount;
	}

	if (ThreadCount != gcvNULL)
	{
		/* Return the shader core count. */
        *ThreadCount = hardware->threadCount;
	}

	if (VertexInstructionCount != gcvNULL)
	{
		/* Return the shader core count. */
        *VertexInstructionCount = hardware->vsInstMax;
	}

	if (FragmentInstructionCount != gcvNULL)
	{
		/* Return the shader core count. */
        *FragmentInstructionCount = hardware->psInstMax;
	}

OnError:
    /* Success. */
	gcmFOOTER();
	return status;
}

/*******************************************************************************
**
**	gcoHARDWARE_QueryShaderCapsEx
**
**	Query the shader capabilities.
**
**	INPUT:
**
**		Nothing.
**
**	OUTPUT:
**
**		gctUINT64 * LocalMemSize
**			Pointer to a variable receiving the size of the local memory
**			available to the shader program.
**
**		gctUINT * AddressBits
**			Pointer to a variable receiving the number of address bits.
**
**		gctUINT * GlobalMemCachelineSize
**			Pointer to a variable receiving the size of global memory cache
**          line in bytes.
**
**		gctUINT * GlobalMemCacheSize
**			Pointer to a variable receiving the size of global memory cache
**          in bytes.
**
**		gctUINT * ClockFrequency
**			Pointer to a variable receiving the shader core clock.
*/
gceSTATUS
gcoHARDWARE_QueryShaderCapsEx(
	OUT gctUINT64 * LocalMemSize,
	OUT gctUINT * AddressBits,
	OUT gctUINT * GlobalMemCachelineSize,
	OUT gctUINT * GlobalMemCacheSize,
	OUT gctUINT * ClockFrequency
	)
{
    gcoHARDWARE hardware;
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 globalMemCachelineSize;

	gcmHEADER_ARG("LocalMemSize=0x%x AddressBits=0x%x GlobalMemCachelineSize=0x%x "
                  "GlobalMemCacheSize=0x%x ClockFrequency=0x%x",
					LocalMemSize, AddressBits, GlobalMemCachelineSize,
                    GlobalMemCacheSize, ClockFrequency);

    gcmGETHARDWARE(hardware);

	if (LocalMemSize != gcvNULL)
	{
        if (hardware->chipModel == gcv4000)
        {
            /* Local memory size in bytes. */
		    *LocalMemSize = 8192;
        }
        else
        {
            /* Local memory size in bytes. */
		    *LocalMemSize = 1024;
        }
	}

	if (AddressBits != gcvNULL)
	{
		/* XAQ2 supports 32 bit addresses. */
		*AddressBits = 32;
	}

    /* 64 byte cache lines. */
    globalMemCachelineSize = 64;

	if (GlobalMemCachelineSize != gcvNULL)
	{
        *GlobalMemCachelineSize = globalMemCachelineSize;
    }

    if (GlobalMemCacheSize != gcvNULL)
	{
        /* globalMemCacheSize = 64*16*subbanks*banks
         * gc4000: subbanks=4 banks=4
         * gc2100: subbanks=4 banks=1
         * gc2200: subbanks=4 banks=2
         */
        if (hardware->chipModel == gcv4000)
        {
            /* Global Memory Cache size in bytes. */
		    *GlobalMemCacheSize = globalMemCachelineSize * 16 * 4 * 4;
        }
        else if (hardware->chipModel == gcv2100
             ||  (hardware->chipModel == gcv2000 && hardware->chipRevision == 0x5108))
        {
            /* Global Memory Cache size in bytes. */
		    *GlobalMemCacheSize = globalMemCachelineSize * 16 * 4 * 1;
        }
        else /* if (hardware->chipModel == gcv2200) */
        {
            /* Global Memory Cache size in bytes. */
		    *GlobalMemCacheSize = globalMemCachelineSize * 16 * 4 * 2;
        }
	}

	if (ClockFrequency != gcvNULL)
	{
		/* Return the shader core clock in Mhz. */
        /* TODO. */
        *ClockFrequency = 500;
	}

OnError:
    /* Success. */
	gcmFOOTER();
	return status;
}

/*******************************************************************************
**
**	gcoHARDWARE_QueryTileStatus
**
**	Query the linear size for a tile size buffer.
**
**	INPUT:
**
**		gctUINT Width, Height
**          Width and height of the surface.
**
**		gctSZIE_T Bytes
**          Size of the surface in bytes.
**
**	OUTPUT:
**
**		gctSIZE_T_PTR Size
**			Pointer to a variable receiving the number of bytes required to
**          store the tile status buffer.
**
**		gctSIZE_T_PTR Alignment
**			Pointer to a variable receiving the alignment requirement.
**
**		gctUINT32_PTR Filler
**			Pointer to a variable receiving the tile status filler for fast
**			clear.
*/
gceSTATUS
gcoHARDWARE_QueryTileStatus(
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctSIZE_T Bytes,
    OUT gctSIZE_T_PTR Size,
    OUT gctUINT_PTR Alignment,
	OUT gctUINT32_PTR Filler
    )
{
    gceSTATUS status;
    gctUINT width, height;
	gctBOOL is2BitPerTile;
	gcoHARDWARE hardware;

	gcmHEADER_ARG("Width=%d Height=%d "
					"Bytes=%d Size=0x%x Alignment=0x%x "
					"Filler=0x%x",
					Width, Height,
					Bytes, Size, Alignment,
					Filler);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Size != gcvNULL);

	/* See if tile status is supported. */
	if (!(((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 0:0)) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) ))
	{
    	gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
	}

	/* Check tile status size. */
	is2BitPerTile = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 10:10) & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1)))))));

	if ((hardware->chipModel == gcv500)
	&&  (hardware->chipRevision > 2)
	)
	{
		/* Compute the aligned number of tiles for the surface. */
		width  = gcmALIGN(Width, 4) >> 2;
		height = gcmALIGN(Height, 4) >> 2;

		/* Return the linear size. */
		*Size = gcmALIGN(width * height * 4 / 8, 256);
	}
	else
	{
		if ((Width == 0) && (Height == 0))
		{
			*Size = Bytes / 16 / 4;
		}
		else
		{
			/* Return the linear size. */
			*Size = is2BitPerTile ? (Bytes >> 8) : (Bytes >> 7);
		}

		/* Align the tile status. */
		*Size = gcmALIGN(*Size,
						 hardware->resolveAlignmentX *
						 hardware->resolveAlignmentY *
						 hardware->pixelPipes *
						 4);
	}

	if (Alignment != gcvNULL)
	{
		/* Set alignment. */
		*Alignment = 64;
	}

	if (Filler != gcvNULL)
	{
		*Filler = ((hardware->chipModel == gcv500)
				  && (hardware->chipRevision > 2)
				  )
				  ? 0xFFFFFFFF
				  : is2BitPerTile ? 0x55555555
								  : 0x11111111;
	}

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_QuerySamplerBase(
	OUT gctSIZE_T * VertexCount,
	OUT gctINT_PTR VertexBase,
	OUT gctSIZE_T * FragmentCount,
	OUT gctINT_PTR FragmentBase
	)
{
    gceSTATUS status;
	gcoHARDWARE hardware;

	gcmHEADER_ARG("VertexCount=0x%x VertexBase=0x%x "
					"FragmentCount=0x%x FragmentBase=0x%x",
					VertexCount, VertexBase,
					FragmentCount, FragmentBase);

    gcmGETHARDWARE(hardware);

	gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

	if (VertexCount != gcvNULL)
	{
		if (((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
		{
			*VertexCount = 16;
		}
		else if (hardware->chipModel > gcv500)
		{
			*VertexCount = 4;
		}
		else
		{
			*VertexCount = 0;
		}
	}

	if (VertexBase != gcvNULL)
	{
		if (((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
		{
			*VertexBase = 16;
		}
        else
        {
		    *VertexBase = 8;
        }
	}

	if (FragmentCount != gcvNULL)
	{
		if (((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
		{
    		*FragmentCount = 16;
		}
		else
		{
    		*FragmentCount = 8;
        }
	}

	if (FragmentBase != gcvNULL)
	{
		*FragmentBase = 0;
	}

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**  gcoHARDWARE_IsFeatureAvailable
**
**  Verifies whether the specified feature is available in hardware.
**
**  INPUT:
**
**      gceFEATURE Feature
**          Feature to be verified.
*/
gceSTATUS
gcoHARDWARE_IsFeatureAvailable(
    IN gceFEATURE Feature
    )
{
    gceSTATUS status;
    gctBOOL available = gcvFALSE;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Feature=%d", Feature);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    switch (Feature)
    {
    /* Generic features. */
    case gcvFEATURE_PIPE_2D:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 9:9) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1)))))));
        break;

    case gcvFEATURE_PIPE_3D:
#ifndef VIVANTE_NO_3D
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 2:2) & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))));
#else
        available = gcvFALSE;
#endif
        break;

    case gcvFEATURE_FULL_DIRECTFB:
        available = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 16:16) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))));
        break;

    case gcvFEATURE_2D_TILING:
        /* Fall through. */
    case gcvFEATURE_2D_YUV_BLIT:
        available = hardware->hw2DOPF;
        break;

    case gcvFEATURE_2D_MULTI_SOURCE_BLT:
        available = hardware->hw2DMultiSrcBlit;
        break;

    case gcvFEATURE_2D_MULTI_SOURCE_BLT_EX:
        available = hardware->hw2DNewFeature0;
        break;

    case gcvFEATURE_2D_FILTERBLIT_PLUS_ALPHABLEND:
        /* Fall through. */
    case gcvFEATURE_2D_DITHER:
        available = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 16:16) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))));
        break;

    case gcvFEATURE_2D_A8_TARGET:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 29:29) & ((gctUINT32) ((((1 ? 29:29) - (0 ? 29:29) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:29) - (0 ? 29:29) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 29:29) - (0 ? 29:29) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:29) - (0 ? 29:29) + 1)))))));
        break;

    case gcvFEATURE_2D_FILTERBLIT_FULLROTATION:
        available = hardware->fullFilterBlitRotation;
        break;

    case gcvFEATURE_2D_BITBLIT_FULLROTATION:
        available = hardware->fullBitBlitRotation;
        break;

    case gcvFEATURE_YUV420_SCALER:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 6:6) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))));
        break;

    case gcvFEATURE_2DPE20:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 7:7) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))));
        break;

    case gcvFEATURE_2D_NO_COLORBRUSH_INDEX8:
        available = hardware->hw2DNoIndex8_Brush;
        break;

    /* Filter Blit. */
    case gcvFEATURE_SCALER:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 20:20) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))) == (0x0  & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))));
        break;

    /* Rotation stall fix. */
    case gcvFEATURE_2D_ROTATION_STALL_FIX:
        available = ((((gctUINT32) (hardware->chipMinorFeatures3)) >> (0 ? 0:0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))));
        break;

    /* Load/Store L1 cache hang fix. */
    case gcvFEATURE_BUG_FIXES10:
        available = ((((gctUINT32) (hardware->chipMinorFeatures3)) >> (0 ? 10:10) & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1)))))));
        break;

    case gcvFEATURE_BUG_FIXES11:
        available = ((((gctUINT32) (hardware->chipMinorFeatures3)) >> (0 ? 12:12) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))));
        break;

#ifndef VIVANTE_NO_3D
    /* Generic features. */
    case gcvFEATURE_PIPE_VG:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 26:26) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1)))))));
        break;

    case gcvFEATURE_DC:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 8:8) & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1)))))));
        break;

    case gcvFEATURE_HIGH_DYNAMIC_RANGE:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 12:12) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))));
        break;

    case gcvFEATURE_MODULE_CG:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 14:14) & ((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1)))))));
        break;

    case gcvFEATURE_MIN_AREA:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 15:15) & ((gctUINT32) ((((1 ? 15:15) - (0 ? 15:15) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:15) - (0 ? 15:15) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 15:15) - (0 ? 15:15) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:15) - (0 ? 15:15) + 1)))))));
        break;

    case gcvFEATURE_BUFFER_INTERLEAVING:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 18:18) & ((gctUINT32) ((((1 ? 18:18) - (0 ? 18:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:18) - (0 ? 18:18) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 18:18) - (0 ? 18:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:18) - (0 ? 18:18) + 1)))))));
        break;

    case gcvFEATURE_BYTE_WRITE_2D:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 19:19) & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))));
        break;

    case gcvFEATURE_ENDIANNESS_CONFIG:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 2:2) & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))));
        break;

    case gcvFEATURE_DUAL_RETURN_BUS:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 1:1) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))));
        break;

    case gcvFEATURE_DEBUG_MODE:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 4:4) & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1)))))));
        break;

    /* Resolve. */
    case gcvFEATURE_FAST_CLEAR:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 0:0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))));
        break;

    case gcvFEATURE_YUV420_TILER:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 13:13) & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1)))))));
        break;

    case gcvFEATURE_YUY2_AVERAGING:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 21:21) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))));
        break;

    case gcvFEATURE_FLIP_Y:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 0:0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))));
        break;

    /* Depth. */
    case gcvFEATURE_EARLY_Z:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 16:16) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) == (0x0  & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))));
        break;

    case gcvFEATURE_Z_COMPRESSION:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 5:5) & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))));
        break;

    /* MSAA. */
    case gcvFEATURE_MSAA:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 7:7) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))));
        break;

    case gcvFEATURE_SPECIAL_ANTI_ALIASING:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 1:1) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))));
        break;

    case gcvFEATURE_SPECIAL_MSAA_LOD:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 5:5) & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))));
        break;

    case gcvFEATURE_VAA:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 25:25) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1)))))));
        break;

    /* Texture. */
    case gcvFEATURE_422_TEXTURE_COMPRESSION:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 17:17) & ((gctUINT32) ((((1 ? 17:17) - (0 ? 17:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:17) - (0 ? 17:17) + 1)))))) == (0x0  & ((gctUINT32) ((((1 ? 17:17) - (0 ? 17:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:17) - (0 ? 17:17) + 1)))))));
        break;

    case gcvFEATURE_DXT_TEXTURE_COMPRESSION:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 3:3) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))));
        break;

    case gcvFEATURE_ETC1_TEXTURE_COMPRESSION:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 10:10) & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1)))))));
        break;

    case gcvFEATURE_CORRECT_TEXTURE_CONVERTER:
#ifdef GC_FEATURES_CORRECT_TEXTURE_CONVERTER
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) & ((gctUINT32) ((((1 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) - (0 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) + 1) == 32) ? ~0 : (~(~0 << ((1 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) - (0 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) + 1)))))) == (GC_FEATURES_CORRECT_TEXTURE_CONVERTER_AVAILABLE  & ((gctUINT32) ((((1 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) - (0 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) + 1) == 32) ? ~0 : (~(~0 << ((1 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) - (0 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) + 1)))))));
#else
        available = gcvFALSE;
#endif
        break;

    case gcvFEATURE_TEXTURE_8K:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 3:3) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))));
        break;

    case gcvFEATURE_YUY2_RENDER_TARGET:
        available = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 24:24) & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1)))))));
        break;

    case gcvFEATURE_FRAGMENT_PROCESSOR:
        available = gcvFALSE;
        break;

    case gcvFEATURE_SHADER_HAS_W:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 19:19) & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))));
        break;

    case gcvFEATURE_HZ:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 27:27) & ((gctUINT32) ((((1 ? 27:27) - (0 ? 27:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:27) - (0 ? 27:27) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 27:27) - (0 ? 27:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:27) - (0 ? 27:27) + 1)))))));
        break;

    case gcvFEATURE_CORRECT_STENCIL:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 30:30) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1)))))));
        break;

    case gcvFEATURE_MC20:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 22:22) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1)))))));
        break;

    case gcvFEATURE_SUPER_TILED:
        available = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 12:12) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))));
        break;

    case gcvFEATURE_WIDE_LINE:
        available = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 29:29) & ((gctUINT32) ((((1 ? 29:29) - (0 ? 29:29) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:29) - (0 ? 29:29) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 29:29) - (0 ? 29:29) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:29) - (0 ? 29:29) + 1)))))));
        break;

    case gcvFEATURE_FC_FLUSH_STALL:
        available = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 31:31) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))));
        break;

    case gcvFEATURE_TEXTURE_FLOAT_HALF_FLOAT:
        /* Fall through. */
    case gcvFEATURE_HALF_FLOAT_PIPE:
        available = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))));
        break;

    case gcvFEATURE_NON_POWER_OF_TWO:
        available = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 21:21) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))));
        break;

    case gcvFEATURE_VERTEX_10_10_10_2:
        /* Fall through. */
    case gcvFEATURE_TEXTURE_ANISOTROPIC_FILTERING:
        /* Fall through. */
    case gcvFEATURE_TEXTURE_10_10_10_2:
        /* Fall through. */
    case gcvFEATURE_3D_TEXTURE:
        /* Fall through. */
    case gcvFEATURE_TEXTURE_ARRAY:
        available = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 23:23) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))));
        break;

    case gcvFEATURE_LINE_LOOP:
        available = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 0:0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))));
        break;

    case gcvFEATURE_TILE_FILLER:
        available = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 19:19) & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))));
        break;

    case gcvFEATURE_LOGIC_OP:
        available = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 1:1) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))));
        break;

    case gcvFEATURE_COMPOSITION:
        available = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 6:6) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))));
        break;

    case gcvFEATURE_MIXED_STREAMS:
        available = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 25:25) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1)))))));
        break;

    case gcvFEATURE_TEXTURE_TILED_READ:
        available = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 29:29) & ((gctUINT32) ((((1 ? 29:29) - (0 ? 29:29) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:29) - (0 ? 29:29) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 29:29) - (0 ? 29:29) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:29) - (0 ? 29:29) + 1)))))))
            | ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 6:6) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))));
        break;

    case gcvFEATURE_DEPTH_BIAS_FIX:
        if (((hardware->chipModel == gcv2000)
             && ((hardware->chipRevision == 0x5020)
                || (hardware->chipRevision == 0x5026)
                || (hardware->chipRevision == 0x5113)
                || (hardware->chipRevision == 0x5108)))

           || ((hardware->chipModel == gcv2100)
               && ((hardware->chipRevision == 0x5108)
                  || (hardware->chipRevision == 0x5113)))

           || ((hardware->chipModel == gcv4000)
               && ((hardware->chipRevision == 0x5222)
                  || (hardware->chipRevision == 0x5208)))
           )
        {
            available = gcvFALSE;
        }
        else
        {
            available = gcvTRUE;
        }
        break;

    case gcvFEATURE_RECT_PRIMITIVE:
        available = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 5:5) & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))));
        break;

    case gcvFEATURE_SUPERTILED_TEXTURE:
        available = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 3:3) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))));
        break;
#endif

    default:
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    status = available
        ? gcvSTATUS_TRUE
        : gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_Is2DAvailable
**
**  Verifies whether 2D engine is available.
**
**  INPUT:
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_Is2DAvailable(
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER();

    gcmGETHARDWARE(hardware);

    status = hardware->hw2DEngine
        ? gcvSTATUS_TRUE
        : gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


