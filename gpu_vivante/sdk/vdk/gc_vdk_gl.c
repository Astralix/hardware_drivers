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




#define _CRT_SECURE_NO_WARNINGS

#include "gc_vdk.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef UNDER_CE
#include <windows.h>
#endif

#ifndef GL_GENERATE_MIPMAP
# define GL_GENERATE_MIPMAP 0x8191
#endif

typedef void (GL_APIENTRY *
GL_GEN_TEXTURES)(
    GLsizei n,
    GLuint * textures
    );

typedef GLenum (GL_APIENTRY *
GL_GET_ERROR)(
    void
    );

typedef void (GL_APIENTRY *
GL_GET_INTEGERV)(
    GLenum pname,
    GLint * params
    );

typedef void (GL_APIENTRY *
GL_TEX_IMAGE_2D)(
    GLenum target,
    GLint level,
    GLint internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLenum format,
    GLenum type,
    const GLvoid *pixels
    );

typedef void (GL_APIENTRY *
GL_COMPRESSED_TEX_IMAGE_2D)(
    GLenum target,
    GLint level,
    GLenum internalformat,
    GLsizei width,
    GLsizei height,
    GLint border,
    GLsizei imageSize,
    const void *data
    );

typedef void (GL_APIENTRY *
GL_PIXEL_STOREI)(
    GLenum pname,
    GLint param
    );

typedef void (GL_APIENTRY *
GL_TEX_PARAMETERI)(
    GLenum target,
    GLenum pname,
    GLint param
    );

typedef void (GL_APIENTRY *
GL_DELETE_TEXTURES)(
    GLsizei n,
    const GLuint *textures
    );

typedef void (GL_APIENTRY *
GL_BIND_TEXTURE)(
    GLenum target,
    GLuint texture
    );

typedef void (GL_APIENTRY *
GL_GENERATE_MIPMAP_OES)(
    GLenum target
    );

typedef GLuint (GL_APIENTRY *
GL_CREATE_SHADER)(
    GLenum type
    );

typedef void (GL_APIENTRY *
GL_DELETE_SHADER)(
    GLuint shader
    );

typedef GLuint (GL_APIENTRY *
GL_CREATE_PROGRAM)(
    void
    );

typedef void (GL_APIENTRY *
GL_DELETE_PROGRAM)(
    GLuint program
    );

typedef void (GL_APIENTRY *
GL_ATTACH_SHADER)(
    GLuint program,
    GLuint shader
    );

typedef void (GL_APIENTRY *
GL_GET_PROGRAMIV)(
    GLuint program,
    GLenum pname,
    GLint *params
    );

typedef void (GL_APIENTRY *
GL_GET_PROGRAM_INFO_LOG)(
    GLuint program,
    GLsizei bufsize,
    GLsizei *length,
    char* infolog
    );

typedef void (GL_APIENTRY *
GL_GET_SHADERIV)(
    GLuint shader,
    GLenum pname,
    GLint* params
    );

typedef void (GL_APIENTRY *
GL_GET_SHADER_INFO_LOG)(
    GLuint shader,
    GLsizei bufsize,
    GLsizei* length,
    char* infolog
    );

typedef void (GL_APIENTRY *
GL_LINK_PROGRAM)(
    GLuint program
    );

typedef void (GL_APIENTRY *
GL_COMPILE_SHADER)(
    GLuint shader
    );

typedef void (GL_APIENTRY *
GL_SHADER_SOURCE)(
    GLuint shader,
    GLsizei count,
    const char** string,
    const GLint* length
    );

#pragma pack(1)
struct TGA_HEADER
{
    unsigned char IDLength;
    unsigned char ColorMapType;
    unsigned char ImageType;
    unsigned char FirstEntryIndexLow;
    unsigned char FirstEntryIndexHigh;
    unsigned char ColorMapLengthLow;
    unsigned char ColorMapLengthHigh;
    unsigned char ColorMapEntrySize;
    unsigned char XOriginOfImageLow;
    unsigned char XOriginOfImageHigh;
    unsigned char YOriginOfImageLow;
    unsigned char YOriginOfImageHigh;
    unsigned char ImageWidthLow;
    unsigned char ImageWidthHigh;
    unsigned char ImageHeightLow;
    unsigned char ImageHeightHigh;
    unsigned char PixelDepth;
    unsigned char ImageDescriptor;
};
#pragma pack()

