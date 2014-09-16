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

#ifndef VIVANTE_NO_3D

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

static gctINT _GetTextureFormat(
    gceSURF_FORMAT Format
    )
{
    static const gctINT32 swizzle_rgba = (0x0 << 20)
                           |(0x1   << 23)
                           |(0x2     << 26)
                           |(0x3   << 29);

    static const gctINT32 swizzle_rgbx = (0x0 << 20)
                           |(0x1   << 23)
                           |(0x2     << 26)
                           |(0x5     << 29);

    static const gctINT32 swizzle_a = (0x4 << 20)
                           |(0x4    << 23)
                           |(0x4     << 26)
                           |(0x3   << 29);

    static const gctINT32 swizzle_la = (0x0 << 20)
                           |(0x0     << 23)
                           |(0x0      << 26)
                           |(0x1   << 29);

    static const gctINT32 swizzle_l = (0x0 << 20)
                           |(0x0     << 23)
                           |(0x0      << 26)
                           |(0x5     << 29);

    switch (Format)
    {
    case gcvSURF_A8:          return 0x01
                                   | swizzle_rgba;
    case gcvSURF_L8:          return 0x02
                                   | swizzle_rgba;
    case gcvSURF_A8L8:        return 0x04
                                   | swizzle_rgba;
    case gcvSURF_X4R4G4B4:    return 0x06
                                   | swizzle_rgbx;
    case gcvSURF_A4R4G4B4:    return 0x05
                                   | swizzle_rgba;
    case gcvSURF_X1R5G5B5:    return 0x0D
                                   | swizzle_rgbx;
    case gcvSURF_A1R5G5B5:    return 0x0C
                                   | swizzle_rgba;
    case gcvSURF_R5G6B5:      return 0x0B
                                   | swizzle_rgba;
    case gcvSURF_X8R8G8B8:    return 0x08
                                   | swizzle_rgbx;
    case gcvSURF_A8R8G8B8:    return 0x07
                                   | swizzle_rgba;
    case gcvSURF_X8B8G8R8:    return 0x0A
                                   | swizzle_rgbx;
    case gcvSURF_A8B8G8R8:    return 0x09
                                   | swizzle_rgba;
    case gcvSURF_YUY2:        return 0x0E
                                   | swizzle_rgba;
    case gcvSURF_UYVY:        return 0x0F
                                   | swizzle_rgba;
    case gcvSURF_D16:         return 0x10
                                   | swizzle_rgba;
    case gcvSURF_D24S8:       return 0x11
                                   | swizzle_rgba;
    case gcvSURF_D24X8:       return 0x11
                                   | swizzle_rgbx;
    case gcvSURF_DXT1:        return 0x13
                                   | swizzle_rgba;
    case gcvSURF_DXT2:        return 0x14
                                   | swizzle_rgba;
    case gcvSURF_DXT3:        return 0x14
                                   | swizzle_rgba;
    case gcvSURF_DXT4:        return 0x15
                                   | swizzle_rgba;
    case gcvSURF_DXT5:        return 0x15
                                   | swizzle_rgba;
    case gcvSURF_ETC1:        return 0x1E
                                   | swizzle_rgba;

     /* Pass the extended formats and swizzles on upper 12 bit. */
    case gcvSURF_X2B10G10R10:   return (0x0C << 12)
                                   | swizzle_rgbx;

    case gcvSURF_A2B10G10R10:   return (0x0C << 12)
                                   | swizzle_rgba
                                    ;
    case gcvSURF_X16B16G16R16F: return (0x09 << 12)
                                   | swizzle_rgbx;

    case gcvSURF_A16B16G16R16F: return (0x09 << 12)
                                   | swizzle_rgba;

    case gcvSURF_A16F:          return (0x07 << 12)
                                   | swizzle_a;

    case gcvSURF_L16F:          return (0x07 << 12)
                                   | swizzle_l;

    case gcvSURF_A16L16F:       return (0x08 << 12)
                                   | swizzle_la;

    case gcvSURF_A32F:          return (0x0A << 12)
                                   | swizzle_a;

    case gcvSURF_L32F:          return (0x0A << 12)
                                   | swizzle_l;

    case gcvSURF_A32L32F:       return (0x0B << 12)
                                   | swizzle_la;


    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gcvZONE_TEXTURE,
                      "Unknown color format.");
        break;
    }

    /* Format is not supported. */
    return -1;
}

static gctINT _GetMCTextureFormat(
    gceSURF_FORMAT Format
    )
{
    switch (Format)
    {
    case gcvSURF_X4R4G4B4:
        /* Fall through. */
    case gcvSURF_A4R4G4B4:    return 0x0;

    case gcvSURF_X1R5G5B5:
        /* Fall through. */
    case gcvSURF_A1R5G5B5:    return 0x1;

    case gcvSURF_R5G6B5:      return 0x2;

    case gcvSURF_X8R8G8B8:
        /* Fall through. */
    case gcvSURF_A8R8G8B8:    return 0x3;

    case gcvSURF_D24S8:
        /* Fall through. */
    case gcvSURF_D24X8:       return 0x5;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gcvZONE_TEXTURE,
                      "Unknown color format.");
        break;
    }

    /* Format is not supported. */
    return -1;
}

/******************************************************************************\
|*************************** Texture Management Code **************************|
\******************************************************************************/

