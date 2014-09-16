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


#include "gc_glsh_precomp.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <KHR/khrvivante.h>

#define gcdCOPY_TEX_SHADER 1  /* Enable me for performance - might have side effects. */

void
_glshInitializeTexture(
    GLContext Context
    )
{
    /* No textures bound yet. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(Context->texture2D,
                                  sizeof(Context->texture2D)));
    gcmVERIFY_OK(gcoOS_ZeroMemory(Context->textureCube,
                                  sizeof(Context->textureCube)));
    gcmVERIFY_OK(gcoOS_ZeroMemory(Context->texture3D,
                                  sizeof(Context->texture3D)));
    gcmVERIFY_OK(gcoOS_ZeroMemory(Context->textureExternal,
                                  sizeof(Context->textureExternal)));

    /* No current texture. */
    Context->textureUnit = 0;

    /* Initialize default textures. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(&Context->default2D,
                                  sizeof(Context->default2D)));
    Context->default2D.target      = GL_TEXTURE_2D;
    Context->default2D.minFilter   = GL_NEAREST_MIPMAP_NEAREST;
    Context->default2D.magFilter   = GL_NEAREST;
    Context->default2D.anisoFilter = 1;
    Context->default2D.wrapS       = GL_REPEAT;
    Context->default2D.wrapT       = GL_REPEAT;
    Context->default2D.wrapR       = GL_REPEAT;
    Context->default2D.maxLevel    = 1000;

    Context->default2D.states.s           = gcvTEXTURE_WRAP;
    Context->default2D.states.t           = gcvTEXTURE_WRAP;
    Context->default2D.states.r           = gcvTEXTURE_WRAP;
    Context->default2D.states.minFilter   = gcvTEXTURE_POINT;
    Context->default2D.states.mipFilter   = gcvTEXTURE_POINT;
    Context->default2D.states.magFilter   = gcvTEXTURE_POINT;
    Context->default2D.states.anisoFilter = 1;
    Context->default2D.states.border[0]   = 0;
    Context->default2D.states.border[1]   = 0;
    Context->default2D.states.border[2]   = 0;
    Context->default2D.states.border[3]   = 0;
    Context->default2D.states.lodBias     = 0;
    Context->default2D.states.lodMin      = 0;
    Context->default2D.states.lodMax      = -1;
    Context->default2D.dirty              = gcvTRUE;
    Context->default2D.direct.dirty       = gcvFALSE;
    Context->default2D.direct.source      = gcvNULL;
    Context->default2D.direct.temp        = gcvNULL;
    gcoOS_ZeroMemory(
        Context->default2D.direct.texture, sizeof(gcoSURF) * 16
        );

    gcmVERIFY_OK(gcoOS_ZeroMemory(&Context->defaultCube,
                                  sizeof(Context->defaultCube)));
    Context->defaultCube.target      = GL_TEXTURE_CUBE_MAP;
    Context->defaultCube.minFilter   = GL_NEAREST_MIPMAP_NEAREST;
    Context->defaultCube.magFilter   = GL_NEAREST;
    Context->defaultCube.anisoFilter = 1;
    Context->defaultCube.wrapS       = GL_REPEAT;
    Context->defaultCube.wrapT       = GL_REPEAT;
    Context->defaultCube.wrapR       = GL_REPEAT;
    Context->defaultCube.maxLevel    = 1000;

    Context->defaultCube.states.s           = gcvTEXTURE_WRAP;
    Context->defaultCube.states.t           = gcvTEXTURE_WRAP;
    Context->defaultCube.states.r           = gcvTEXTURE_WRAP;
    Context->defaultCube.states.minFilter   = gcvTEXTURE_POINT;
    Context->defaultCube.states.mipFilter   = gcvTEXTURE_POINT;
    Context->defaultCube.states.magFilter   = gcvTEXTURE_POINT;
    Context->defaultCube.states.anisoFilter = 1;
    Context->defaultCube.states.border[0]   = 0;
    Context->defaultCube.states.border[1]   = 0;
    Context->defaultCube.states.border[2]   = 0;
    Context->defaultCube.states.border[3]   = 0;
    Context->defaultCube.states.lodBias     = 0;
    Context->defaultCube.states.lodMin      = 0;
    Context->defaultCube.states.lodMax      = -1;
    Context->defaultCube.dirty              = gcvTRUE;
    Context->defaultCube.direct.dirty       = gcvFALSE;
    Context->defaultCube.direct.source      = gcvNULL;
    Context->defaultCube.direct.temp        = gcvNULL;
    gcoOS_ZeroMemory(
        Context->defaultCube.direct.texture, sizeof(gcoSURF) * 16
        );

    gcmVERIFY_OK(gcoOS_ZeroMemory(&Context->default3D,
                                  sizeof(Context->default3D)));
    Context->default3D.target      = GL_TEXTURE_3D_OES;
    Context->default3D.minFilter   = GL_NEAREST_MIPMAP_NEAREST;
    Context->default3D.magFilter   = GL_NEAREST;
    Context->default3D.anisoFilter = 1;
    Context->default3D.wrapS       = GL_REPEAT;
    Context->default3D.wrapT       = GL_REPEAT;
    Context->default3D.wrapR       = GL_REPEAT;
    Context->default3D.maxLevel    = 1000;

    Context->default3D.states.s           = gcvTEXTURE_WRAP;
    Context->default3D.states.t           = gcvTEXTURE_WRAP;
    Context->default3D.states.r           = gcvTEXTURE_WRAP;
    Context->default3D.states.minFilter   = gcvTEXTURE_POINT;
    Context->default3D.states.mipFilter   = gcvTEXTURE_POINT;
    Context->default3D.states.magFilter   = gcvTEXTURE_POINT;
    Context->default3D.states.anisoFilter = 1;
    Context->default3D.states.border[0]   = 0;
    Context->default3D.states.border[1]   = 0;
    Context->default3D.states.border[2]   = 0;
    Context->default3D.states.border[3]   = 0;
    Context->default3D.states.lodBias     = 0;
    Context->default3D.states.lodMin      = 0;
    Context->default3D.states.lodMax      = -1;
    Context->default3D.dirty              = gcvTRUE;
    Context->default3D.direct.dirty       = gcvFALSE;
    Context->default3D.direct.source      = gcvNULL;
    Context->default3D.direct.temp        = gcvNULL;
    gcoOS_ZeroMemory(
        Context->default3D.direct.texture, sizeof(gcoSURF) * 16
        );

    gcmVERIFY_OK(gcoOS_ZeroMemory(&Context->defaultExternal,
                                  sizeof(Context->defaultExternal)));
    Context->defaultExternal.target      = GL_TEXTURE_EXTERNAL_OES;
    Context->defaultExternal.minFilter   = GL_LINEAR;
    Context->defaultExternal.magFilter   = GL_LINEAR;
    Context->defaultExternal.anisoFilter = 1;
    Context->defaultExternal.wrapS       = GL_CLAMP_TO_EDGE;
    Context->defaultExternal.wrapT       = GL_CLAMP_TO_EDGE;
    Context->defaultExternal.wrapR       = GL_CLAMP_TO_EDGE;
    Context->defaultExternal.maxLevel    = 1;

    Context->defaultExternal.states.s           = gcvTEXTURE_CLAMP;
    Context->defaultExternal.states.t           = gcvTEXTURE_CLAMP;
    Context->defaultExternal.states.r           = gcvTEXTURE_CLAMP;
    Context->defaultExternal.states.minFilter   = gcvTEXTURE_LINEAR;
    Context->defaultExternal.states.mipFilter   = gcvTEXTURE_POINT;
    Context->defaultExternal.states.magFilter   = gcvTEXTURE_LINEAR;
    Context->defaultExternal.states.anisoFilter = 1;
    Context->defaultExternal.states.border[0]   = 0;
    Context->defaultExternal.states.border[1]   = 0;
    Context->defaultExternal.states.border[2]   = 0;
    Context->defaultExternal.states.border[3]   = 0;
    Context->defaultExternal.states.lodBias     = 0;
    Context->defaultExternal.states.lodMin      = 0;
    Context->defaultExternal.states.lodMax      = -1;
    Context->defaultExternal.dirty              = gcvTRUE;
    Context->defaultExternal.direct.dirty       = gcvFALSE;
    Context->defaultExternal.direct.source      = gcvNULL;
    Context->defaultExternal.direct.temp        = gcvNULL;
    gcoOS_ZeroMemory(
        Context->defaultExternal.direct.texture, sizeof(gcoSURF) * 16
        );

    /* Zero buffers. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(&Context->textureObjects,
                                  sizeof(Context->textureObjects)));
}

#if gcdNULL_DRIVER < 3
static gctBOOL
_gl2gcFilter(
    IN GLenum Filter,
    OUT gceTEXTURE_FILTER * MipFilter,
    OUT gceTEXTURE_FILTER * TexFilter
    )
{
    gcmASSERT(MipFilter != gcvNULL);
    gcmASSERT(TexFilter != gcvNULL);

    switch (Filter)
    {
    case GL_NEAREST:
        *MipFilter = gcvTEXTURE_NONE;
        *TexFilter = gcvTEXTURE_POINT;
        return gcvTRUE;

    case GL_LINEAR:
        *MipFilter = gcvTEXTURE_NONE;
        *TexFilter = gcvTEXTURE_LINEAR;
        return gcvTRUE;

    case GL_NEAREST_MIPMAP_NEAREST:
        *MipFilter = gcvTEXTURE_POINT;
        *TexFilter = gcvTEXTURE_POINT;
        return gcvTRUE;

    case GL_NEAREST_MIPMAP_LINEAR:
        *MipFilter = gcvTEXTURE_LINEAR;
        *TexFilter = gcvTEXTURE_POINT;
        return gcvTRUE;

    case GL_LINEAR_MIPMAP_NEAREST:
        *MipFilter = gcvTEXTURE_POINT;
        *TexFilter = gcvTEXTURE_LINEAR;
        return gcvTRUE;

    case GL_LINEAR_MIPMAP_LINEAR:
        *MipFilter = gcvTEXTURE_LINEAR;
        *TexFilter = gcvTEXTURE_LINEAR;
        return gcvTRUE;

    default:
        return gcvFALSE;
    }
}

static gctBOOL
_gl2gcMode(
    IN GLenum Mode,
    OUT gceTEXTURE_ADDRESSING * TexAddressing
    )
{
    gcmASSERT(TexAddressing != gcvNULL);

    switch (Mode)
    {
    case GL_REPEAT:
        *TexAddressing = gcvTEXTURE_WRAP;
        return gcvTRUE;

    case GL_CLAMP_TO_EDGE:
        *TexAddressing = gcvTEXTURE_CLAMP;
        return gcvTRUE;

    case GL_MIRRORED_REPEAT:
        *TexAddressing = gcvTEXTURE_MIRROR;
        return gcvTRUE;

    default:
        return gcvFALSE;
    }
}

static gceSTATUS
_NewTextureObject(
    GLContext Context,
    GLTexture Texture
    )
{
    gceSTATUS status;
    gceTEXTURE_FILTER filter, mipmap;
    gceTEXTURE_ADDRESSING addressingS, addressingT, addressingR;

    do
    {
        gcmERR_BREAK(gcoTEXTURE_Construct(Context->hal, &Texture->texture));

        if (!_gl2gcFilter(Texture->minFilter, &mipmap, &filter))
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        Texture->states.minFilter = filter;
        Texture->states.mipFilter = mipmap;

        if (!_gl2gcFilter(Texture->magFilter, &mipmap, &filter))
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        Texture->states.magFilter = filter;

        Texture->states.anisoFilter = Texture->anisoFilter;

        if (!_gl2gcMode(Texture->wrapS, &addressingS))
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }
        if (!_gl2gcMode(Texture->wrapT, &addressingT))
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }
        if (!_gl2gcMode(Texture->wrapR, &addressingR))
        {
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        Texture->states.s = addressingS;
        Texture->states.t = addressingT;
        Texture->states.r = addressingR;

        Texture->states.border[0] = 0;
        Texture->states.border[1] = 0;
        Texture->states.border[2] = 0;
        Texture->states.border[3] = 0;

        Texture->states.lodBias =  0;
        Texture->states.lodMin  =  0;
        Texture->states.lodMax  = -1;

        Texture->dirty = gcvTRUE;

        Texture->direct.dirty    = gcvFALSE;
        Texture->direct.source   = gcvNULL;
        Texture->direct.temp     = gcvNULL;
        gcoOS_ZeroMemory(Texture->direct.texture, sizeof(gcoSURF) * 16);

#if defined(ANDROID) && (ANDROID_SDK_VERSION >= 7)
        Texture->source              = gcvNULL;
        Texture->privHandle          = gcvNULL;
#endif
#if gldFBO_DATABASE
        Texture->modified = GL_FALSE;
        Texture->owner    = gcvNULL;
#endif

        Texture->width               = 0;
        Texture->height              = 0;
    }
    while (gcvFALSE);

    return status;
}

static gceENDIAN_HINT
_gl2gcEndianHint(
    GLenum Format,
    GLenum Type
    )
{
    gceENDIAN_HINT endianHint = gcvENDIAN_NO_SWAP;

    /* Change endianHint, if it's not gcvENDIAN_NO_SWAP. */
    switch (Format)
    {
    case GL_ALPHA:
        switch (Type)
        {
        case GL_UNSIGNED_SHORT:
            endianHint = gcvENDIAN_SWAP_WORD;
            break;

        case GL_UNSIGNED_INT:
            endianHint = gcvENDIAN_SWAP_DWORD;
            break;
        }
        break;

    case GL_RGB:
        switch (Type)
        {
        case GL_UNSIGNED_SHORT_4_4_4_4:
        case GL_UNSIGNED_SHORT_5_5_5_1:
        case GL_UNSIGNED_SHORT_5_6_5:
        case GL_UNSIGNED_SHORT:
            endianHint = gcvENDIAN_SWAP_WORD;
            break;
        }
        break;

    case GL_RGBA:
        switch (Type)
        {
        case GL_UNSIGNED_SHORT_4_4_4_4:
        case GL_UNSIGNED_SHORT_5_5_5_1:
        case GL_UNSIGNED_SHORT_5_6_5:
        case GL_UNSIGNED_SHORT:
            endianHint = gcvENDIAN_SWAP_WORD;
            break;
        }
        break;

    case GL_LUMINANCE:
        switch (Type)
        {
        case GL_UNSIGNED_SHORT:
            endianHint = gcvENDIAN_SWAP_WORD;
            break;

        case GL_UNSIGNED_INT:
            endianHint = gcvENDIAN_SWAP_DWORD;
            break;
        }
        break;

    case GL_LUMINANCE_ALPHA:
        switch (Type)
        {
        case GL_UNSIGNED_SHORT:
            endianHint = gcvENDIAN_SWAP_WORD;
            break;
        }
        break;
    }

    return endianHint;
}

static GLenum
_gl2gcFormat(
    GLenum Format,
    GLenum Type,
    gceSURF_FORMAT* ImageFormat,
    gceSURF_FORMAT* TextureFormat,
    GLsizei * Bpp
    )
{
    gceSURF_FORMAT imageFormat = gcvSURF_UNKNOWN;
    GLsizei bpp = 1;

    /* Check for a valid type. */
    switch (Type)
    {
        case GL_UNSIGNED_BYTE:
        case GL_UNSIGNED_SHORT:
        case GL_UNSIGNED_INT:
        case GL_UNSIGNED_SHORT_4_4_4_4:
        case GL_UNSIGNED_SHORT_5_5_5_1:
        case GL_UNSIGNED_SHORT_5_6_5:
        case GL_UNSIGNED_INT_2_10_10_10_REV_EXT:
        case GL_UNSIGNED_INT_24_8_OES:
        case GL_HALF_FLOAT_OES:
        case GL_FLOAT:
        case GL_DEPTH_COMPONENT24_OES:
        case GL_DEPTH_COMPONENT32_OES:
            break;
        default:
            return GL_INVALID_ENUM;
    }

    switch (Format)
    {
    case GL_DEPTH_COMPONENT:
        switch (Type)
        {
        case GL_UNSIGNED_SHORT:
            bpp = 16;
            imageFormat = gcvSURF_D16;
            break;

        case GL_DEPTH_COMPONENT24_OES:
            /* Some applications use DEPTH_COMPONENT24_OES,
               even though it's not part of the spec. */
            bpp = 32;
            imageFormat = gcvSURF_D24X8;
            break;

        case GL_DEPTH_COMPONENT32_OES:
            /* Fall through */
        case GL_UNSIGNED_INT:
            bpp = 32;
            imageFormat = gcvSURF_D32;
            break;
        }
        break;

    case GL_ALPHA:
        switch (Type)
        {
        case GL_UNSIGNED_BYTE:
            bpp = 8;
            imageFormat = gcvSURF_A8;
            break;

        case GL_UNSIGNED_SHORT:
            bpp = 16;
            imageFormat = gcvSURF_A16;
            break;

        case GL_UNSIGNED_INT:
            bpp = 32;
            imageFormat = gcvSURF_A32;
            break;

        case GL_HALF_FLOAT_OES:
            bpp = 16;
            imageFormat = gcvSURF_A16F;
            break;

        case GL_FLOAT:
            bpp = 32;
            imageFormat = gcvSURF_A32F;
            break;
        }
        break;

    case GL_RGB:
        switch (Type)
        {
        case GL_UNSIGNED_SHORT_4_4_4_4:
            bpp = 16;
            imageFormat = gcvSURF_X4R4G4B4;
            break;

        case GL_UNSIGNED_SHORT_5_5_5_1:
            bpp = 16;
            imageFormat = gcvSURF_X1R5G5B5;
            break;

        case GL_UNSIGNED_SHORT_5_6_5:
            bpp = 16;
            imageFormat = gcvSURF_R5G6B5;
            break;

        case GL_UNSIGNED_BYTE:
            bpp = 24;
            imageFormat = gcvSURF_B8G8R8;
            break;

        case GL_UNSIGNED_INT_2_10_10_10_REV_EXT:
            bpp = 32;
            imageFormat = gcvSURF_X2B10G10R10;
            break;

        case GL_HALF_FLOAT_OES:
            bpp = 64;
            imageFormat = gcvSURF_X16B16G16R16F;
            break;
        }
        break;

    case GL_RGBA:
        switch (Type)
        {
        case GL_UNSIGNED_BYTE:
            bpp = 32;
            imageFormat = gcvSURF_A8B8G8R8;
            break;

        case GL_UNSIGNED_SHORT_4_4_4_4:
            bpp = 16;
            imageFormat = gcvSURF_R4G4B4A4;
            break;

        case GL_UNSIGNED_SHORT_5_5_5_1:
            bpp = 16;
            imageFormat = gcvSURF_R5G5B5A1;
            break;

        case GL_UNSIGNED_INT_2_10_10_10_REV_EXT:
            bpp = 32;
            imageFormat = gcvSURF_A2B10G10R10;
            break;

        case GL_HALF_FLOAT_OES:
            bpp = 64;
            imageFormat = gcvSURF_A16B16G16R16F;
            break;
        }
        break;

    case GL_LUMINANCE:
        switch (Type)
        {
        case GL_UNSIGNED_BYTE:
            bpp = 8;
            imageFormat = gcvSURF_L8;
            break;
        case GL_UNSIGNED_SHORT:
            bpp = 16;
            imageFormat = gcvSURF_L16;
            break;

        case GL_UNSIGNED_INT:
            bpp = 32;
            imageFormat = gcvSURF_L32;
            break;

        case GL_HALF_FLOAT_OES:
            bpp = 16;
            imageFormat = gcvSURF_L16F;
            break;

        case GL_FLOAT:
            bpp = 32;
            imageFormat = gcvSURF_L32F;
            break;
        }
        break;

    case GL_LUMINANCE_ALPHA:
        switch (Type)
        {
        case GL_UNSIGNED_BYTE:
            bpp = 16;
            imageFormat = gcvSURF_A8L8;
            break;

        case GL_UNSIGNED_SHORT:
            bpp = 32;
            imageFormat = gcvSURF_A16L16;
            break;

        case GL_HALF_FLOAT_OES:
            bpp = 32;
            imageFormat = gcvSURF_A16L16F;
            break;

        case GL_FLOAT:
            bpp = 64;
            imageFormat = gcvSURF_A32L32F;
            break;
        }
        break;

    case GL_BGRA_EXT:
        switch (Type)
        {
        case GL_UNSIGNED_BYTE:
            bpp = 32;
            imageFormat = gcvSURF_A8R8G8B8;
            break;
        }
        break;

    case GL_DEPTH_STENCIL_OES:
        switch (Type)
        {
        case GL_UNSIGNED_INT_24_8_OES:
            bpp = 32;
            imageFormat = gcvSURF_D24S8;
            break;
        }
        break;

    default:
        gcmBREAK();
        return GL_INVALID_ENUM;
    }

    /* Did we find a valid Format-Type combination?. */
    if (imageFormat == gcvSURF_UNKNOWN)
    {
        return GL_INVALID_OPERATION;
    }

    if (ImageFormat)
    {
        *ImageFormat = imageFormat;
    }

    if (TextureFormat)
    {
        if(!gcmIS_SUCCESS(
             gcoTEXTURE_GetClosestFormat(gcvNULL,
                                         imageFormat,
                                         TextureFormat)))
        {
            *TextureFormat = gcvSURF_UNKNOWN;
            gcmBREAK();
            return GL_INVALID_ENUM;
        }

        if (imageFormat == gcvSURF_D16)
        {
            /* Until the hardware has a D16 decompression part.
            format = gcvSURF_D16; */
            *TextureFormat = gcvSURF_D24X8;
        }
    }

    if (Bpp)
    {
        *Bpp = bpp;
    }

    /* Success. */
    return GL_NO_ERROR;
}

static gceSTATUS
_gl2gcCompressedFormat(
    GLenum Format,
    gceSURF_FORMAT* TextureFormat
    )
{
    gceSURF_FORMAT imageFormat = gcvSURF_UNKNOWN;
    gceSTATUS status;

    switch (Format)
    {
    case GL_PALETTE4_RGBA4_OES:
    case GL_PALETTE8_RGBA4_OES:
        imageFormat = gcvSURF_A4R4G4B4;
        break;

    case GL_PALETTE4_RGB5_A1_OES:
    case GL_PALETTE8_RGB5_A1_OES:
        imageFormat = gcvSURF_A1R5G5B5;
        break;

    case GL_PALETTE4_R5_G6_B5_OES:
    case GL_PALETTE8_R5_G6_B5_OES:
        imageFormat = gcvSURF_R5G6B5;
        break;

    case GL_PALETTE4_RGB8_OES:
    case GL_PALETTE8_RGB8_OES:
        imageFormat = gcvSURF_X8R8G8B8;
        break;

    case GL_PALETTE4_RGBA8_OES:
    case GL_PALETTE8_RGBA8_OES:
        imageFormat = gcvSURF_A8R8G8B8;
        break;

    case GL_ETC1_RGB8_OES:
        imageFormat = gcvSURF_ETC1;
        break;

    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        imageFormat = gcvSURF_DXT1;
        break;

    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        imageFormat = gcvSURF_DXT3;
        break;

    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        imageFormat = gcvSURF_DXT5;
        break;


    default:
        gcmBREAK();
        return GL_INVALID_ENUM;
    }

    status = gcoTEXTURE_GetClosestFormat(gcvNULL,
                                         imageFormat,
                                         TextureFormat);
    if (gcmIS_SUCCESS(status))
    {
        return gcvSTATUS_OK;
    }
    else
    {
        return status;
    }
}
#endif