void *
vdkLoadTGA(
    FILE * File,
    GLenum * Format,
    GLsizei * Width,
    GLsizei * Height
    )
{
    struct TGA_HEADER tga;
    size_t bytes;
    unsigned char * bits;
    unsigned short imageWidth, imageHeight;

    /* Read the TGA file header. */
    if (fread(&tga, sizeof(tga), 1, File) != 1)
    {
        return NULL;
    }

    /* For now we only support uncompressed true-color images. */
    if (tga.ImageType != 2)
    {
        return NULL;
    }

    /* We only support top-left and bottom-left images. */
    if (tga.ImageDescriptor & 0x10)
    {
        return NULL;
    }

    /* Check pixel depth. */
    switch (tga.PixelDepth)
    {
    case 16:
        /* 16-bpp RGB. */
        *Format = GL_UNSIGNED_SHORT_5_6_5;
        break;

    case 24:
        /* 24-bpp RGB. */
        *Format = GL_RGB;
        break;

    case 32:
        /* 32-bpp RGB. */
        *Format = GL_RGBA;
        break;

    default:
        /* Invalid pixel depth. */
        return NULL;
    }

    imageWidth = ((unsigned short)tga.ImageWidthHigh << 8) + (unsigned short)tga.ImageWidthLow;
    imageHeight = ((unsigned short)tga.ImageHeightHigh << 8) + (unsigned short)tga.ImageHeightLow;

    /* Return texture dimension. */
    *Width  = imageWidth;
    *Height = imageHeight;

    /* Compute number of bytes. */
    bytes = imageWidth * imageHeight * tga.PixelDepth / 8;

    /* Skip ID field. */
    if (tga.IDLength)
    {
        fseek(File, tga.IDLength, SEEK_SET);
    }

    /* Allocate the bits. */
    bits = (unsigned char *) malloc(bytes);
	/* Initialize the bits */
	memset(bits, 0, bytes);

    if (bits != NULL)
    {
        if (tga.ImageDescriptor & 0x20)
        {
            /* Read the bits from the TGA file. */
            if (fread(bits, 1, bytes, File) != bytes)
            {
                /* Error reading bits. */
                free(bits);
                bits = NULL;
            }
        }
        else
        {
            GLsizei y;
            GLsizei stride = bytes / imageHeight;

            /* Bottom up - copy line by line. */
            for (y = *Height - 1; y >= 0; --y)
            {
                if ((GLsizei) fread(bits + y * stride, 1, stride, File) != stride)
                {
                    /* Error reading bits. */
                    free(bits);
                    bits = NULL;
                    break;
                }
            }
        }

        if (bits != NULL)
        {
            size_t i;

            /* Convert all RGB pixels into GL pixels. */
            for (i = 0; i < bytes; i += tga.PixelDepth / 8)
            {
                unsigned char save;

                switch (tga.PixelDepth)
                {
                case 16:
                    /* Swap red and blue channel in 16-bpp. */
                    save        = bits[i + 0] & 0x1F;
                    bits[i + 0] = (bits[i + 0] & ~0x1F) | (bits[1] >> 3);
                    bits[i + 1] = (bits[i + 1] & ~0xF8) | (save << 3);
                    break;

                case 24:
                case 32:
                    /* Swap red and blue channel in 24-bpp or 32-bpp. */
                    save        = bits[i + 0];
                    bits[i + 0] = bits[i + 2];
                    bits[i + 2] = save;
                    break;
                }
            }
        }
    }

    /* Return the bits. */
    return bits;
}

#pragma pack(1)
struct PKM_HEADER
{
    unsigned char Magic[4];
    unsigned char Version[2];
    unsigned char Type[2];
    unsigned char Width[2];
    unsigned char Height[2];
    unsigned char ActiveWidth[2];
    unsigned char ActiveHeight[2];
};
#pragma pack()