/*******************************************************************************
**
**  gcoHARDWARE_GetClosestTextureFormat
**
**  Returns the closest supported format to the one specified in the call.
**
**  INPUT:
**
**      gceSURF_FORMAT APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Closest supported API value.
*/
gceSTATUS
gcoHARDWARE_GetClosestTextureFormat(
    IN gceSURF_FORMAT InFormat,
    OUT gceSURF_FORMAT* OutFormat
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("InFormat=%d", InFormat);

    gcmGETHARDWARE(hardware);

    /* Dispatch on format. */
    switch (InFormat)
    {
    case gcvSURF_A4:
        /* Fall through */
    case gcvSURF_A8:
        /* Fall through */
    case gcvSURF_A12:
        /* Fall through */
    case gcvSURF_A16:
        /* Fall through */
    case gcvSURF_A32:
        *OutFormat = gcvSURF_A8;
        break;

    case gcvSURF_L4:
        /* Fall through */
    case gcvSURF_L8:
        /* Fall through */
    case gcvSURF_L12:
        /* Fall through */
    case gcvSURF_L16:
        /* Fall through */
    case gcvSURF_L32:
        *OutFormat = gcvSURF_L8;
        break;

    case gcvSURF_A4L4:
        /* Fall through */
    case gcvSURF_A2L6:
        /* Fall through */
    case gcvSURF_A8L8:
        /* Fall through */
    case gcvSURF_A4L12:
        /* Fall through */
    case gcvSURF_A12L12:
        /* Fall through */
    case gcvSURF_A16L16:
        *OutFormat = gcvSURF_A8L8;
        break;

    case gcvSURF_R3G3B2:
        /* Fall through */
    case gcvSURF_R5G6B5:
        /* Fall through */
    case gcvSURF_B5G6R5:
        *OutFormat = gcvSURF_R5G6B5;
        break;

    case gcvSURF_X4R4G4B4:
        *OutFormat = gcvSURF_X4R4G4B4;
        break;

    case gcvSURF_A2R2G2B2:
        /* Fall through */
    case gcvSURF_A4R4G4B4:
        /* Fall through */
    case gcvSURF_A4B4G4R4:
        /* Fall through */
    case gcvSURF_R4G4B4A4:
        *OutFormat = gcvSURF_A4R4G4B4;
        break;

    case gcvSURF_X1R5G5B5:
        *OutFormat = gcvSURF_X1R5G5B5;
        break;

    case gcvSURF_A1R5G5B5:
        /* Fall through */
    case gcvSURF_R5G5B5A1:
        /* Fall through */
    case gcvSURF_A1B5G5R5:
        *OutFormat = gcvSURF_A1R5G5B5;
        break;

    case gcvSURF_B8G8R8:
        if (hardware->chipModel >= gcv4000
        &&  hardware->chipRevision >= 0x5200)
        {
            *OutFormat = gcvSURF_A8R8G8B8;
		}
        else
        {
            *OutFormat = gcvSURF_X8R8G8B8;
        }
        break;

    case gcvSURF_R8G8B8:
        /* Fall through */
    case gcvSURF_X8R8G8B8:
        /* Fall through */
    case gcvSURF_X8B8G8R8:
        /* Fall through */
    case gcvSURF_X12R12G12B12:
        /* Fall through */
    case gcvSURF_X16R16G16B16:
        *OutFormat = gcvSURF_X8R8G8B8;
        break;

    case gcvSURF_A8R3G3B2:
        /* Fall through */
    case gcvSURF_A8R8G8B8:
        /* Fall through */
    case gcvSURF_R8G8B8A8:
        /* Fall through */
    case gcvSURF_B8G8R8A8:
        /* Fall through */
    case gcvSURF_A8B8G8R8:
        /* Fall through */
    case gcvSURF_A12R12G12B12:
        /* Fall through */
    case gcvSURF_A16R16G16B16:
        /* Fall through */
    case gcvSURF_A16B16G16R16:
        *OutFormat = gcvSURF_A8R8G8B8;
        break;

    case gcvSURF_A2B10G10R10:
        /* Fall through */
    case gcvSURF_X2B10G10R10:
        /* Fall through */
    case gcvSURF_X16B16G16R16F:
        /* Fall through */
    case gcvSURF_A16B16G16R16F:
        /* Fall through */
    case gcvSURF_A16F:
        /* Fall through */
    case gcvSURF_A32F:
        /* Fall through */
    case gcvSURF_L16F:
        /* Fall through */
    case gcvSURF_L32F:
        /* Fall through */
    case gcvSURF_A16L16F:
        /* Fall through */
    case gcvSURF_A32L32F:
        if (((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 23:23) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))))
        {
            *OutFormat = InFormat;
            break;
        }

        /* Not supported. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        break;

    case gcvSURF_D16:
        *OutFormat = gcvSURF_D16;
        break;

    case gcvSURF_D24X8:
        /* Fall through */
    case gcvSURF_D32:
        *OutFormat = gcvSURF_D24X8;
        break;

    case gcvSURF_D24S8:
        *OutFormat = gcvSURF_D24S8;
        break;

    case gcvSURF_DXT1:
        if (((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 3:3) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))))
        {
            *OutFormat = InFormat;
            break;
        }
        *OutFormat = gcvSURF_A1R5G5B5;
        break;

    case gcvSURF_DXT2:
        /* Fall through */
    case gcvSURF_DXT3:
        /* Fall through */
    case gcvSURF_DXT4:
        /* Fall through */
    case gcvSURF_DXT5:
        if (((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 3:3) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))))
        {
            *OutFormat = InFormat;
            break;
        }
        *OutFormat = gcvSURF_A8R8G8B8;
        break;

    case gcvSURF_ETC1:
        if (((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 10:10) & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1))))))))
        {
            *OutFormat = InFormat;
            break;
        }
        *OutFormat = gcvSURF_X8R8G8B8;
        break;

    case gcvSURF_YV12:
        if (((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 13:13) & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1))))))))
        {
            *OutFormat = gcvSURF_YUY2;
            break;
        }

        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        break;

    case gcvSURF_YUY2:
        *OutFormat = InFormat;
        break;

    default:
        /* Not supported. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_QueryTexture
**
**  Query if the requested texture is supported.
**
**  INPUT:
**
**      gceSURF_FORMAT Format
**          Format of texture.
**
**      gctUINT Level
**          Level of texture map.
**
**      gctUINT Width
**          Width of texture.
**
**      gctUINT Height
**          Height of texture.
**
**      gctUINT Depth
**          Depth of texture.
**
**      gctUINT Faces
**          Number of faces for texture.
**
**  OUTPUT:
**
**      gctUINT * BlockWidth
**          Pointer to a variable receiving the width of a block of texels.
**
**      gctUINT * BlockHeight
**          Pointer to a variable receiving the height of a block of texels.
**
*/
gceSTATUS
gcoHARDWARE_QueryTexture(
    IN gceSURF_FORMAT Format,
    IN gctUINT Level,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gctUINT Faces,
    OUT gctUINT * BlockWidth,
    OUT gctUINT * BlockHeight
    )
{
    gceSTATUS status;
    /*gctUINT bitsPerPixel = 0;*/

    gcmHEADER_ARG("Format=%d Level=%u Width=%u Height=%u "
                  "Depth=%u Faces=%u",
                  Format, Level, Width, Height, Depth, Faces);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(BlockWidth != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(BlockHeight != gcvNULL);

    /* Check for valid formats. */
    switch (Format)
    {
    case gcvSURF_A8:
        /* Fall through */
    case gcvSURF_L8:
        /* Fall through */
    case gcvSURF_R8:
        /* 8-bits per pixel, no alignment requirement. */
        /*bitsPerPixel = 8;*/
        *BlockWidth  = *BlockHeight = 1;
        break;

    case gcvSURF_A8L8:
        /* Fall through */
    case gcvSURF_X4R4G4B4:
        /* Fall through */
    case gcvSURF_A4R4G4B4:
        /* Fall through */
    case gcvSURF_X1R5G5B5:
        /* Fall through */
    case gcvSURF_A1R5G5B5:
        /* Fall through */
    case gcvSURF_R5G6B5:
        /* Fall through */
    case gcvSURF_D16:
        /* Fall through */
    case gcvSURF_X8R8:
        /* Fall through */
    case gcvSURF_G8R8:
        /* Fall through */
    case gcvSURF_A8R8:
        /* Fall through */
    case gcvSURF_R16F:
        /* Fall through */
    case gcvSURF_A16F:
        /* Fall through */
    case gcvSURF_L16F:
        /* 16-bits per pixel, no alignment requirement. */
        /*bitsPerPixel = 16;*/
        *BlockWidth  = *BlockHeight = 1;
        break;

    case gcvSURF_A8B8G8R8:
        /* Fall through */
    case gcvSURF_X8B8G8R8:
        /* Fall through */
    case gcvSURF_X8R8G8B8:
        /* Fall through */
    case gcvSURF_A8R8G8B8:
        /* Fall through */
    case gcvSURF_D24X8:
        /* Fall through */
    case gcvSURF_X2B10G10R10:
        /* Fall through */
    case gcvSURF_A2B10G10R10:
        /* Fall through */
    case gcvSURF_R32F:
        /* Fall through */
    case gcvSURF_A32F:
        /* Fall through */
    case gcvSURF_L32F:
        /* Fall through */
    case gcvSURF_D24S8:
        /* 32-bits per pixel, no alignment requirement. */
        /*bitsPerPixel = 32;*/
        *BlockWidth  = *BlockHeight = 1;
        break;

    case gcvSURF_X16B16G16R16:
        /* Fall through */
    case gcvSURF_A16B16G16R16:
        /* Fall through */
    case gcvSURF_X16B16G16R16F:
        /* Fall through */
    case gcvSURF_A16B16G16R16F:
        /* 64-bits per pixel, no alignment requirement. */
        /*bitsPerPixel = 64;*/
        *BlockWidth  = *BlockHeight = 1;
        break;

    case gcvSURF_X32B32G32R32:
        /* Fall through */
    case gcvSURF_A32B32G32R32:
        /* Fall through */
    case gcvSURF_X32B32G32R32F:
        /* Fall through */
    case gcvSURF_A32B32G32R32F:
        /* 128-bits per pixel, no alignment requirement. */
        /*bitsPerPixel = 128;*/
        *BlockWidth  = *BlockHeight = 1;
        break;

    case gcvSURF_YUY2:
        /* Fall through */
    case gcvSURF_UYVY:
        /*bitsPerPixel = 16;*/
        *BlockWidth  = 2;
        *BlockHeight = 1;
        break;

    case gcvSURF_DXT1:
        /* Fall through */
    case gcvSURF_ETC1:
        /*bitsPerPixel = 4;*/
        *BlockWidth  = *BlockHeight = 4;
        break;

    case gcvSURF_DXT2:
        /* Fall through */
    case gcvSURF_DXT3:
        /* Fall through */
    case gcvSURF_DXT4:
        /* Fall through */
    case gcvSURF_DXT5:
        /*bitsPerPixel = 8;*/
        *BlockWidth  = *BlockHeight = 4;
        break;

    default:
        /* Texture format not supported. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* We don't support volume maps. */
    if (Depth > 1)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Is HW support non-power of two */
    if(gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_NON_POWER_OF_TWO) != gcvSTATUS_TRUE)
    {
        /* We don't support non-power of two textures beyond level 0. */
        if ((Level > 0)
        &&  ((Width  & (Width  - 1))
            || (Height & (Height - 1))
            )
        )
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    /* Check maximum width and height. */
    if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_TEXTURE_8K))
    {
        if ((Width > 8192) || (Height > 8192))
        {
            gcmONERROR(gcvSTATUS_MIPMAP_TOO_LARGE);
        }
    }
    else
    {
        if ((Width > 2048) || (Height > 2048))
        {
            gcmONERROR(gcvSTATUS_MIPMAP_TOO_LARGE);
        }
    }

    /* Success. */
    gcmFOOTER_ARG("*BlockWidth=%u *BlockHeight=%u",
                  *BlockWidth, *BlockHeight);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_DisableTextureSampler
**
**  Disable a specific sampler.
**
**  INPUT:
**
**      gctINT Sampler
**          Sampler number.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_DisableTextureSampler(
    IN gctINT Sampler
    )
{
    gceSTATUS status;
    gctUINT32 hwMode;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Sampler=%d",
                  Sampler);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    if (((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
    {
        /* Make sure the sampler index is within range. */
        if ((Sampler < 0) || (Sampler >= gcdSAMPLERS))
        {
            /* Invalid sampler. */
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }
    else
    {
        /* Make sure the sampler index is within range. */
        if ((Sampler < 0) || (Sampler >= 16))
        {
            /* Invalid sampler. */
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    /* Setting the Texture Sampler mode to 0, shall disable the sampler. */
    hwMode = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)));

    if (hwMode != hardware->hwTxSamplerMode[Sampler])
    {
        /* Save dirty sampler mode register. */
        hardware->hwTxSamplerMode[Sampler] = hwMode;
        hardware->hwTxSamplerModeDirty    |= 1 << Sampler;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


static void
_UploadA8toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* A8 to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = srcLine[0] << 24;
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* A8 to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = srcLine[0] << 24;
                ((gctUINT32_PTR) trgLine)[1] = srcLine[1] << 24;
                ((gctUINT32_PTR) trgLine)[2] = srcLine[2] << 24;
                ((gctUINT32_PTR) trgLine)[3] = srcLine[3] << 24;
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* A8 to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[ 0] = srcLine[SourceStride * 0] << 24;
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            gctUINT8_PTR src;

            /* A8 to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[ 1] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[ 2] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[ 3] = src[3] << 24;

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[ 5] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[ 6] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[ 7] = src[3] << 24;

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[ 9] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[ 0] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[11] = src[3] << 24;

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = src[0] << 24;
            ((gctUINT32_PTR) trgLine)[13] = src[1] << 24;
            ((gctUINT32_PTR) trgLine)[14] = src[2] << 24;
            ((gctUINT32_PTR) trgLine)[15] = src[3] << 24;

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1;
        }
    }
}

static void
_UploadBGRtoARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 3);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[1] << 8) | srcLine[2];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[ 1] << 8) | srcLine[ 2];
                ((gctUINT32_PTR) trgLine)[1] = 0xFF000000 | (srcLine[3] << 16) | (srcLine[ 4] << 8) | srcLine[ 5];
                ((gctUINT32_PTR) trgLine)[2] = 0xFF000000 | (srcLine[6] << 16) | (srcLine[ 7] << 8) | srcLine[ 8];
                ((gctUINT32_PTR) trgLine)[3] = 0xFF000000 | (srcLine[9] << 16) | (srcLine[10] << 8) | srcLine[11];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[1] << 8) | srcLine[2];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            gctUINT8_PTR src;

            /* BGR to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[10] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[11] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = 0xFF000000 | (src[0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[13] = 0xFF000000 | (src[3] << 16) | (src[ 4] << 8) | src[ 5];
            ((gctUINT32_PTR) trgLine)[14] = 0xFF000000 | (src[6] << 16) | (src[ 7] << 8) | src[ 8];
            ((gctUINT32_PTR) trgLine)[15] = 0xFF000000 | (src[9] << 16) | (src[10] << 8) | src[11];

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 3;
        }
    }
}

static void
_UploadBGRtoABGR(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 3);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

            /* BGR to ABGR. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | (srcLine[2] << 16) | (srcLine[1] << 8) | srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ABGR: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | (srcLine[ 2] << 16) | (srcLine[ 1] << 8) | srcLine[0];
                ((gctUINT32_PTR) trgLine)[1] = 0xFF000000 | (srcLine[ 5] << 16) | (srcLine[ 4] << 8) | srcLine[3];
                ((gctUINT32_PTR) trgLine)[2] = 0xFF000000 | (srcLine[ 8] << 16) | (srcLine[ 7] << 8) | srcLine[6];
                ((gctUINT32_PTR) trgLine)[3] = 0xFF000000 | (srcLine[11] << 16) | (srcLine[10] << 8) | srcLine[9];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

                /* BGR to ABGR: part tile row. */
                ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | (srcLine[2] << 16) | (srcLine[1] << 8) | srcLine[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 3;

        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            gctUINT8_PTR src;

            /* BGR to ABGR: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | (src[ 2] << 16) | (src[ 1] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF000000 | (src[ 5] << 16) | (src[ 4] << 8) | src[3];
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF000000 | (src[ 8] << 16) | (src[ 7] << 8) | src[6];
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF000000 | (src[11] << 16) | (src[10] << 8) | src[9];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF000000 | (src[ 2] << 16) | (src[ 1] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF000000 | (src[ 5] << 16) | (src[ 4] << 8) | src[3];
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF000000 | (src[ 8] << 16) | (src[ 7] << 8) | src[6];
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF000000 | (src[11] << 16) | (src[10] << 8) | src[9];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF000000 | (src[ 2] << 16) | (src[ 1] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF000000 | (src[ 5] << 16) | (src[ 4] << 8) | src[3];
            ((gctUINT32_PTR) trgLine)[10] = 0xFF000000 | (src[ 8] << 16) | (src[ 7] << 8) | src[6];
            ((gctUINT32_PTR) trgLine)[11] = 0xFF000000 | (src[11] << 16) | (src[10] << 8) | src[9];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = 0xFF000000 | (src[ 2] << 16) | (src[ 1] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[13] = 0xFF000000 | (src[ 5] << 16) | (src[ 4] << 8) | src[3];
            ((gctUINT32_PTR) trgLine)[14] = 0xFF000000 | (src[ 8] << 16) | (src[ 7] << 8) | src[6];
            ((gctUINT32_PTR) trgLine)[15] = 0xFF000000 | (src[11] << 16) | (src[10] << 8) | src[9];

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 3;
        }
    }
}

static void
_UploadABGRtoARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 4);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* ABGR to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = (srcLine[3] << 24) | (srcLine[0] << 16) | (srcLine[1] << 8) | srcLine[2];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* ABGR to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 3] << 24) | (srcLine[ 0] << 16) | (srcLine[ 1] << 8) | srcLine[ 2];
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[ 7] << 24) | (srcLine[ 4] << 16) | (srcLine[ 5] << 8) | srcLine[ 6];
                ((gctUINT32_PTR) trgLine)[2] = (srcLine[11] << 24) | (srcLine[ 8] << 16) | (srcLine[ 9] << 8) | srcLine[10];
                ((gctUINT32_PTR) trgLine)[3] = (srcLine[15] << 24) | (srcLine[12] << 16) | (srcLine[13] << 8) | srcLine[14];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* ABGR to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[3] << 24) | (srcLine[0] << 16) | (srcLine[1] << 8) | srcLine[2];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            gctUINT8_PTR src;

            /* ABGR to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = (src[ 3] << 24) | (src[ 0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 1] = (src[ 7] << 24) | (src[ 4] << 16) | (src[ 5] << 8) | src[ 6];
            ((gctUINT32_PTR) trgLine)[ 2] = (src[11] << 24) | (src[ 8] << 16) | (src[ 9] << 8) | src[10];
            ((gctUINT32_PTR) trgLine)[ 3] = (src[15] << 24) | (src[12] << 16) | (src[13] << 8) | src[14];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = (src[ 3] << 24) | (src[ 0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 5] = (src[ 7] << 24) | (src[ 4] << 16) | (src[ 5] << 8) | src[ 6];
            ((gctUINT32_PTR) trgLine)[ 6] = (src[11] << 24) | (src[ 8] << 16) | (src[ 9] << 8) | src[10];
            ((gctUINT32_PTR) trgLine)[ 7] = (src[15] << 24) | (src[12] << 16) | (src[13] << 8) | src[14];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = (src[ 3] << 24) | (src[ 0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[ 9] = (src[ 7] << 24) | (src[ 4] << 16) | (src[ 5] << 8) | src[ 6];
            ((gctUINT32_PTR) trgLine)[10] = (src[11] << 24) | (src[ 8] << 16) | (src[ 9] << 8) | src[10];
            ((gctUINT32_PTR) trgLine)[11] = (src[15] << 24) | (src[12] << 16) | (src[13] << 8) | src[14];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = (src[ 3] << 24) | (src[ 0] << 16) | (src[ 1] << 8) | src[ 2];
            ((gctUINT32_PTR) trgLine)[13] = (src[ 7] << 24) | (src[ 4] << 16) | (src[ 5] << 8) | src[ 6];
            ((gctUINT32_PTR) trgLine)[14] = (src[11] << 24) | (src[ 8] << 16) | (src[ 9] << 8) | src[10];
            ((gctUINT32_PTR) trgLine)[15] = (src[15] << 24) | (src[12] << 16) | (src[13] << 8) | src[14];

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 4;
        }
    }
}

static void
_UploadL8toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* L8 to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[0] << 8) | srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* L8 to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[0] << 8) | srcLine[0];
                ((gctUINT32_PTR) trgLine)[1] = 0xFF000000 | (srcLine[1] << 16) | (srcLine[1] << 8) | srcLine[1];
                ((gctUINT32_PTR) trgLine)[2] = 0xFF000000 | (srcLine[2] << 16) | (srcLine[2] << 8) | srcLine[2];
                ((gctUINT32_PTR) trgLine)[3] = 0xFF000000 | (srcLine[3] << 16) | (srcLine[3] << 8) | srcLine[3];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* L8 to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | (srcLine[0] << 16) | (srcLine[0] << 8) | srcLine[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            gctUINT8_PTR src;

            /* L8 to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | (src[0] << 16) | (src[0] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF000000 | (src[1] << 16) | (src[1] << 8) | src[1];
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF000000 | (src[2] << 16) | (src[2] << 8) | src[2];
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF000000 | (src[3] << 16) | (src[3] << 8) | src[3];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF000000 | (src[0] << 16) | (src[0] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF000000 | (src[1] << 16) | (src[1] << 8) | src[1];
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF000000 | (src[2] << 16) | (src[2] << 8) | src[2];
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF000000 | (src[3] << 16) | (src[3] << 8) | src[3];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF000000 | (src[0] << 16) | (src[0] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF000000 | (src[1] << 16) | (src[1] << 8) | src[1];
            ((gctUINT32_PTR) trgLine)[10] = 0xFF000000 | (src[2] << 16) | (src[2] << 8) | src[2];
            ((gctUINT32_PTR) trgLine)[11] = 0xFF000000 | (src[3] << 16) | (src[3] << 8) | src[3];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = 0xFF000000 | (src[0] << 16) | (src[0] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[13] = 0xFF000000 | (src[1] << 16) | (src[1] << 8) | src[1];
            ((gctUINT32_PTR) trgLine)[14] = 0xFF000000 | (src[2] << 16) | (src[2] << 8) | src[2];
            ((gctUINT32_PTR) trgLine)[15] = 0xFF000000 | (src[3] << 16) | (src[3] << 8) | src[3];

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1;
        }
    }
}

static void
_UploadA8L8toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

            /* A8L8 to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = (srcLine[1] << 24) | (srcLine[0] << 16) | (srcLine[0] << 8) | srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* A8L8 to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[1] << 24) | (srcLine[0] << 16) | (srcLine[0] << 8) | srcLine[0];
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[3] << 24) | (srcLine[2] << 16) | (srcLine[2] << 8) | srcLine[2];
                ((gctUINT32_PTR) trgLine)[2] = (srcLine[5] << 24) | (srcLine[4] << 16) | (srcLine[4] << 8) | srcLine[4];
                ((gctUINT32_PTR) trgLine)[3] = (srcLine[7] << 24) | (srcLine[6] << 16) | (srcLine[6] << 8) | srcLine[6];
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

                /* A8L8 to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[1] << 24) | (srcLine[0] << 16) | (srcLine[0] << 8) | srcLine[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 2;

        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            gctUINT8_PTR src;

            /* A8 to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = (src[1] << 24) | (src[0] << 16) | (src[0] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[ 1] = (src[3] << 24) | (src[2] << 16) | (src[2] << 8) | src[2];
            ((gctUINT32_PTR) trgLine)[ 2] = (src[5] << 24) | (src[4] << 16) | (src[4] << 8) | src[4];
            ((gctUINT32_PTR) trgLine)[ 3] = (src[7] << 24) | (src[6] << 16) | (src[6] << 8) | src[6];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 4] = (src[1] << 24) | (src[0] << 16) | (src[0] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[ 5] = (src[3] << 24) | (src[2] << 16) | (src[2] << 8) | src[2];
            ((gctUINT32_PTR) trgLine)[ 6] = (src[5] << 24) | (src[4] << 16) | (src[4] << 8) | src[4];
            ((gctUINT32_PTR) trgLine)[ 7] = (src[7] << 24) | (src[6] << 16) | (src[6] << 8) | src[6];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[ 8] = (src[1] << 24) | (src[0] << 16) | (src[0] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[ 9] = (src[3] << 24) | (src[2] << 16) | (src[2] << 8) | src[2];
            ((gctUINT32_PTR) trgLine)[10] = (src[5] << 24) | (src[4] << 16) | (src[4] << 8) | src[4];
            ((gctUINT32_PTR) trgLine)[11] = (src[7] << 24) | (src[6] << 16) | (src[6] << 8) | src[6];

            src += SourceStride;
            ((gctUINT32_PTR) trgLine)[12] = (src[1] << 24) | (src[0] << 16) | (src[0] << 8) | src[0];
            ((gctUINT32_PTR) trgLine)[13] = (src[3] << 24) | (src[2] << 16) | (src[2] << 8) | src[2];
            ((gctUINT32_PTR) trgLine)[14] = (src[5] << 24) | (src[4] << 16) | (src[4] << 8) | src[4];
            ((gctUINT32_PTR) trgLine)[15] = (src[7] << 24) | (src[6] << 16) | (src[6] << 8) | src[6];

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 2;
        }
    }
}

static void
_Upload32bppto32bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 4);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

            /* 32bpp to 32bpp. */
            ((gctUINT32_PTR) trgLine)[0] = (srcLine[3] << 24) | (srcLine[2] << 16) | (srcLine[1] << 8) | srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* 32 bpp to 32 bpp: one tile row. */
                if ((((gctUINT32) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                    ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) srcLine)[2];
                    ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) srcLine)[3];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 3] << 24) | (srcLine[ 2] << 16) | (srcLine[ 1] << 8) | srcLine[ 0];
                    ((gctUINT32_PTR) trgLine)[1] = (srcLine[ 7] << 24) | (srcLine[ 6] << 16) | (srcLine[ 5] << 8) | srcLine[ 4];
                    ((gctUINT32_PTR) trgLine)[2] = (srcLine[11] << 24) | (srcLine[10] << 16) | (srcLine[ 9] << 8) | srcLine[ 8];
                    ((gctUINT32_PTR) trgLine)[3] = (srcLine[15] << 24) | (srcLine[14] << 16) | (srcLine[13] << 8) | srcLine[12];
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

                /* 32 bpp to 32 bpp: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 3] << 24) | (srcLine[ 2] << 16) | (srcLine[ 1] << 8) | srcLine[ 0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 4;

        if ((((gctUINT32) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned source. */
            for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
            {
                gctUINT8_PTR src;

                /* 32 bpp to 32 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 1] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 2] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 3] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 4] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 5] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 6] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 7] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 9] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[10] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[11] = ((gctUINT32_PTR) src)[3];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[12] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[13] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[14] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[15] = ((gctUINT32_PTR) src)[3];

                /* Move to next tile. */
                trgLine += 16 * 4;
                srcLine +=  4 * 4;
            }
        }
        else
        {
            for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
            {
                gctUINT8_PTR src;

                /* 32 bpp to 32 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = (src[ 3] << 24) | (src[ 2] << 16) | (src[ 1] << 8) | src[ 0];
                ((gctUINT32_PTR) trgLine)[ 1] = (src[ 7] << 24) | (src[ 6] << 16) | (src[ 5] << 8) | src[ 4];
                ((gctUINT32_PTR) trgLine)[ 2] = (src[11] << 24) | (src[10] << 16) | (src[ 9] << 8) | src[ 8];
                ((gctUINT32_PTR) trgLine)[ 3] = (src[15] << 24) | (src[14] << 16) | (src[13] << 8) | src[12];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 4] = (src[ 3] << 24) | (src[ 2] << 16) | (src[ 1] << 8) | src[ 0];
                ((gctUINT32_PTR) trgLine)[ 5] = (src[ 7] << 24) | (src[ 6] << 16) | (src[ 5] << 8) | src[ 4];
                ((gctUINT32_PTR) trgLine)[ 6] = (src[11] << 24) | (src[10] << 16) | (src[ 9] << 8) | src[ 8];
                ((gctUINT32_PTR) trgLine)[ 7] = (src[15] << 24) | (src[14] << 16) | (src[13] << 8) | src[12];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = (src[ 3] << 24) | (src[ 2] << 16) | (src[ 1] << 8) | src[ 0];
                ((gctUINT32_PTR) trgLine)[ 9] = (src[ 7] << 24) | (src[ 6] << 16) | (src[ 5] << 8) | src[ 4];
                ((gctUINT32_PTR) trgLine)[10] = (src[11] << 24) | (src[10] << 16) | (src[ 9] << 8) | src[ 8];
                ((gctUINT32_PTR) trgLine)[11] = (src[15] << 24) | (src[14] << 16) | (src[13] << 8) | src[12];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[12] = (src[ 3] << 24) | (src[ 2] << 16) | (src[ 1] << 8) | src[ 0];
                ((gctUINT32_PTR) trgLine)[13] = (src[ 7] << 24) | (src[ 6] << 16) | (src[ 5] << 8) | src[ 4];
                ((gctUINT32_PTR) trgLine)[14] = (src[11] << 24) | (src[10] << 16) | (src[ 9] << 8) | src[ 8];
                ((gctUINT32_PTR) trgLine)[15] = (src[15] << 24) | (src[14] << 16) | (src[13] << 8) | src[12];

                /* Move to next tile. */
                trgLine += 16 * 4;
                srcLine +=  4 * 4;
            }
        }
    }
}