void _glshDereferenceTexture(
    GLContext Context,
    GLTexture Texture
    )
{
    gcmASSERT((Texture->object.reference - 1) >= 0);

    if (--Texture->object.reference == 0)
    {
        /* Zero texture2D */
        gctINT i;

        for(i = 0; i < 32; i++)
        {
            if(Context->texture2D[i] == Texture)
            {
                Context->texture2D[i] = gcvNULL;
            }

            if(Context->textureCube[i] == Texture)
            {
                Context->textureCube[i] = gcvNULL;
            }

            if(Context->texture3D[i] == Texture)
            {
                Context->texture3D[i] = gcvNULL;
            }

            if(Context->textureExternal[i] == Texture)
            {
                Context->textureExternal[i] = gcvNULL;
            }
        }

        /* Destroy the texture object. */
        if (Texture->texture != gcvNULL)
        {
            gcmVERIFY_OK(gcoTEXTURE_Destroy(Texture->texture));
        }

        /* Destroy dirty texture states. */
        if (Texture->direct.source != gcvNULL)
        {
            /* Unlock the source surface. */
            gcmVERIFY_OK(gcoSURF_Unlock(
                Texture->direct.source,
                gcvNULL
                ));

            /* Destroy the source surface. */
            if (Texture->fromImage == 0)
            {
                gcmVERIFY_OK(gcoSURF_Destroy(
                    Texture->direct.source
                    ));
            }

            Texture->direct.source = gcvNULL;

            /* Destroy the temporary surface. */
            if (Texture->direct.temp != gcvNULL)
            {
                gcmVERIFY_OK(gcoSURF_Destroy(
                    Texture->direct.temp
                    ));

                Texture->direct.temp = gcvNULL;
            }

            /* Clear the LOD array. */
            gcoOS_ZeroMemory(
                Texture->direct.texture, sizeof(gcoSURF) * 16
                );
        }

        /* Free the texture. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Texture));
    }
}


void
_glshDeleteTexture(
    GLContext Context,
    GLTexture Texture
    )
{
#ifdef EGL_API_XXX
    Texture->seqNo = -1;
#endif
    /* Remove the texture from the object list. */
    _glshRemoveObject(&Context->textureObjects, &Texture->object);

    _glshDereferenceTexture(Context, Texture);
}

GL_APICALL void GL_APIENTRY
glActiveTexture(
    GLenum texture
    )
{
#if gcdNULL_DRIVER < 3
    GLint unit;

	glmENTER1(glmARGENUM, texture)
	{
    gcmDUMP_API("${ES20 glActiveTexture 0x%08X}", texture);

    glmPROFILE(context, GLES2_ACTIVETEXTURE, 0);

    unit = texture - GL_TEXTURE0;

    if ( (texture < GL_TEXTURE0) || (texture > GL_TEXTURE31) )
    {
        gcmFATAL("glActiveTexture: Unknown texture=0x%04X", texture);
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    if ((gctUINT) unit >= context->vertexSamplers + context->fragmentSamplers)
    {
        gcmFATAL("%s(%d): texture=TEXTURE%d", __FUNCTION__, __LINE__, unit);
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_TEXTURE,
                  "%s(%d): texture=TEXTURE%d",
                  __FUNCTION__, __LINE__, unit);

    context->textureUnit = unit;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

#if gcdNULL_DRIVER < 3
static GLTexture
_NewTexture(
    IN GLContext Context,
    GLuint Name
    )
{
    GLTexture texture;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    /* Allocate memory for the texture object. */
    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                   sizeof(struct _GLTexture),
                                   &pointer)))
    {
        /* Out of memory error. */
        gcmFATAL("%s(%d): Out of memory", __FUNCTION__, __LINE__);
        gl2mERROR(GL_OUT_OF_MEMORY);
        return gcvNULL;
    }

    /* Initialize memory. */
    gcmVERIFY_OK(
        gcoOS_ZeroMemory(pointer, sizeof(struct _GLTexture)));

    texture = pointer;

    /* Create a new object. */
    if (!_glshInsertObject(&Context->textureObjects,
                           &texture->object,
                           GLObject_Texture,
                           Name))
    {
        /* Roll back. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, texture));

        /* Out of memory error. */
        gcmFATAL("%s(%d): Out of memory", __FUNCTION__, __LINE__);
        gl2mERROR(GL_OUT_OF_MEMORY);
        return gcvNULL;
    }

    /* The texture object is not yet bounded. */
    texture->target = 0;

    /* Create the texture object. */
    status = gcoTEXTURE_Construct(Context->hal, &texture->texture);

    if (gcmIS_ERROR(status))
    {
        /* Roll back. */
        _glshRemoveObject(&Context->textureObjects, &texture->object);
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, texture));

        /* Out of memory error. */
        gcmFATAL("%s(%d): Out of memory", __FUNCTION__, __LINE__);
        gl2mERROR(GL_OUT_OF_MEMORY);
        return gcvNULL;
    }

    /* Initialize OpenGL ES 2.0 texture states. */
    texture->wrapS     = GL_REPEAT;
    texture->wrapT     = GL_REPEAT;
    texture->wrapR     = GL_REPEAT;
    texture->minFilter = GL_LINEAR;
    texture->magFilter = GL_LINEAR;
    texture->dirty     = GL_FALSE;
    texture->needFlush = GL_FALSE;
    texture->fromImage = GL_FALSE;
    texture->anisoFilter = 1;
    texture->maxLevel  = 1000;

    texture->object.reference = 1;

    texture->states.s           = gcvTEXTURE_WRAP;
    texture->states.t           = gcvTEXTURE_WRAP;
    texture->states.r           = gcvTEXTURE_WRAP;
    texture->states.minFilter   = gcvTEXTURE_LINEAR;
    texture->states.magFilter   = gcvTEXTURE_LINEAR;
    texture->states.mipFilter   = gcvTEXTURE_NONE;
    texture->states.anisoFilter = 1;
    texture->states.border[0]   = 0;
    texture->states.border[1]   = 0;
    texture->states.border[2]   = 0;
    texture->states.border[3]   = 0;
    texture->states.lodBias     = 0;
    texture->states.lodMin      = 0;
    texture->states.lodMax      = -1;
    texture->dirty              = gcvTRUE;
    texture->direct.dirty       = gcvFALSE;
    texture->direct.source      = gcvNULL;
    texture->direct.temp        = gcvNULL;
    gcoOS_ZeroMemory(texture->direct.texture, sizeof(gcoSURF) * 16);
#if gldFBO_DATABASE
    texture->owner    = gcvNULL;
    texture->modified = gcvFALSE;
#endif

#ifdef EGL_API_XXX
    texture->seqNo        = -1;
#endif

    /* Return the texture object. */
    return texture;
}
#endif

GL_APICALL void GL_APIENTRY
glGenTextures(
    GLsizei n,
    GLuint * textures
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture texture;
    GLsizei i;

	glmENTER2(glmARGINT, n, glmARGPTR, textures)
	{
    glmPROFILE(context, GLES2_GENTEXTURES, 0);

    if (n <= 0)
    {
        gcmFATAL("%s(%d): n=%d", __FUNCTION__, __LINE__, n);
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Loop while there are textures to generate. */
    for (i = 0; i < n; ++i)
    {
        /* Create a new texture. */
        texture = _NewTexture(context, 0);
        if (texture == gcvNULL)
        {
            break;
        }

        /* Return texture name. */
        gcmTRACE(gcvLEVEL_VERBOSE, "glGenTextures ==> %u", texture->object.name);
        textures[i] = texture->object.name;
    }

    gcmDUMP_API("${ES20 glGenTextures 0x%08X (0x%08X)", n, textures);
    gcmDUMP_API_ARRAY(textures, n);
    gcmDUMP_API("$}");
	}
	glmLEAVE();
#else
    while (n-- > 0)
    {
        *textures++ = 5;
    }
#endif
}

GL_APICALL void GL_APIENTRY
glDeleteTextures(
    GLsizei n,
    const GLuint *textures
    )
{
#if gcdNULL_DRIVER < 3
    GLsizei i;

	glmENTER2(glmARGINT, n, glmARGPTR, textures)
	{
    gcmDUMP_API("${ES20 glDeleteTextures 0x%08X (0x%08X)", n, textures);
    gcmDUMP_API_ARRAY(textures, n);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_DELETETEXTURES, 0);

    if (n < 0)
    {
        gcmFATAL("%s(%d): n=%d", __FUNCTION__, __LINE__, n);
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    for (i = 0; i < n; ++i)
    {
        GLTexture object = gcvNULL;

        if (textures[i] == 0)
        {
            continue;
        }
        else
        {
            object = (GLTexture) _glshFindObject(&context->textureObjects, textures[i]);
        }

        if ((object != gcvNULL) &&
            (object->object.type == GLObject_Texture) )
        {
            if (gcvNULL != context->framebuffer)
            {
                GLint type = 0;
                GLint name = 0;

                /* color0 */

                glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);

                if ((GL_TEXTURE == type))
                {
                    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);

                    if (textures[i] == (GLuint) name)
                    {
                        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 0);

                        /* Disable batch optmization. */
                        context->batchDirty = GL_TRUE;
                    }
                }

                /* depth */

                glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);

                if (GL_TEXTURE == type)
                {
                    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
                    if (textures[i] == (GLuint) name)
                    {
                        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);

                        /* Disable batch optmization. */
                        context->batchDirty = GL_TRUE;
                    }
                }

                /* stencil */

                glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);


                if (GL_TEXTURE == type)
                {
                    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
                    if((textures[i] == (GLuint) name))
                    {
                        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);

                        /* Disable batch optmization. */
                        context->batchDirty = GL_TRUE;
                    }
                }
            }

            _glshDeleteTexture(context, object);
        }
    }

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glBindTexture(
    GLenum target,
    GLuint texture
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture object;

	glmENTER2(glmARGENUM, target, glmARGUINT, texture)
	{
    gcmDUMP_API("${ES20 glBindTexture 0x%08X 0x%08X}", target, texture);

    glmPROFILE(context, GLES2_BINDTEXTURE, 0);

    /* Nothing to do for texture 0. */
    if (texture == 0)
    {
        object = gcvNULL;
    }
    else
    {
        /* Find the object. */
        object = (GLTexture) _glshFindObject(&context->textureObjects, texture);

        if (object == gcvNULL)
        {
            object = _NewTexture(context, texture);
            if (object == gcvNULL)
            {
                break;
            }
        }

        /* Set default values for external texture. */
        if ((target == GL_TEXTURE_EXTERNAL_OES)
         && (object->target == 0))
        {
            object->minFilter        = GL_LINEAR;
            object->magFilter        = GL_LINEAR;
            object->anisoFilter      = 1;
            object->wrapS            = GL_CLAMP_TO_EDGE;
            object->wrapT            = GL_CLAMP_TO_EDGE;
            object->wrapR            = GL_CLAMP_TO_EDGE;
            object->maxLevel         = 1;
            object->states.s         = gcvTEXTURE_CLAMP;
            object->states.t         = gcvTEXTURE_CLAMP;
            object->states.r         = gcvTEXTURE_CLAMP;
            object->states.minFilter = gcvTEXTURE_LINEAR;
            object->states.mipFilter = gcvTEXTURE_POINT;
            object->states.magFilter = gcvTEXTURE_LINEAR;
        }

        /* Set target for texture. */
        object->target = target;
    }

    /* Set texture object for target. */
    switch (target)
    {
    case GL_TEXTURE_2D:
        context->texture2D[context->textureUnit] = object;
        break;

    case GL_TEXTURE_3D_OES:
        context->texture3D[context->textureUnit] = object;
        break;

    case GL_TEXTURE_CUBE_MAP:
        context->textureCube[context->textureUnit] = object;
        break;

    case GL_TEXTURE_EXTERNAL_OES:
        context->textureExternal[context->textureUnit] = object;
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X", __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glTexImage2D(
    GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLenum format,
    GLenum type,
    const void * pixels
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture texture = gcvNULL;
    gceSTATUS status;
    gcoSURF surface;
    gceTEXTURE_FACE face = gcvFACE_NONE;
    int faces = 0;
    GLsizei bpp = 0;
    gceSURF_FORMAT imageFormat = gcvSURF_UNKNOWN;
    gceSURF_FORMAT textureFormat = gcvSURF_UNKNOWN;
    GLsizei stride = 0;
    GLenum gl2gcError = GL_NO_ERROR;
	GLboolean goToEnd = GL_FALSE;

	glmENTER8(glmARGENUM, target, glmARGINT, level, glmARGENUM, internalformat,
              glmARGINT, width, glmARGINT, height, glmARGENUM, format,
              glmARGENUM, type, glmARGPTR, pixels)
	{

    gcmDUMP_API("${ES20 glTexImage2D 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X (0x%08X)",
                target, level, internalformat, width, height, border, format,
                type, pixels);

    if (context != gcvNULL)
    {
        gl2gcError = _gl2gcFormat(format, type, &imageFormat, &textureFormat, &bpp);
        if (pixels == gcvNULL)
        {
            imageFormat = gcvSURF_UNKNOWN;
        }
        stride = gcmALIGN(bpp * width / 8, context->unpackAlignment);
    }

    gcmDUMP_API_DATA(pixels, stride * height);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_TEXIMAGE2D, 0);

    if (gl2gcError != GL_NO_ERROR)
    {
        gl2mERROR(gl2gcError);
        break;
    }

    if ( (border != 0) ||
         /* Valid Width and Height. */
         (width <= 0) || (height <= 0) ||
         (width  > (GLsizei)context->maxTextureWidth)  ||
         (height > (GLsizei)context->maxTextureHeight) ||
         /* Valid level. */
         (level  < 0) ||
         (level  > (GLint)gcoMATH_Ceiling(
                              gcoMATH_Log2(
                                 (gctFLOAT)context->maxTextureWidth)))
       )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    if ((GLenum) internalformat != format)
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    switch (target)
    {
    case GL_TEXTURE_2D:
        texture = context->texture2D[context->textureUnit];
        face    = gcvFACE_NONE;
        faces   = 0;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_X;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_X;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_Y;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_Y;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_Z;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_Z;
        faces   = 6;
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X", __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
		goToEnd = GL_TRUE;
        break;
    }

	if (goToEnd)
		break;

    /* See if this is the default texture. */
    if (texture == gcvNULL)
    {
        switch (target)
        {
        case GL_TEXTURE_2D:
            texture = &context->default2D;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            texture = &context->defaultCube;
            break;

        default:
            gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, target);
            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = GL_TRUE;
			break;
        }

		if (goToEnd)
			break;
    }

    /* Destroy current texture if we need to upload level 0. */
    if ( (level == 0) && (texture->texture != gcvNULL))
    {
        if ( (target == GL_TEXTURE_2D)
            || ((texture->width != width) || (texture->height != height))
           )
        {
            gcmVERIFY_OK(gcoTEXTURE_Destroy(texture->texture));
            texture->texture = gcvNULL;
        }
    }

    /* Create a new texture object. */
    if (texture->texture == gcvNULL)
    {
        status = _NewTextureObject(context, texture);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): Could not create default texture", __FUNCTION__, __LINE__);
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }

        /* Set endian hint */
        gcmVERIFY_OK(gcoTEXTURE_SetEndianHint(texture->texture,
                                              _gl2gcEndianHint(internalformat, type)));
    }

    gcmASSERT(texture != gcvNULL);
    gcmASSERT(texture->texture != gcvNULL);

    /* Save texture properties at level 0. */
    if (level == 0)
    {
        texture->width  = width;
        texture->height = height;
    }

    status = gcoTEXTURE_GetMipMap(texture->texture, level, &surface);

    if (gcmIS_ERROR(status))
    {
        status = gcoTEXTURE_AddMipMap(texture->texture,
                                      level,
                                      textureFormat,
                                      width, height, 0,
                                      faces,
                                      gcvPOOL_DEFAULT,
                                      &surface);

        if (gcmIS_ERROR(status))
        {
            if (status == gcvSTATUS_OUT_OF_MEMORY)
            {
                gcmFATAL("%s(%d): Could not create the mip map level due to out of memory.",
                         __FUNCTION__, __LINE__);
                gl2mERROR(GL_OUT_OF_MEMORY);
            }
            else if (status == gcvSTATUS_NOT_SUPPORTED)
            {
                gcmFATAL("%s(%d): Could not create the mip map level due to unsupported feature.",
                         __FUNCTION__, __LINE__);
                gl2mERROR(GL_INVALID_VALUE);
            }
            else
            {
                gcmFATAL("%s(%d): Could not create the mip map level.", __FUNCTION__, __LINE__);
                gl2mERROR(GL_INVALID_VALUE);
            }
            break;
        }
    }

    if (pixels != gcvNULL)
    {
        status = gcoTEXTURE_Upload(texture->texture,
                                   face,
                                   width, height, 0,
                                   pixels,
                                   stride,
                                   imageFormat);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): gcoTEXTURE_Upload returned %d(%s)", __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
            gl2mERROR(GL_INVALID_OPERATION);
            break;
        }

        texture->dirty     = GL_TRUE;
        texture->needFlush = GL_TRUE;

        glmPROFILE(context, GL2_TEXUPLOAD_SIZE, bpp*width*height);
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glTexImage3DOES(
    GLenum target,
    GLint level,
    GLenum internalformat,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLint border,
    GLenum format,
    GLenum type,
    const void * pixels
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture texture = gcvNULL;
    gceSTATUS status;
    gcoSURF surface;
    gceTEXTURE_FACE face = gcvFACE_NONE;
    int faces = 0;
    GLsizei bpp = 0;
    gceSURF_FORMAT imageFormat = gcvSURF_UNKNOWN;
    gceSURF_FORMAT textureFormat = gcvSURF_UNKNOWN;
    GLsizei stride = 0;
    GLint i;
    GLenum gl2gcError = GL_NO_ERROR;
	GLboolean goToEnd = GL_FALSE;

	glmENTER10(glmARGENUM, target, glmARGINT, level, glmARGENUM, internalformat,
              glmARGINT, width, glmARGINT, height, glmARGINT, depth, glmARGINT, border,
			  glmARGENUM, format, glmARGENUM, type, glmARGPTR, pixels)
 	{

    gcmDUMP_API("${ES20 glTexImage3DOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X (0x%08X)",
                target, level, internalformat, width, height,depth, border, format,
                type, pixels);

    if (context != gcvNULL)
    {
        gl2gcError = _gl2gcFormat(format, type, &imageFormat, &textureFormat, &bpp);
        if (pixels == gcvNULL)
        {
            imageFormat = gcvSURF_UNKNOWN;
        }
        stride = gcmALIGN(bpp * width / 8, context->unpackAlignment);
    }

    gcmDUMP_API_DATA(pixels, stride * height * depth);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_TEXIMAGE3DOES, 0);

    if (gl2gcError != GL_NO_ERROR)
    {
        gl2mERROR(gl2gcError);
        break;
    }

    if ( (width <= 0) || (height <= 0) || (depth <= 0))
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    if ((format == GL_BGRA_EXT)
     && (internalformat != GL_RGBA))
    {
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }
    else if ((GLenum) internalformat != format)
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    switch (target)
    {
    case GL_TEXTURE_3D_OES:
        texture = context->texture3D[context->textureUnit];
        face    = gcvFACE_NONE;
        faces   = 0;
        break;

    default:
        /*gcmFATAL("%s(%d): Invalid target: 0x%04X", __FUNCTION__, __LINE__, target);*/
        gl2mERROR(GL_INVALID_ENUM);
		goToEnd = GL_TRUE;
        break;
    }

	if (goToEnd)
		break;

    /* See if this is the default texture. */
    if (texture == gcvNULL)
    {
        switch (target)
        {
        case GL_TEXTURE_3D_OES:
            texture = &context->default3D;
            break;

        default:
            gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, target);
            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = GL_TRUE;
			break;
		}
		if (goToEnd)
			break;
    }

    /* Destroy current texture if we need to upload level 0. */
    if ( (level == 0) && (texture->texture != gcvNULL)
        && (target == GL_TEXTURE_3D_OES))
    {
        gcmVERIFY_OK(gcoTEXTURE_Destroy(texture->texture));
        texture->texture = gcvNULL;
    }

    /* Create a new texture object. */
    if (texture->texture == gcvNULL)
    {
        status = _NewTextureObject(context, texture);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): Could not create default texture", __FUNCTION__, __LINE__);
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }

        /* Set endian hint */
        gcmVERIFY_OK(gcoTEXTURE_SetEndianHint(texture->texture,
            _gl2gcEndianHint(internalformat, type)));
    }

    gcmASSERT(texture != gcvNULL);
    gcmASSERT(texture->texture != gcvNULL);

    status = gcoTEXTURE_GetMipMap(texture->texture, level, &surface);

    if (gcmIS_ERROR(status))
    {
        status = gcoTEXTURE_AddMipMap(texture->texture,
                                      level,
                                      textureFormat,
                                      width, height, depth,
                                      faces,
                                      gcvPOOL_DEFAULT,
                                      &surface);

        if (gcmIS_ERROR(status))
        {
            if (status == gcvSTATUS_OUT_OF_MEMORY)
            {
                gcmFATAL("%s(%d): Could not create the mip map level due to out of memory.",
                         __FUNCTION__, __LINE__);
                gl2mERROR(GL_OUT_OF_MEMORY);
            }
            else if (status == gcvSTATUS_NOT_SUPPORTED)
            {
                gcmFATAL("%s(%d): Could not create the mip map level due to unsupported feature.",
                         __FUNCTION__, __LINE__);
                gl2mERROR(GL_INVALID_VALUE);
            }
            else
            {
                gcmFATAL("%s(%d): Could not create the mip map level.",
                         __FUNCTION__, __LINE__);
                gl2mERROR(GL_INVALID_VALUE);
            }
            break;
        }
    }

    if (pixels != gcvNULL)
    {
        for(i = 0; i < depth; i++)
        {
            status = gcoTEXTURE_Upload(texture->texture,
                                       face,
                                       width, height,i,
                                       (const void *)((GLbyte *)pixels + stride * height * i),
                                       stride,
                                       imageFormat);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s(%d): gcoTEXTURE_Upload returned %d", __FUNCTION__, __LINE__, status);
                gl2mERROR(GL_INVALID_OPERATION);
				goToEnd = GL_TRUE;
                break;
            }
        }

		if (goToEnd)
			break;

        texture->dirty     = GL_TRUE;
        texture->needFlush = GL_TRUE;

        glmPROFILE(context, GL2_TEXUPLOAD_SIZE, bpp*width*height*depth);
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glTexSubImage2D(
    GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    const void *pixels
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture texture = gcvNULL;
    gceSTATUS status;
    gceTEXTURE_FACE face = gcvFACE_NONE;
    GLsizei bpp = 0;
    gceSURF_FORMAT imageFormat = gcvSURF_UNKNOWN;
    gceSURF_FORMAT textureFormat = gcvSURF_UNKNOWN;
    GLsizei stride = 0;
    GLenum gl2gcError = GL_NO_ERROR;
	GLboolean goToEnd = GL_FALSE;

	glmENTER9(glmARGENUM, target, glmARGINT, level, glmARGINT, xoffset, glmARGINT, yoffset,
              glmARGINT, width, glmARGINT, height, glmARGENUM, format,
			  glmARGENUM, type, glmARGPTR, pixels)
	{
    gcmDUMP_API("${ES20 glTexSubImage2D 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X 0x%08X (0x%08X)",
                target, level, xoffset, yoffset, width, height, format, type,
                pixels);
    if (context != gcvNULL)
    {
        if (pixels == gcvNULL)
        {
            imageFormat = gcvSURF_UNKNOWN;
        }
        else
        {
            gl2gcError = _gl2gcFormat(format, type, &imageFormat, &textureFormat, &bpp);
        }
        stride = gcmALIGN(bpp * width / 8, context->unpackAlignment);
    }
    gcmDUMP_API_DATA(pixels, stride * height);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_TEXSUBIMAGE2D, 0);

    if (gl2gcError != GL_NO_ERROR)
    {
        gl2mERROR(gl2gcError);
        break;
    }

    if ( (width <= 0) || (height <= 0) )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    switch (target)
    {
    case GL_TEXTURE_2D:
        texture = context->texture2D[context->textureUnit];
        face    = gcvFACE_NONE;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_X;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_X;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_Y;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_Y;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_Z;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_Z;
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X", __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    /* See if this is the default texture. */
    if (texture == gcvNULL)
    {
        switch (target)
        {
        case GL_TEXTURE_2D:
            texture = &context->default2D;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            texture = &context->defaultCube;
            break;

        default:
            gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, target);
            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = GL_TRUE;
			break;
		}
		if (goToEnd)
			break;

        /* Create a new texture object. */
        if (texture->texture == gcvNULL)
        {
            status = _NewTextureObject(context, texture);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s(%d): Could not create default texture",
                         __FUNCTION__, __LINE__);
                gl2mERROR(GL_OUT_OF_MEMORY);
                break;
            }

            /* Set endian hint */
            gcmVERIFY_OK(gcoTEXTURE_SetEndianHint(texture->texture,
                                                  _gl2gcEndianHint(format, type)));
        }
    }

    gcmASSERT(texture != gcvNULL);
    gcmASSERT(texture->texture != gcvNULL);

    if (pixels != gcvNULL)
    {
        status = gcoTEXTURE_UploadSub(texture->texture,
                                      level,
                                      face,
                                      xoffset, yoffset,
                                      width, height,
                                      0,
                                      pixels,
                                      stride,
                                      imageFormat);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): gcoTEXTURE_UploadSub returned %d(%s)",
                     __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
            gl2mERROR(GL_INVALID_OPERATION);
            break;
        }

        texture->dirty     = GL_TRUE;
        texture->needFlush = GL_TRUE;
    }

    glmPROFILE(context, GL2_TEXUPLOAD_SIZE, bpp*width*height);

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glTexSubImage3DOES(
    GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint zoffset,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLenum format,
    GLenum type,
    const void *pixels
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture texture = gcvNULL;
    gceSTATUS status;
    /*gceTEXTURE_FACE face;*/
    GLsizei bpp = 0;
    gceSURF_FORMAT imageFormat = gcvSURF_UNKNOWN;
    gceSURF_FORMAT textureFormat = gcvSURF_UNKNOWN;
#if gcdDUMP_API
    GLsizei stride = 0;
#endif
    GLint i;
    GLenum gl2gcError = GL_NO_ERROR;
	GLboolean goToEnd = GL_FALSE;

	glmENTER11(glmARGENUM, target, glmARGINT, level, glmARGINT, xoffset, glmARGINT, yoffset,
		      glmARGINT, zoffset, glmARGINT, width, glmARGINT, height, glmARGINT, depth,
			  glmARGENUM, format, glmARGENUM, type, glmARGPTR, pixels)
	{
    gcmDUMP_API("${ES20 glTexSubImage3DOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X 0x%08X (0x%08X)",
                target, level, xoffset, yoffset, zoffset, width, height, depth, format, type,
                pixels);

    if (context != gcvNULL)
    {
        if (pixels == gcvNULL)
        {
            imageFormat = gcvSURF_UNKNOWN;
        }
        else
        {
            gl2gcError = _gl2gcFormat(format, type, &imageFormat, &textureFormat, &bpp);
        }
#if gcdDUMP_API
        stride = gcmALIGN(bpp * width/ 8, context->unpackAlignment);
#endif
    }

    gcmDUMP_API_DATA(pixels, stride * height * depth);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_TEXSUBIMAGE3DOES, 0);

    if (gl2gcError != GL_NO_ERROR)
    {
        gl2mERROR(gl2gcError);
        break;
    }

    if ( (width <= 0) || (height <= 0) || (depth <= 0))
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    switch (target)
    {
    case GL_TEXTURE_3D_OES:
        texture = context->texture3D[context->textureUnit];
        /*face    = gcvFACE_NONE;*/
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X", __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
		goToEnd = GL_TRUE;
        break;
    }
	if (goToEnd)
		break;

    /* See if this is the default texture. */
    if (texture == gcvNULL)
    {
        switch (target)
        {
        case GL_TEXTURE_3D_OES:
            texture = &context->default3D;
            break;

        default:
            gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, target);
            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = GL_TRUE;
			break;
		}
		if (goToEnd)
			break;

        /* Create a new texture object. */
        if (texture->texture == gcvNULL)
        {
            status = _NewTextureObject(context, texture);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s(%d): Could not create default texture",
                         __FUNCTION__, __LINE__);
                gl2mERROR(GL_OUT_OF_MEMORY);
                break;
            }

            /* Set endian hint */
            gcmVERIFY_OK(gcoTEXTURE_SetEndianHint(texture->texture,
                _gl2gcEndianHint(format, type)));
        }
    }

    gcmASSERT(texture != gcvNULL);
    gcmASSERT(texture->texture != gcvNULL);

    if (pixels != gcvNULL)
    {
        for(i = 0; i < depth; i++)
        {
            /* Need upload 3D texture sub image */
            gcmASSERT(0);
        }

        texture->dirty     = GL_TRUE;
        texture->needFlush = GL_TRUE;
    }

    glmPROFILE(context, GL2_TEXUPLOAD_SIZE, bpp*width*height*depth);

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