void *
vdkLoadPKM(
    FILE * File,
    GLenum * Format,
    GLsizei * Width,
    GLsizei * Height,
    GLsizei * Bytes
    )
{
    struct PKM_HEADER pkm;
    size_t bytes;
    unsigned char * bits;
    unsigned short type;
    unsigned short width, height;
    unsigned short activeWidth, activeHeight;

    /* Read the PKM file header. */
    if (fread(&pkm, sizeof(pkm), 1, File) != 1)
    {
        return NULL;
    }

    /* Validate magic data. */
    if ((pkm.Magic[0] != 'P')
    ||  (pkm.Magic[1] != 'K')
    ||  (pkm.Magic[2] != 'M')
    ||  (pkm.Magic[3] != ' ')
    )
    {
        return NULL;
    }

    /* Convert from big endian to numbers. */
    type         = (pkm.Type[0]         << 8) | pkm.Type[1];
    width        = (pkm.Width[0]        << 8) | pkm.Width[1];
    height       = (pkm.Height[0]       << 8) | pkm.Height[1];
    activeWidth  = (pkm.ActiveWidth[0]  << 8) | pkm.ActiveWidth[1];
    activeHeight = (pkm.ActiveHeight[0] << 8) | pkm.ActiveHeight[1];

    /* For now we only support ETC1_RGB_NO_MIPMAPS. */
    if (type != 0)
    {
        return NULL;
    }

    /* ETC1 RGB texture format. */
    *Format = GL_ETC1_RGB8_OES;

    /* Return texture dimension. */
    *Width  = activeWidth;
    *Height = activeHeight;

    /* Compute number of bytes. */
    bytes = ((width + 3)/ 4) * ((height + 3) / 4) * 8;

    *Bytes = bytes;

    /* Allocate the bits. */
    bits = (unsigned char *) malloc(bytes);
	/* Initialize the bits */
	memset(bits, 0, bytes);

    if (bits != NULL)
    {
        /* Read the bits from the PKM file. */
        if (fread(bits, 1, bytes, File) != bytes)
        {
            /* Error reading bits. */
            free(bits);
            bits = NULL;
        }
    }

    /* Return the bits. */
    return bits;
}

#define GET_FUNCTION(type, var, name, error) \
    do \
    { \
        if (var == NULL) \
        { \
            var = (type) Egl->eglGetProcAddress(name); \
            if ((var == NULL) && error) \
            { \
                return 0; \
            } \
        } \
    } \
    while (0)