static void
_Upload64bppto64bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 8);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 8;

            /* 64bpp to 64bpp. */
            ((gctUINT32_PTR) trgLine)[0] = (srcLine[3] << 24) | (srcLine[2] << 16) | (srcLine[1] << 8) | srcLine[0];
            ((gctUINT32_PTR) trgLine)[1] = (srcLine[7] << 24) | (srcLine[6] << 16) | (srcLine[5] << 8) | srcLine[4];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 8;

                /* 32 bpp to 32 bpp: one tile row. */
                if ((((gctUINT32) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                    ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) srcLine)[2];
                    ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) srcLine)[3];
                    ((gctUINT32_PTR) trgLine)[4] = ((gctUINT32_PTR) srcLine)[4];
                    ((gctUINT32_PTR) trgLine)[5] = ((gctUINT32_PTR) srcLine)[5];
                    ((gctUINT32_PTR) trgLine)[6] = ((gctUINT32_PTR) srcLine)[6];
                    ((gctUINT32_PTR) trgLine)[7] = ((gctUINT32_PTR) srcLine)[7];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = (srcLine[ 3] << 24) | (srcLine[ 2] << 16) | (srcLine[ 1] << 8) | srcLine[ 0];
                    ((gctUINT32_PTR) trgLine)[1] = (srcLine[ 7] << 24) | (srcLine[ 6] << 16) | (srcLine[ 5] << 8) | srcLine[ 4];
                    ((gctUINT32_PTR) trgLine)[2] = (srcLine[11] << 24) | (srcLine[10] << 16) | (srcLine[ 9] << 8) | srcLine[ 8];
                    ((gctUINT32_PTR) trgLine)[3] = (srcLine[15] << 24) | (srcLine[14] << 16) | (srcLine[13] << 8) | srcLine[12];
                    ((gctUINT32_PTR) trgLine)[4] = (srcLine[19] << 24) | (srcLine[18] << 16) | (srcLine[17] << 8) | srcLine[16];
                    ((gctUINT32_PTR) trgLine)[5] = (srcLine[23] << 24) | (srcLine[22] << 16) | (srcLine[21] << 8) | srcLine[20];
                    ((gctUINT32_PTR) trgLine)[6] = (srcLine[27] << 24) | (srcLine[26] << 16) | (srcLine[25] << 8) | srcLine[24];
                    ((gctUINT32_PTR) trgLine)[7] = (srcLine[31] << 24) | (srcLine[30] << 16) | (srcLine[29] << 8) | srcLine[28];
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 8;

                /* 32 bpp to 32 bpp: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = (srcLine[3] << 24) | (srcLine[2] << 16) | (srcLine[1] << 8) | srcLine[0];
                ((gctUINT32_PTR) trgLine)[1] = (srcLine[7] << 24) | (srcLine[6] << 16) | (srcLine[5] << 8) | srcLine[4];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 8;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 8;

        if ((((gctUINT32) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned source. */
            for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
            {
                gctUINT8_PTR src;

                /* 64 bpp to 64 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 1] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[ 2] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[ 3] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[ 4] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[ 5] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[ 6] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[ 7] = ((gctUINT32_PTR) src)[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[ 9] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[10] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[11] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[12] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[13] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[14] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[15] = ((gctUINT32_PTR) src)[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[16] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[17] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[18] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[19] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[20] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[21] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[22] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[23] = ((gctUINT32_PTR) src)[7];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[24] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[25] = ((gctUINT32_PTR) src)[1];
                ((gctUINT32_PTR) trgLine)[26] = ((gctUINT32_PTR) src)[2];
                ((gctUINT32_PTR) trgLine)[27] = ((gctUINT32_PTR) src)[3];
                ((gctUINT32_PTR) trgLine)[28] = ((gctUINT32_PTR) src)[4];
                ((gctUINT32_PTR) trgLine)[29] = ((gctUINT32_PTR) src)[5];
                ((gctUINT32_PTR) trgLine)[30] = ((gctUINT32_PTR) src)[6];
                ((gctUINT32_PTR) trgLine)[31] = ((gctUINT32_PTR) src)[7];

                /* Move to next tile. */
                trgLine += 16 * 8;
                srcLine +=  4 * 8;
            }
        }
        else
        {
            for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
            {
                gctUINT8_PTR src;

                /* 64 bpp to 64 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[ 0] = (src[ 3] << 24) | (src[ 2] << 16) | (src[ 1] << 8) | src[ 0];
                ((gctUINT32_PTR) trgLine)[ 1] = (src[ 7] << 24) | (src[ 6] << 16) | (src[ 5] << 8) | src[ 4];
                ((gctUINT32_PTR) trgLine)[ 2] = (src[11] << 24) | (src[10] << 16) | (src[ 9] << 8) | src[ 8];
                ((gctUINT32_PTR) trgLine)[ 3] = (src[15] << 24) | (src[14] << 16) | (src[13] << 8) | src[12];
                ((gctUINT32_PTR) trgLine)[ 4] = (src[19] << 24) | (src[18] << 16) | (src[17] << 8) | src[16];
                ((gctUINT32_PTR) trgLine)[ 5] = (src[23] << 24) | (src[22] << 16) | (src[21] << 8) | src[20];
                ((gctUINT32_PTR) trgLine)[ 6] = (src[27] << 24) | (src[26] << 16) | (src[25] << 8) | src[24];
                ((gctUINT32_PTR) trgLine)[ 7] = (src[31] << 24) | (src[30] << 16) | (src[29] << 8) | src[28];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[ 8] = (src[ 3] << 24) | (src[ 2] << 16) | (src[ 1] << 8) | src[ 0];
                ((gctUINT32_PTR) trgLine)[ 9] = (src[ 7] << 24) | (src[ 6] << 16) | (src[ 5] << 8) | src[ 4];
                ((gctUINT32_PTR) trgLine)[10] = (src[11] << 24) | (src[10] << 16) | (src[ 9] << 8) | src[ 8];
                ((gctUINT32_PTR) trgLine)[11] = (src[15] << 24) | (src[14] << 16) | (src[13] << 8) | src[12];
                ((gctUINT32_PTR) trgLine)[12] = (src[19] << 24) | (src[18] << 16) | (src[17] << 8) | src[16];
                ((gctUINT32_PTR) trgLine)[13] = (src[23] << 24) | (src[22] << 16) | (src[21] << 8) | src[20];
                ((gctUINT32_PTR) trgLine)[14] = (src[27] << 24) | (src[26] << 16) | (src[25] << 8) | src[24];
                ((gctUINT32_PTR) trgLine)[15] = (src[31] << 24) | (src[30] << 16) | (src[29] << 8) | src[28];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[16] = (src[ 3] << 24) | (src[ 2] << 16) | (src[ 1] << 8) | src[ 0];
                ((gctUINT32_PTR) trgLine)[17] = (src[ 7] << 24) | (src[ 6] << 16) | (src[ 5] << 8) | src[ 4];
                ((gctUINT32_PTR) trgLine)[18] = (src[11] << 24) | (src[10] << 16) | (src[ 9] << 8) | src[ 8];
                ((gctUINT32_PTR) trgLine)[19] = (src[15] << 24) | (src[14] << 16) | (src[13] << 8) | src[12];
                ((gctUINT32_PTR) trgLine)[20] = (src[19] << 24) | (src[18] << 16) | (src[17] << 8) | src[16];
                ((gctUINT32_PTR) trgLine)[21] = (src[23] << 24) | (src[22] << 16) | (src[21] << 8) | src[20];
                ((gctUINT32_PTR) trgLine)[22] = (src[27] << 24) | (src[26] << 16) | (src[25] << 8) | src[24];
                ((gctUINT32_PTR) trgLine)[23] = (src[31] << 24) | (src[30] << 16) | (src[29] << 8) | src[28];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[24] = (src[ 3] << 24) | (src[ 2] << 16) | (src[ 1] << 8) | src[ 0];
                ((gctUINT32_PTR) trgLine)[25] = (src[ 7] << 24) | (src[ 6] << 16) | (src[ 5] << 8) | src[ 4];
                ((gctUINT32_PTR) trgLine)[26] = (src[11] << 24) | (src[10] << 16) | (src[ 9] << 8) | src[ 8];
                ((gctUINT32_PTR) trgLine)[27] = (src[15] << 24) | (src[14] << 16) | (src[13] << 8) | src[12];
                ((gctUINT32_PTR) trgLine)[28] = (src[19] << 24) | (src[18] << 16) | (src[17] << 8) | src[16];
                ((gctUINT32_PTR) trgLine)[29] = (src[23] << 24) | (src[22] << 16) | (src[21] << 8) | src[20];
                ((gctUINT32_PTR) trgLine)[30] = (src[27] << 24) | (src[26] << 16) | (src[25] << 8) | src[24];
                ((gctUINT32_PTR) trgLine)[31] = (src[31] << 24) | (src[30] << 16) | (src[29] << 8) | src[28];

                /* Move to next tile. */
                trgLine += 16 * 8;
                srcLine +=  4 * 8;
            }
        }
    }
}