GL_APICALL GLboolean GL_APIENTRY
glIsTexture(
    GLuint texture
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture object;
    GLboolean isTexture = GL_FALSE;

	glmENTER1(glmARGUINT, texture)
	{
    glmPROFILE(context, GLES2_ISTEXTURE, 0);

    /* Find the object. */
    object = (GLTexture) _glshFindObject(&context->textureObjects, texture);

    isTexture = (object != gcvNULL)
             && (object->object.type == GLObject_Texture)
             && (object->target != 0);

    gcmTRACE(gcvLEVEL_VERBOSE,
             "glIsTexture ==> %s",
             isTexture ? "GL_TRUE" : "GL_FALSE");

    gcmDUMP_API("${ES20 glIsTexture 0x%08X := 0x%08X}", texture, isTexture);
	}
	glmLEAVE1(glmARGINT, isTexture);
    return isTexture;
#else
    return (texture == 5) ? GL_TRUE : GL_FALSE;
#endif
}

#if gcdNULL_DRIVER < 3
static void
_GetTexParameter(
    GLContext Context,
    GLenum target,
    GLenum pname,
    GLint* params
    )
{
    GLTexture texture;

    switch (target)
    {
    case GL_TEXTURE_2D:
        texture = Context->texture2D[Context->textureUnit];

        if (texture == gcvNULL)
        {
            texture = &Context->default2D;
        }
        break;

    case GL_TEXTURE_3D_OES:
        texture = Context->texture3D[Context->textureUnit];

        if (texture == gcvNULL)
        {
            texture = &Context->default3D;
        }
        break;

    case GL_TEXTURE_CUBE_MAP:
        texture = Context->textureCube[Context->textureUnit];

        if (texture == gcvNULL)
        {
            texture = &Context->defaultCube;
        }
        break;

    case GL_TEXTURE_EXTERNAL_OES:
        texture = Context->textureExternal[Context->textureUnit];

        if (texture == gcvNULL)
        {
            texture = &Context->defaultExternal;
        }
        break;

    default:
        gcmFATAL("%s(%d): Invalid target=0x%04X", __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
        return;
    }

    gcmASSERT(texture != gcvNULL);

    switch (pname)
    {
    case GL_TEXTURE_MAG_FILTER:
        *params = texture->magFilter;
        break;

    case GL_TEXTURE_MIN_FILTER:
        *params = texture->minFilter;
        break;

    case GL_TEXTURE_MAX_ANISOTROPY_EXT:
        *params = texture->anisoFilter;
        break;

    case GL_TEXTURE_WRAP_S:
        *params = texture->wrapS;
        break;

    case GL_TEXTURE_WRAP_T:
        *params = texture->wrapT;
        break;

    case GL_TEXTURE_WRAP_R_OES:
        *params = texture->wrapR;
        break;

    case GL_TEXTURE_MAX_LEVEL_APPLE:
        *params = texture->maxLevel;
        break;

    case GL_REQUIRED_TEXTURE_IMAGE_UNITS_OES:
        /* We require only 1 texture unit. */
        *params = 1;
        break;

    default:
        gcmFATAL("%s(%d): Invalid parameter name: 0x%04X", __FUNCTION__, __LINE__, pname);
        gl2mERROR(GL_INVALID_ENUM);
        return;
    }
}
#endif

GL_APICALL void GL_APIENTRY
glGetTexParameteriv(
    GLenum target,
    GLenum pname,
    GLint* params
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER2(glmARGENUM, target, glmARGENUM, pname)
	{
    glmPROFILE(context, GLES2_GETTEXPARAMETERIV, 0);

    _GetTexParameter(context, target, pname, params);

    gcmDUMP_API("${ES20 glGetTexParameteriv 0x%08X 0x%08X (0x%08X)",
                target, pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");
	}
	glmLEAVE1(glmARGINT, *params);
#else
    *params = 0;
#endif
}

GL_APICALL void GL_APIENTRY
glGetTexParameterfv(
    GLenum target,
    GLenum pname,
    GLfloat* params
    )
{
#if gcdNULL_DRIVER < 3
    GLint i = 0;

	glmENTER2(glmARGENUM, target, glmARGENUM, pname)
	{
    glmPROFILE(context, GLES2_GETTEXPARAMETERFV, 0);

    _GetTexParameter(context, target, pname, &i);
    *params = (GLfloat) i;

    gcmDUMP_API("${ES20 glGetTexParameterfv 0x%08X 0x%08X (0x%08X)",
                target, pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");
	}
	glmLEAVE1(glmARGFLOAT, *params);
#else
    *params = 0;
#endif
}

void glshSetTexParameter(GLContext Context,
                         GLTexture Texture,
                         GLenum Target,
                         GLenum Parameter,
                         GLint Value)
{
    gceTEXTURE_FILTER       filter, mipmap;
    gceTEXTURE_ADDRESSING   addressing;

    switch (Parameter)
    {
        case GL_TEXTURE_MAG_FILTER:
            switch (Value)
            {
                case GL_NEAREST:
                    filter = gcvTEXTURE_POINT;
                    break;

                case GL_LINEAR:
                    filter = gcvTEXTURE_LINEAR;
                    break;

                default:
                    gcmFATAL("%s(%d): Invalid mag filter: 0x%04X",
                             __FUNCTION__, __LINE__, Value);
                    gl2mERROR(GL_INVALID_ENUM);
                    return;
            }

            if (Texture->magFilter != Value)
            {
                Texture->magFilter = Value;

                Texture->states.magFilter = filter;
                Texture->dirty      = gcvTRUE;
            }
            break;

        case GL_TEXTURE_MIN_FILTER:
            if (!_gl2gcFilter(Value, &mipmap, &filter))
            {
                gcmFATAL("%s(%d): Invalid min filter: 0x%04X",
                         __FUNCTION__, __LINE__, Value);
                gl2mERROR(GL_INVALID_ENUM);
                return;
            }

            if ((Target == GL_TEXTURE_EXTERNAL_OES)
                &&
                ((Value != GL_NEAREST)
                 &&
                 (Value != GL_LINEAR)
                 )
                )
            {
                gl2mERROR(GL_INVALID_ENUM);
                return;
            }

            if (Texture->minFilter != Value)
            {
                Texture->minFilter = Value;

                Texture->states.minFilter = filter;
                Texture->states.mipFilter = mipmap;
                Texture->dirty      = gcvTRUE;
            }
            break;

        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            if (Value < 1)
            {
                gl2mERROR(GL_INVALID_VALUE);
                return;
            }

            Value = gcmMIN(Value, (GLint) Context->maxAniso);

            if (Texture->anisoFilter != Value)
            {
                Texture->anisoFilter = Value;

                Texture->states.anisoFilter = Value;
                Texture->dirty        = gcvTRUE;
            }
            break;

        case GL_TEXTURE_WRAP_S:
            if (Texture->wrapS != Value)
            {
                if ((Target == GL_TEXTURE_EXTERNAL_OES)
                    &&
                    (Value != GL_CLAMP_TO_EDGE)
                    )
                {
                    gl2mERROR(GL_INVALID_ENUM);
                    return;
                }

                Texture->wrapS = Value;

                if (!_gl2gcMode(Texture->wrapS, &addressing))
                {
                    gl2mERROR(GL_INVALID_ENUM);
                    return;
                }

                Texture->states.s    = addressing;
                Texture->dirty = gcvTRUE;
            }
            break;

        case GL_TEXTURE_WRAP_T:
            if (Texture->wrapT != Value)
            {
                if ((Target == GL_TEXTURE_EXTERNAL_OES)
                    &&
                    (Value != GL_CLAMP_TO_EDGE)
                    )
                {
                    gl2mERROR(GL_INVALID_ENUM);
                    return;
                }

                Texture->wrapT = Value;

                if (!_gl2gcMode(Texture->wrapT, &addressing))
                {
                    gl2mERROR(GL_INVALID_ENUM);
                    return;
                }

                Texture->states.t    = addressing;
                Texture->dirty = gcvTRUE;
            }
            break;

        case GL_TEXTURE_WRAP_R_OES:
            if (Texture->wrapR != Value)
            {
                if ((Target == GL_TEXTURE_EXTERNAL_OES)
                    &&
                    (Value != GL_CLAMP_TO_EDGE)
                    )
                {
                    gl2mERROR(GL_INVALID_ENUM);
                    return;
                }

                Texture->wrapR = Value;

                if (!_gl2gcMode(Texture->wrapR, &addressing))
                {
                    gl2mERROR(GL_INVALID_ENUM);
                    return;
                }

                Texture->states.r    = addressing;
                Texture->dirty = gcvTRUE;
            }
            break;

        case GL_TEXTURE_MAX_LEVEL_APPLE:
            if (Texture->maxLevel != Value)
            {
                Texture->maxLevel    = Value;
                Texture->dirty = gcvTRUE;
            }
            break;

        default:
            gcmFATAL("%s(%d): Invalid parameter: 0x%04X",
                     __FUNCTION__, __LINE__, Parameter);
            gl2mERROR(GL_INVALID_ENUM);
            return;
    }

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;
}

#if gcdNULL_DRIVER < 3
static void
_SetTexParameter(
    GLContext Context,
    GLenum Target,
    GLenum Parameter,
    GLint Value
    )
{
    GLTexture texture;

    switch (Target)
    {
    case GL_TEXTURE_2D:
        texture = Context->texture2D[Context->textureUnit];

        if (texture == gcvNULL)
        {
            texture = &Context->default2D;
        }
        break;

    case GL_TEXTURE_3D_OES:
        texture = Context->texture3D[Context->textureUnit];

        if (texture == gcvNULL)
        {
            texture = &Context->default3D;
        }
        break;


    case GL_TEXTURE_CUBE_MAP:
        texture = Context->textureCube[Context->textureUnit];

        if (texture == gcvNULL)
        {
            texture = &Context->defaultCube;
        }
        break;

    case GL_TEXTURE_EXTERNAL_OES:
        texture = Context->textureExternal[Context->textureUnit];

        if (texture == gcvNULL)
        {
            texture = &Context->defaultExternal;
        }
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%4X",
                 __FUNCTION__, __LINE__, Target);
        gl2mERROR(GL_INVALID_ENUM);
        return;
    }

    gcmASSERT(texture != gcvNULL);

    glshSetTexParameter(Context, texture, Target, Parameter, Value);
}
#endif

#define VERTEX_INDEX        0
#define TEXTURE_INDEX       1

gceSTATUS
_CopyTexSubImage2DShader(
    GLContext Context,
    gcoSURF Draw,
    gctUINT DrawWidth,
    gctUINT DrawHeight,
    GLTexture Texture,
    gctUINT TextureWidth,
    gctUINT TextureHeight,
    GLint XOffset,
    GLint YOffset,
    GLint X,
    GLint Y,
    GLsizei Width,
    GLsizei Height
    )
{
    /* Define result. */
    gceSTATUS status = gcvSTATUS_OK;

    do
    {
        /* Viewport bounding box size. */
        GLfloat widthViewport, heightViewport;

        /* Normalized coordinates. */
        GLfloat normLeft, normTop, normWidth, normHeight;

        /* Rectangle coordinates. */
        GLfloat left, top, right, bottom;
        GLfloat vertexBuffer[4 * 2];
        GLfloat textureBuffer[4 * 2];

        /* Color states. */
        GLboolean   colorMask[4];

        GLboolean   alphaBlend    = Context->blendEnable;
        GLboolean   depthTest     = Context->depthTest;
        GLboolean   stencilTest   = Context->stencilEnable;
        GLboolean   cullEnable    = Context->cullEnable;
        GLboolean   scissorTest   = Context->scissorEnable;
        GLProgram   program       = Context->program;
        GLBuffer    currentBuffer = Context->arrayBuffer;

        GLuint      imageTexture;
        GLTexture   texture;
        GLint       currentActiveUnit;
        GLTexture   currentTexture;

        GLint       locTexture      = 0;
        gctBOOL     rectPrimAvail = gcvFALSE;

        GLuint      fbo           = Context->framebuffer == gcvNULL
                                    ? 0
                                    : Context->framebuffer->object.name;
		GLint       viewportX      = Context->viewportX;
		GLint       viewportY      = Context->viewportY;
		GLsizei     viewportWidth  = Context->viewportWidth;
		GLsizei     viewportHeight = Context->viewportHeight;

        /* Restore vertex data */
#if GL_USE_VERTEX_ARRAY
        GLint       size[2];
        GLenum      type[2];
        GLboolean   normalized[2];
        GLsizei     stride[2];
        GLBuffer    buffer[2];
        gctBOOL     attribEnable[2];
        const void* ptr[2] = {gcvNULL, gcvNULL};
#else
        GLint       size[2];
        GLenum      type[2];
        GLboolean   normalized[2];
        GLsizei     stride[2];
        GLBuffer    buffer[2];
        GLboolean   attribEnable[2];
        const void* ptr[2] = {gcvNULL, gcvNULL};
#endif

        GLint       shaderCompiled;
        GLint       shaderLinked;

        GLchar*     vertSource = "  attribute vec2 a_position;                  \n\
                                    attribute vec2 a_texture;                   \n\
                                    varying   vec2 v_texture;                   \n\
                                    void main(void)                             \n\
                                    {                                           \n\
                                        gl_Position = vec4(a_position, 0.0, 1.0);       \n\
                                        v_texture = a_texture;                  \n\
                                    }";

        GLchar*     fragSource = "  varying vec2 v_texture;                     \n\
                                    uniform sampler2D my_Tex;                   \n\
                                    void main(void)                             \n\
                                    {                                           \n\
                                        gl_FragColor = texture2D(my_Tex, v_texture.xy); \n\
                                    }";

        /* Initialize 3D clear shader */
        if (!Context->copyTexShaderExist)
        {
            Context->copyTexProgram = glCreateProgram();
            Context->copyTexVertShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(Context->copyTexVertShader, 1, (const char**)&vertSource, gcvNULL);
            glCompileShader(Context->copyTexVertShader);
            glGetShaderiv(Context->copyTexVertShader, GL_COMPILE_STATUS, &shaderCompiled);

            if (!shaderCompiled)
            {
	            gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_DRIVER,
				              "Vertex shader compile failed\n");
                return gcvSTATUS_FALSE;
            }

            Context->copyTexFragShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(Context->copyTexFragShader, 1, (const char**)&fragSource, gcvNULL);
            glCompileShader(Context->copyTexFragShader);
            glGetShaderiv(Context->copyTexFragShader, GL_COMPILE_STATUS, &shaderCompiled);

            if (!shaderCompiled)
            {
	            gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_DRIVER,
				              "Fragment shader compile failed\n");
                return gcvSTATUS_FALSE;
            }

            glAttachShader(Context->copyTexProgram, Context->copyTexVertShader);
            glAttachShader(Context->copyTexProgram, Context->copyTexFragShader);

			glBindAttribLocation(Context->copyTexProgram, VERTEX_INDEX, "a_position");
			glBindAttribLocation(Context->copyTexProgram, TEXTURE_INDEX, "a_texture");

            glLinkProgram(Context->copyTexProgram);
            glGetProgramiv(Context->copyTexProgram, GL_LINK_STATUS, &shaderLinked);

            if (!shaderLinked)
            {
	            gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_DRIVER,
				              "Program linked failed\n");
                return gcvSTATUS_FALSE;
            }

            locTexture   = glGetUniformLocation(Context->copyTexProgram, "my_Tex");
            glUseProgram(Context->copyTexProgram);
            glUniform1i(locTexture, 0);

		    glGenFramebuffers(1, &Context->copyTexFBO);

            if (Context->error != GL_NO_ERROR)
            {
                if (program)
                {
                    glUseProgram(program->object.name);
                }
                status = gcvSTATUS_FALSE;
                return gcvSTATUS_FALSE;
            }

            Context->copyTexShaderExist = GL_TRUE;
        }

        /* Get color states. */
        colorMask[0] = Context->colorEnableRed != 0;
        colorMask[1] = Context->colorEnableGreen != 0;
        colorMask[2] = Context->colorEnableBlue != 0;
        colorMask[3] = Context->colorEnableAlpha != 0;

        /* Must restore previous vertex data */
#if GL_USE_VERTEX_ARRAY
        size[0]            = Context->vertexArray[VERTEX_INDEX].size;
        type[0]            = Context->vertexArrayGL[VERTEX_INDEX].type;
        normalized[0]      = (GLboolean)Context->vertexArray[VERTEX_INDEX].normalized;
        stride[0]          = Context->vertexArrayGL[VERTEX_INDEX].stride;
        buffer[0]          = Context->vertexArrayGL[VERTEX_INDEX].buffer;
        attribEnable[0]    = Context->vertexArray[VERTEX_INDEX].enable;
        ptr[0]             = Context->vertexArray[VERTEX_INDEX].pointer;

        size[1]            = Context->vertexArray[TEXTURE_INDEX].size;
        type[1]            = Context->vertexArrayGL[TEXTURE_INDEX].type;
        normalized[1]      = (GLboolean)Context->vertexArray[TEXTURE_INDEX].normalized;
        stride[1]          = Context->vertexArrayGL[TEXTURE_INDEX].stride;
        buffer[1]          = Context->vertexArrayGL[TEXTURE_INDEX].buffer;
        attribEnable[1]    = Context->vertexArray[TEXTURE_INDEX].enable;
        ptr[1]             = Context->vertexArray[TEXTURE_INDEX].pointer;
#else
        size[0]            = Context->vertexArray[VERTEX_INDEX].size;
        type[0]            = Context->vertexArray[VERTEX_INDEX].type;
        normalized[0]      = Context->vertexArray[VERTEX_INDEX].normalized;
        stride[0]          = Context->vertexArray[VERTEX_INDEX].stride;
        buffer[0]          = Context->vertexArray[VERTEX_INDEX].buffer;
        attribEnable[0]    = Context->vertexArray[VERTEX_INDEX].enable;

        if (buffer[0] == gcvNULL)
        {
            ptr[0] = Context->vertexArray[VERTEX_INDEX].ptr;
        }
        else
        {
            /* offset */
            ptr[0] = (const void*)Context->vertexArray[VERTEX_INDEX].offset;
        }

        size[1]            = Context->vertexArray[TEXTURE_INDEX].size;
        type[1]            = Context->vertexArray[TEXTURE_INDEX].type;
        normalized[1]      = Context->vertexArray[TEXTURE_INDEX].normalized;
        stride[1]          = Context->vertexArray[TEXTURE_INDEX].stride;
        buffer[1]          = Context->vertexArray[TEXTURE_INDEX].buffer;
        attribEnable[1]    = Context->vertexArray[TEXTURE_INDEX].enable;
        if (buffer[1] == gcvNULL)
        {
            ptr[1] = Context->vertexArray[TEXTURE_INDEX].ptr;
        }
        else
        {
            /* offset */
            ptr[1] = (const void*)Context->vertexArray[TEXTURE_INDEX].offset;
        }

#endif
        /* Program color states. */
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        if(depthTest)
        {
            glDisable(GL_DEPTH_TEST);
        }

        if(stencilTest)
        {
            glDisable(GL_STENCIL_TEST);
        }

        if(alphaBlend)
        {
            glDisable(GL_BLEND);
        }

        if(cullEnable)
        {
            glDisable(GL_CULL_FACE);
        }

        if(scissorTest)
        {
            glDisable(GL_SCISSOR_TEST);
        }

        if (Context->error != GL_NO_ERROR)
        {
            status = gcvSTATUS_FALSE;
            goto OnError;
        }

        /* Compute Vertex position attribute based on the Texture. */
        widthViewport = (GLfloat)TextureWidth;
        heightViewport = (GLfloat)TextureHeight;

        /* Normalize coordinates. */
        normLeft   = XOffset / widthViewport;
        normTop    = YOffset / heightViewport;
        normWidth  = (Width) / widthViewport;
        normHeight = (Height) / heightViewport;

        /* Transform coordinates. */
        left   = (normLeft   * 2.0f  - 1.0f);
        top    = (normTop    * 2.0f  - 1.0f);
        right  = (normWidth  * 2.0f  + left);
        bottom = (normHeight * 2.0f  + top);

        /* Create two triangles. */
        vertexBuffer[0] = left;
        vertexBuffer[1] = top;

        vertexBuffer[2] = right;
        vertexBuffer[3] = top;

        vertexBuffer[4] = left;
        vertexBuffer[5] = bottom;

        vertexBuffer[6] = right;
        vertexBuffer[7] = bottom;

        /* Compute Texture coordinate attribute based on the Draw surface. */
        widthViewport = (GLfloat)DrawWidth;
        heightViewport = (GLfloat)DrawHeight;

        /* Normalize coordinates. */
        left   = X / widthViewport;
        top    = Y / heightViewport;
        right  = (X + Width) / widthViewport;
        bottom = (Y + Height) / heightViewport;

        /* Create two triangles. */
        textureBuffer[0] = left;
        textureBuffer[1] = top;

        textureBuffer[2] = right;
        textureBuffer[3] = top;

        textureBuffer[4] = left;
        textureBuffer[5] = bottom;

        textureBuffer[6] = right;
        textureBuffer[7] = bottom;

        glUseProgram(Context->copyTexProgram);

        if (Context->error != GL_NO_ERROR)
        {
            status = gcvSTATUS_FALSE;
            goto OnError;
        }

        /* Setup the vertex arrays. */
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glEnableVertexAttribArray(VERTEX_INDEX);
        glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, vertexBuffer);

        glEnableVertexAttribArray(TEXTURE_INDEX);
        glVertexAttribPointer(TEXTURE_INDEX, 2, GL_FLOAT, GL_FALSE, 0, textureBuffer);

        /* Setup the Draw surface as texture. */
        glGenTextures(1, &imageTexture);
        currentActiveUnit =Context->textureUnit;
        currentTexture = Context->texture2D[Context->textureUnit];

        glActiveTexture(GL_TEXTURE0);
	    glBindTexture(GL_TEXTURE_2D, imageTexture);
        /* retrieve the texture object, and set the properties manually. */
        texture = Context->texture2D[Context->textureUnit];

        gcoTEXTURE_AddMipMapFromClient(
            texture->texture, 0, Draw
            );

        /* Make sure to flush the caches. */
        gcoTEXTURE_Flush(texture->texture);

        /* Sema stall from RA to PE. */
        gco3D_Semaphore(gcvNULL, gcvWHERE_RASTER, gcvWHERE_PIXEL, gcvHOW_SEMAPHORE_STALL);

		glViewport(0,0,TextureWidth,TextureHeight);

        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);

        if (Context->error != GL_NO_ERROR)
        {
            status = gcvSTATUS_FALSE;
            goto OnError;
        }

		glBindFramebuffer(GL_FRAMEBUFFER,Context->copyTexFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,Texture->object.name,0);

		rectPrimAvail = gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_RECT_PRIMITIVE);

        /* Draw the clear rect. */
        if (rectPrimAvail)
        {
            glDrawArrays(0x7, 0, 4);
        }
        else
        {
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,0,0);
        glActiveTexture(currentActiveUnit + GL_TEXTURE0);
        Context->texture2D[Context->textureUnit] = currentTexture;
        glDeleteTextures(1, &imageTexture);

        glDisableVertexAttribArray(VERTEX_INDEX);
        glDisableVertexAttribArray(TEXTURE_INDEX);