VDKAPI unsigned int VDKLANG
vdkLoadTexture(
    vdkEGL * Egl,
    const char * FileName,
    vdkTextureType Type,
    vdkTextureFace Face
    )
{
    GLuint texture = 0;
    FILE * f;

    static GL_GEN_TEXTURES            glGenTextures          = NULL;
    static GL_TEX_IMAGE_2D            glTexImage2D           = NULL;
    static GL_PIXEL_STOREI            glPixelStorei          = NULL;
    static GL_TEX_PARAMETERI          glTexParameteri        = NULL;
    static GL_DELETE_TEXTURES         glDeleteTextures       = NULL;
    static GL_BIND_TEXTURE            glBindTexture          = NULL;
    static GL_GET_ERROR               glGetError             = NULL;
    static GL_GET_INTEGERV            glGetIntegerv          = NULL;
    static GL_GENERATE_MIPMAP_OES     glGenerateMipmap       = NULL;
    static GL_GENERATE_MIPMAP_OES     glGenerateMipmapOES    = NULL;
    static GL_COMPRESSED_TEX_IMAGE_2D glCompressedTexImage2D = NULL;

    GET_FUNCTION(GL_GEN_TEXTURES,
                 glGenTextures,
                 "glGenTextures",
                 1);
    GET_FUNCTION(GL_TEX_IMAGE_2D,
                 glTexImage2D,
                 "glTexImage2D",
                 1);
    GET_FUNCTION(GL_COMPRESSED_TEX_IMAGE_2D,
                 glCompressedTexImage2D,
                 "glCompressedTexImage2D",
                 1);
    GET_FUNCTION(GL_PIXEL_STOREI,
                 glPixelStorei,
                 "glPixelStorei",
                 1);
    GET_FUNCTION(GL_TEX_PARAMETERI,
                 glTexParameteri,
                 "glTexParameteri",
                 1);
    GET_FUNCTION(GL_DELETE_TEXTURES,
                 glDeleteTextures,
                 "glDeleteTextures",
                 1);
    GET_FUNCTION(GL_BIND_TEXTURE,
                 glBindTexture,
                 "glBindTexture",
                 1);
    GET_FUNCTION(GL_GET_ERROR,
                 glGetError,
                 "glGetError",
                 1);
    GET_FUNCTION(GL_GET_INTEGERV,
                 glGetIntegerv,
                 "glGetIntegerv",
                 1);
    GET_FUNCTION(GL_GENERATE_MIPMAP_OES,
                 glGenerateMipmap,
                 "glGenerateMipmap",
                 0);

    if (glGenerateMipmap == NULL)
    {
        /* Get optional OES 1.1 function name. */
        glGenerateMipmap = (GL_GENERATE_MIPMAP_OES)
            Egl->eglGetProcAddress("glGenerateMipmapOES");
    }

    /* Combine function pointers. */
    if (glGenerateMipmap == NULL)
    {
        glGenerateMipmap = glGenerateMipmapOES;
    }

    /* Open the file. */
    f = fopen(FileName, "rb");
#ifdef UNDER_CE
    if (f == NULL)
    {
        wchar_t moduleName[MAX_PATH];
        char path[MAX_PATH], * p;
        GetModuleFileName(NULL, moduleName, MAX_PATH);
        wcstombs(path, moduleName, MAX_PATH);
        p = strrchr(path, '\\');
        strcpy(p + 1, FileName);
        f = fopen(path, "rb");
    }
#endif
    if (f == NULL)
    {
        /* Error opening the file. */
        return 0;
    }

    do
    {
        GLenum format, target = 0;
        void * bits;
        GLsizei width, height, bytes;

        if ((Face == VDK_2D) || (Face == VDK_POSITIVE_X))
        {
            /* Generate a texture. */
            glGenTextures(1, &texture);
        }
        else
        {
            /* Get currently bound cubic map texture. */
            glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, (GLint *) &texture);
        }

        if (glGetError() != GL_NO_ERROR)
        {
            /* Error generating texture. */
            break;
        }

        /* Bind the texture. */
        switch (Face)
        {
        case VDK_2D:
            glBindTexture(target = GL_TEXTURE_2D, texture);
            break;

        case VDK_POSITIVE_X:
            glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
            break;

        case VDK_NEGATIVE_X:
            target = GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
            break;

        case VDK_POSITIVE_Y:
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
            break;

        case VDK_NEGATIVE_Y:
            target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
            break;

        case VDK_POSITIVE_Z:
            target = GL_TEXTURE_CUBE_MAP_POSITIVE_Z;
            break;

        case VDK_NEGATIVE_Z:
            target = GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
            break;
        }

        /* Test for any error. */
        if (glGetError() != GL_NO_ERROR)
        {
            glDeleteTextures(1, &texture);
            texture = 0;
            break;
        }

        /* Dispatch on texture type. */
        switch (Type)
        {
        case VDK_TGA: /* TGA file. */
            bits = vdkLoadTGA(f, &format, &width, &height);

            if (bits == NULL)
            {
                glDeleteTextures(1, &texture);
                texture = 0;
                break;
            }

            /* Set auto-mipmap flag if glGenerateMipmap functions are not     *\
            \* there.                                                         */
            if ((glGenerateMipmap == NULL) && (glGenerateMipmapOES == NULL))
            {
                glTexParameteri((Face == VDK_2D) ? GL_TEXTURE_2D
                                                 : GL_TEXTURE_CUBE_MAP,
                                GL_GENERATE_MIPMAP,
                                GL_TRUE);
            }

            /* Set unpack alignment. */
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            /* Load texture. */
            glTexImage2D(target,
                         0,
                         format,
                         width,
                         height,
                         0,
                         format,
                         GL_UNSIGNED_BYTE,
                         bits);

            if ((glGenerateMipmap != NULL)
            &&  ((Face == VDK_2D) || (Face == VDK_NEGATIVE_Z))
            )
            {
                /* Generate mipmaps. */
                glGenerateMipmap((Face == VDK_2D) ? GL_TEXTURE_2D
                                                  : GL_TEXTURE_CUBE_MAP);
            }

            /* Free the bits. */
            free(bits);
            break;

        case VDK_PKM: /* PKM file. */
            bits = vdkLoadPKM(f, &format, &width, &height, &bytes);

            if (bits == NULL)
            {
                glDeleteTextures(1, &texture);
                texture = 0;
                break;
            }

            /* Set unpack alignment. */
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            /* Load texture. */
            glCompressedTexImage2D(target,
                                   0,
                                   format,
                                   width,
                                   height,
                                   0,
                                   bytes,
                                   bits);

            /* Free the bits. */
            free(bits);
            break;

        default:
            /* Unknown texture type. */
            break;
        }
    }
    while (0);

    /* Close the file. */
    fclose(f);

    /* Return the texture. */
    return texture;
}