static void
_UploadRGB565toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGB565 to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | ((srcLine[0] & 0xF800) << 8) | ((srcLine[0] & 0x7E0) << 5) | ((srcLine[0] & 0x1F) << 3);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGB565 to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | ((srcLine[0] & 0xF800) << 8) | ((srcLine[0] & 0x7E0) << 5) | ((srcLine[0] & 0x1F) << 3);
                ((gctUINT32_PTR) trgLine)[1] = 0xFF000000 | ((srcLine[1] & 0xF800) << 8) | ((srcLine[1] & 0x7E0) << 5) | ((srcLine[1] & 0x1F) << 3);
                ((gctUINT32_PTR) trgLine)[2] = 0xFF000000 | ((srcLine[2] & 0xF800) << 8) | ((srcLine[2] & 0x7E0) << 5) | ((srcLine[2] & 0x1F) << 3);
                ((gctUINT32_PTR) trgLine)[3] = 0xFF000000 | ((srcLine[3] & 0xF800) << 8) | ((srcLine[3] & 0x7E0) << 5) | ((srcLine[3] & 0x1F) << 3);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGB565 to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = 0xFF000000 | ((srcLine[0] & 0xF800) << 8) | ((srcLine[0] & 0x7E0) << 5) | ((srcLine[0] & 0x1F) << 3);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            gctUINT16_PTR src;

            /* RGB565 to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = 0xFF000000 | ((src[0] & 0xF800) << 8) | ((src[0] & 0x7E0) << 5) | ((src[0] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[ 1] = 0xFF000000 | ((src[1] & 0xF800) << 8) | ((src[1] & 0x7E0) << 5) | ((src[1] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[ 2] = 0xFF000000 | ((src[2] & 0xF800) << 8) | ((src[2] & 0x7E0) << 5) | ((src[2] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[ 3] = 0xFF000000 | ((src[3] & 0xF800) << 8) | ((src[3] & 0x7E0) << 5) | ((src[3] & 0x1F) << 3);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[ 4] = 0xFF000000 | ((src[0] & 0xF800) << 8) | ((src[0] & 0x7E0) << 5) | ((src[0] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[ 5] = 0xFF000000 | ((src[1] & 0xF800) << 8) | ((src[1] & 0x7E0) << 5) | ((src[1] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[ 6] = 0xFF000000 | ((src[2] & 0xF800) << 8) | ((src[2] & 0x7E0) << 5) | ((src[2] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[ 7] = 0xFF000000 | ((src[3] & 0xF800) << 8) | ((src[3] & 0x7E0) << 5) | ((src[3] & 0x1F) << 3);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[ 8] = 0xFF000000 | ((src[0] & 0xF800) << 8) | ((src[0] & 0x7E0) << 5) | ((src[0] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[ 9] = 0xFF000000 | ((src[1] & 0xF800) << 8) | ((src[1] & 0x7E0) << 5) | ((src[1] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[10] = 0xFF000000 | ((src[2] & 0xF800) << 8) | ((src[2] & 0x7E0) << 5) | ((src[2] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[11] = 0xFF000000 | ((src[3] & 0xF800) << 8) | ((src[3] & 0x7E0) << 5) | ((src[3] & 0x1F) << 3);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[12] = 0xFF000000 | ((src[0] & 0xF800) << 8) | ((src[0] & 0x7E0) << 5) | ((src[0] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[13] = 0xFF000000 | ((src[1] & 0xF800) << 8) | ((src[1] & 0x7E0) << 5) | ((src[1] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[14] = 0xFF000000 | ((src[2] & 0xF800) << 8) | ((src[2] & 0x7E0) << 5) | ((src[2] & 0x1F) << 3);
            ((gctUINT32_PTR) trgLine)[15] = 0xFF000000 | ((src[3] & 0xF800) << 8) | ((src[3] & 0x7E0) << 5) | ((src[3] & 0x1F) << 3);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}

static void
_UploadRGBA4444toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = ((srcLine[0] & 0xF) << 28) | ((srcLine[0] & 0xF000) << 8) | ((srcLine[0] & 0xF00) << 4) | (srcLine[0] & 0xF0);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = ((srcLine[0] & 0xF) << 28) | ((srcLine[0] & 0xF000) << 8) | ((srcLine[0] & 0xF00) << 4) | (srcLine[0] & 0xF0);
                ((gctUINT32_PTR) trgLine)[1] = ((srcLine[1] & 0xF) << 28) | ((srcLine[1] & 0xF000) << 8) | ((srcLine[1] & 0xF00) << 4) | (srcLine[1] & 0xF0);
                ((gctUINT32_PTR) trgLine)[2] = ((srcLine[2] & 0xF) << 28) | ((srcLine[2] & 0xF000) << 8) | ((srcLine[2] & 0xF00) << 4) | (srcLine[2] & 0xF0);
                ((gctUINT32_PTR) trgLine)[3] = ((srcLine[3] & 0xF) << 28) | ((srcLine[3] & 0xF000) << 8) | ((srcLine[3] & 0xF00) << 4) | (srcLine[3] & 0xF0);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = ((srcLine[0] & 0xF) << 28) | ((srcLine[0] & 0xF000) << 8) | ((srcLine[0] & 0xF00) << 4) | (srcLine[0] & 0xF0);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            gctUINT16_PTR src;

            /* RGBA4444 to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = ((src[0] & 0xF) << 28) | ((src[0] & 0xF000) << 8) | ((src[0] & 0xF00) << 4) | (src[0] & 0xF0);
            ((gctUINT32_PTR) trgLine)[ 1] = ((src[1] & 0xF) << 28) | ((src[1] & 0xF000) << 8) | ((src[1] & 0xF00) << 4) | (src[1] & 0xF0);
            ((gctUINT32_PTR) trgLine)[ 2] = ((src[2] & 0xF) << 28) | ((src[2] & 0xF000) << 8) | ((src[2] & 0xF00) << 4) | (src[2] & 0xF0);
            ((gctUINT32_PTR) trgLine)[ 3] = ((src[3] & 0xF) << 28) | ((src[3] & 0xF000) << 8) | ((src[3] & 0xF00) << 4) | (src[3] & 0xF0);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[ 4] = ((src[0] & 0xF) << 28) | ((src[0] & 0xF000) << 8) | ((src[0] & 0xF00) << 4) | (src[0] & 0xF0);
            ((gctUINT32_PTR) trgLine)[ 5] = ((src[1] & 0xF) << 28) | ((src[1] & 0xF000) << 8) | ((src[1] & 0xF00) << 4) | (src[1] & 0xF0);
            ((gctUINT32_PTR) trgLine)[ 6] = ((src[2] & 0xF) << 28) | ((src[2] & 0xF000) << 8) | ((src[2] & 0xF00) << 4) | (src[2] & 0xF0);
            ((gctUINT32_PTR) trgLine)[ 7] = ((src[3] & 0xF) << 28) | ((src[3] & 0xF000) << 8) | ((src[3] & 0xF00) << 4) | (src[3] & 0xF0);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[ 8] = ((src[0] & 0xF) << 28) | ((src[0] & 0xF000) << 8) | ((src[0] & 0xF00) << 4) | (src[0] & 0xF0);
            ((gctUINT32_PTR) trgLine)[ 9] = ((src[1] & 0xF) << 28) | ((src[1] & 0xF000) << 8) | ((src[1] & 0xF00) << 4) | (src[1] & 0xF0);
            ((gctUINT32_PTR) trgLine)[10] = ((src[2] & 0xF) << 28) | ((src[2] & 0xF000) << 8) | ((src[2] & 0xF00) << 4) | (src[2] & 0xF0);
            ((gctUINT32_PTR) trgLine)[11] = ((src[3] & 0xF) << 28) | ((src[3] & 0xF000) << 8) | ((src[3] & 0xF00) << 4) | (src[3] & 0xF0);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[12] = ((src[0] & 0xF) << 28) | ((src[0] & 0xF000) << 8) | ((src[0] & 0xF00) << 4) | (src[0] & 0xF0);
            ((gctUINT32_PTR) trgLine)[13] = ((src[1] & 0xF) << 28) | ((src[1] & 0xF000) << 8) | ((src[1] & 0xF00) << 4) | (src[1] & 0xF0);
            ((gctUINT32_PTR) trgLine)[14] = ((src[2] & 0xF) << 28) | ((src[2] & 0xF000) << 8) | ((src[2] & 0xF00) << 4) | (src[2] & 0xF0);
            ((gctUINT32_PTR) trgLine)[15] = ((src[3] & 0xF) << 28) | ((src[3] & 0xF000) << 8) | ((src[3] & 0xF00) << 4) | (src[3] & 0xF0);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}

static void
_UploadRGBA5551toARGB(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB. */
            ((gctUINT32_PTR) trgLine)[0] = ((srcLine[0] & 0x1) << 31) | ((srcLine[0] & 0xF800) << 8) | ((srcLine[0] & 0x7C0) << 5) | ((srcLine[0] & 0x3E) << 2);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = ((srcLine[0] & 0x1) << 31) | ((srcLine[0] & 0xF800) << 8) | ((srcLine[0] & 0x7C0) << 5) | ((srcLine[0] & 0x3E) << 2);
                ((gctUINT32_PTR) trgLine)[1] = ((srcLine[1] & 0x1) << 31) | ((srcLine[1] & 0xF800) << 8) | ((srcLine[1] & 0x7C0) << 5) | ((srcLine[1] & 0x3E) << 2);
                ((gctUINT32_PTR) trgLine)[2] = ((srcLine[2] & 0x1) << 31) | ((srcLine[2] & 0xF800) << 8) | ((srcLine[2] & 0x7C0) << 5) | ((srcLine[2] & 0x3E) << 2);
                ((gctUINT32_PTR) trgLine)[3] = ((srcLine[3] & 0x1) << 31) | ((srcLine[3] & 0xF800) << 8) | ((srcLine[3] & 0x7C0) << 5) | ((srcLine[3] & 0x3E) << 2);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB: part tile row. */
                ((gctUINT32_PTR) trgLine)[0] = ((srcLine[0] & 0x1) << 31) | ((srcLine[0] & 0xF800) << 8) | ((srcLine[0] & 0x7C0) << 5) | ((srcLine[0] & 0x3E) << 2);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 4;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            gctUINT16_PTR src;

            /* RGBA5551 to ARGB: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[ 0] = ((src[0] & 0x1) << 31) | ((src[0] & 0xF800) << 8) | ((src[0] & 0x7C0) << 5) | ((src[0] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[ 1] = ((src[1] & 0x1) << 31) | ((src[1] & 0xF800) << 8) | ((src[1] & 0x7C0) << 5) | ((src[1] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[ 2] = ((src[2] & 0x1) << 31) | ((src[2] & 0xF800) << 8) | ((src[2] & 0x7C0) << 5) | ((src[2] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[ 3] = ((src[3] & 0x1) << 31) | ((src[3] & 0xF800) << 8) | ((src[3] & 0x7C0) << 5) | ((src[3] & 0x3E) << 2);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[ 4] = ((src[0] & 0x1) << 31) | ((src[0] & 0xF800) << 8) | ((src[0] & 0x7C0) << 5) | ((src[0] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[ 5] = ((src[1] & 0x1) << 31) | ((src[1] & 0xF800) << 8) | ((src[1] & 0x7C0) << 5) | ((src[1] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[ 6] = ((src[2] & 0x1) << 31) | ((src[2] & 0xF800) << 8) | ((src[2] & 0x7C0) << 5) | ((src[2] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[ 7] = ((src[3] & 0x1) << 31) | ((src[3] & 0xF800) << 8) | ((src[3] & 0x7C0) << 5) | ((src[3] & 0x3E) << 2);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[ 8] = ((src[0] & 0x1) << 31) | ((src[0] & 0xF800) << 8) | ((src[0] & 0x7C0) << 5) | ((src[0] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[ 9] = ((src[1] & 0x1) << 31) | ((src[1] & 0xF800) << 8) | ((src[1] & 0x7C0) << 5) | ((src[1] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[10] = ((src[2] & 0x1) << 31) | ((src[2] & 0xF800) << 8) | ((src[2] & 0x7C0) << 5) | ((src[2] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[11] = ((src[3] & 0x1) << 31) | ((src[3] & 0xF800) << 8) | ((src[3] & 0x7C0) << 5) | ((src[3] & 0x3E) << 2);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[12] = ((src[0] & 0x1) << 31) | ((src[0] & 0xF800) << 8) | ((src[0] & 0x7C0) << 5) | ((src[0] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[13] = ((src[1] & 0x1) << 31) | ((src[1] & 0xF800) << 8) | ((src[1] & 0x7C0) << 5) | ((src[1] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[14] = ((src[2] & 0x1) << 31) | ((src[2] & 0xF800) << 8) | ((src[2] & 0x7C0) << 5) | ((src[2] & 0x3E) << 2);
            ((gctUINT32_PTR) trgLine)[15] = ((src[3] & 0x1) << 31) | ((src[3] & 0xF800) << 8) | ((src[3] & 0x7C0) << 5) | ((src[3] & 0x3E) << 2);

            /* Move to next tile. */
            trgLine += 16 * 4;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}