OnError:
        /* Restoring!  */

		glViewport(viewportX,viewportY,viewportWidth,viewportHeight);

		glBindFramebuffer(GL_FRAMEBUFFER,fbo);

        glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);

        if(scissorTest)
        {
            glEnable(GL_SCISSOR_TEST);
        }

        if(depthTest)
        {
            glEnable(GL_DEPTH_TEST);
        }

        if(stencilTest)
        {
            glEnable(GL_STENCIL_TEST);
        }

        if (alphaBlend)
        {
            glEnable(GL_BLEND);
        }

        if (depthTest)
        {
            glEnable(GL_DEPTH_TEST);
        }

        if (stencilTest)
        {
            glEnable(GL_STENCIL_TEST);
        }

        if (cullEnable)
        {
            glEnable(GL_CULL_FACE);
        }

        if (program)
        {
            glUseProgram(program->object.name);
        }

        if (buffer[0] == gcvNULL)
        {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glVertexAttribPointer(VERTEX_INDEX, size[0], type[0], normalized[0], stride[0], ptr[0]);
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, buffer[0]->object.name);
            glVertexAttribPointer(VERTEX_INDEX, size[0], type[0], normalized[0], stride[0], ptr[0]);
        }

        if (buffer[1] == gcvNULL)
        {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glVertexAttribPointer(TEXTURE_INDEX, size[1], type[1], normalized[1], stride[1], ptr[1]);
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, buffer[1]->object.name);
            glVertexAttribPointer(TEXTURE_INDEX, size[1], type[1], normalized[1], stride[1], ptr[1]);
        }

        if (currentBuffer != gcvNULL)
        {
            glBindBuffer(GL_ARRAY_BUFFER, currentBuffer->object.name);
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        if (attribEnable[0])
        {
            glEnableVertexAttribArray(VERTEX_INDEX);
        }

        if (attribEnable[1])
        {
            glEnableVertexAttribArray(TEXTURE_INDEX);
        }

        gcmASSERT(Context->error == GL_NO_ERROR);
    }
    while (gcvFALSE);

    /* Return status. */
    return status;
}

GL_APICALL void GL_APIENTRY
glTexParameteri(
    GLenum target,
    GLenum pname,
    GLint param
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER3(glmARGENUM, target, glmARGENUM, pname, glmARGINT, param)
	{
    gcmDUMP_API("${ES20 glTexParameteri 0x%08X 0x%08X 0x%08X}",
                target, pname, param);

    glmPROFILE(context, GLES2_TEXPARAMETERIV, 0);

    _SetTexParameter(context, target, pname, param);

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glTexParameterf(
    GLenum target,
    GLenum pname,
    GLfloat param
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER3(glmARGENUM, target, glmARGENUM, pname, glmARGFLOAT, param)
	{
    gcmDUMP_API("${ES20 glTexParameterf 0x%08X 0x%08X 0x%08X}",
                target, pname, param);

    glmPROFILE(context, GLES2_TEXPARAMETERF, 0);

    _SetTexParameter(context, target, pname, (GLint) param);

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glTexParameteriv(
    GLenum target,
    GLenum pname,
    const GLint *params
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER3(glmARGENUM, target, glmARGENUM, pname, glmARGPTR, params)
	{
    gcmDUMP_API("${ES20 glTexParameteriv 0x%08X 0x%08X (0x%08X)",
                target, pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_TEXPARAMETERIV, 0);

    if (params == gcvNULL)
    {
        gcmFATAL("%s(%d): Invalid params: gcvNULL", __FUNCTION__, __LINE__);
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    _SetTexParameter(context, target, pname, *params);

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glTexParameterfv(
    GLenum target,
    GLenum pname,
    const GLfloat *params
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER3(glmARGENUM, target, glmARGENUM, pname, glmARGPTR, params)
	{
    gcmDUMP_API("${ES20 glTexParameterfv 0x%08X 0x%08X (0x%08X)",
                target, pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_TEXPARAMETERFV, 0);

    if (params == gcvNULL)
    {
        gcmFATAL("%s(%d): Invalid params=0x%08X", __FUNCTION__, __LINE__, params);
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    _SetTexParameter(context, target, pname, (GLint) *params);

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glCopyTexImage2D(
    GLenum target,
    GLint level,
    GLenum internalformat,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLint border
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture texture = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;
    gcoSURF surface;
    gceTEXTURE_FACE face = gcvFACE_NONE;
    int faces = 0;
    gcoSURF read = gcvNULL;
    gctUINT mapHeight, mapWidth;
    gctBOOL unalignedCopy = gcvFALSE;
	GLboolean goToEnd = GL_FALSE;

	glmENTER8(glmARGENUM, target, glmARGINT, level, glmARGENUM, internalformat,
		       glmARGINT, x, glmARGINT, y, glmARGINT, width, glmARGINT, height, glmARGINT, border)
	{

    gcmDUMP_API("${ES20 glCopyTexImage2D 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X 0x%08X}",
                target, level, internalformat, x, y, width, height, border);

    glmPROFILE(context, GLES2_COPYTEXIMAGE2D, 0);

#if gldFBO_DATABASE
    if ((context->framebuffer != gcvNULL)
        &&
        (context->framebuffer->currentDB != gcvNULL)
        )
    {
        /* Play current database. */
        if (gcmIS_ERROR(glshPlayDatabase(context,
                                         context->framebuffer->currentDB)))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            break;
        }
    }
#endif

    if ( (border != 0) ||
         /* Valid Width and Height. */
         (width <= 0) || (height <= 0) ||
         (width  > (GLsizei)context->maxTextureWidth)  ||
         (height > (GLsizei)context->maxTextureHeight) ||
         /* Valid level. */
         (level  < 0) ||
         (level  > (GLint)gcoMATH_Ceiling(
                              gcoMATH_Log2(
                                 (gctFLOAT)context->maxTextureWidth)))
       )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    switch (target)
    {
    case GL_TEXTURE_2D:
        texture = context->texture2D[context->textureUnit];
        face    = gcvFACE_NONE;
        faces   = 0;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_X;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_X;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_Y;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_Y;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_Z;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_Z;
        faces   = 6;
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X", __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    /* See if this is the default texture. */
    if (texture == gcvNULL)
    {
        switch (target)
        {
        case GL_TEXTURE_2D:
            texture = &context->default2D;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            texture = &context->defaultCube;
            break;

        default:
            gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, target);
            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = GL_TRUE;
			break;
		}
		if (goToEnd)
			break;
    }

    /* Destroy current texture if we need to upload level 0. */
    if ( (level == 0) && (texture->texture != gcvNULL))
    {
        if ( (target == GL_TEXTURE_2D)
            || ((texture->width != width) || (texture->height != height))
           )
        {
            gcmVERIFY_OK(gcoTEXTURE_Destroy(texture->texture));
            texture->texture = gcvNULL;
        }
    }

    /* Create a new texture object. */
    if (texture->texture == gcvNULL)
    {
        status = _NewTextureObject(context, texture);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): Could not create default texture", __FUNCTION__, __LINE__);
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }
    }

    gcmASSERT(texture != gcvNULL);
    gcmASSERT(texture->texture != gcvNULL);

    /* Save texture properties at level 0. */
    if (level == 0)
    {
        texture->width  = width;
        texture->height = height;
    }

    status = gcoTEXTURE_GetMipMap(texture->texture, level, &surface);

    if (gcmIS_ERROR(status))
    {
        gceSURF_FORMAT textureFormat;

        GLenum gl2gcError = _gl2gcFormat(internalformat,
                                    GL_UNSIGNED_BYTE,
                                    gcvNULL,
                                    &textureFormat,
                                    gcvNULL);

        if (gl2gcError != GL_NO_ERROR)
        {
            gl2mERROR(gl2gcError);
            break;
        }

        status = gcoTEXTURE_AddMipMap(texture->texture,
                                      level,
                                      textureFormat,
                                      width, height, 0,
                                      faces,
                                      gcvPOOL_DEFAULT,
                                      &surface);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): Could not create the mip map level", __FUNCTION__, __LINE__);
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }
    }

    /* Dont use shader path if resolve can handle the copy. */
    unalignedCopy = ((x & 0xF) || (y && 0x8)
                     || (width & 0xF) || (height & 0x8));


#if gcdCOPY_TEX_SHADER
    /* Check if we have super tiled textures. */
    if (unalignedCopy && gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_SUPERTILED_TEXTURE))
    {
        /* Make sure the texture is renderable. */
        gcmONERROR(gcoTEXTURE_RenderIntoMipMap(texture->texture, level));

        /* Fetch the new surface. */
        gcmONERROR(gcoTEXTURE_GetMipMap(texture->texture, level, &surface));
    }
#endif

    /* Check if we are copying to a valid region in the surface. */
    status = gcoSURF_GetSize(surface, &mapWidth, &mapHeight, gcvNULL);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    if ((width  > (GLint) mapWidth)
    ||  (height > (GLint) mapHeight)
    )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    if (context->framebuffer != gcvNULL)
    {
        read = (context->framebuffer->color.object == gcvNULL)
             ? gcvNULL
             : _glshGetFramebufferSurface(&context->framebuffer->color);
    }
    if (read == gcvNULL)
    {
        read = context->draw;
    }

    /* Update frame buffer. */
    _glshFrameBuffer(context);

    /* Upload Cube map surface. */
    if (face != gcvFACE_NONE)
    {
        gctPOINTER pixels = gcvNULL;
        gcoSURF    temp   = gcvNULL;
        gctUINT    size   = 0;

        do
        {
            GLsizei bpp = 0;
            gceSURF_FORMAT textureFormat = gcvSURF_UNKNOWN;

            GLenum gl2gcError = _gl2gcFormat(internalformat,
                                    GL_UNSIGNED_BYTE,
                                    gcvNULL,
                                    &textureFormat,
                                    &bpp);

            if (gl2gcError != GL_NO_ERROR)
            {
                gl2mERROR(gl2gcError);
                goToEnd = GL_TRUE;
				break;
            }

            size = width * height * bpp / 4;

            /* Construct a temp surface. */
            gcmERR_BREAK(gcoOS_Allocate(context->os, size, &pixels));

            gcmERR_BREAK(gcoSURF_Construct(context->hal,
                                           width, height, 1,
                                           gcvSURF_BITMAP,
                                           textureFormat,
                                           gcvPOOL_USER,
                                           &temp));

            gcmERR_BREAK(gcoSURF_MapUserSurface(temp,
                                                0,
                                                pixels, 0));

            /* Copy to the temp surface first. */
            gcmERR_BREAK(gcoSURF_CopyPixels(read, temp,
                                            x, y,
                                            0, 0,
                                            width, height));

            /* Upload to correct face. */
            gcmERR_BREAK(gcoTEXTURE_Upload(texture->texture,
                                           face,
                                           width, height, 0,
                                           pixels,
                                           0,
                                           textureFormat
                                           ));
        }
        while (gcvFALSE);

		if (goToEnd)
			break;

        /* Clear memory and temp surface. */
        if (temp != gcvNULL)
        {
            gcmVERIFY_OK(gcoSURF_Destroy(temp));

            temp = gcvNULL;
        }

        if (pixels != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_Free(context->os, pixels));

            pixels = gcvNULL;
        }
    }
    else
    {
        /* Enable gcoTEXTURE_RenderIntoMipMap earlier in this function,
           before enabling this path. */
        if ( gcdCOPY_TEX_SHADER
         && (level == 0)
         && (context->samples <= 1)
         && unalignedCopy
         && (gcoTEXTURE_IsRenderable(texture->texture, level) == gcvSTATUS_OK))
        {
            gctUINT32 srcWidth, srcHeight;

            gcmONERROR(gcoSURF_GetSize(read, &srcWidth, &srcHeight, gcvNULL));

            /* Disable the tile status and decompress the buffers as framebuffer will be used as texture. */
            gcmONERROR(gcoSURF_DisableTileStatus(read, gcvTRUE));

            gcmONERROR(_CopyTexSubImage2DShader(
                context,
                read,
                srcWidth,
                srcHeight,
				texture,
                mapWidth,
                mapHeight,
                0,
                0,
                x,
                y,
                width,
                height));
        }
        else
        {
            gcsPOINT source, target, rectSize;
            gceORIENTATION srcOrient,dstOrient;
            source.x = x;
            source.y = y;
            target.x = 0;
            target.y = 0;
            rectSize.x = width;
            rectSize.y = height;

            /* Flush the surfaces. */
            if (gcmIS_ERROR(gcoSURF_Flush(read)))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                break;
            }

            /* Disable the tile status and decompress the buffers. */
            gcmONERROR(gcoSURF_DisableTileStatus(read, gcvTRUE));

            /* Disable the tile status for the destination. */
            gcmONERROR(gcoSURF_DisableTileStatus(surface, gcvTRUE));

            /* Set orientation the same with render buffer */
            gcoSURF_QueryOrientation(read,&srcOrient);
            gcoSURF_QueryOrientation(surface,&dstOrient);

            gcoSURF_SetOrientation(surface,srcOrient);

            status = gcoSURF_ResolveRect(read,
                                         surface,
                                         &source,
                                         &target,
                                         &rectSize);

            /* Restore orientation */
            gcmONERROR(gcoSURF_SetOrientation(surface,dstOrient));
        }
    }

    if (gcmIS_ERROR(status))
    {
        gcmFATAL("%s(%d): HAL produced error %d(%s)", __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    /* Mark texture cache as dirty. */
    texture->dirty     = GL_TRUE;
    texture->needFlush = GL_TRUE;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
	return;

OnError:
    gcmFOOTER();
    return;
#endif
}

GL_APICALL void GL_APIENTRY
glCopyTexSubImage2D(
    GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture       texture = gcvNULL;
    gceSTATUS       status  = gcvSTATUS_OK;
    gcoSURF         surface;
    gctUINT         mapHeight, mapWidth;
    gcoSURF         read = gcvNULL;
    GLint           dx, dy;
    GLint           sx, sy;
    GLint           Width, Height;
    gctUINT         dstWidth, dstHeight;
    gctUINT         srcWidth, srcHeight;
    gceTEXTURE_FACE face = gcvFACE_NONE;
	GLboolean goToEnd = GL_FALSE;
    /*int             faces;*/

	glmENTER8(glmARGENUM, target, glmARGINT, level,  glmARGINT, xoffset, glmARGINT, yoffset,
 		       glmARGINT, x, glmARGINT, y, glmARGINT, width, glmARGINT, height)
	{
    gcmDUMP_API("${ES20 glCopyTexSubImage2D 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X 0x%08X}",
                target, level, xoffset, yoffset, x, y, width, height);

    glmPROFILE(context, GLES2_COPYTEXSUBIMAGE2D, 0);

#if gldFBO_DATABASE
    if ((context->framebuffer != gcvNULL)
        &&
        (context->framebuffer->currentDB != gcvNULL)
        )
    {
        /* Play current database. */
        if (gcmIS_ERROR(glshPlayDatabase(context,
                                         context->framebuffer->currentDB)))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            break;
        }
    }
#endif

    if ( (xoffset < 0) || (yoffset < 0) ||
         /* Valid Width and Height. */
         (width <= 0) || (height <= 0) ||
         (width  > (GLsizei)context->maxTextureWidth)  ||
         (height > (GLsizei)context->maxTextureHeight) ||
         /* Valid level. */
         (level  < 0) ||
         (level  > (GLint)gcoMATH_Ceiling(
                              gcoMATH_Log2(
                                 (gctFLOAT)context->maxTextureWidth)))
       )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    switch (target)
    {
    case GL_TEXTURE_2D:
        texture = context->texture2D[context->textureUnit];
        face    = gcvFACE_NONE;
        /*faces   = 0;*/
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_X;
        /*faces   = 6;*/
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_X;
        /*faces   = 6;*/
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_Y;
        /*faces   = 6;*/
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_Y;
        /*faces   = 6;*/
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_Z;
        /*faces   = 6;*/
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_Z;
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X", __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    /* See if this is the default texture. */
    if (texture == gcvNULL)
    {
        switch (target)
        {
        case GL_TEXTURE_2D:
            texture = &context->default2D;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            texture = &context->defaultCube;
            break;

        default:
            gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, target);
            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = GL_TRUE;
			break;
		}
		if (goToEnd)
			break;

        /* Create a new texture object. */
        if (texture->texture == gcvNULL)
        {
            status = _NewTextureObject(context, texture);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s(%d): Could not create default texture",
                         __FUNCTION__, __LINE__);
                gl2mERROR(GL_OUT_OF_MEMORY);
                break;
            }
        }
    }

    gcmASSERT(texture != gcvNULL);
    gcmASSERT(texture->texture != gcvNULL);

#if gcdCOPY_TEX_SHADER
    /* Check if we have super tiled textures. */
    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_SUPERTILED_TEXTURE))
    {
        /* Make sure the texture is renderable. */
        if (gcmIS_ERROR(gcoTEXTURE_RenderIntoMipMap(texture->texture, level)))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            break;
        }
    }
#endif

    status = gcoTEXTURE_GetMipMap(texture->texture, level, &surface);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Check if we are copying to a valid region in the surface. */
    status = gcoSURF_GetSize(surface, &mapWidth, &mapHeight, gcvNULL);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    if (((xoffset + width)  > (GLint) mapWidth)
    ||  ((yoffset + height) > (GLint) mapHeight)
    )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    if (context->framebuffer != gcvNULL)
    {
        read = (context->framebuffer->color.object == gcvNULL)
             ? gcvNULL
             : _glshGetFramebufferSurface(&context->framebuffer->color);
    }
    if (read == gcvNULL)
    {
        read = context->draw;
    }

    sx      = x;
    sy      = y;
    dx      = xoffset;
    dy      = yoffset;
    Width   = width;
    Height  = height;

    status = gcoSURF_GetSize(read, &srcWidth, &srcHeight, gcvNULL);
    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }
    status = gcoSURF_GetSize(surface, &dstWidth, &dstHeight, gcvNULL);
    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    if (!_glshCalculateArea(&dx, &dy, &sx, &sy, &Width, &Height, dstWidth, dstHeight, srcWidth, srcHeight))
    {
        gcmFATAL("glCopyTexSubImage2D: HAL produced error %d", status);
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    /* Update frame buffer. */
    _glshFrameBuffer(context);

    /* Upload Cube map surface. */
    if (face != gcvFACE_NONE)
    {
        gctPOINTER pixels = gcvNULL;
        gcoSURF    temp   = gcvNULL;
        gctUINT    size   = 0;

        do
        {
            gceSURF_FORMAT  textureFormat       = gcvSURF_UNKNOWN;
            gctINT          textureStride       = 0;
            gctUINT         textureAlignedWidth = 0;

            gcmERR_BREAK(gcoSURF_GetFormat(read, gcvNULL, &textureFormat));
            gcmERR_BREAK(gcoSURF_GetAlignedSize(read, &textureAlignedWidth, gcvNULL, &textureStride));

            size = Width * Height * textureStride / textureAlignedWidth;

            /* Construct a temp surface. */
            gcmERR_BREAK(gcoOS_Allocate(context->os, size, &pixels));

            gcmERR_BREAK(gcoSURF_Construct(context->hal,
                                           width, height, 1,
                                           gcvSURF_BITMAP,
                                           textureFormat,
                                           gcvPOOL_USER,
                                           &temp));

            gcmERR_BREAK(gcoSURF_MapUserSurface(temp,
                                                0,
                                                pixels, 0));

            /* Copy to the temp surface first. */
            gcmERR_BREAK(gcoSURF_CopyPixels(read, temp,
                                            sx, sy,
                                            dx, dy,
                                            Width, Height));

            /* Upload to correct face. */
            gcmERR_BREAK(gcoTEXTURE_Upload(texture->texture,
                                           face,
                                           width, height,
                                           0,
                                           pixels,
                                           0,
                                           textureFormat
                                           ));
        }
        while (gcvFALSE);

        /* Clear memory and temp surface. */
        if (temp != gcvNULL)
        {
            gcmVERIFY_OK(gcoSURF_Destroy(temp));

            temp = gcvNULL;
        }

        if (pixels != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_Free(context->os, pixels));

            pixels = gcvNULL;
        }
    }
    else
    {
        /* Enable gcoTEXTURE_RenderIntoMipMap earlier in this function,
           before enabling this path. */
        if ( gcdCOPY_TEX_SHADER
         && (level == 0)
         && (context->samples <= 1)
         && (gcoTEXTURE_IsRenderable(texture->texture, level) == gcvSTATUS_OK))
        {
            gctUINT32 srcWidth, srcHeight;

            gcmONERROR(gcoSURF_GetSize(read, &srcWidth, &srcHeight, gcvNULL));

            /* Disable the tile status and decompress the buffers as framebuffer will be used as texture. */
            gcmONERROR(gcoSURF_DisableTileStatus(read, gcvTRUE));


            gcmONERROR(_CopyTexSubImage2DShader(
                context,
                read,
                srcWidth,
                srcHeight,
				texture,
                mapWidth,
                mapHeight,
                xoffset,
                yoffset,
                x,
                y,
                width,
                height));
        }
        else
        {
            gcoSURF_CopyPixels(read,
                               surface,
                               sx, sy,
                               dx, dy,
                               Width, Height);
        }
    }

    if (gcmIS_ERROR(status))
    {
        gcmFATAL("%s(%d): HAL produced error %d", __FUNCTION__, __LINE__, status);
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    /* Mark texture cache as dirty. */
    texture->dirty     = GL_TRUE;
    texture->needFlush = GL_TRUE;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
	return;

OnError:
    gcmFOOTER();
    return;
#endif
}