unsigned int
vdkCompileShader(
    vdkEGL * Egl,
    const char * Shader,
    GLenum Type,
    char ** Log
    )
{
    FILE  *in       = NULL;
    GLuint shader   = 0;
    char  *source   = NULL;
    GLint  compiled = GL_FALSE;
    GLint  length;

    static GL_CREATE_SHADER       glCreateShader     = NULL;
    static GL_SHADER_SOURCE       glShaderSource     = NULL;
    static GL_COMPILE_SHADER      glCompileShader    = NULL;
    static GL_GET_ERROR           glGetError         = NULL;
    static GL_GET_SHADERIV        glGetShaderiv      = NULL;
    static GL_GET_SHADER_INFO_LOG glGetShaderInfoLog = NULL;
    static GL_DELETE_SHADER       glDeleteShader     = NULL;

    GET_FUNCTION(GL_CREATE_SHADER,
                 glCreateShader,
                 "glCreateShader",
                 1);
    GET_FUNCTION(GL_SHADER_SOURCE,
                 glShaderSource,
                 "glShaderSource",
                 1);
    GET_FUNCTION(GL_COMPILE_SHADER,
                 glCompileShader,
                 "glCompileShader",
                 1);
    GET_FUNCTION(GL_GET_ERROR,
                 glGetError,
                 "glGetError",
                 1);
    GET_FUNCTION(GL_GET_SHADERIV,
                 glGetShaderiv,
                 "glGetShaderiv",
                 1);
    GET_FUNCTION(GL_GET_SHADER_INFO_LOG,
                 glGetShaderInfoLog,
                 "glGetShaderInfoLog",
                 1);
    GET_FUNCTION(GL_DELETE_SHADER,
                 glDeleteShader,
                 "glDeleteShader",
                 1);

    shader = glCreateShader(Type);
    if (shader == 0)
    {
        goto error;
    }

    in = fopen(Shader, "rb");
#ifdef UNDER_CE
    if (in == NULL)
    {
        wchar_t moduleName[MAX_PATH];
        char path[MAX_PATH], * p;
        GetModuleFileName(NULL, moduleName, MAX_PATH);
        wcstombs(path, moduleName, MAX_PATH);
        p = strrchr(path, '\\');
        strcpy(p + 1, Shader);
        in = fopen(path, "rb");
    }
#endif
    if (in == NULL)
    {
        length = strlen(Shader);
        source = malloc(length + 1);
        if (source != NULL)
        {
            strcpy(source, Shader);
        }
    }
    else
    {
        int ret;
        if(fseek(in, 0, SEEK_END))
		{
			goto error;
		}
        length = ftell(in);
        if(fseek(in, 0, SEEK_SET))
		{
			goto error;
		}

        if (length < 0)
        {
            goto error;
        }

        source = (char *) malloc(length + 1);
        if (source == NULL)
        {
            goto error;
        }

        ret = fread(source, length, 1, in);
        assert(ret);
        source[length] = '\0';
    }

    glShaderSource(shader, 1, (const char **) &source, &length);
    glCompileShader(shader);

    if (glGetError() != GL_NO_ERROR)
    {
        goto error;
    }

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        if (Log != NULL)
        {
            GLint length;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

            *Log = (char *) malloc(length + 1);
            if (*Log != NULL)
            {
                glGetShaderInfoLog(shader, length, &length, *Log);
                (*Log)[length] = '\0';
            }
        }

        goto error;
    }

    free(source);
    if (in != NULL)
    {
        fclose(in);
    }

    return shader;