static void
_Upload8bppto8bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT8_PTR srcLine;
    gctUINT8_PTR trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 1);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
            srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

            /* 8 bpp to 8 bpp. */
            trgLine[0] = srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* 8 bpp to 8 bpp: one tile row. */
                if ((((gctUINT32) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned source. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR)srcLine)[0];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = (srcLine[3] << 24) | (srcLine[2] << 16) | (srcLine[1] << 8) | srcLine[0];
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
                srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

                /* 8 bpp to 8 bpp: part tile row. */
                trgLine[0] = srcLine[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 1;
        srcLine = (gctUINT8_PTR) Memory  + y  * SourceStride + x  * 1;

        if ((((gctUINT32) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned source. */
            for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
            {
                ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) (srcLine + SourceStride * 0))[0];
                ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) (srcLine + SourceStride * 1))[0];
                ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) (srcLine + SourceStride * 2))[0];
                ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) (srcLine + SourceStride * 3))[0];

                /* Move to next tile. */
                trgLine += 16 * 1;
                srcLine +=  4 * 1;
            }
        }
        else
        {
            for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
            {
                gctUINT8_PTR src;

                /* 8 bpp to 8 bpp: one tile. */
                src  = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[1] = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[2] = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];

                src += SourceStride;
                ((gctUINT32_PTR) trgLine)[3] = (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];

                /* Move to next tile. */
                trgLine += 16 * 1;
                srcLine +=  4 * 1;
            }
        }
    }
}

static void
_Upload16bppto16bpp(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* 16 bpp to 16 bpp. */
            ((gctUINT16_PTR) trgLine)[0] = srcLine[0];
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* 16 bpp to 16 bpp: one tile row. */
                if ((((gctUINT32) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
                {
                    /* Special optimization for aligned case. */
                    ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) srcLine)[0];
                    ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) srcLine)[1];
                }
                else
                {
                    ((gctUINT32_PTR) trgLine)[0] = (srcLine[1] << 16) | srcLine[0];
                    ((gctUINT32_PTR) trgLine)[1] = (srcLine[3] << 16) | srcLine[2];
                }
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* 16 bpp to 16 bpp: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = srcLine[0];
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

        if ((((gctUINT32) srcLine & 3) == 0) && ((SourceStride & 3) == 0))
        {
            /* Special optimization for aligned case. */
            for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
            {
                gctUINT16_PTR src;

                /* 16 bpp to 16 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[1] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[2] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[3] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[4] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[5] = ((gctUINT32_PTR) src)[1];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[6] = ((gctUINT32_PTR) src)[0];
                ((gctUINT32_PTR) trgLine)[7] = ((gctUINT32_PTR) src)[1];

                /* Move to next tile. */
                trgLine += 16 * 2;
                srcLine +=  4 * 1; /* srcLine is in 16 bits. */
            }
        }
        else
        {
            for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
            {
                gctUINT16_PTR src;

                /* 16 bpp to 16 bpp: one tile. */
                src = srcLine;
                ((gctUINT32_PTR) trgLine)[0] = (src[1] << 16) | src[0];
                ((gctUINT32_PTR) trgLine)[1] = (src[3] << 16) | src[2];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[2] = (src[1] << 16) | src[0];
                ((gctUINT32_PTR) trgLine)[3] = (src[3] << 16) | src[2];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[4] = (src[1] << 16) | src[0];
                ((gctUINT32_PTR) trgLine)[5] = (src[3] << 16) | src[2];

                src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
                ((gctUINT32_PTR) trgLine)[6] = (src[1] << 16) | src[0];
                ((gctUINT32_PTR) trgLine)[7] = (src[3] << 16) | src[2];

                /* Move to next tile. */
                trgLine += 16 * 2;
                srcLine +=  4 * 1; /* srcLine is in 16 bits. */
            }
        }
    }
}

static void
_UploadRGBA4444toARGB4444(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA4444 to ARGB4444. */
            ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB4444: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = ((srcLine[1] & 0xFFF0) << 12) | ((srcLine[1] & 0xF) << 28) | (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12);
                ((gctUINT32_PTR) trgLine)[1] = ((srcLine[3] & 0xFFF0) << 12) | ((srcLine[3] & 0xF) << 28) | (srcLine[2] >> 4) | ((srcLine[2] & 0xF) << 12);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA4444 to ARGB4444: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 4) | ((srcLine[0] & 0xF) << 12);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            gctUINT16_PTR src;

            /* RGBA4444 to ARGB4444: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[0] = ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) | (src[0] >> 4) | ((src[0] & 0xF) << 12);
            ((gctUINT32_PTR) trgLine)[1] = ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) | (src[2] >> 4) | ((src[2] & 0xF) << 12);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[2] = ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) | (src[0] >> 4) | ((src[0] & 0xF) << 12);
            ((gctUINT32_PTR) trgLine)[3] = ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) | (src[2] >> 4) | ((src[2] & 0xF) << 12);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[4] = ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) | (src[0] >> 4) | ((src[0] & 0xF) << 12);
            ((gctUINT32_PTR) trgLine)[5] = ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) | (src[2] >> 4) | ((src[2] & 0xF) << 12);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[6] = ((src[1] & 0xFFF0) << 12) | ((src[1] & 0xF) << 28) | (src[0] >> 4) | ((src[0] & 0xF) << 12);
            ((gctUINT32_PTR) trgLine)[7] = ((src[3] & 0xFFF0) << 12) | ((src[3] & 0xF) << 28) | (src[2] >> 4) | ((src[2] & 0xF) << 12);

            /* Move to next tile. */
            trgLine += 16 * 2;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}

static void
_UploadRGBA5551toARGB1555(
    IN gctPOINTER Logical,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT EdgeX[],
    IN gctUINT EdgeY[],
    IN gctUINT CountX,
    IN gctUINT CountY,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride
    )
{
    gctUINT x, y;
    gctUINT i, j;
    gctUINT xt, yt;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT16_PTR srcLine;
    gctUINT8_PTR  trgLine;

    /* Move linear Memory to [0,0] origin. */
    Memory = (gctPOINTER) ((gctUINT8_PTR) Memory - SourceStride * Y - X * 2);

    /* Y edge - X edge. */
    for (j = 0; j < CountY; j++)
    {
        y  = EdgeY[j];
        yt = y & ~0x03;

        for (i = 0; i < CountX; i++)
        {
            x  = EdgeX[i];
            xt = ((x &  0x03) << 0) +
                 ((y &  0x03) << 2) +
                 ((x & ~0x03) << 2);

            trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
            srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

            /* RGBA5551 to ARGB1555. */
            ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15);
        }
    }

    if (CountY)
    {
        /* Y edge - X middle. */
        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            for (j = 0; j < CountY; j++)
            {
                y  = EdgeY[j];

                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);
                yt = y & ~0x03;

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB1555: one tile row. */
                ((gctUINT32_PTR) trgLine)[0] = ((srcLine[1] & 0xFFFE) << 15) | ((srcLine[1] & 0x1) << 31) | (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15);
                ((gctUINT32_PTR) trgLine)[1] = ((srcLine[3] & 0xFFFE) << 15) | ((srcLine[3] & 0x1) << 31) | (srcLine[2] >> 1) | ((srcLine[2] & 0x1) << 15);
            }
        }
    }

    if (CountX)
    {
        /* Y middle - X edge. */
        for (y = ((Y + 3) & ~3); y < (bottom & ~3); y++)
        {
            yt = y & ~0x03;

            for (i = 0; i < CountX; i++)
            {
                x  = EdgeX[i];
                xt = ((x &  0x03) << 0) +
                     ((y &  0x03) << 2) +
                     ((x & ~0x03) << 2);

                trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
                srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

                /* RGBA5551 to ARGB1555: part tile row. */
                ((gctUINT16_PTR) trgLine)[0] = (srcLine[0] >> 1) | ((srcLine[0] & 0x1) << 15);
            }
        }
    }

    /* Y middle - X middle. */
    for (y = ((Y + 3) & ~3); y < (bottom & ~3); y += 4)
    {
        x  = ((X + 3) & ~3);

        xt = ((x &  0x03) << 0) +
             ((y &  0x03) << 2) +
             ((x & ~0x03) << 2);
        yt = y & ~0x03;

        trgLine = (gctUINT8_PTR) Logical + yt * TargetStride + xt * 2;
        srcLine = (gctUINT16_PTR) ((gctUINT8_PTR) Memory  + y  * SourceStride + x * 2);

        for (x = ((X + 3) & ~3); x < (right & ~3); x += 4)
        {
            gctUINT16_PTR src;

            /* RGBA5551 to ARGB1555: one tile. */
            src = srcLine;
            ((gctUINT32_PTR) trgLine)[0] = ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31) | (src[0] >> 1) | ((src[0] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[1] = ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31) | (src[2] >> 1) | ((src[2] & 0x1) << 15);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[2] = ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31) | (src[0] >> 1) | ((src[0] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[3] = ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31) | (src[2] >> 1) | ((src[2] & 0x1) << 15);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[4] = ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31) | (src[0] >> 1) | ((src[0] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[5] = ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31) | (src[2] >> 1) | ((src[2] & 0x1) << 15);

            src = (gctUINT16_PTR) ((gctUINT8_PTR) src + SourceStride);
            ((gctUINT32_PTR) trgLine)[6] = ((src[1] & 0xFFFE) << 15) | ((src[1] & 0x1) << 31) | (src[0] >> 1) | ((src[0] & 0x1) << 15);
            ((gctUINT32_PTR) trgLine)[7] = ((src[3] & 0xFFFE) << 15) | ((src[3] & 0x1) << 31) | (src[2] >> 1) | ((src[2] & 0x1) << 15);

            /* Move to next tile. */
            trgLine += 16 * 2;
            srcLine +=  4 * 1; /* srcLine is in 16 bits. */
        }
    }
}