GL_APICALL void GL_APIENTRY
glCopyTexSubImage3DOES(
    GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint zoffset,
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture texture = gcvNULL;
    gceSTATUS status;
    gcoSURF surface;
    gctUINT mapHeight, mapWidth, mapDepth;
    gcoSURF read = gcvNULL;
    GLint dx, dy, sx, sy, Width, Height;
    gctUINT dstWidth, dstHeight, srcWidth, srcHeight;
	GLboolean goToEnd = GL_FALSE;

	glmENTER9(glmARGENUM, target, glmARGINT, level,  glmARGINT, xoffset, glmARGINT, yoffset,
 		      glmARGINT, zoffset, glmARGINT, x, glmARGINT, y, glmARGINT, width, glmARGINT, height)
	{
    gcmDUMP_API("${ES20 glCopyTexSubImage3DOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X 0x%08X}",
                target, level, xoffset, yoffset, zoffset, x, y, width, height);

    glmPROFILE(context, GLES2_COPYSUBIMAGE3DOES, 0);

#if gldFBO_DATABASE
    if ((context->framebuffer != gcvNULL)
        &&
        (context->framebuffer->currentDB != gcvNULL)
        )
    {
        /* Play current database. */
        if (gcmIS_ERROR(glshPlayDatabase(context,
                                         context->framebuffer->currentDB)))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            break;
        }
    }
#endif

    if ( (width <= 0) || (height <= 0) )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    switch (target)
    {
    case GL_TEXTURE_3D_OES:
        texture = context->texture3D[context->textureUnit];
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X", __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    /* See if this is the default texture. */
    if (texture == gcvNULL)
    {
        switch (target)
        {
        case GL_TEXTURE_3D_OES:
            texture = &context->default3D;
            break;

        default:
            gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, target);
            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = GL_TRUE;
			break;
		}
		if (goToEnd)
			break;

        /* Create a new texture object. */
        if (texture->texture == gcvNULL)
        {
            status = _NewTextureObject(context, texture);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s(%d): Could not create default texture",
                         __FUNCTION__, __LINE__);
                gl2mERROR(GL_OUT_OF_MEMORY);
                break;
            }
        }
    }

    gcmASSERT(texture != gcvNULL);
    gcmASSERT(texture->texture != gcvNULL);

    status = gcoTEXTURE_GetMipMap(texture->texture, level, &surface);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    status = gcoSURF_GetSize(surface, &mapWidth, &mapHeight, &mapDepth);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    if (((xoffset + width)  > (GLint) mapWidth)
    ||  ((yoffset + height) > (GLint) mapHeight)
    ||  (zoffset > (GLint) mapDepth)
    )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    if (context->framebuffer != gcvNULL)
    {
        read = (context->framebuffer->color.object == gcvNULL)
             ? gcvNULL
             : _glshGetFramebufferSurface(&context->framebuffer->color);
    }
    if (read == gcvNULL)
    {
        read = context->draw;
    }

    sx = x;
    sy = y;
    dx = xoffset;
    dy = yoffset;
    Width = width;
    Height = height;

    status = gcoSURF_GetSize(read, &srcWidth, &srcHeight, gcvNULL);
    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    status = gcoSURF_GetSize(surface, &dstWidth, &dstHeight, gcvNULL);
    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    if (!_glshCalculateArea(&dx, &dy, &sx, &sy, &Width, &Height, dstWidth, dstHeight, srcWidth, srcHeight))
    {
        gcmFATAL("glCopyTexSubImage2D: HAL produced error %d(%s)", status, gcoOS_DebugStatus2Name(status));
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    /* Update frame buffer. */
    _glshFrameBuffer(context);

    status = gcoSURF_CopyPixels(read,
                                surface,
                                sx, sy,
                                dx, dy,
                                Width, Height);

    if (gcmIS_ERROR(status))
    {
        gcmFATAL("%s(%d): HAL produced error %d(%s)", __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    /* Mark texture cache as dirty. */
    texture->dirty     = GL_TRUE;
    texture->needFlush = GL_TRUE;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

#if gcdNULL_DRIVER < 3
static gceSTATUS
_OrphaningMipmap(
    GLContext context,
    GLTexture texture
    )
{
    gcoSURF surface = gcvNULL;
    gcoSURF maxMip = gcvNULL;
    gcoSURF target = gcvNULL;
    gctUINT width, height, depth;
    gceSURF_FORMAT format;
    GLint level = 0;
    gctINT32 refCount;
    gceSTATUS status;

    status = gcoTEXTURE_GetMipMap(texture->texture,
                                  0,
                                  &surface);

    if (gcmIS_ERROR(status))
    {
        return gcvSTATUS_OK;
    }

    status = gcoSURF_QueryReferenceCount(surface, &refCount);

    /* Not an eglImage sibling, doesn't do orphaning. */
    if (gcmIS_ERROR(status) || (refCount == 1))
    {
        return gcvSTATUS_OK;
    }

    /* Check if the mipmap is completed. */
    status = gcoSURF_GetSize(surface, &width, &height, gcvNULL);

    if (gcmIS_ERROR(status))
    {
        return gcvSTATUS_OK;
    }

    while ((width > 1) || (height > 1))
    {
        width  = width >> 1;
        height = height >> 1;
        level++;
    }

    status = gcoTEXTURE_GetMipMap(texture->texture,
                                  level,
                                  &maxMip);

    /* Mipmap is finished, doesn't need to do orphaning. */
    if (gcmIS_SUCCESS(status))
    {
        return gcvSTATUS_OK;
    }

    /* Else, begin to orphan. */
    do
    {
        gcmERR_BREAK(gcoSURF_GetFormat(surface, gcvNULL, &format));

        gcmERR_BREAK(gcoSURF_GetSize(surface, &width, &height, &depth));

        /* Construct a new surface and add it to the texture. */
        gcmERR_BREAK(gcoSURF_Construct(context->hal,
                                       width, height, depth,
                                       gcvSURF_TEXTURE,
                                       format,
                                       gcvPOOL_DEFAULT,
                                       &target));

        gcmERR_BREAK(gcoSURF_Copy(target, surface));

        gcmERR_BREAK(gcoTEXTURE_Destroy(texture->texture));

        gcmERR_BREAK(_NewTextureObject(context, texture));

        gcmERR_BREAK(gcoTEXTURE_AddMipMapFromSurface(texture->texture,
                                                     0,
                                                     target));
        texture->dirty = GL_TRUE;
    }
    while (gcvFALSE);

    if(gcmIS_ERROR(status) && (target != gcvNULL))
    {
        gcmVERIFY_OK(gcoSURF_Destroy(target));
    }

    return status;
}
#endif

GL_APICALL void GL_APIENTRY
glGenerateMipmap(
    GLenum target
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture texture = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;
    GLint level, faces = 0;
    gcoSURF curr = gcvNULL, next = gcvNULL;
	GLboolean goToEnd = GL_FALSE;

	glmENTER1(glmARGENUM, target)
	{
    gcmDUMP_API("${ES20 glGenerateMipmap 0x%08X}", target);

    glmPROFILE(context, GLES2_GENERATEMIPMAP, 0);

    switch (target)
    {
    case GL_TEXTURE_2D:
        texture = context->texture2D[context->textureUnit];
        faces   = 0;
        break;

    case GL_TEXTURE_3D_OES:
        texture = context->texture3D[context->textureUnit];
        faces   = 0;
        break;

    case GL_TEXTURE_CUBE_MAP:
        texture = context->textureCube[context->textureUnit];
        faces   = 6;
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
		goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    /* See if this is the default texture. */
    if (texture == gcvNULL)
    {
        switch (target)
        {
        case GL_TEXTURE_2D:
            texture = &context->default2D;
            break;

        case GL_TEXTURE_3D_OES:
            texture = &context->default3D;
            break;

        case GL_TEXTURE_CUBE_MAP:
            texture = &context->defaultCube;
            break;

        default:
            gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, target);
            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = GL_TRUE;
			break;
		}
		if (goToEnd)
			break;
    }

    gcmASSERT(texture != gcvNULL);

    if (texture->texture == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGenerateMipMap: No texture object created for target %04X",
                 target);
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    if (gcmIS_ERROR(_OrphaningMipmap(context, texture)))
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGenerateMipMap: Orphaning eglImage sibling error %04X",
                 target);
        gl2mERROR(GL_OUT_OF_MEMORY);
        break;
    }

    if (texture->direct.source != gcvNULL)
    {
        int lod = 0;
        gctUINT lod_width, lod_height;
        if (texture->direct.temp != gcvNULL)
        {
            gcoSURF_FilterBlit(
                texture->direct.source,
                texture->direct.temp,
                gcvNULL, gcvNULL, gcvNULL
                );

            /* Stall the hardware. */
            gcmONERROR(gcoHAL_Commit(context->hal, gcvTRUE));

            /* Create the first LOD. */
            gcmONERROR(gcoSURF_Resolve(
                texture->direct.temp,
                texture->direct.texture[0]
                ));
        }
        else
        {
            if (gcmIS_ERROR(gcoSURF_Flush(texture->direct.texture[0])))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                break;
            }

            gco3D_Semaphore(context->engine,
                            gcvWHERE_RASTER,
                            gcvWHERE_PIXEL,
                            gcvHOW_SEMAPHORE_STALL);
            if (context->ditherEnable)
            {
                /*disable the dither for resolve*/
                gco3D_EnableDither(context->engine, gcvFALSE);
            }

            /* Create the first LOD. */
            if (gcmIS_ERROR(gcoSURF_Resolve(texture->direct.source,
                                            texture->direct.texture[0])))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                break;
            }

            if (context->ditherEnable)
            {
                /* restore the dither */
                gco3D_EnableDither(context->engine, gcvTRUE);
            }
        }
        /* Generate LODs. */
        lod_width = texture->direct.lodWidth;
        lod_height = texture->direct.lodHeight;
        for (lod = 1; lod <= texture->direct.maxLod; lod++)
        {
            lod_width = lod_width / 2;
            lod_height = lod_height / 2;

            if (texture->direct.texture[lod] == gcvNULL)
            {
                /* Allocate the LOD surface. */
                gcoTEXTURE_AddMipMap(
                    texture->texture,
                    lod,                      /* LOD #. */
                    texture->direct.textureFormat,          /* Format. */
                    lod_width, lod_height, 0, 0,
                    gcvPOOL_DEFAULT,
                    &texture->direct.texture[lod]
                    );
            }

            gcoSURF_Resample(
                texture->direct.texture[lod - 1],
                texture->direct.texture[lod]
                );
        }

        /* wait all the pixels done */
        gco3D_Semaphore(context->engine,
                                    gcvWHERE_RASTER,
                                    gcvWHERE_PIXEL,
                                    gcvHOW_SEMAPHORE_STALL);

        /* Reset planar dirty flag. */
        texture->direct.dirty = gcvFALSE;

    }
    else
    for (level = 0;; ++level)
    {
        gcoSURF surface[2];
        gctUINT width, height, depth;
        gceSURF_FORMAT format;

        gcmERR_BREAK(gcoTEXTURE_GetMipMap(texture->texture,
                                          level,
                                          &surface[0]));

        gcmERR_BREAK(gcoSURF_GetFormat(surface[0], gcvNULL, &format));

        gcmERR_BREAK(gcoSURF_GetSize(surface[0], &width, &height, &depth));

        if ((width == 1) && (height == 1))
        {
            status = gcvSTATUS_OK;
            break;
        }

        status = gcoTEXTURE_GetMipMap(texture->texture,
                                      level + 1,
                                      &surface[1]);

        if (gcmIS_ERROR(status))
        {
            gcmERR_BREAK(gcoTEXTURE_AddMipMap(texture->texture,
                                              level + 1,
                                              format,
                                              gcmMAX((width  >> 1), 1),
                                              gcmMAX((height >> 1), 1),
                                              0,
                                              faces,
                                              gcvPOOL_DEFAULT,
                                              &surface[1]));
        }

        if (texture->dirty)
        {
            /* Power of two texture. */
            if (!(width & (width -1)) && !(height & (height -1)))
            {
                /* Resample to create the lower level. */
                gcmERR_BREAK(gcoSURF_Resample(surface[0], surface[1]));
            }
            else
            /* None power of two texture, use filter blit. */
            {
                gctPOINTER address;
                gctUINT    newWidth, newHeight;
                gctINT     stride;

                newWidth  = gcmMAX((width  >> 1), 1);
                newHeight = gcmMAX((height >> 1), 1);

                if (curr == gcvNULL)
                {
                    /* Construct a none power of surface for curren level. */
                    gcmONERROR(gcoSURF_Construct(context->hal,
                                                 width,
                                                 height,
                                                 1,
                                                 gcvSURF_BITMAP,
                                                 format,
                                                 gcvPOOL_DEFAULT,
                                                 &curr));

                    gcmONERROR(gcoSURF_SetOrientation(curr,
                                                      gcvORIENTATION_TOP_BOTTOM));

                    /* Convert original texture to linear format. */
                    gcmONERROR(gcoSURF_CopyPixels(surface[0], curr,
                                                  0, 0, 0, 0,
                                                  width, height));
                }

                /* Construct a none power of 2 surface for next level. */
                gcmONERROR(gcoSURF_Construct(context->hal,
                                             newWidth,
                                             newHeight,
                                             1,
                                             gcvSURF_BITMAP,
                                             format,
                                             gcvPOOL_DEFAULT,
                                             &next));

                gcmONERROR(gcoHAL_SetHardwareType(context->hal,
                                                  gcvHARDWARE_2D));

                /* Filter blit to scale down. */
                gcmONERROR(gcoSURF_FilterBlit(curr, next,
                                              gcvNULL, gcvNULL,
                                              gcvNULL));

                gcmONERROR(gcoHAL_Commit(context->hal, gcvTRUE));

                gcmONERROR(gcoHAL_SetHardwareType(context->hal,
                                                  gcvHARDWARE_3D));

                gcmONERROR(gcoSURF_Lock(next, gcvNULL, &address));

                gcmONERROR(gcoSURF_GetAlignedSize(next, gcvNULL, gcvNULL, &stride));

                /* Upload from a linear address. */
                gcmONERROR(gcoTEXTURE_Upload(texture->texture,
                                             faces,
                                             newWidth,
                                             newHeight,
                                             0,
                                             address,
                                             stride,
                                             format));

                gcmONERROR(gcoSURF_Destroy(curr));

                curr = next;
                next = gcvNULL;
            }

            /* Mark texture cache as dirty. */
            texture->needFlush = GL_TRUE;
        }
    }

    if (gcmIS_ERROR(status))
    {
        gcmFATAL("%s(%d): HAL produced error %d", __FUNCTION__, __LINE__, status);
        gl2mERROR(GL_INVALID_OPERATION);
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    /* Destory temp surface. */
    if (curr != gcvNULL)
    {
        gcoSURF_Destroy(curr);
        curr = gcvNULL;
    }

	}
	glmLEAVE();
    return;
OnError:

    /* Make sure hardware type is set back correctly. */
    gcoHAL_SetHardwareType(context->hal, gcvHARDWARE_3D);

    if (curr != gcvNULL)
    {
        gcoSURF_Destroy(curr);
        curr = gcvNULL;
    }

    if (next != gcvNULL)
    {
        gcoSURF_Destroy(next);
        next = gcvNULL;
    }

    gcmFOOTER();
#endif
}

#if gcdNULL_DRIVER < 3

static void
_DecodeETC1Block(
    GLubyte * Output,
    GLsizei Stride,
    GLsizei Width,
    GLsizei Height,
    const GLubyte * Data
    )
{
    GLubyte base[2][3];
    GLboolean flip, diff;
    GLbyte index[2];
    GLint i, j, x, y, offset;
    static GLubyte table[][2] =
    {
        {  2,   8 },
        {  5,  17 },
        {  9,  29 },
        { 13,  42 },
        { 18,  60 },
        { 24,  80 },
        { 33, 106 },
        { 47, 183 },
    };

    diff = Data[3] & 0x2;
    flip = Data[3] & 0x1;

    if (diff)
    {
        GLbyte delta[3];

        base[0][0] = (Data[0] & 0xF8) | (Data[0] >> 5);
        base[0][1] = (Data[1] & 0xF8) | (Data[1] >> 5);
        base[0][2] = (Data[2] & 0xF8) | (Data[1] >> 5);

        delta[0] = (GLbyte) ((Data[0] & 0x7) << 5) >> 2;
        delta[1] = (GLbyte) ((Data[1] & 0x7) << 5) >> 2;
        delta[2] = (GLbyte) ((Data[2] & 0x7) << 5) >> 2;
        base[1][0] = base[0][0] + delta[0];
        base[1][1] = base[0][1] + delta[1];
        base[1][2] = base[0][2] + delta[2];
        base[1][0] |= base[1][0] >> 5;
        base[1][1] |= base[1][1] >> 5;
        base[1][2] |= base[1][2] >> 5;
    }
    else
    {
        base[0][0] = (Data[0] & 0xF0) | (Data[0] >> 4  );
        base[0][1] = (Data[1] & 0xF0) | (Data[1] >> 4  );
        base[0][2] = (Data[2] & 0xF0) | (Data[2] >> 4  );
        base[1][0] = (Data[0] << 4  ) | (Data[0] & 0x0F);
        base[1][1] = (Data[1] << 4  ) | (Data[1] & 0x0F);
        base[1][2] = (Data[2] << 4  ) | (Data[2] & 0x0F);
    }

    index[0] = (Data[3] & 0xE0) >> 5;
    index[1] = (Data[3] & 0x1C) >> 2;

    for (i = x = y = offset = 0; i < 2; ++i)
    {
        GLubyte msb = Data[5 - i];
        GLubyte lsb = Data[7 - i];

        for (j = 0; j < 8; ++j)
        {
            GLuint delta = 0;
            GLint r, g, b;
            GLint block = flip
                        ? (y < 2) ? 0 : 1
                        : (x < 2) ? 0 : 1;

            switch (((msb & 1) << 1) | (lsb & 1))
            {
            case 0x3: delta = -table[index[block]][1]; break;
            case 0x2: delta = -table[index[block]][0]; break;
            case 0x0: delta =  table[index[block]][0]; break;
            case 0x1: delta =  table[index[block]][1]; break;
            }

            r = base[block][0] + delta; r = gcmMAX(0x00, gcmMIN(r, 0xFF));
            g = base[block][1] + delta; g = gcmMAX(0x00, gcmMIN(g, 0xFF));
            b = base[block][2] + delta; b = gcmMAX(0x00, gcmMIN(b, 0xFF));

            if ((x < Width) && (y < Height))
            {
                Output[offset + 0] = (GLubyte) r;
                Output[offset + 1] = (GLubyte) g;
                Output[offset + 2] = (GLubyte) b;
            }

            offset += Stride;
            if (++y == 4)
            {
                y = 0;
                ++x;

                offset += 3 - 4 * Stride;
            }

            msb >>= 1;
            lsb >>= 1;
        }
    }
}