error:
    if (source != NULL)
    {
        free(source);
    }

    if (in != NULL)
    {
        fclose(in);
    }

    if (shader != 0)
    {
        glDeleteShader(shader);
    }

    return 0;
}

VDKAPI unsigned int VDKLANG
vdkMakeProgram(
    vdkEGL * Egl,
    const char * VertexShader,
    const char * FragmentShader,
    char ** Log
    )
{
    GLuint vertexShader   = 0;
    GLuint fragmentShader = 0;
    GLuint program        = 0;
    GLint  linked         = GL_FALSE;

    static GL_CREATE_PROGRAM       glCreateProgram     = NULL;
    static GL_ATTACH_SHADER        glAttachShader      = NULL;
    static GL_LINK_PROGRAM         glLinkProgram       = NULL;
    static GL_GET_ERROR            glGetError          = NULL;
    static GL_GET_PROGRAMIV        glGetProgramiv      = NULL;
    static GL_GET_PROGRAM_INFO_LOG glGetProgramInfoLog = NULL;
    static GL_DELETE_PROGRAM       glDeleteProgram     = NULL;
    static GL_DELETE_SHADER        glDeleteShader      = NULL;

    GET_FUNCTION(GL_CREATE_PROGRAM,
                 glCreateProgram,
                 "glCreateProgram",
                 1);
    GET_FUNCTION(GL_ATTACH_SHADER,
                 glAttachShader,
                 "glAttachShader",
                 1);
    GET_FUNCTION(GL_LINK_PROGRAM,
                 glLinkProgram,
                 "glLinkProgram",
                 1);
    GET_FUNCTION(GL_GET_ERROR,
                 glGetError,
                 "glGetError",
                 1);
    GET_FUNCTION(GL_GET_PROGRAMIV,
                 glGetProgramiv,
                 "glGetProgramiv",
                 1);
    GET_FUNCTION(GL_GET_PROGRAM_INFO_LOG,
                 glGetProgramInfoLog,
                 "glGetProgramInfoLog",
                 1);
    GET_FUNCTION(GL_DELETE_PROGRAM,
                 glDeleteProgram,
                 "glDeleteProgram",
                 1);
    GET_FUNCTION(GL_DELETE_SHADER,
                 glDeleteShader,
                 "glDeleteShader",
                 1);

    vertexShader = vdkCompileShader(Egl, VertexShader, GL_VERTEX_SHADER, Log);
    if (vertexShader == 0)
    {
        goto error;
    }

    fragmentShader = vdkCompileShader(Egl, FragmentShader, GL_FRAGMENT_SHADER, Log);
    if (fragmentShader == 0)
    {
        goto error;
    }

    program = glCreateProgram();
    if (program == 0)
    {
        goto error;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    if (glGetError() != GL_NO_ERROR)
    {
        goto error;
    }

    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

        if (Log != NULL)
        {
            *Log = (char *)malloc(length + 1);
            if (*Log != NULL)
            {
                glGetProgramInfoLog(program, length, &length, *Log);
                (*Log)[length] = '\0';
            }
        }
        goto error;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;

error:
    if (program != 0)
        glDeleteProgram(program);

    if (fragmentShader != 0)
        glDeleteShader(fragmentShader);

    if (vertexShader != 0)
        glDeleteShader(vertexShader);

    return 0;
}

VDKAPI int VDKLANG
vdkDeleteProgram(
    vdkEGL * Egl,
    unsigned int Program
    )
{
    static GL_DELETE_PROGRAM glDeleteProgram = NULL;

    GET_FUNCTION(GL_DELETE_PROGRAM,
                 glDeleteProgram,
                 "glDeleteProgram",
                 1);

    glDeleteProgram(Program);

    return 1;
}