/*******************************************************************************
**
**  gcoHARDWARE_UploadTexture
**
**  Upload one slice into a texture.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gceSURF_FORMAT TargetFormat
**          Destination texture format.
**
**      gctUINT32 Address
**          Hardware specific base address for mip map.
**
**      gctPOINTER Logical
**          Logical base address for mip map.
**
**      gctUINT32 Offset
**          Offset into mip map to upload.
**
**      gctINT TargetStride
**          Stride of the destination texture.
**
**      gctUINT X
**          X origin of data to upload.
**
**      gctUINT Y
**          Y origin of data to upload.
**
**      gctUINT Width
**          Width of texture to upload.
**
**      gctUINT Height
**          Height of texture to upload.
**
**      gctCONST_POINTER Memory
**          Pointer to user buffer containing the data to upload.
**
**      gctINT SourceStride
**          Stride of user buffer.
**
**      gceSURF_FORMAT SourceFormat
**          Format of the source texture to upload.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_UploadTexture(
    IN gceSURF_FORMAT TargetFormat,
    IN gctUINT32 Address,
    IN gctPOINTER Logical,
    IN gctUINT32 Offset,
    IN gctINT TargetStride,
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctCONST_POINTER Memory,
    IN gctINT SourceStride,
    IN gceSURF_FORMAT SourceFormat
    )
{
    gceSTATUS status;
    /*gcoHARDWARE hardware;*/
    gctUINT x, y;
    gctUINT edgeX[6];
    gctUINT edgeY[6];
    gctUINT countX = 0;
    gctUINT countY = 0;
    gctUINT32 bitsPerPixel;
    gctUINT32 bytesPerLine;
    gctUINT32 bytesPerTile;
    gctUINT8 srcOdd, trgOdd;
    gctUINT right  = X + Width;
    gctUINT bottom = Y + Height;
    gctUINT srcBit;
    gctUINT srcBitStep;
    gctINT srcStride;
    gctUINT8* srcPixel;
    gctUINT8* srcLine;
    gcsSURF_FORMAT_INFO_PTR srcFormat[2];
    gctUINT trgBit;
    gctUINT trgBitStep;
    gctUINT8* trgPixel;
    gctUINT8* trgLine;
    gctUINT8* trgTile;
    gcsSURF_FORMAT_INFO_PTR trgFormat[2];

    gcmHEADER_ARG("TargetFormat=%d Address=%08x Logical=0x%x "
                  "Offset=%u TargetStride=%d X=%u Y=%u Width=%u Height=%u "
                  "Memory=0x%x SourceStride=%d SourceFormat=%d",
                  TargetFormat, Address, Logical, Offset,
                  TargetStride, X, Y, Width, Height, Memory, SourceStride,
                  SourceFormat);

    /*gcmGETHARDWARE(hardware);*/

    /* Compute dest logical. */
    Logical = (gctPOINTER) ((gctUINT8_PTR) Logical + Offset);

    /* Compute edge coordinates. */
    if (Width < 4)
    {
        for (x = X; x < right; x++)
        {
           edgeX[countX++] = x;
        }
    }
    else
    {
        for (x = X; x < ((X + 3) & ~3); x++)
        {
            edgeX[countX++] = x;
        }

        for (x = (right & ~3); x < right; x++)
        {
            edgeX[countX++] = x;
        }
    }

    if (Height < 4)
    {
       for (y = Y; y < bottom; y++)
       {
           edgeY[countY++] = y;
       }
    }
    else
    {
        for (y = Y; y < ((Y + 3) & ~3); y++)
        {
           edgeY[countY++] = y;
        }

        for (y = (bottom & ~3); y < bottom; y++)
        {
           edgeY[countY++] = y;
        }
    }

    /* Fast path(s) for all common OpenGL ES formats. */
    if ((TargetFormat == gcvSURF_A8R8G8B8)
    ||  (TargetFormat == gcvSURF_X8R8G8B8)
    )
    {
        switch (SourceFormat)
        {
        case gcvSURF_A8:
            _UploadA8toARGB(Logical,
                            TargetStride,
                            X, Y,
                            Width, Height,
                            edgeX, edgeY,
                            countX, countY,
                            Memory,
                            (SourceStride == 0)
                            ? (gctINT) Width
                            : SourceStride);

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_B8G8R8:
            _UploadBGRtoARGB(Logical,
                             TargetStride,
                             X, Y,
                             Width, Height,
                             edgeX, edgeY,
                             countX, countY,
                             Memory,
                             (SourceStride == 0)
                             ? (gctINT) Width * 3
                             : SourceStride);

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_A8B8G8R8:
            _UploadABGRtoARGB(Logical,
                              TargetStride,
                              X, Y,
                              Width, Height,
                              edgeX, edgeY,
                              countX, countY,
                              Memory,
                              (SourceStride == 0)
                              ? (gctINT) Width * 4
                              : SourceStride);

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_L8:
            _UploadL8toARGB(Logical,
                            TargetStride,
                            X, Y,
                            Width, Height,
                            edgeX, edgeY,
                            countX, countY,
                            Memory,
                            (SourceStride == 0)
                            ? (gctINT) Width
                            : SourceStride);

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_A8L8:
            _UploadA8L8toARGB(Logical,
                              TargetStride,
                              X, Y,
                              Width, Height,
                              edgeX, edgeY,
                              countX, countY,
                              Memory,
                              (SourceStride == 0)
                              ? (gctINT) Width * 2
                              : SourceStride);

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_A8R8G8B8:
            _Upload32bppto32bpp(Logical,
                                TargetStride,
                                X, Y,
                                Width, Height,
                                edgeX, edgeY,
                                countX, countY,
                                Memory,
                                (SourceStride == 0)
                                ? (gctINT) Width * 4
                                : SourceStride);

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_R5G6B5:
            _UploadRGB565toARGB(Logical,
                                TargetStride,
                                X, Y,
                                Width, Height,
                                edgeX, edgeY,
                                countX, countY,
                                Memory,
                                (SourceStride == 0)
                                ? (gctINT) Width * 2
                                : SourceStride);

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_R4G4B4A4:
            _UploadRGBA4444toARGB(Logical,
                                  TargetStride,
                                  X, Y,
                                  Width, Height,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case gcvSURF_R5G5B5A1:
            _UploadRGBA5551toARGB(Logical,
                                  TargetStride,
                                  X, Y,
                                  Width, Height,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width * 2
                                  : SourceStride);

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        default:
            break;
        }
    }

    else
    {
        switch (SourceFormat)
        {
        case gcvSURF_A8:
            if (TargetFormat == gcvSURF_A8)
            {
                _Upload8bppto8bpp(Logical,
                                  TargetStride,
                                  X, Y,
                                  Width, Height,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_B8G8R8:
            if (TargetFormat == gcvSURF_X8R8G8B8)
            {
               /* Same as BGR to ARGB. */
                _UploadBGRtoARGB(Logical,
                                 TargetStride,
                                 X, Y,
                                 Width, Height,
                                 edgeX, edgeY,
                                 countX, countY,
                                 Memory,
                                 (SourceStride == 0)
                                 ? (gctINT) Width * 3
                                 : SourceStride);

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            else if (TargetFormat == gcvSURF_X8B8G8R8)
            {
                _UploadBGRtoABGR(Logical,
                                 TargetStride,
                                 X, Y,
                                 Width, Height,
                                 edgeX, edgeY,
                                 countX, countY,
                                 Memory,
                                 (SourceStride == 0)
                                 ? (gctINT) Width * 3
                                 : SourceStride);

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }

            break;

        /* case gcvSURF_A8B8G8R8: */

        case gcvSURF_L8:
            if (TargetFormat == gcvSURF_L8)
            {
                _Upload8bppto8bpp(Logical,
                                  TargetStride,
                                  X, Y,
                                  Width, Height,
                                  edgeX, edgeY,
                                  countX, countY,
                                  Memory,
                                  (SourceStride == 0)
                                  ? (gctINT) Width
                                  : SourceStride);

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_A8L8:
            if (TargetFormat == gcvSURF_A8L8)
            {
                _Upload16bppto16bpp(Logical,
                                    TargetStride,
                                    X, Y,
                                    Width, Height,
                                    edgeX, edgeY,
                                    countX, countY,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width * 2
                                    : SourceStride);

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        /* case gcvSURF_A8R8G8B8: */

        case gcvSURF_R5G6B5:
            if (TargetFormat == gcvSURF_R5G6B5)
            {
                _Upload16bppto16bpp(Logical,
                                    TargetStride,
                                    X, Y,
                                    Width, Height,
                                    edgeX, edgeY,
                                    countX, countY,
                                    Memory,
                                    (SourceStride == 0)
                                    ? (gctINT) Width * 2
                                    : SourceStride);

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_R4G4B4A4:
            if (TargetFormat == gcvSURF_A4R4G4B4)
            {
                _UploadRGBA4444toARGB4444(Logical,
                                          TargetStride,
                                          X, Y,
                                          Width, Height,
                                          edgeX, edgeY,
                                          countX, countY,
                                          Memory,
                                          (SourceStride == 0)
                                          ? (gctINT) Width * 2
                                          : SourceStride);

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        case gcvSURF_R5G5B5A1:
            if (TargetFormat == gcvSURF_A1R5G5B5)
            {
                _UploadRGBA5551toARGB1555(Logical,
                                          TargetStride,
                                          X, Y,
                                          Width, Height,
                                          edgeX, edgeY,
                                          countX, countY,
                                          Memory,
                                          (SourceStride == 0)
                                          ? (gctINT) Width * 2
                                          : SourceStride);

                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
            break;

        default:
            break;
        }
    }

    /* Get number of bytes per pixel and tile for the format. */
    gcmONERROR(gcoHARDWARE_ConvertFormat(TargetFormat,
                                         &bitsPerPixel,
                                         &bytesPerTile));

    if
    (
        /* Check for OpenCL image. */
        SourceFormat & 0x80000000
    )
    {
        /* OpenCL image has to be linear without alignment. */
        /* There is no format conversion dur to OpenCL Map API. */
        gctUINT32 bytesPerPixel = bitsPerPixel / 8;

        /* Compute proper memory stride. */
        srcStride = (SourceStride == 0)
                  ? (gctINT) (Width * bytesPerPixel)
                  : SourceStride;

        srcLine = (gctUINT8_PTR) Memory + bytesPerPixel * X;
        trgLine = (gctUINT8_PTR) Logical + Offset + bytesPerPixel * X;
        for (y = Y; y < Height; y++)
        {
            gcmVERIFY_OK(
                gcoOS_MemCopy(trgLine, srcLine, bytesPerPixel * Width));

            srcLine += SourceStride;
            trgLine += SourceStride;
        }

        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Compute number of bytes per tile line. */
    bytesPerLine = bitsPerPixel * 4 / 8;

    /* See if the memory format equals the texture format (easy case). */
    if ((SourceFormat == TargetFormat)
	||  ((SourceFormat == gcvSURF_A8R8G8B8) && (TargetFormat == gcvSURF_X8R8G8B8))
	||  ((SourceFormat == gcvSURF_A8B8G8R8) && (TargetFormat == gcvSURF_X8B8G8R8)))
    {
		switch (bitsPerPixel)
		{
		case 8:
	        _Upload8bppto8bpp(Logical,
	                          TargetStride,
                              X, Y,
                              Width, Height,
                              edgeX, edgeY,
                              countX, countY,
	                          Memory,
	                          (SourceStride == 0)
                              ? (gctINT) Width
	                          : SourceStride);

            /* Success. */
            gcmFOOTER_NO();
    	    return gcvSTATUS_OK;

		case 16:
	        _Upload16bppto16bpp(Logical,
	                            TargetStride,
                                X, Y,
                                Width, Height,
                                edgeX, edgeY,
                                countX, countY,
	                            Memory,
	                            (SourceStride == 0)
                                ? (gctINT) Width * 2
	                            : SourceStride);

            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;

		case 32:
	        _Upload32bppto32bpp(Logical,
	                            TargetStride,
                                X, Y,
                                Width, Height,
                                edgeX, edgeY,
                                countX, countY,
	                            Memory,
	                            (SourceStride == 0)
                                ? (gctINT) Width * 4
	                            : SourceStride);

            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;

		case 64:
	        _Upload64bppto64bpp(Logical,
	                            TargetStride,
                                X, Y,
                                Width, Height,
                                edgeX, edgeY,
                                countX, countY,
	                            Memory,
	                            (SourceStride == 0)
                                ? (gctINT) Width * 8
	                            : SourceStride);

            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;

		default:
			/* Invalid format. */
			gcmASSERT(gcvTRUE);
		}

        /* Compute proper memory stride. */
        srcStride = (SourceStride == 0)
                  ? (gctINT) (Width * bitsPerPixel / 8)
                  : SourceStride;

        if (SourceFormat == gcvSURF_DXT1)
        {
            gcmASSERT((X == 0) && (Y == 0));

            srcLine      = (gctUINT8_PTR) Memory;
            trgLine      = (gctUINT8_PTR) Logical;
            bytesPerLine = Width * bytesPerTile / 4;

            for (y = Y; y < Height; y += 4)
            {
                gcmVERIFY_OK(
                    gcoOS_MemCopy(trgLine, srcLine, bytesPerLine));

                trgLine += TargetStride * 4;
                srcLine += srcStride    * 4;
            }

            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        /* Success. */
        gcmFOOTER_ARG("bitsPerPixel=%d not supported", bitsPerPixel);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmTRACE_ZONE(
        gcvLEVEL_WARNING, gcvZONE_TEXTURE,
        "Slow path: SourceFormat=%d TargetFormat=%d",
        SourceFormat, TargetFormat
        );

    /* Query format specifics. */
    gcmONERROR(gcoSURF_QueryFormat(TargetFormat, trgFormat));

    gcmONERROR(gcoSURF_QueryFormat(SourceFormat, srcFormat));

    /* Determine bit step. */
    srcBitStep = srcFormat[0]->interleaved
               ? srcFormat[0]->bitsPerPixel * 2
               : srcFormat[0]->bitsPerPixel;

    trgBitStep = trgFormat[0]->interleaved
               ? trgFormat[0]->bitsPerPixel * 2
               : trgFormat[0]->bitsPerPixel;

    /* Compute proper memory stride. */
    srcStride = (SourceStride == 0)
              ? (gctINT) Width * ((srcFormat[0]->bitsPerPixel + 7) >> 3)
              : SourceStride;

    /* Compute the starting source and target addresses. */
    srcLine = (gctUINT8_PTR) Memory;

    /* Compute Address of line inside the tile in which X,Y reside. */
    trgLine = (gctUINT8_PTR) Logical
            + (Y & ~3) * TargetStride + (Y & 3) * bytesPerLine
            + (X >> 2) * bytesPerTile;

    /* Walk all vertical lines. */
    for (y = Y; y < bottom; y++)
    {
        /* Get starting source and target addresses. */
        srcPixel = srcLine;
        trgTile  = trgLine;
        trgPixel = trgTile + (X & 3) * bitsPerPixel / 8;

        /* Reset bit positions. */
        srcBit = 0;
        trgBit = 0;

        /* Walk all horizontal pixels. */
        for (x = X; x < right; x++)
        {
            /* Determine whether to use odd format descriptor for the current
            ** pixel. */
            srcOdd = x & srcFormat[0]->interleaved;
            trgOdd = x & trgFormat[0]->interleaved;

            /* Convert the current pixel. */
            gcmONERROR(gcoHARDWARE_ConvertPixel(srcPixel,
                                                trgPixel,
                                                srcBit,
                                                trgBit,
                                                srcFormat[srcOdd],
                                                trgFormat[trgOdd],
                                                gcvNULL,
                                                gcvNULL));

            /* Move to the next pixel in the source. */
            if (!srcFormat[0]->interleaved || srcOdd)
            {
                srcBit   += srcBitStep;
                srcPixel += srcBit >> 3;
                srcBit   &= 7;
            }

            /* Move to the next pixel in the target. */
            if ((x & 3) < 3)
            {
                /* Still within a tile, update to next pixel. */
                if (!trgFormat[0]->interleaved || trgOdd)
                {
                    trgBit   += trgBitStep;
                    trgPixel += trgBit >> 3;
                    trgBit   &= 7;
                }
            }
            else
            {
                /* Move to next tiled address in target. */
                trgTile += bytesPerTile;
                trgPixel = trgTile;
                trgBit = 0;
            }
        }

        /* Move to next line. */
        srcLine += srcStride;

        if ((y & 3) < 3)
        {
            /* Still within a tile, update to next line inside the tile. */
            trgLine += bytesPerLine;
        }
        else
        {
            /* New tile, move to beginning of new tile. */
            trgLine += TargetStride * 4 - 3 * bytesPerLine;
        }
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
**  gcoHARDWARE_FlushTexture
**
**  Flish the texture cache.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_FlushTexture(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Set texture flush flag. */
    Hardware->hwTxFlush = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_BindTexture
**
**  Bind texture info to a sampler.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctINT Sampler
**          Sampler number.
**
**      gcsSAMPLER_PTR SamplerInfo
**          Pointer to a gcsSAMPLER structure.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_BindTexture(
    IN gctINT Sampler,
    IN gcsSAMPLER_PTR SamplerInfo
    )
{
    gceSTATUS status;
    gctBOOL type;
    gctUINT32 width, height;
    gctUINT32 endian;
    gctUINT32 bias = 0, lodmin = 0, lodmax = 0;
    gctINT format;
    gctUINT32 i;
    gctUINT32 sampleMode, sampleWH, sampleLogWH, sampleLOD, sampleModeEx;
    gctUINT32 sampleTSConfig, sampleTSBuffer, sampleTSClearValue;
    gcoHARDWARE hardware;
    gctUINT32 samplerHeight;
    gceTEXTURE_FILTER mipFilter;

    static const gctINT32 addressXlate[] =
    {
        /* gcvTEXTURE_WRAP */
        0x0,
        /* gcvTEXTURE_CLAMP */
        0x2,
        /* gcvTEXTURE_BORDER */
        0x3,
        /* gcvTEXTURE_MIRROR */
        0x1,
        /* gcvTEXTURE_MIRROR_ONCE */
        -1,
    };

    static const gctINT32 magXlate[] =
    {
        /* gcvTEXTURE_NONE */
        0x0,
        /* gcvTEXTURE_POINT */
        0x1,
        /* gcvTEXTURE_LINEAR */
        0x2,
        /* gcvTEXTURE_ANISOTROPIC */
        0x3,
    };

    static const gctINT32 minXlate[] =
    {
        /* gcvTEXTURE_NONE */
        0x0,
        /* gcvTEXTURE_POINT */
        0x1,
        /* gcvTEXTURE_LINEAR */
        0x2,
        /* gcvTEXTURE_ANISOTROPIC */
        0x3,
    };

    static const gctINT32 mipXlate[] =
    {
        /* gcvTEXTURE_NONE */
        0x0,
        /* gcvTEXTURE_POINT */
        0x1,
        /* gcvTEXTURE_LINEAR */
        0x2,
        /* gcvTEXTURE_ANISOTROPIC */
        0x3,
    };

    static const gctINT32 alignXlate[] =
    {
        /* gcvSURF_FOUR */
        0x0,
        /* gcvSURF_SIXTEEN */
        0x1,
        /* gcvSURF_SUPER_TILED */
        0x2,
        /* gcvSURF_SPLIT_TILED */
        0x3,
        /* gcvSURF_SPLIT_SUPER_TILED */
        0x4,
    };

    gcmHEADER_ARG("Sampler=%d SamplerInfo=0x%x",
                  Sampler, SamplerInfo);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    if (((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
    {
        gcmDEBUG_VERIFY_ARGUMENT((Sampler >= 0) && (Sampler < gcdSAMPLERS));
    }
    else
    {
        gcmDEBUG_VERIFY_ARGUMENT((Sampler >= 0) && (Sampler < 16));
    }
    gcmDEBUG_VERIFY_ARGUMENT(SamplerInfo != gcvNULL);

    if (SamplerInfo->depth > 1)
    {
        /* 3D texture. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    samplerHeight = SamplerInfo->height;
    mipFilter     = SamplerInfo->textureInfo->mipFilter;

    if (SamplerInfo->width == 0)
    {
        type = 0x0;
    }
    else if (samplerHeight > 1)
    {
        if((hardware->chipModel == gcv4000)
        && (hardware->chipRevision == 0x5208)
        && (mipFilter == gcvTEXTURE_LINEAR))
        {
            mipFilter = gcvTEXTURE_POINT;
        }

        if (SamplerInfo->faces == 6)
        {
            type = 0x5;
        }
        else
        {
            type = 0x2;
        }
    }
    else
    {
        if (SamplerInfo->faces == 6)
        {
            type = 0x5;
        }
        else
        {
            type = 0x2;
        }

        samplerHeight  = 1;
    }

    switch (SamplerInfo->endianHint)
    {
    case gcvENDIAN_SWAP_WORD:
        endian = hardware->bigEndian
               ? 0x1
               : 0x0;
        break;

    case gcvENDIAN_SWAP_DWORD:
        endian = hardware->bigEndian
               ? 0x2
               : 0x0;
        break;

    default:
        endian = 0x0;
        break;
    }

    /* Convert the format. */
    format = _GetTextureFormat(SamplerInfo->format);

    if (format == -1)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    if (((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
    {
        /* Set the gcregTXConfig register value. */
        sampleMode = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (type) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:13) - (0 ? 17:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:13) - (0 ? 17:13) + 1))))))) << (0 ? 17:13))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 17:13) - (0 ? 17:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:13) - (0 ? 17:13) + 1))))))) << (0 ? 17:13)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:3) - (0 ? 4:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:3) - (0 ? 4:3) + 1))))))) << (0 ? 4:3))) | (((gctUINT32) ((gctUINT32) (addressXlate[SamplerInfo->textureInfo->s]) & ((gctUINT32) ((((1 ? 4:3) - (0 ? 4:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:3) - (0 ? 4:3) + 1))))))) << (0 ? 4:3)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:5) - (0 ? 6:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:5) - (0 ? 6:5) + 1))))))) << (0 ? 6:5))) | (((gctUINT32) ((gctUINT32) (addressXlate[SamplerInfo->textureInfo->t]) & ((gctUINT32) ((((1 ? 6:5) - (0 ? 6:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:5) - (0 ? 6:5) + 1))))))) << (0 ? 6:5)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:7) - (0 ? 8:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:7) - (0 ? 8:7) + 1))))))) << (0 ? 8:7))) | (((gctUINT32) ((gctUINT32) (minXlate[SamplerInfo->textureInfo->minFilter]) & ((gctUINT32) ((((1 ? 8:7) - (0 ? 8:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:7) - (0 ? 8:7) + 1))))))) << (0 ? 8:7)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:11) - (0 ? 12:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:11) - (0 ? 12:11) + 1))))))) << (0 ? 12:11))) | (((gctUINT32) ((gctUINT32) (magXlate[SamplerInfo->textureInfo->magFilter]) & ((gctUINT32) ((((1 ? 12:11) - (0 ? 12:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:11) - (0 ? 12:11) + 1))))))) << (0 ? 12:11)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:9) - (0 ? 10:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:9) - (0 ? 10:9) + 1))))))) << (0 ? 10:9))) | (((gctUINT32) ((gctUINT32) (mipXlate[mipFilter]) & ((gctUINT32) ((((1 ? 10:9) - (0 ? 10:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:9) - (0 ? 10:9) + 1))))))) << (0 ? 10:9)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1))))))) << (0 ? 19:19))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->roundUV) & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1))))))) << (0 ? 19:19)))
            /* Compute log2 in 5.5 format. */
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) ((SamplerInfo->textureInfo->anisoFilter == 1) ? 0 : gcoMATH_Log2in5dot5(SamplerInfo->textureInfo->anisoFilter)) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
            ;

        /* Set the gcregTXExtConfig register value. */
        sampleModeEx = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) ((format >> 12)) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8))) | (((gctUINT32) ((gctUINT32) ((format >> 20)) & ((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12))) | (((gctUINT32) ((gctUINT32) ((format >> 23)) & ((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16))) | (((gctUINT32) ((gctUINT32) ((format >> 26)) & ((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:20) - (0 ? 22:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20))) | (((gctUINT32) ((gctUINT32) ((format >> 29)) & ((gctUINT32) ((((1 ? 22:20) - (0 ? 22:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->hasTileStatus) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:26) - (0 ? 28:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:26) - (0 ? 28:26) + 1))))))) << (0 ? 28:26))) | (((gctUINT32) ((gctUINT32) (alignXlate[SamplerInfo->hAlignment]) & ((gctUINT32) ((((1 ? 28:26) - (0 ? 28:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:26) - (0 ? 28:26) + 1))))))) << (0 ? 28:26)));

        /* Set the gcregTXSize register value. */
        sampleWH = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->width) & ((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16))) | (((gctUINT32) ((gctUINT32) (samplerHeight) & ((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16)));

        /* Compute log2 in 5.5 format for width and height. */
        width  = gcoMATH_Log2in5dot5(SamplerInfo->width);
        height = gcoMATH_Log2in5dot5(samplerHeight);

        /* Set the gcregTXLogSize register value. */
        sampleLogWH = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0))) | (((gctUINT32) ((gctUINT32) (width) & ((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:10) - (0 ? 19:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:10) - (0 ? 19:10) + 1))))))) << (0 ? 19:10))) | (((gctUINT32) ((gctUINT32) (height) & ((gctUINT32) ((((1 ? 19:10) - (0 ? 19:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:10) - (0 ? 19:10) + 1))))))) << (0 ? 19:10)));

        /* Lod bias. */
        bias = SamplerInfo->textureInfo->lodBias;

        /* Lod Max. */
        if (mipFilter != gcvTEXTURE_NONE)
        {
            lodmax = SamplerInfo->textureInfo->lodMax;
        }
        else
        {
            lodmax = 0;
        }

        /* Lod Min.*/
        lodmin = SamplerInfo->textureInfo->lodMin;

        /* Set gcregTXLod register value. */
        sampleLOD = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (((bias == 0) ? 0 : 1)) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:1) - (0 ? 10:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:1) - (0 ? 10:1) + 1))))))) << (0 ? 10:1))) | (((gctUINT32) ((gctUINT32) (lodmax >> 11) & ((gctUINT32) ((((1 ? 10:1) - (0 ? 10:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:1) - (0 ? 10:1) + 1))))))) << (0 ? 10:1)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:11) - (0 ? 20:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:11) - (0 ? 20:11) + 1))))))) << (0 ? 20:11))) | (((gctUINT32) ((gctUINT32) (lodmin >> 11) & ((gctUINT32) ((((1 ? 20:11) - (0 ? 20:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:11) - (0 ? 20:11) + 1))))))) << (0 ? 20:11)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:21) - (0 ? 30:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:21) - (0 ? 30:21) + 1))))))) << (0 ? 30:21))) | (((gctUINT32) ((gctUINT32) (bias >> 11) & ((gctUINT32) ((((1 ? 30:21) - (0 ? 30:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:21) - (0 ? 30:21) + 1))))))) << (0 ? 30:21)))
            ;
    }
    else
    {
        /* Set the AQTextureSampleMode register value. */
        sampleMode = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (type) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:13) - (0 ? 17:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:13) - (0 ? 17:13) + 1))))))) << (0 ? 17:13))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 17:13) - (0 ? 17:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:13) - (0 ? 17:13) + 1))))))) << (0 ? 17:13)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:3) - (0 ? 4:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:3) - (0 ? 4:3) + 1))))))) << (0 ? 4:3))) | (((gctUINT32) ((gctUINT32) (addressXlate[SamplerInfo->textureInfo->s]) & ((gctUINT32) ((((1 ? 4:3) - (0 ? 4:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:3) - (0 ? 4:3) + 1))))))) << (0 ? 4:3)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:5) - (0 ? 6:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:5) - (0 ? 6:5) + 1))))))) << (0 ? 6:5))) | (((gctUINT32) ((gctUINT32) (addressXlate[SamplerInfo->textureInfo->t]) & ((gctUINT32) ((((1 ? 6:5) - (0 ? 6:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:5) - (0 ? 6:5) + 1))))))) << (0 ? 6:5)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:7) - (0 ? 8:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:7) - (0 ? 8:7) + 1))))))) << (0 ? 8:7))) | (((gctUINT32) ((gctUINT32) (minXlate[SamplerInfo->textureInfo->minFilter]) & ((gctUINT32) ((((1 ? 8:7) - (0 ? 8:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:7) - (0 ? 8:7) + 1))))))) << (0 ? 8:7)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:11) - (0 ? 12:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:11) - (0 ? 12:11) + 1))))))) << (0 ? 12:11))) | (((gctUINT32) ((gctUINT32) (magXlate[SamplerInfo->textureInfo->magFilter]) & ((gctUINT32) ((((1 ? 12:11) - (0 ? 12:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:11) - (0 ? 12:11) + 1))))))) << (0 ? 12:11)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:9) - (0 ? 10:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:9) - (0 ? 10:9) + 1))))))) << (0 ? 10:9))) | (((gctUINT32) ((gctUINT32) (mipXlate[mipFilter]) & ((gctUINT32) ((((1 ? 10:9) - (0 ? 10:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:9) - (0 ? 10:9) + 1))))))) << (0 ? 10:9)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1))))))) << (0 ? 19:19))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->roundUV) & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1))))))) << (0 ? 19:19)))
            /* Compute log2 in 5.5 format. */
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) ((SamplerInfo->textureInfo->anisoFilter == 1) ? 0 : gcoMATH_Log2in5dot5(SamplerInfo->textureInfo->anisoFilter)) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
            ;

        /* Set the gcregTXMode register value. */
        sampleModeEx = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) ((format >> 12)) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8))) | (((gctUINT32) ((gctUINT32) ((format >> 20)) & ((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12))) | (((gctUINT32) ((gctUINT32) ((format >> 23)) & ((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16))) | (((gctUINT32) ((gctUINT32) ((format >> 26)) & ((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:20) - (0 ? 22:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20))) | (((gctUINT32) ((gctUINT32) ((format >> 29)) & ((gctUINT32) ((((1 ? 22:20) - (0 ? 22:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->hasTileStatus) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:26) - (0 ? 28:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:26) - (0 ? 28:26) + 1))))))) << (0 ? 28:26))) | (((gctUINT32) ((gctUINT32) (alignXlate[SamplerInfo->hAlignment]) & ((gctUINT32) ((((1 ? 28:26) - (0 ? 28:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:26) - (0 ? 28:26) + 1))))))) << (0 ? 28:26)));

        /* Set the AQTextureSampleWH register value. */
        sampleWH = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0))) | (((gctUINT32) ((gctUINT32) (SamplerInfo->width) & ((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16))) | (((gctUINT32) ((gctUINT32) (samplerHeight) & ((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16)));

        /* Compute log2 in 5.5 format for width and height. */
        width  = gcoMATH_Log2in5dot5(SamplerInfo->width);
        height = gcoMATH_Log2in5dot5(samplerHeight);

        /* Set the AQTextureSampleLogWH register value. */
        sampleLogWH = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0))) | (((gctUINT32) ((gctUINT32) (width) & ((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:10) - (0 ? 19:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:10) - (0 ? 19:10) + 1))))))) << (0 ? 19:10))) | (((gctUINT32) ((gctUINT32) (height) & ((gctUINT32) ((((1 ? 19:10) - (0 ? 19:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:10) - (0 ? 19:10) + 1))))))) << (0 ? 19:10)));

        /* Lod bias. */
        bias = SamplerInfo->textureInfo->lodBias;

        /* Lod Max. */
        if (mipFilter != gcvTEXTURE_NONE)
        {
            lodmax = SamplerInfo->textureInfo->lodMax;
        }
        else
        {
            lodmax = 0;
        }

        /* Lod Min.*/
        lodmin = SamplerInfo->textureInfo->lodMin;

        /* Set AQTextureSampleLod register value. */
        sampleLOD = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (((bias == 0) ? 0 : 1)) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:1) - (0 ? 10:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:1) - (0 ? 10:1) + 1))))))) << (0 ? 10:1))) | (((gctUINT32) ((gctUINT32) (lodmax >> 11) & ((gctUINT32) ((((1 ? 10:1) - (0 ? 10:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:1) - (0 ? 10:1) + 1))))))) << (0 ? 10:1)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:11) - (0 ? 20:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:11) - (0 ? 20:11) + 1))))))) << (0 ? 20:11))) | (((gctUINT32) ((gctUINT32) (lodmin >> 11) & ((gctUINT32) ((((1 ? 20:11) - (0 ? 20:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:11) - (0 ? 20:11) + 1))))))) << (0 ? 20:11)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:21) - (0 ? 30:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:21) - (0 ? 30:21) + 1))))))) << (0 ? 30:21))) | (((gctUINT32) ((gctUINT32) (bias >> 11) & ((gctUINT32) ((((1 ? 30:21) - (0 ? 30:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:21) - (0 ? 30:21) + 1))))))) << (0 ? 30:21)))
            ;
    }

    /* Program tile status information. */
    if (SamplerInfo->hasTileStatus)
    {
        gctINT mcFormat;

        /* Convert the format. */
        mcFormat = _GetMCTextureFormat(SamplerInfo->format);

        if (mcFormat == -1)
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }

        sampleTSConfig     = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
                           | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (mcFormat) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)));

        sampleTSClearValue = SamplerInfo->tileStatusClearValue;
        sampleTSBuffer     = SamplerInfo->tileStatusAddr;
    }
    else
    {
        sampleTSConfig     = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)));
        sampleTSClearValue = 0x0;
        sampleTSBuffer     = 0x00C0FFEE;
    }

    /* Save changed sampler mode register. */
    if (hardware->hwTxSamplerMode[Sampler] != sampleMode)
    {
        hardware->hwTxSamplerMode[Sampler] = sampleMode;
        hardware->hwTxSamplerModeDirty    |= 1 << Sampler;
    }

    if (hardware->hwTxSamplerModeEx[Sampler] != sampleModeEx)
    {
        hardware->hwTxSamplerModeEx[Sampler] = sampleModeEx;
        hardware->hwTxSamplerModeDirty    |= 1 << Sampler;
    }

    /* Save changed sampler size register. */
    if (hardware->hwTxSamplerSize[Sampler] != sampleWH)
    {
        hardware->hwTxSamplerSize[Sampler] = sampleWH;
        hardware->hwTxSamplerSizeDirty    |= 1 << Sampler;
    }

    /* Save changed sampler log2 size register. */
    if (hardware->hwTxSamplerSizeLog[Sampler] != sampleLogWH)
    {
        hardware->hwTxSamplerSizeLog[Sampler] = sampleLogWH;
        hardware->hwTxSamplerSizeLogDirty    |= 1 << Sampler;
    }

    /* Save changed sampler LOD register. */
    if (hardware->hwTxSamplerLOD[Sampler] != sampleLOD)
    {
        hardware->hwTxSamplerLOD[Sampler] = sampleLOD;
        hardware->hwTxSamplerLODDirty    |= 1 << Sampler;
    }

    /* Set each LOD. */
    for(i = 0; i < SamplerInfo->lodNum; i++)
    {
        if (hardware->hwTxSamplerAddress[i][Sampler] != SamplerInfo->lodAddr[i])
        {
            hardware->hwTxSamplerAddress[i][Sampler] = SamplerInfo->lodAddr[i];
            hardware->hwTxSamplerAddressDirty[i]    |= 1 << Sampler;
        }
    }

    /* Save changed sampler TS registers. */
    /* HW only has 8 TS configures for texture.
       Do we need another map from sampler to TS? */
    if (Sampler < gcdSAMPLER_TS)
    {
        if ((hardware->hwTXSampleTSConfig[Sampler]    != sampleTSConfig)
        || (hardware->hwTXSampleTSBuffer[Sampler]     != sampleTSBuffer)
        || (hardware->hwTXSampleTSClearValue[Sampler] != sampleTSClearValue))
        {
            hardware->hwTXSampleTSConfig[Sampler] = sampleTSConfig;
            hardware->hwTXSampleTSBuffer[Sampler] = sampleTSBuffer;
            hardware->hwTXSampleTSClearValue[Sampler] = sampleTSClearValue;
            hardware->hwTxSamplerTSDirty    |= 1 << Sampler;
        }
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
**  gcoHARDWARE_ProgramTexture
**
**  Program all dirty texture states.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_ProgramTexture(
    IN gcoHARDWARE Hardware
    )
{
    static const gctUINT32 address[gcdLOD_LEVELS] =
    {
        0x0900,
        0x0910,
        0x0920,
        0x0930,
        0x0940,
        0x0950,
        0x0960,
        0x0970,
        0x0980,
        0x0990,
        0x09A0,
        0x09B0,
        0x09C0,
        0x09D0
    };

    gceSTATUS status;
    gctUINT dirty, sampler, i, samplerCount, semaStall;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (((((gctUINT32) (Hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
    {
        samplerCount = gcdSAMPLERS;
    }
    else
    {
        samplerCount = 16;
    }
    /***************************************************************************
    ** Determine the size of the buffer to reserve.
    */

    reserveSize = 0;

    /* Check for dirty sampler mode registers. */
    for (dirty = Hardware->hwTxSamplerModeDirty; dirty != 0; dirty >>= 1)
    {
        if (dirty & 1)
        {
            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
        }
    }

    /* Check for dirty sampler size registers. */
    for (dirty = Hardware->hwTxSamplerSizeDirty; dirty != 0; dirty >>= 1)
    {
        if (dirty & 1)
        {
            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
        }
    }

    /* Check for dirty sampler log2 size registers. */
    for (dirty = Hardware->hwTxSamplerSizeLogDirty; dirty != 0; dirty >>= 1)
    {
        if (dirty & 1)
        {
            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
        }
    }

    /* Check for dirty sampler LOD registers. */
    for (dirty = Hardware->hwTxSamplerLODDirty; dirty != 0; dirty >>= 1)
    {
        if (dirty & 1)
        {
            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
        }
    }

    /* Walk all LOD levels. */
    for (i = 0; i < gcdLOD_LEVELS; ++i)
    {
        /* Check for dirty sampler address registers. */
        for (dirty = Hardware->hwTxSamplerAddressDirty[i]; dirty != 0; dirty >>= 1)
        {
            if (dirty & 1)
            {
                reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
            }
        }
    }

    /* Check for dirty sampler TS registers. */
    for (dirty = Hardware->hwTxSamplerTSDirty; dirty != 0; dirty >>= 1)
    {
        if (dirty & 1)
        {
            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
        }
    }

    if (Hardware->hwTxFlush)
    {
        reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 1;
    }

	if (Hardware->hwTxSamplerTSDirty)
	{
		semaStall = 1;
	}
	else
	{
		semaStall = 0;
	}

	if (semaStall)
	{
		reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2;
	}


    /***************************************************************************
    ** Load texture states.
    */

    if (reserveSize != 0)
    {
        /* Switch to 3D pipe. */
        gcmONERROR(gcoHARDWARE_SelectPipe(Hardware, gcvPIPE_3D));

        /* Reserve space in the command buffer. */
        gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

        /* Check for dirty sampler mode registers. */
        for (dirty = Hardware->hwTxSamplerModeDirty, sampler = 0;
             (dirty != 0) && (sampler < samplerCount);
             dirty >>= 1, ++sampler
        )
        {
            /* Check if this sampler is dirty. */
            if (dirty & 1)
            {
                if (((((gctUINT32) (Hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x4000 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x4000 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x4000 + sampler, Hardware->hwTxSamplerMode[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                    /* Extra state to specify texture alignment. */
                    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x40E0 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x40E0 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x40E0 + sampler, Hardware->hwTxSamplerModeEx[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                }
                else
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0800 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0800 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0800 + sampler, Hardware->hwTxSamplerMode[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                    /* Extra state to specify texture alignment. */
                    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0870 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0870 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0870 + sampler, Hardware->hwTxSamplerModeEx[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                }
            }
        }

        /* Check for dirty sampler size registers. */
        for (dirty = Hardware->hwTxSamplerSizeDirty, sampler = 0;
             (dirty != 0) && (sampler < samplerCount);
             dirty >>= 1, ++sampler
        )
        {
            /* Check if this sampler is dirty. */
            if (dirty & 1)
            {
                if (((((gctUINT32) (Hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x4020 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x4020 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x4020 + sampler, Hardware->hwTxSamplerSize[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                }
                else
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0810 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0810 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0810 + sampler, Hardware->hwTxSamplerSize[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                }
            }
        }

        /* Check for dirty sampler log2 size registers. */
        for (dirty = Hardware->hwTxSamplerSizeLogDirty, sampler = 0;
             (dirty != 0) && (sampler < samplerCount);
             dirty >>= 1, ++sampler
        )
        {
            /* Check if this sampler is dirty. */
            if (dirty & 1)
            {
                if (((((gctUINT32) (Hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x4040 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x4040 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x4040 + sampler, Hardware->hwTxSamplerSizeLog[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                }
                else
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0820 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0820 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0820 + sampler, Hardware->hwTxSamplerSizeLog[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                }
            }
        }

        /* Check for dirty sampler LOD registers. */
        for (dirty = Hardware->hwTxSamplerLODDirty, sampler = 0;
             (dirty != 0) && (sampler < samplerCount);
             dirty >>= 1, ++sampler
        )
        {
            /* Check if this sampler is dirty. */
            if (dirty & 1)
            {
                if (((((gctUINT32) (Hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x4060 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x4060 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x4060 + sampler, Hardware->hwTxSamplerLOD[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                }
                else
                {
                    /* Save dirty state in state buffer. */
                    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0830 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0830 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0830 + sampler, Hardware->hwTxSamplerLOD[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                }
            }
        }

        /* Walk all LOD levels. */
        for (i = 0; i < gcdLOD_LEVELS; ++i)
        {
            /* Check for dirty sampler address registers. */
            for (dirty = Hardware->hwTxSamplerAddressDirty[i], sampler = 0;
                 (dirty != 0) && (sampler < samplerCount);
                 dirty >>= 1, ++sampler
            )
            {
                /* Check if this sampler is dirty. */
                if (dirty & 1)
                {
                    if (((((gctUINT32) (Hardware->chipMinorFeatures2)) >> (0 ? 11:11) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))))
                    {
                        /* Save dirty state in state buffer. */
                        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x4200 + (sampler << 4) + i, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x4200 + (sampler << 4) + i) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x4200 + (sampler << 4) + i, Hardware->hwTxSamplerAddress[i][sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                    }
                    else
                    {
                        /* Save dirty state in state buffer. */
                        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, address[i] + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (address[i] + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, address[i] + sampler, Hardware->hwTxSamplerAddress[i][sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                    }
                }
            }
        }

        /* Check for dirty sampler TS registers. */
        for (dirty = Hardware->hwTxSamplerTSDirty, sampler = 0;
             (dirty != 0) && (sampler < 8);
             dirty >>= 1, ++sampler
        )
        {
            /* Check if this sampler is dirty. */
            if (dirty & 1)
            {
                /* Save dirty state in state buffer. */
                {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05C8 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05C8 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x05C8 + sampler, Hardware->hwTXSampleTSConfig[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05D0 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05D0 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x05D0 + sampler, Hardware->hwTXSampleTSBuffer[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
                {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05D8 + sampler, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05D8 + sampler) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x05D8 + sampler, Hardware->hwTXSampleTSClearValue[sampler] );     gcmENDSTATEBATCH(reserve, memory);};
            }
        }
        /* Remove dirty flags. */
        Hardware->hwTxSamplerTSDirty = 0;

        /* Check for texture cache flush. */
        if (Hardware->hwTxFlush)
        {
            /* Flush the texture unit. */
            {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E03, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1))))))) << (0 ? 2:2))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1))))))) << (0 ? 2:2))) |((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) |((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );gcmENDSTATEBATCH(reserve, memory);};

            if (Hardware->chipModel == gcv700
#if gcd6000_SUPPORT
                || 1
#endif
                )
            {
                /* Flush the L2 cache as well. */
                Hardware->flushL2 = gcvTRUE;
            }

            /* Flushed the texture cache. */
            Hardware->hwTxFlush = gcvFALSE;
		}

		if (semaStall)
		{
            {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E02, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0E02, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) );gcmENDSTATEBATCH(reserve, memory);};

            {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0F00, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0F00) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0F00, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) );gcmENDSTATEBATCH(reserve, memory);};
		}

        gcmENDSTATEBUFFER(reserve, memory, reserveSize);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#endif /* VIVANTE_NO_3D */