static void *
_DecompressETC1(
    IN GLContext Context,
    IN GLsizei Width,
    IN GLsizei Height,
    IN GLsizei ImageSize,
    IN const void * Data,
    OUT gceSURF_FORMAT * Format
    )
{
    GLubyte * pixels, * line;
    const GLubyte * data;
    GLsizei x, y, stride;
    gctPOINTER pointer = gcvNULL;

    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                   Width * Height * 3,
                                   &pointer)))
    {
        gl2mERROR(GL_OUT_OF_MEMORY);
        return gcvNULL;
    }

    pixels = pointer;

    stride = Width * 3;
    data   = Data;

    for (y = 0, line = pixels; y < Height; y += 4, line += stride * 4)
    {
        GLubyte * p = line;

        for (x = 0; x < Width; x += 4)
        {
            gcmASSERT(ImageSize >= 8);

            _DecodeETC1Block(p,
                             stride,
                             gcmMIN(Width - x, 4),
                             gcmMIN(Height - y, 4),
                             data);

            p         += 4 * 3;
            data      += 8;
            ImageSize -= 8;
        }
    }

    *Format = gcvSURF_B8G8R8;

    return pixels;
}


#define glmRED(c)   ( (c) >> 11 )
#define glmGREEN(c) ( ((c) >> 5) & 0x3F )
#define glmBLUE(c)  ( (c) & 0x1F )

/* Decode 64-bits of color information. */
static void
_DecodeDXTColor16(
    IN GLsizei Width,
    IN GLsizei Height,
    IN GLsizei Stride,
    IN const GLubyte * Data,
    OUT GLubyte * Output
    )
{
    GLushort c0, c1;
    GLushort color[4];
    GLushort r, g, b;
    GLint x, y;

    /* Decode color 0. */
	c0 = *(GLushort*)Data;
    color[0] = 0x8000 | (c0 & 0x001F) | ((c0 & 0xFFC0) >> 1);

    /* Decode color 1. */
	c1 = *(GLushort*)(Data + 2);
    color[1] = 0x8000 | (c1 & 0x001F) | ((c1 & 0xFFC0) >> 1);

	if (c0 > c1)
	{
		/* Compute color 2: (c0 * 2 + c1) / 3. */
		r = (2 * glmRED  (c0) + glmRED  (c1)) / 3;
		g = (2 * glmGREEN(c0) + glmGREEN(c1)) / 3;
		b = (2 * glmBLUE (c0) + glmBLUE (c1)) / 3;
        color[2] = 0x8000 | (r << 10) | ((g & 0x3E) << 4) | b;


		/* Compute color 3: (c0 + 2 * c1) / 3. */
		r = (glmRED  (c0) + 2 * glmRED  (c1)) / 3;
		g = (glmGREEN(c0) + 2 * glmGREEN(c1)) / 3;
		b = (glmBLUE (c0) + 2 * glmBLUE (c1)) / 3;
        color[3] = 0x8000 | (r << 10) | ((g & 0x3E) << 4) | b;

	}
	else
	{
        /* Compute color 2: (c0 + c1) / 2. */

        r = (glmRED  (color[0]) + glmRED  (color[1])) / 2;

        g = (glmGREEN(color[0]) + glmGREEN(color[1])) / 2;

        b = (glmBLUE (color[0]) + glmBLUE (color[1])) / 2;

        color[2] = (r << 11) | ((g & 0x3F) << 5) | b;



        /* Color 3 is opaque black. */

        color[3] = 0;

	}
    /* Walk all lines. */
    for (y = 0; y < Height; y++)
    {
        /* Get lookup for line. */
        GLubyte bits = Data[4 + y];

        /* Walk all pixels. */
        for (x = 0; x < Width; x++, bits >>= 2, Output += 2)
        {
            /* Copy the color. */
            *(GLshort *) Output = color[bits & 3];
        }

        /* Next line. */
        Output += Stride - Width * 2;
    }
}

#define glmEXPAND_RED(c)   ( (((c) & 0xF800) << 8) | (((c) & 0xE000) << 3) )
#define glmEXPAND_GREEN(c) ( (((c) & 0x07E0) << 5) | (((c) & 0x0600) >> 1) )
#define glmEXPAND_BLUE(c)  ( (((c) & 0x001F) << 3) | (((c) & 0x001C) >> 2) )

/* Decode 64-bits of color information. */
static void
_DecodeDXTColor32(
    IN GLsizei Width,
    IN GLsizei Height,
    IN GLsizei Stride,
    IN const GLubyte * Data,
    IN const GLubyte *Alpha,
    OUT GLubyte * Output
    )
{
    GLuint color[4];
    GLushort c0, c1;
    GLint x, y;
    GLuint r, g, b;

    /* Decode color 0. */
    c0       = Data[0] | (Data[1] << 8);
    color[0] = glmEXPAND_RED(c0) | glmEXPAND_GREEN(c0) | glmEXPAND_BLUE(c0);

    /* Decode color 1. */
    c1       = Data[2] | (Data[3] << 8);
    color[1] = glmEXPAND_RED(c1) | glmEXPAND_GREEN(c1) | glmEXPAND_BLUE(c1);

    /* Compute color 2: (c0 * 2 + c1) / 3. */
    r = (2 * (color[0] & 0xFF0000) + (color[1] & 0xFF0000)) / 3;
    g = (2 * (color[0] & 0x00FF00) + (color[1] & 0x00FF00)) / 3;
    b = (2 * (color[0] & 0x0000FF) + (color[1] & 0x0000FF)) / 3;
    color[2] = (r & 0xFF0000) | (g & 0x00FF00) | (b & 0x0000FF);

    /* Compute color 3: (c0 + 2 * c1) / 3. */
    r = ((color[0] & 0xFF0000) + 2 * (color[1] & 0xFF0000)) / 3;
    g = ((color[0] & 0x00FF00) + 2 * (color[1] & 0x00FF00)) / 3;
    b = ((color[0] & 0x0000FF) + 2 * (color[1] & 0x0000FF)) / 3;
    color[3] = (r & 0xFF0000) | (g & 0x00FF00) | (b & 0x0000FF);

    /* Walk all lines. */
    for (y = 0; y < Height; y++)
    {
        /* Get lookup for line. */
        GLubyte bits = Data[4 + y];
        GLint   a    = y << 2;

        /* Walk all pixels. */
        for (x = 0; x < Width; x++, bits >>= 2, Output += 4)
        {
            /* Copmbine the lookup color with the alpha value. */
            GLuint c = color[bits & 3] | (Alpha[a++] << 24);
            *(GLuint *) Output = c;
        }

        /* Next line. */
        Output += Stride - Width * 4;
    }
}

/* Decode 64-bits of alpha information. */
static void
_DecodeDXT3Alpha(
    IN const GLubyte * Data,
    OUT GLubyte * Output
    )
{
    GLint i;
    GLubyte a;

    /* Walk all alpha pixels. */
    for (i = 0; i < 8; i++, Data++)
    {
        /* Get even alpha and expand into 8-bit. */
        a = *Data & 0x0F;
        *Output++ = a | (a << 4);

        /* Get odd alpha and expand into 8-bit. */
        a = *Data >> 4;
        *Output++ = a | (a << 4);
    }
}

/* Decode 64-bits of alpha information. */
static void
_DecodeDXT5Alpha(
    IN const GLubyte * Data,
    OUT GLubyte * Output
    )
{
    GLint i, j, n;
    GLubyte a[8];
    GLushort bits = 0;

    /* Load alphas 0 and 1. */
    a[0] = Data[0];
    a[1] = Data[1];

    if (a[0] > a[1])
    {
        /* Interpolate alphas 2 through 7. */
        a[2] = (GLubyte) ((6 * a[0] + 1 * a[1]) / 7);
        a[3] = (GLubyte) ((5 * a[0] + 2 * a[1]) / 7);
        a[4] = (GLubyte) ((4 * a[0] + 3 * a[1]) / 7);
        a[5] = (GLubyte) ((3 * a[0] + 4 * a[1]) / 7);
        a[6] = (GLubyte) ((2 * a[0] + 5 * a[1]) / 7);
        a[7] = (GLubyte) ((1 * a[0] + 6 * a[1]) / 7);
    }
    else
    {
        /* Interpolate alphas 2 through 5. */
        a[2] = (GLubyte) ((4 * a[0] + 1 * a[1]) / 5);
        a[3] = (GLubyte) ((3 * a[0] + 2 * a[1]) / 5);
        a[4] = (GLubyte) ((2 * a[0] + 3 * a[1]) / 5);
        a[5] = (GLubyte) ((1 * a[0] + 4 * a[1]) / 5);

        /* Set alphas 6 and 7. */
        a[6] = 0;
        a[7] = 255;
    }

    /* Walk all pixels. */
    for (i = 0, j = 2, n = 0; i < 16; i++, bits >>= 3, n -= 3)
    {
        /* Test if we have enough bits in the accumulator. */
        if (n < 3)
        {
            /* Load another chunk of bits in the accumulator. */
            bits |= Data[j++] << n;
            n += 8;
        }

        /* Copy decoded alpha value. */
        Output[i] = a[bits & 0x7];
    }
}

/* Decompress a DXT texture. */
static void *
_DecompressDXT(
    IN GLContext Context,
    IN GLsizei Width,
    IN GLsizei Height,
    IN GLsizei ImageSize,
    IN const void * Data,
    IN GLenum InternalFormat,
    IN gceSURF_FORMAT Format
    )
{
    GLubyte * pixels, * line;
    const GLubyte * data;
    GLsizei x, y, stride;
    gctPOINTER pointer = gcvNULL;
    GLubyte alpha[16];
    GLint bpp;

    /* Determine bytes per pixel. */
    bpp = ((Format == gcvSURF_A1R5G5B5) || (Format == gcvSURF_R5G6B5)) ? 2 : 4;

    /* Allocate the decompressed memory. */
    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL, Width * Height * bpp, &pointer)))
    {
        /* Error. */
        gl2mERROR(GL_OUT_OF_MEMORY);
        return gcvNULL;
    }

    /* Initialize the variables. */
    pixels = pointer;
    stride = Width * bpp;
    data   = Data;

    /* Walk all lines, 4 lines per block. */
    for (y = 0, line = pixels; y < Height; y += 4, line += stride * 4)
    {
        GLubyte * p = line;

        /* Walk all pixels, 4 pixels per block. */
        for (x = 0; x < Width; x += 4)
        {
            /* Dispatch on format. */
            switch (InternalFormat)
            {
            default:
                gcmASSERT(ImageSize >= 8);

                /* Decompress one color block. */
                _DecodeDXTColor16(gcmMIN(Width - x, 4),
                                  gcmMIN(Height - y, 4),
                                  stride,
                                  data,
                                  p);

                /* 8 bytes per block. */
                data      += 8;
                ImageSize -= 8;
                break;

            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
                gcmASSERT(ImageSize >= 16);

                /* Decode DXT3 alpha. */
                _DecodeDXT3Alpha(data, alpha);

                /* Decompress one color block. */
                _DecodeDXTColor32(gcmMIN(Width - x, 4),
                                  gcmMIN(Height - y, 4),
                                  stride,
                                  data + 8,
                                  alpha,
                                  p);

                /* 16 bytes per block. */
                data      += 16;
                ImageSize -= 16;
                break;

            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                gcmASSERT(ImageSize >= 16);

                /* Decode DXT5 alpha. */
                _DecodeDXT5Alpha(data, alpha);

                /* Decompress one color block. */
                _DecodeDXTColor32(gcmMIN(Width - x, 4),
                                  gcmMIN(Height - y, 4),
                                  stride,
                                  data + 8,
                                  alpha,
                                  p);

                /* 16 bytes per block. */
                data      += 16;
                ImageSize -= 16;
                break;
            }

            /* Next block. */
            p += 4 * bpp;
        }
    }

    /* Return pointer to decompressed data. */
    return pixels;
}


static void *
_DecompressPalette(
    IN GLContext Context,
    IN GLenum InternalFormat,
    IN GLsizei Width,
    IN GLsizei Height,
    IN GLint Level,
    IN GLsizei ImageSize,
    IN const void * Data,
    OUT gceSURF_FORMAT * Format
    )
{
    GLint pixelBits = 0, paletteBytes = 0;
    const GLubyte * palette;
    const GLubyte * data;
    GLubyte * pixels;
    GLsizei x, y, bytes;
    GLuint offset;
    gctPOINTER pointer = gcvNULL;

    switch (InternalFormat)
    {
    case GL_PALETTE4_RGBA4_OES:
        pixelBits    = 4;
        paletteBytes = 16 / 8;
        *Format      = gcvSURF_R4G4B4A4;
        break;

    case GL_PALETTE4_RGB5_A1_OES:
        pixelBits    = 4;
        paletteBytes = 16 / 8;
        *Format      = gcvSURF_R5G5B5A1;
        break;

    case GL_PALETTE4_R5_G6_B5_OES:
        pixelBits    = 4;
        paletteBytes = 16 / 8;
        *Format      = gcvSURF_R5G6B5;
        break;

    case GL_PALETTE4_RGB8_OES:
        pixelBits    = 4;
        paletteBytes = 24 / 8;
        *Format      = gcvSURF_B8G8R8;
        break;

    case GL_PALETTE4_RGBA8_OES:
        pixelBits    = 4;
        paletteBytes = 32 / 8;
        *Format      = gcvSURF_A8B8G8R8;
        break;

    case GL_PALETTE8_RGBA4_OES:
        pixelBits    = 8;
        paletteBytes = 16 / 8;
        *Format      = gcvSURF_R4G4B4A4;
        break;

    case GL_PALETTE8_RGB5_A1_OES:
        pixelBits    = 8;
        paletteBytes = 16 / 8;
        *Format      = gcvSURF_R5G5B5A1;
        break;

    case GL_PALETTE8_R5_G6_B5_OES:
        pixelBits    = 8;
        paletteBytes = 16 / 8;
        *Format      = gcvSURF_R5G6B5;
        break;

    case GL_PALETTE8_RGB8_OES:
        pixelBits    = 8;
        paletteBytes = 24 / 8;
        *Format      = gcvSURF_B8G8R8;
        break;

    case GL_PALETTE8_RGBA8_OES:
        pixelBits    = 8;
        paletteBytes = 32 / 8;
        *Format      = gcvSURF_A8B8G8R8;
        break;
    }

    palette = Data;

    bytes = paletteBytes << pixelBits;
    data  = (const GLubyte *) palette + bytes;

    gcmASSERT(ImageSize > bytes);
    ImageSize -= bytes;

    while (Level-- > 0)
    {
        bytes  = gcmALIGN(Width * pixelBits, 8) / 8 * Height;
        data  += bytes;

        gcmASSERT(ImageSize > bytes);
        ImageSize -= bytes;

        Width  = Width / 2;
        Height = Height / 2;
    }

    bytes = gcmALIGN(Width * paletteBytes, Context->unpackAlignment) * Height;

    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL, bytes, &pointer)))
    {
        gl2mERROR(GL_OUT_OF_MEMORY);
        return gcvNULL;
    }

    pixels = pointer;

    for (y = 0, offset = 0; y < Height; ++y)
    {
        for (x = 0; x < Width; ++x)
        {
            GLubyte pixel;

            gcmASSERT(ImageSize > 0);
            if (pixelBits == 4)
            {
                if (x & 1)
                {
                    pixel = *data++ & 0xF;
                    --ImageSize;
                }
                else
                {
                    pixel = *data >> 4;
                }
            }
            else
            {
                pixel = *data++;
                --ImageSize;
            }

            gcmVERIFY_OK(gcoOS_MemCopy(pixels + offset,
                                       palette + pixel * paletteBytes,
                                       paletteBytes));

            offset += paletteBytes;
        }

        offset = gcmALIGN(offset, Context->unpackAlignment);

        if (x & 1)
        {
            ++data;
            --ImageSize;
        }
    }

    return pixels;
}
#endif


GL_APICALL void GL_APIENTRY
glCompressedTexImage2D(
    GLenum target,
    GLint level,
    GLenum internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLsizei imageSize,
    const void *data
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture texture = gcvNULL;
    gceSTATUS status;
    gcoSURF surface;
    gceTEXTURE_FACE face = gcvFACE_NONE;
    GLint faces = 0, levels = 0;
    gceSURF_FORMAT format;
	GLboolean goToEnd = GL_FALSE;

	glmENTER8(glmARGENUM, target, glmARGINT, level, glmARGENUM, internalformat,
		      glmARGINT, width, glmARGINT, height, glmARGINT, border, glmARGINT, imageSize, glmARGPTR, data)
	{
    gcmDUMP_API("${ES20 glCompressedTexImage2D 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X 0x%08X (0x%08X)",
                target, level, internalformat, width, height, border, imageSize,
                data);
    gcmDUMP_API_DATA(data, imageSize);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_COMPRESSEDTEXIMAGE2D, 0);

    if ( (border != 0) ||
         /* Valid Width and Height. */
         (width  < 0) || (height < 0) ||
         (width  > ((GLsizei)context->maxTextureWidth + 2))  ||
         (height > ((GLsizei)context->maxTextureHeight + 2))
       )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    switch (target)
    {
    case GL_TEXTURE_2D:
        texture = context->texture2D[context->textureUnit];
        face    = gcvFACE_NONE;
        faces   = 0;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_X;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_X;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_Y;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_Y;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_POSITIVE_Z;
        faces   = 6;
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        texture = context->textureCube[context->textureUnit];
        face    = gcvFACE_NEGATIVE_Z;
        faces   = 6;
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X", __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    /* See if this is the default texture. */
    if (texture == gcvNULL)
    {
        switch (target)
        {
        case GL_TEXTURE_2D:
            texture = &context->default2D;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            texture = &context->defaultCube;
            break;

        default:
            gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, target);
            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = GL_TRUE;
			break;
		}
		if (goToEnd)
			break;
    }

    /* Destroy current texture if we need to upload level 0. */
    if ( (level <= 0) && (texture->texture != gcvNULL))
    {
        if ((target == GL_TEXTURE_2D)
            || ((texture->width != width)|| (texture->height != height))
           )
        {
            gcmVERIFY_OK(gcoTEXTURE_Destroy(texture->texture));
            texture->texture = gcvNULL;
        }
    }

    /* Create a new texture object. */
    if (texture->texture == gcvNULL)
    {
        status = _NewTextureObject(context, texture);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): Could not create default texture",
                     __FUNCTION__, __LINE__);
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }

        /* Set endian hint */
        gcmVERIFY_OK(gcoTEXTURE_SetEndianHint(texture->texture,
                                              _gl2gcEndianHint(internalformat, 0)));
    }

    gcmASSERT(texture != gcvNULL);
    gcmASSERT(texture->texture != gcvNULL);

    /* Save texture properties at level 0. */
    if (level <= 0)
    {
        texture->width  = width;
        texture->height = height;
    }

    if(_gl2gcCompressedFormat(internalformat, &format) != gcvSTATUS_OK)
    {
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    switch (internalformat)
    {
    case GL_PALETTE4_RGBA4_OES:
    case GL_PALETTE4_RGB5_A1_OES:
    case GL_PALETTE4_R5_G6_B5_OES:
    case GL_PALETTE4_RGB8_OES:
    case GL_PALETTE4_RGBA8_OES:
    case GL_PALETTE8_RGBA4_OES:
    case GL_PALETTE8_RGB5_A1_OES:
    case GL_PALETTE8_R5_G6_B5_OES:
    case GL_PALETTE8_RGB8_OES:
    case GL_PALETTE8_RGBA8_OES:
        if (level > 0)
        {
            gl2mERROR(GL_INVALID_VALUE);
            goToEnd = GL_TRUE;
			break;
        }

        levels = 1 - level;
        level  = 0;
        break;

    case GL_ETC1_RGB8_OES:
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        if (level < 0)
        {
            gl2mERROR(GL_INVALID_VALUE);
            goToEnd = GL_TRUE;
			break;;
        }

        levels = 1;
        break;

    default:
        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    /* Valid level. */
    if((level  > (GLint)gcoMATH_Ceiling(
                            gcoMATH_Log2(
                               (gctFLOAT)context->maxTextureWidth)))
       )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    while (levels-- > 0)
    {
        gceSURF_FORMAT imageFormat = gcvSURF_UNKNOWN;

        status = gcoTEXTURE_GetMipMap(texture->texture, level, &surface);

        if (gcmIS_ERROR(status))
        {
            status = gcoTEXTURE_AddMipMap(texture->texture,
                                          level,
                                          format,
                                          width, height, 0,
                                          faces,
                                          gcvPOOL_DEFAULT,
                                          &surface);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s(%d): Could not create the mip map "
                         "level %d",
                         __FUNCTION__, __LINE__, level);
                gl2mERROR(GL_OUT_OF_MEMORY);
                goToEnd = GL_TRUE;
				break;
            }
        }

        if ((imageSize > 0) && (data != gcvNULL))
        {
            void * pixels = gcvNULL;

            switch (internalformat)
            {
            case GL_ETC1_RGB8_OES:
                if (format != gcvSURF_ETC1)
                {
                    /* Decompress ETC1 texture since hardware doesn't support
                    ** it. */
                    pixels = _DecompressETC1(context,
                                             width, height,
                                             imageSize, data,
                                             &imageFormat);

                    if (pixels == gcvNULL)
                    {
                        /* Could not decompress the ETC1 texture. */
                        goToEnd = GL_TRUE;
						break;
                    }
                }
                break;

            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                if ((format < gcvSURF_DXT1) || (format > gcvSURF_DXT5))
                {
                    /* Decompress DXT texture since hardware doesn't support
                    ** it. */
                    pixels = _DecompressDXT(context,
                                            width, height,
                                            imageSize, data,
                                            internalformat,
                                            imageFormat = format);

                    if (pixels == gcvNULL)
                    {
                        /* Could not decompress the DXT texture. */
                        goToEnd = GL_TRUE;
						break;
                    }
                }
                break;


            case GL_PALETTE4_RGBA4_OES:
            case GL_PALETTE4_RGB5_A1_OES:
            case GL_PALETTE4_R5_G6_B5_OES:
            case GL_PALETTE4_RGB8_OES:
            case GL_PALETTE4_RGBA8_OES:
            case GL_PALETTE8_RGBA4_OES:
            case GL_PALETTE8_RGB5_A1_OES:
            case GL_PALETTE8_R5_G6_B5_OES:
            case GL_PALETTE8_RGB8_OES:
            case GL_PALETTE8_RGBA8_OES:
                pixels = _DecompressPalette(context,
                                            internalformat,
                                            width, height,
                                            level,
                                            imageSize, data,
                                            &imageFormat);

                if (pixels == gcvNULL)
                {
                    goToEnd = GL_TRUE;
				}
                break;

            default:
                gl2mERROR(GL_INVALID_ENUM);
                goToEnd = GL_TRUE;
				break;
            }
			if (goToEnd)
				break;

            if (pixels != gcvNULL)
            {
                status = gcoTEXTURE_Upload(texture->texture,
                                           face,
                                           width, height, 0,
                                           pixels, 0,
                                           imageFormat);

                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, pixels));

                if (gcmIS_ERROR(status))
                {
                    gcmFATAL("%s(%d): gcoTEXTURE_Upload "
                             "returned %d(%s)",
                             __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
                    gl2mERROR(GL_INVALID_OPERATION);
                    goToEnd = GL_TRUE;
					break;
                }
            }
            else
            {
                status = gcoTEXTURE_UploadCompressed(texture->texture,
                                                     face,
                                                     width, height, 0,
                                                     data,
                                                     imageSize);

                if (gcmIS_ERROR(status))
                {
                    gcmFATAL("%s(%d): "
                             "gcoTEXTURE_UploadCompressed returned %d(%s)",
                             __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
                    gl2mERROR(GL_INVALID_OPERATION);
                    goToEnd = GL_TRUE;
					break;
                }
            }

            texture->dirty     = GL_TRUE;
            texture->needFlush = GL_TRUE;
        }

        ++level;
    }
	if (goToEnd)
		break;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glCompressedTexImage3DOES(
    GLenum target,
    GLint level,
    GLenum internalformat,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLint border,
    GLsizei imageSize,
    const void *data
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture texture = gcvNULL;
    gceSTATUS status;
    gcoSURF surface;
    /*gceTEXTURE_FACE face;*/
    GLint faces = 0, levels = 0;
    gceSURF_FORMAT format;
	GLboolean goToEnd = GL_FALSE;

	glmENTER9(glmARGENUM, target, glmARGINT, level, glmARGENUM, internalformat,
		      glmARGINT, width, glmARGINT, height, glmARGINT, depth, glmARGINT, border,
			  glmARGINT, imageSize, glmARGPTR, data)
	{
    gcmDUMP_API("${ES20 glCompressedTexImage3DOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X 0x%08X (0x%08X)",
                target, level, internalformat, width, height,depth, border, imageSize,
                data);
    gcmDUMP_API_DATA(data, imageSize);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_COMPRESSEDTEXIMAGE3DOES, 0);

    if ( (width <= 0) || (height <= 0) || (depth <= 0))
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    switch (target)
    {
    case GL_TEXTURE_3D_OES:
        texture = context->texture3D[context->textureUnit];
        /*face    = gcvFACE_NONE;*/
        faces   = 0;
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X", __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    /* See if this is the default texture. */
    if (texture == gcvNULL)
    {
        switch (target)
        {
        case GL_TEXTURE_3D_OES:
            texture = &context->default3D;
            break;

        default:
            gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, target);
            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = GL_TRUE;
			break;
		}
		if (goToEnd)
			break;

        /* Destroy current texture if we need to upload level 0. */
        if ( (level <= 0) && (texture->texture != gcvNULL) )
        {
            gcmVERIFY_OK(gcoTEXTURE_Destroy(texture->texture));
            texture->texture = gcvNULL;
        }

        /* Create a new texture object. */
        if (texture->texture == gcvNULL)
        {
            status = _NewTextureObject(context, texture);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s(%d): Could not create default texture",
                         __FUNCTION__, __LINE__);
                gl2mERROR(GL_OUT_OF_MEMORY);
                break;
            }

            /* Set endian hint */
            gcmVERIFY_OK(gcoTEXTURE_SetEndianHint(texture->texture,
                _gl2gcEndianHint(internalformat, 0)));
        }
    }

    gcmASSERT(texture != gcvNULL);
    gcmASSERT(texture->texture != gcvNULL);

    if(_gl2gcCompressedFormat(internalformat, &format) != gcvSTATUS_OK)
    {
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    switch (internalformat)
    {
    case GL_PALETTE4_RGBA4_OES:
    case GL_PALETTE4_RGB5_A1_OES:
    case GL_PALETTE4_R5_G6_B5_OES:
    case GL_PALETTE4_RGB8_OES:
    case GL_PALETTE4_RGBA8_OES:
    case GL_PALETTE8_RGBA4_OES:
    case GL_PALETTE8_RGB5_A1_OES:
    case GL_PALETTE8_R5_G6_B5_OES:
    case GL_PALETTE8_RGB8_OES:
    case GL_PALETTE8_RGBA8_OES:
        if (level > 0)
        {
            gl2mERROR(GL_INVALID_VALUE);
            goToEnd = GL_TRUE;
			break;
		}

        levels = 1 - level;
        level  = 0;
        break;

    case GL_ETC1_RGB8_OES:
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        if (level < 0)
        {
            gl2mERROR(GL_INVALID_VALUE);
            goToEnd = GL_TRUE;
			break;
        }

        levels = 1;
        break;

    default:
        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    /* Valid level. */
    if((level  > (GLint)gcoMATH_Ceiling(
                            gcoMATH_Log2(
                               (gctFLOAT)context->maxTextureWidth)))
       )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    while (levels-- > 0)
    {
        status = gcoTEXTURE_GetMipMap(texture->texture, level, &surface);

        if (gcmIS_ERROR(status))
        {
            status = gcoTEXTURE_AddMipMap(texture->texture,
                                          level,
                                          format,
                                          width, height, depth,
                                          faces,
                                          gcvPOOL_DEFAULT,
                                          &surface);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s(%d): Could not create the mip map "
                         "level %d",
                         __FUNCTION__, __LINE__, level);
                gl2mERROR(GL_OUT_OF_MEMORY);
				goToEnd = GL_TRUE;
				break;
            }
        }

        if ((imageSize > 0) && (data != gcvNULL))
        {
            /*void * pixels = gcvNULL;*/

            switch (internalformat)
            {
            case GL_ETC1_RGB8_OES:
                if (format != gcvSURF_ETC1)
                {
                    /* Need implement 3D texture decompress */
                    gcmASSERT(0);
                }
                break;

            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                break;

            case GL_PALETTE4_RGBA4_OES:
            case GL_PALETTE4_RGB5_A1_OES:
            case GL_PALETTE4_R5_G6_B5_OES:
            case GL_PALETTE4_RGB8_OES:
            case GL_PALETTE4_RGBA8_OES:
            case GL_PALETTE8_RGBA4_OES:
            case GL_PALETTE8_RGB5_A1_OES:
            case GL_PALETTE8_R5_G6_B5_OES:
            case GL_PALETTE8_RGB8_OES:
            case GL_PALETTE8_RGBA8_OES:

                /* Need implement 3D texture decompress */
                gcmASSERT(0);
                break;

            default:
                gl2mERROR(GL_INVALID_ENUM);
                goToEnd = GL_TRUE;
				break;
            }
			if (goToEnd)
				break;

            /* Need upload 3D texture */
            gcmASSERT(0);

            texture->dirty     = GL_TRUE;
            texture->needFlush = GL_TRUE;
        }

        ++level;
    }
	if (goToEnd)
		break;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glCompressedTexSubImage2D(
    GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLsizei imageSize,
    const void *data
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER9(glmARGENUM, target, glmARGINT, level, glmARGINT, xoffset, glmARGINT, yoffset,
		      glmARGINT, width, glmARGINT, height, glmARGENUM, format, glmARGINT, imageSize, glmARGPTR, data)
	{
    gcmDUMP_API("${ES20 glCompressedTexSubImage2D 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X 0x%08X 0x%08X (0x%08X)",
                target, level, xoffset, yoffset, width, height, format,
                imageSize, data);
    gcmDUMP_API_DATA(data, imageSize);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_COMPRESSEDTEXSUBIMAGE2D, 0);

    gl2mERROR(GL_INVALID_OPERATION);

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glCompressedTexSubImage3DOES(
    GLenum target,
    GLint level,
    GLint xoffset,
    GLint yoffset,
    GLint zoffset,
    GLsizei width,
    GLsizei height,
    GLsizei depth,
    GLenum format,
    GLsizei imageSize,
    const void *data
    )
{
#if gcdNULL_DRIVER < 3
	glmENTER11(glmARGENUM, target, glmARGINT, level, glmARGINT, xoffset, glmARGINT, yoffset, glmARGINT, zoffset,
		      glmARGINT, width, glmARGINT, height, glmARGINT, depth, glmARGENUM, format, glmARGINT, imageSize, glmARGPTR, data)
	{
    gcmDUMP_API("${ES20 glCompressedTexSubImage3DOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X"
                "0x%08X 0x%08X 0x%08X 0x%08X 0x%08X (0x%08X)",
                target, level, xoffset, yoffset, zoffset, width, height, depth, format,
                imageSize, data);
    gcmDUMP_API_DATA(data, imageSize);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_COMPRESSEDTEXSUBIMAGE3DOES, 0);

    gl2mERROR(GL_INVALID_OPERATION);

    }
	glmLEAVE();
#endif
}

#if gcdNULL_DRIVER < 3
/**************** YUV Texture support ***********************/

static gceSTATUS _ResetTextureWrapper(
    GLContext Context,
    GLTexture Texture
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    do
    {
        /* Reset the number of levels. */
        /*Texture->maxLevel = -1;*/

        /* Remove existing YUV direct structure. */
        if (Texture->direct.source != gcvNULL)
        {
            /* Unlock the source surface. */
            gcmERR_BREAK(gcoSURF_Unlock(
                Texture->direct.source,
                gcvNULL
                ));

            /* Destroy the source surface. */
            if (Texture->fromImage == 0)
            {
                gcmERR_BREAK(gcoSURF_Destroy(
                    Texture->direct.source
                    ));
            }

            Texture->direct.source = gcvNULL;

            /* Destroy the temporary surface. */
            if (Texture->direct.temp != gcvNULL)
            {
                gcmERR_BREAK(gcoSURF_Destroy(
                    Texture->direct.temp
                    ));

                Texture->direct.temp = gcvNULL;
            }

            /* Free the LOD array. */
            gcmERR_BREAK(gcoOS_ZeroMemory(
                Texture->direct.texture, gcmSIZEOF(gcoSURF) * 16
                ));
        }
    }
    while (gcvFALSE);

    /* Return status. */
    return status;
}
#endif

#if gcdNULL_DRIVER < 3
static gceSTATUS _GetTextureAttribFromImage(
   khrEGL_IMAGE_PTR Image,
   gctINT_PTR       Width,
   gctINT_PTR       Height,
   gctINT_PTR       Stride,
   gceSURF_FORMAT  *Format,
   gctINT_PTR       Level,
   gctUINT32_PTR    Offset,
   gctPOINTER      *Pixel
   )
{
    gcoSURF   surface;
    gceSTATUS status = gcvSTATUS_OK;

    /* Get texture attributes. */
    switch (Image->type)
    {
    case KHR_IMAGE_TEXTURE_2D:
    case KHR_IMAGE_TEXTURE_CUBE:
    case KHR_IMAGE_RENDER_BUFFER:
    case KHR_IMAGE_ANDROID_NATIVE_BUFFER:
        {
            surface = Image->surface;

            if (!surface)
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            /* Query width and height from source. */
            gcmERR_BREAK(
                gcoSURF_GetSize(surface,
                                (gctUINT_PTR) Width,
                                (gctUINT_PTR) Height,
                                gcvNULL));

            /* Query source surface format. */
            gcmERR_BREAK(gcoSURF_GetFormat(surface, gcvNULL, Format));

            /* Query srouce surface stride. */
            gcmERR_BREAK(gcoSURF_GetAlignedSize(surface, gcvNULL, gcvNULL, Stride));

            if (Image->type == KHR_IMAGE_TEXTURE_CUBE)
            {
                *Offset = Image->u.texture.offset;
            }
            else
            {
                *Offset = 0;
            }

            *Level = 0;
            *Pixel = gcvNULL;
        }
        break;

    case KHR_IMAGE_PIXMAP:
        *Width  = Image->u.pixmap.width;
        *Height = Image->u.pixmap.height;
        *Stride = Image->u.pixmap.stride;
        *Format = Image->u.pixmap.format;
        *Pixel  = Image->u.pixmap.address;
        *Level  = 0;
        *Offset = 0;
        break;

    default:
        status = gcvSTATUS_INVALID_ARGUMENT;
        break;
    }

    return status;
}
#endif

GL_APICALL void GL_APIENTRY
glEGLImageTargetTexture2DOES(
    GLenum Target,
    GLeglImageOES Image
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    khrEGL_IMAGE * image = (khrEGL_IMAGE_PTR)(Image);
    GLTexture texture;
    gcoSURF surface;
    gctINT width = 0, height = 0;
    gctINT stride = 0;
    gctINT level = 0;
    gctUINT32 offset = 0;
    gceSURF_FORMAT format = gcvSURF_UNKNOWN;
    gctPOINTER pixel = gcvNULL;
    gceSTATUS status;
    gceSURF_FORMAT dstFormat = gcvSURF_UNKNOWN;
    gctBOOL yuvFormat = gcvFALSE;

    gcmHEADER_ARG("target=0x%04x image=0x%x", Target, Image);
    gcmDUMP_API("${ES20 glEGLImageTargetTexture2DOES 0x%08X 0x%08X}",
                Target, Image);

    /* Test context. */
    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    /* Validate Target. */
    switch (Target)
    {
    case GL_TEXTURE_2D:
        /* Get current tuxture. */
        texture = context->texture2D[context->textureUnit];

        /* See if this is the default texture. */
        if (texture == gcvNULL)
        {
            texture = &context->default2D;
        }
        break;

    case GL_TEXTURE_EXTERNAL_OES:
        /* Get current tuxture. */
        texture = context->textureExternal[context->textureUnit];

        /* See if this is the default texture. */
        if (texture == gcvNULL)
        {
            texture = &context->defaultExternal;
        }
        break;

    default:
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    /* Test if image is valid */
    if ((image == gcvNULL)
    ||  (image->magic != KHR_EGL_IMAGE_MAGIC_NUM)
    )
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Test image content. */
    if (image->surface == gcvNULL)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Get texture attributes from eglImage. */
    if (gcmIS_ERROR(
        _GetTextureAttribFromImage(image,
                                   &width, &height,
                                   &stride,
                                   &format,
                                   &level,
                                   &offset,
                                   &pixel)))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Check the texture size. */
    if ((width <= 0) || (height <= 0))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if ((Target == GL_TEXTURE_EXTERNAL_OES)
     && (level != 0))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Destroy current texture if we need to upload level 0. */
    if (texture->texture != gcvNULL)
    {
        gcmVERIFY_OK(gcoTEXTURE_Destroy(texture->texture));
        gcmVERIFY_OK(_ResetTextureWrapper(context,texture));

        texture->texture = gcvNULL;
    }

    /* Create a new texture object. */
    status = _NewTextureObject(context, texture);

    if (gcmIS_ERROR(status))
    {
        gcmFATAL("%s(%d): Could not create default texture",
                 __FUNCTION__, __LINE__);
        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_NO();
        return;
    }

    gcmASSERT(texture != gcvNULL);
    gcmASSERT(texture->texture != gcvNULL);
    texture->fromImage = GL_TRUE;

    if (Target == GL_TEXTURE_2D)
    {
        /* Get closest supported destination. */
        if (gcmIS_ERROR(gcoTEXTURE_GetClosestFormat(context->hal,
                                                    format,
                                                    &dstFormat)))
        {
            gl2mERROR(GL_INVALID_ENUM);
            gcmFOOTER_NO();
            return;
        }
    }
    else
    {
        /**********************************************************************
        ** Validate the format.
        */
        switch (format)
        {
        case gcvSURF_YV12:
        case gcvSURF_I420:
        case gcvSURF_NV12:
        case gcvSURF_NV21:
            texture->direct.textureFormat = gcvSURF_YUY2;
			yuvFormat = gcvTRUE;
            break;

        case gcvSURF_YUY2:
            /* Fall through. */
        case gcvSURF_UYVY:
            texture->direct.textureFormat = format;
			yuvFormat = gcvTRUE;
            break;

        default:
            /* Get closest supported destination. */
            status = gcoTEXTURE_GetClosestFormat(context->hal,
                                                 format,
                                                 &dstFormat);

            if (status == gcvSTATUS_NOT_SUPPORTED)
            {
                gl2mERROR(GL_INVALID_ENUM);
                gcmFOOTER_NO();
                return;
            }
        }
    }

    texture->width  = width;
    texture->height = height;

    /* Get surface object from the eglImage. */
    surface = image->surface;

#if defined(ANDROID) && (ANDROID_SDK_VERSION >= 7)
    /* Android native buffer should always be linear. */
    if (image->type == KHR_IMAGE_ANDROID_NATIVE_BUFFER
    &&  yuvFormat == gcvFALSE
    )
    {
        gceSURF_TYPE type     = gcvSURF_TYPE_UNKNOWN;

        status = gcoSURF_GetFormat(surface, &type, gcvNULL);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("glEGLImageTargetTexture2DOES: "
                     "invalid eglImageKHR");
            gl2mERROR(GL_INVALID_OPERATION);
            gcmFOOTER_NO();
            return;
        }

        if (type != gcvSURF_TEXTURE)
        {
            gcoSURF mipmap = gcvNULL;

            /* Query tiling of the source surface.
             * The surface could be tile or linear. For tile surface, we only
             * need to add it to mipmap level 0. For linear surface, we need
             * create a texture object first and then do texture uploading
             * using resolve or software uploading from source to texture.
             *
             * For linear surface:
             * For EXTERNAL target, texture uploading is done when first time
             * used. Per spec, this type of texture can only be used for read,
             * and it needs to be uploaded only once.
             *
             * For 2D target, texture uploading is done every time when it is
             * read. */
            status = gcoTEXTURE_AddMipMap(texture->texture,
                                          level,
                                          dstFormat,
                                          width, height, 0,
                                          gcvFACE_NONE,
                                          gcvPOOL_DEFAULT,
                                          &mipmap);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("glEGLImageTargetTexture2DOES: "
                        "Could not create the mip map level");
                gl2mERROR(GL_OUT_OF_MEMORY);
                gcmFOOTER_NO();
                return;
            }

            /* TODO: Move texture uploading to a more proper place. */
            if (Target == GL_TEXTURE_2D)
            {
                status = gcoSURF_Resolve(surface, mipmap);

                if (gcmIS_ERROR(status))
                {
                    /* upload texture from a memory address. */
                    status = gcoTEXTURE_Upload(
                        texture->texture,
                        gcvFACE_NONE,
                        width, height, 0,
                        pixel, stride,
                        format);

                    if (gcmIS_ERROR(status))
                    {
                        gcmFATAL("%s: upload texture fail", __FUNCTION__);
                        gl2mERROR(GL_INVALID_VALUE);
                        gcmFOOTER_NO();
                        return;
                    }
                }
            }

            texture->source = surface;
        }
        else
        {
            /* Add mipmap from eglImage surface. Client 'owns' the surface. */
            status = gcoTEXTURE_AddMipMapFromClient(
                texture->texture, level, surface
                );

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s: could not add mipmap surface.", __FUNCTION__);
                gl2mERROR(GL_INVALID_VALUE);
                gcmFOOTER_NO();
                return;
            }
        }
    }
    else
#endif
    if (yuvFormat)
    {
        /* Get surface object from the eglImage. */
        texture->direct.source = surface;

        status = gcoSURF_Lock(texture->direct.source, gcvNULL, gcvNULL);
        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): Could not lock image surface",__FUNCTION__, __LINE__);
            gl2mERROR(GL_INVALID_VALUE);
            gcmFOOTER_NO();
            return;
        }

        /* Allocate the LOD surface. */
        status = gcoTEXTURE_AddMipMap(
                        texture->texture,
                        0,
                        texture->direct.textureFormat,
                        width,
                        height,
                        0, 0,
                        gcvPOOL_DEFAULT,
                        &texture->direct.texture[0]
                        );


        if (gcmIS_ERROR(status))
        {
            gcmFATAL("glEGLImageTargetTexture2DOES: "
                     "Could not add mipmap for direct texture");
            gl2mERROR(GL_INVALID_VALUE);
            gcmFOOTER_NO();
            return;
        }

        texture->direct.mipmapable = gcvFALSE;
        texture->direct.dirty      = gcvTRUE;
        texture->direct.maxLod     = 0;
        texture->direct.lodWidth   = width;
        texture->direct.lodHeight  = height;
    }
#ifdef __QNXNTO__
    else if (pixel != gcvNULL && image->type == KHR_IMAGE_PIXMAP)
    {
        gceSURF_TYPE type     = gcvSURF_TYPE_UNKNOWN;

        status = gcoSURF_GetFormat(surface, &type, gcvNULL);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("glEGLImageTargetTexture2DOES: "
                     "invalid eglImageKHR");
            gl2mERROR(GL_INVALID_OPERATION);
            gcmFOOTER_NO();
            return;
        }

        if (type != gcvSURF_TEXTURE)
        {
            gcoSURF mipmap = gcvNULL;

            /* Query tiling of the source surface.
             * The surface could be tile or linear. For tile surface, we only
             * need to add it to mipmap level 0. For linear surface, we need
             * create a texture object first and then do texture uploading
             * using resolve or software uploading from source to texture.
             *
             * For linear surface:
             * For EXTERNAL target, texture uploading is done when first time
             * used. Per spec, this type of texture can only be used for read,
             * and it needs to be uploaded only once.
             *
             * For 2D target, texture uploading is done every time when it is
             * read. */
            status = gcoTEXTURE_AddMipMap(texture->texture,
                                          level,
                                          dstFormat,
                                          width, height, 0,
                                          gcvFACE_NONE,
                                          gcvPOOL_DEFAULT,
                                          &mipmap);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("glEGLImageTargetTexture2DOES: "
                        "Could not create the mip map level");
                gl2mERROR(GL_OUT_OF_MEMORY);
                gcmFOOTER_NO();
                return;
            }

            /* TODO: Move texture uploading to a more proper place. */
            if (Target == GL_TEXTURE_2D)
            {
                status = gcoSURF_Resolve(surface, mipmap);

                if (gcmIS_ERROR(status))
                {
                    /* upload texture from a memory address. */
                    status = gcoTEXTURE_Upload(
                        texture->texture,
                        gcvFACE_NONE,
                        width, height, 0,
                        pixel, stride,
                        format);

                    if (gcmIS_ERROR(status))
                    {
                        gcmFATAL("%s: upload texture fail", __FUNCTION__);
                        gl2mERROR(GL_INVALID_VALUE);
                        gcmFOOTER_NO();
                        return;
                    }
                }
            }
       }
    }
#endif
    else
    {
        /* Add mipmap from eglImage surface. */
        status = gcoTEXTURE_AddMipMapFromSurface(
            texture->texture, level, surface
            );

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s: could not add mipmap from surface.", __FUNCTION__);
            gl2mERROR(GL_INVALID_VALUE);
            gcmFOOTER_NO();
            return;
        }

        /* Add reference count. */
        status = gcoSURF_ReferenceSurface(surface);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s: could not increase surface reference.", __FUNCTION__);
            gl2mERROR(GL_INVALID_VALUE);
            gcmFOOTER_NO();
            return;
        }
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

#ifdef EGL_API_XXX
    if (image->type == KHR_IMAGE_PIXMAP)
    {
        /* Check the seqNo to see whether it is dirty. If so, we need to sync the content to texture. */
        GLint extSeqNo;
        veglGetPixmapInfo(..., Image, &extSeqNo);    /* Get the seqNo from the pixmap: Please replace this with the correct egl call. */
        if (texture->seqNo != extSeqNo)
        {
            gcoSURF texSurf;
            texture->seqNo = extSeqNo;
            gcmONERROR(gcoTEXTURE_GetMipMap(texture->texture, 0, &texSurf));
            gcmONERROR(gcoSURF_Resolve(texSurf, texSurf));
        }
    }
#endif

    gcmFOOTER_NO();
#endif
}


GL_APICALL void GL_APIENTRY
glTexDirectVIVMap(
    GLenum Target,
    GLsizei Width,
    GLsizei Height,
    GLenum Format,
    GLvoid ** Logical,
    const GLuint * Physical
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceSTATUS status;
    gctBOOL powerOfTwo;
    gctBOOL scalerAvailable;
    gctBOOL tilerAvailable;
    gctBOOL yuy2AveragingAvailable;
    gctBOOL nonpotAvailable;
    gctBOOL packedSource;
    gctBOOL generateMipmap;
    gceSURF_FORMAT sourceFormat;
    gceSURF_FORMAT textureFormat;
    GLTexture  texture;
    gctUINT lodWidth;
    gctUINT lodHeight;
    gctINT maxLevel;
    gctBOOL needTemporary;
    gctINT i;
    gctSIZE_T textureWidth, textureHeight;
	gctINT32 tileWidth, tileHeight;

    gcmHEADER_ARG("Target=0x%04x Width=%d Height=%d Format=0x%04x Logical=0x%x Physical=0x%x",
                                  Target, Width, Height, Format, Logical, Physical);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    /***********************************************************************
     ** Validate parameters.
     */
    /* Validate the target. */
    if (Target != GL_TEXTURE_2D)
    {
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    if ((((*Logical) == gcvNULL) || (((gctUINT)(*Logical) & 0x3F) != 0)) && ((*Physical) == ~0U))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Simple parameter validation. */
    if ((Width < 1) || (Height < 1) ||
        (Width  > (GLsizei)context->maxTextureWidth) ||
        (Height > (GLsizei)context->maxTextureHeight)
       )
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

	gcoHAL_QueryTiled(context->hal,
                      gcvNULL, gcvNULL,
                      &tileWidth, &tileHeight);

	/* Currently hardware only supprot aligned Width and Height */
    if (Width & (tileWidth * 4 - 1))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if (Height & (tileHeight - 1))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Determine whether the texture is a power of two. */
    powerOfTwo
        =  ((Width & (Width - 1)) == 0)
        && ((Height & (Height - 1)) == 0);

    /***********************************************************************
     ** Get the bound texture.
     */


    /* Get a shortcut to the bound texture. */
    texture = context->texture2D[context->textureUnit];
    /* A texture has to be bound. */
    if (texture == gcvNULL)
    {
        texture = &context->default2D;
    }

    /***********************************************************************
     ** Query hardware support.
     */

    scalerAvailable = gcoHAL_IsFeatureAvailable(
        context->hal, gcvFEATURE_YUV420_SCALER
        ) == gcvSTATUS_TRUE;

    tilerAvailable = gcoHAL_IsFeatureAvailable(
        context->hal, gcvFEATURE_YUV420_TILER
        ) == gcvSTATUS_TRUE;

    yuy2AveragingAvailable = gcoHAL_IsFeatureAvailable(
        context->hal, gcvFEATURE_YUY2_AVERAGING
        ) == gcvSTATUS_TRUE;

    nonpotAvailable = gcoHAL_IsFeatureAvailable(
        context->hal, gcvFEATURE_NON_POWER_OF_TWO
        ) == gcvSTATUS_TRUE;


    /***********************************************************************
     ** Validate the format.
     */

    if (Format == GL_VIV_YV12)
    {
        sourceFormat = gcvSURF_YV12;
        textureFormat = gcvSURF_YUY2;
        packedSource = gcvFALSE;
    }
    else if (Format == GL_VIV_NV12)
    {
        sourceFormat = gcvSURF_NV12;
        textureFormat = gcvSURF_YUY2;
        packedSource = gcvFALSE;
    }
    else if (Format == GL_VIV_NV21)
    {
        sourceFormat = gcvSURF_NV21;
        textureFormat = gcvSURF_YUY2;
        packedSource = gcvFALSE;
    }
    else if (Format == GL_VIV_YUY2)
    {
        sourceFormat = gcvSURF_YUY2;
        textureFormat = gcvSURF_YUY2;
        packedSource = gcvTRUE;
    }
    else if (Format == GL_VIV_UYVY)
    {
        sourceFormat = gcvSURF_UYVY;
        textureFormat = gcvSURF_UYVY;
        packedSource = gcvTRUE;
    }
    else
    {
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }
    /***********************************************************************
     ** Check whether the source can be handled.
     */

    if (!packedSource && !scalerAvailable && !tilerAvailable)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /***********************************************************************
     ** Determine whether we are going to generate mipmaps.
     */
    if (powerOfTwo)
    {
        generateMipmap

            /* For planar source we need either the tiler or the scaler
               to take the 4:2:0 source in. */
            = (packedSource || tilerAvailable || scalerAvailable)

            /* We need YUY2 averaging to generate mipmaps. */
            && yuy2AveragingAvailable;
    }
    else
    {
        if (nonpotAvailable)
        {
            generateMipmap

                /* We need the scaler to take 4:2:0 data in and scale
                   to a closest power of 2. */
                = scalerAvailable

                /* We need YUY2 averaging to generate mipmaps. */
                && yuy2AveragingAvailable;
        }
        else
        {
            generateMipmap = gcvFALSE;
        }
    }

    /* Disable mipmap generating. */

    /* Set texture parameters. */
    textureWidth = Width;
    textureHeight = Height;

    /***********************************************************************
     ** Reset the bound texture.
     */

    /* Remove the existing texture. */
    status = _ResetTextureWrapper(context, texture);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    texture->direct.mipmapable = gcvFALSE;

    /***********************************************************************
     ** Construct new texture.
     */

    /* Construct texture object. */
    status = gcoTEXTURE_Construct(context->hal, &texture->texture);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_NO();
        return;
    }

    /* Invalidate normalized crop rectangle. */
    texture->dirtyCropRect = GL_TRUE;

    /* Reset the dirty flag. */
    texture->direct.dirty = gcvFALSE;

    /***********************************************************************
     ** Determine the maximum LOD.
     */
    /* Initialize the maximim LOD to zero. */
    maxLevel = 0;

    /* Are we generating a mipmap? */
    /* Start from level 0. */
    lodWidth  = textureWidth;
    lodHeight = textureHeight;

    if (generateMipmap)
    {
        while (gcvTRUE)
        {
            /* Determine the size of the next level. */
            gctUINT newWidth  = lodWidth / 2;
            gctUINT newHeight = lodHeight / 2;

            /* Limit the minimum width to 16 pixels; two reasons:
               1. Resolve currently does not support surfaces less then
                  16 pixels wide;
               2. For videos it probably isn't that important to have
                  LODs that small in size.
             */
            if (newWidth < 16)
            {
                break;
            }

            /* Is it the same as previous? */
            if ((newWidth == lodWidth) && (newHeight == lodHeight))
            {
                /* We are down to level 1x1. */
                break;
            }

            /* Assume the new size. */
            lodWidth  = newWidth;
            lodHeight = newHeight;

            /* Advance the number of levels of detail. */
            maxLevel++;
        }
    }

    /***********************************************************************
     ** Construct the source and temporary surfaces.
     */

    /* Determine whether we need the temporary surface
       (use scaler instead of tiler). */
    needTemporary =
        (
            /* Scaler needs the temp surface. */
            packedSource &&

              /* This case is for chips with no 4:2:0 tiler support. */
            (!tilerAvailable ||

              /* This case is for unaligned cases, where tiler cannot work. */
             (Width & 0x3)
            )
        );

    /* Construct the source surface. */
    status = gcoSURF_Construct(
        context->hal,
        Width, Height, 1,
        gcvSURF_BITMAP,
        sourceFormat,
        gcvPOOL_USER,
        &texture->direct.source
        );

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_NO();
        return;
    }

    /* Set the user buffer to the surface. */
    status = (gcoSURF_MapUserSurface(
        texture->direct.source,
        0,
        (GLvoid*)(*Logical),
        (gctUINT32)(*Physical)
        ));

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_NO();
        return;
    }

    /* Construct the temporary surface. */
    if (needTemporary)
    {
        status = gcoSURF_Construct(
            context->hal,
            Width,
            Height,
            1,
            gcvSURF_BITMAP,
            gcvSURF_A8R8G8B8,
            gcvPOOL_DEFAULT,
            &texture->direct.temp
            );
        if (gcmIS_ERROR(status))
        {
            gl2mERROR(GL_OUT_OF_MEMORY);
            gcmFOOTER_NO();
            return;
        }


        /* Overwrite the texture format. */
        textureFormat = gcvSURF_A8R8G8B8;
    }

    texture->direct.textureFormat = textureFormat;
    /***********************************************************************
     ** Clear the direct texture array.
     */

    /* Reset the array. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(
        texture->direct.texture, sizeof(gcoSURF) * 16
        ));

    /***********************************************************************
     ** Create the levels of detail.
     */

    /* Start from level 0. */
    lodWidth  = textureWidth;
    lodHeight = textureHeight;

    for (i = 0; i <= maxLevel; i++)
    {
        /* Allocate the LOD surface. */
        gcmERR_BREAK(gcoTEXTURE_AddMipMap(
            texture->texture,
            i,                      /* LOD #. */
            textureFormat,          /* Format. */
            lodWidth, lodHeight, 0, 0,
            gcvPOOL_DEFAULT,
            &texture->direct.texture[i]
            ));

        /* Determine the size of the next level. */
        lodWidth  = lodWidth / 2;
        lodHeight = lodHeight / 2;
		if ((lodWidth == 0) || (lodHeight == 0))
			break;
    }

    texture->direct.maxLod = maxLevel;
    texture->direct.lodWidth = textureWidth;
    texture->direct.lodHeight = textureHeight;

    /* Roll back if error has occurred. */
    if (gcmIS_ERROR(status))
    {
        /* Remove the existing texture. */
        gcmVERIFY_OK(_ResetTextureWrapper(context, texture));

        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_NO();
        return;
    }

    gcmFOOTER_NO();
#endif
}


/*******************************************************************************
**
**  glTexDirectVIV
**
**  glTexDirectVIV function creates a texture with direct access support. This
**  is useful when the application desires to use the same texture over and over
**  while frequently updating its content. This could be used for mapping live
**  video to a texture. Video decoder could write its result directly to the
**  texture and then the texture could be directly rendered onto a 3D shape.
**
**  If the function succeedes, it returns a pointer, or, for some YUV formats,
**  a set of pointers that directly point to the texture. The pointer(s) will
**  be returned in the user-allocated array pointed to by Pixels parameter.
**
**  The width and height of LOD 0 of the texture is specified by the Width
**  and Height parameters. The driver may auto-generate the rest of LODs if
**  the hardware supports high quality scalling (for non-power of 2 textures)
**  and LOD generation. If the hardware does not support high quality scaing
**  and LOD generation, the texture will remain as a single-LOD texture.
**
**  Format parameter supports the following formats: GL_VIV_YV12, GL_VIV_NV12,
**  GL_VIV_YUY2 and GL_VIV_UYVY.
**
**  If Format is GL_VIV_YV12, glTexDirectVIV creates a planar YV12 4:2:0 texture
**  and the format of Pixels array is as follows: (Yplane, Vplane, Uplane).
**
**  If Format is GL_VIV_NV12, glTexDirectVIV creates a planar NV12 4:2:0 texture
**  and the format of Pixels array is as follows: (Yplane, UVplane).
**
**  If Format is GL_VIV_NV21, glTexDirectVIV creates a planar NV21 4:2:0 texture
**  and the format of Pixels array is as follows: (Yplane, VUplane).
**
**  If Format is GL_VIV_YUY2 or GL_VIV_UYVY or, glTexDirectVIV creates a packed
**  4:2:2 texture and the Pixels array contains only one pointer to the packed
**  YUV texture.
**
**  ERRORS:
**
**      GL_INVALID_ENUM
**          - if Target is not GL_TEXTURE_2D;
**          - if Format is not a valid format.
**
**      GL_INVALID_VALUE
**          - if Width or Height parameters are less than 1.
**
**      GL_OUT_OF_MEMORY
**          - if a memory allocation error occures at any point.
**
**      GL_INVALID_OPERATION
**          - if the specified format is not supported by the hardware;
**          - if no texture is bound to the active texture unit;
**          - if any other error occures during the call.
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D.
**
**      Width
**      Height
**          Specifies the size of LOD 0.
**
**      Format
**          One of the following: GL_VIV_YV12, GL_VIV_NV12, GL_VIV_NV21,
**          GL_VIV_YUY2, GL_VIV_UYVY.
**
**      Pixels
**          Pointer to the application-defined array of pointers to the
**          texture created by glTexDirectVIV. There should be enough space
**          allocated to hold as many pointers as needed for the specified
**          format. The count of pointers are: GL_VIV_YV12(3 pointers),
**          GL_VIV_NV12 and GL_VIV_NV21(2 pointers), Others(1 pointer).
**
**  OUTPUT:
**
**      Pixels array initialized with texture pointers.
*/
GL_APICALL void GL_APIENTRY
glTexDirectVIV(
    GLenum Target,
    GLsizei Width,
    GLsizei Height,
    GLenum Format,
    GLvoid ** Pixels
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceSTATUS status;
    /*gctBOOL powerOfTwo;*/
    gctBOOL scalerAvailable;
    gctBOOL tilerAvailable;
    /*gctBOOL yuy2AveragingAvailable;*/
    gctBOOL packedSource;
    gceSURF_FORMAT sourceFormat;
    gceSURF_FORMAT textureFormat;
    GLTexture  texture;
    gctUINT lodWidth;
    gctUINT lodHeight;
    gctINT maxLevel;
    gctBOOL needTemporary;
    gctINT i;
    gctSIZE_T textureWidth, textureHeight;

    gcmHEADER_ARG("Target=0x%04x Width=%d Height=%d Format=0x%04x Pixels=0x%x",
                                  Target, Width, Height, Format, Pixels);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    /***********************************************************************
     ** Validate parameters.
     */
    /* Validate the target. */
    if (Target != GL_TEXTURE_2D)
    {
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    /* Simple parameter validation. */
    if ((Width < 1) || (Height < 1) || (Pixels == gcvNULL) ||
        (Width  > (GLsizei)context->maxTextureWidth) ||
        (Height > (GLsizei)context->maxTextureHeight)
       )
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }


    /***********************************************************************
     ** Get the bound texture.
     */


    /* Get a shortcut to the bound texture. */
    texture = context->texture2D[context->textureUnit];

    /* A texture has to be bound. */
    if (texture == gcvNULL)
    {
        texture = &context->default2D;
    }

    /***********************************************************************
     ** Query hardware support.
     */

    scalerAvailable = gcoHAL_IsFeatureAvailable(
        context->hal, gcvFEATURE_YUV420_SCALER
        ) == gcvSTATUS_TRUE;

    tilerAvailable = gcoHAL_IsFeatureAvailable(
        context->hal, gcvFEATURE_YUV420_TILER
        ) == gcvSTATUS_TRUE;

    /*yuy2AveragingAvailable = gcoHAL_IsFeatureAvailable(
        context->hal, gcvFEATURE_YUY2_AVERAGING
        ) == gcvSTATUS_TRUE;*/

    /***********************************************************************
     ** Validate the format.
     */

    if (Format == GL_VIV_YV12)
    {
        sourceFormat = gcvSURF_YV12;
        textureFormat = gcvSURF_YUY2;
        packedSource = gcvFALSE;
    }
    else if (Format == GL_VIV_NV12)
    {
        sourceFormat = gcvSURF_NV12;
        textureFormat = gcvSURF_YUY2;
        packedSource = gcvFALSE;
    }
    else if (Format == GL_VIV_NV21)
    {
        sourceFormat = gcvSURF_NV21;
        textureFormat = gcvSURF_YUY2;
        packedSource = gcvFALSE;
    }
    else if (Format == GL_VIV_YUY2)
    {
        sourceFormat = gcvSURF_YUY2;
        textureFormat = gcvSURF_YUY2;
        packedSource = gcvTRUE;
    }
    else if (Format == GL_VIV_UYVY)
    {
        sourceFormat = gcvSURF_UYVY;
        textureFormat = gcvSURF_UYVY;
        packedSource = gcvTRUE;
    }
    else
    {
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }
    /***********************************************************************
     ** Check whether the source can be handled.
     */

    if (!packedSource && !scalerAvailable && !tilerAvailable)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /***********************************************************************
     ** Determine whether we are going to generate mipmaps.
     */

    /* Disable mipmap generating. */

    /* Set texture parameters. */
    textureWidth = Width;
    textureHeight = Height;

    /***********************************************************************
     ** Reset the bound texture.
     */

    /* Remove the existing texture. */
    status = _ResetTextureWrapper(context, texture);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    texture->direct.mipmapable = gcvFALSE;

    /***********************************************************************
     ** Construct new texture.
     */

    /* Construct texture object. */
    status = gcoTEXTURE_Construct(context->hal, &texture->texture);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_NO();
        return;
    }

    /* Invalidate normalized crop rectangle. */
    texture->dirtyCropRect = GL_TRUE;

    /* Reset the dirty flag. */
    texture->direct.dirty = gcvFALSE;

    /***********************************************************************
     ** Determine the maximum LOD.
     */
    /* Initialize the maximim LOD to zero. */
    maxLevel = 0;

    /* Are we generating a mipmap? */
    /* Start from level 0. */
    lodWidth  = textureWidth;
    lodHeight = textureHeight;

    while (gcvTRUE)
    {
        /* Determine the size of the next level. */
        gctUINT newWidth  = lodWidth / 2;
        gctUINT newHeight = lodHeight / 2;

        /* Limit the minimum width to 16 pixels; two reasons:
           1. Resolve currently does not support surfaces less then
              16 pixels wide;
           2. For videos it probably isn't that important to have
              LODs that small in size.
         */
        if (newWidth < 16)
        {
            break;
        }

        /* Is it the same as previous? */
        if ((newWidth == lodWidth) && (newHeight == lodHeight))
        {
            /* We are down to level 1x1. */
            break;
        }

        /* Assume the new size. */
        lodWidth  = newWidth;
        lodHeight = newHeight;

        /* Advance the number of levels of detail. */
        maxLevel++;
    }

    /***********************************************************************
     ** Construct the source and temporary surfaces.
     */

    /* Determine whether we need the temporary surface
       (use scaler instead of tiler). */
    needTemporary =
        (
            /* Scaler needs the temp surface. */
            packedSource &&

              /* This case is for chips with no 4:2:0 tiler support. */
            (!tilerAvailable ||

              /* This case is for unaligned cases, where tiler cannot work. */
             (Width & 0x3)
            )
        );

    /* Construct the source surface. */
    status = gcoSURF_Construct(
        context->hal,
        Width, Height, 1,
        gcvSURF_BITMAP,
        sourceFormat,
        gcvPOOL_SYSTEM,
        &texture->direct.source
        );

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_NO();
        return;
    }

    /* Lock the source surface. */
    status = gcoSURF_Lock(
        texture->direct.source,
        gcvNULL,
        Pixels
        );

    /* Construct the temporary surface. */
    if (needTemporary)
    {
        status = gcoSURF_Construct(
            context->hal,
            Width,
            Height,
            1,
            gcvSURF_BITMAP,
            gcvSURF_A8R8G8B8,
            gcvPOOL_DEFAULT,
            &texture->direct.temp
            );
        if (gcmIS_ERROR(status))
        {
            gl2mERROR(GL_OUT_OF_MEMORY);
            gcmFOOTER_NO();
            return;
        }


        /* Overwrite the texture format. */
        textureFormat = gcvSURF_A8R8G8B8;
    }

    texture->direct.textureFormat = textureFormat;
    /***********************************************************************
     ** Clear the direct texture array.
     */

    /* Reset the array. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(
        texture->direct.texture, sizeof(gcoSURF) * 16
        ));

    /***********************************************************************
     ** Create the levels of detail.
     */

    /* Start from level 0. */
    lodWidth  = textureWidth;
    lodHeight = textureHeight;
    for (i = 0; i <= maxLevel; i++)
    {
        /* Allocate the LOD surface. */
        gcmERR_BREAK(gcoTEXTURE_AddMipMap(
            texture->texture,
            i,                      /* LOD #. */
            textureFormat,          /* Format. */
            lodWidth, lodHeight, 0, 0,
            gcvPOOL_DEFAULT,
            &texture->direct.texture[i]
            ));

        /* Determine the size of the next level. */
        lodWidth  = lodWidth / 2;
        lodHeight = lodHeight / 2;
		if ((lodWidth == 0) || (lodHeight == 0))
			break;
    }
    texture->direct.maxLod    = maxLevel;
    texture->direct.lodWidth  = textureWidth;
    texture->direct.lodHeight = textureHeight;

    /* Roll back if error has occurred. */
    if (gcmIS_ERROR(status))
    {
        /* Remove the existing texture. */
        gcmVERIFY_OK(_ResetTextureWrapper(context, texture));

        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_NO();
        return;
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glTexDirectInvalidateVIV(
    GLenum Target
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture texture;
        GLContext context;

        gcmHEADER_ARG("Target=0x%04x", Target);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    /* Validate arguments. */
    if (Target != GL_TEXTURE_2D)
    {
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }


    switch (Target)
    {
    case GL_TEXTURE_2D:
        texture = context->texture2D[context->textureUnit];
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        texture = context->textureCube[context->textureUnit];
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        texture = context->textureCube[context->textureUnit];
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        texture = context->textureCube[context->textureUnit];
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        texture = context->textureCube[context->textureUnit];
        break;

    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        texture = context->textureCube[context->textureUnit];
        break;

    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        texture = context->textureCube[context->textureUnit];
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X", __FUNCTION__, __LINE__, Target);
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    /* See if this is the default texture. */
    if (texture == gcvNULL)
    {
        switch (Target)
        {
        case GL_TEXTURE_2D:
            texture = &context->default2D;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            texture = &context->defaultCube;
            break;

        default:
            gcmFATAL("%s(%d): Invalid target: %04X", __FUNCTION__, __LINE__, Target);
            gl2mERROR(GL_INVALID_ENUM);
            gcmFOOTER_NO();
            return;
        }
    }

    /* A texture has to be bound. */
    if (texture == gcvNULL)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Has to be a planar-sourced texture. */
    if (texture->direct.source == gcvNULL)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Mark texture as dirty to be flushed later. */
    texture->dirty = gcvTRUE;

    /* Set the quick-reference dirty flag. */
    texture->direct.dirty = gcvTRUE;

    gcmFOOTER_NO();
#endif
}

