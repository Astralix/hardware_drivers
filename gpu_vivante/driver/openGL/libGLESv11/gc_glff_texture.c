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




#include "gc_glff_precomp.h"

#define _GC_OBJ_ZONE    glvZONE_TEXTURE

#if defined(ANDROID)
#include <private/ui/android_natives_priv.h>
#include <gc_gralloc_priv.h>
#endif

/******************************************************************************\
***************************** Texture GL Name Arrays ***************************
\******************************************************************************/

/* Possible texture min filters. */
static GLenum _TextureMinFilterNames[] =
{
    GL_NEAREST,                 /* glvNEAREST */
    GL_LINEAR,                  /* glvLINEAR */
    GL_NEAREST_MIPMAP_NEAREST,  /* glvNEAREST_MIPMAP_NEAREST */
    GL_LINEAR_MIPMAP_NEAREST,   /* glvLINEAR_MIPMAP_NEAREST */
    GL_NEAREST_MIPMAP_LINEAR,   /* glvNEAREST_MIPMAP_LINEAR */
    GL_LINEAR_MIPMAP_LINEAR,    /* glvLINEAR_MIPMAP_LINEAR */
};

/* Possible texture mag filters. */
static GLenum _TextureMagFilterNames[] =
{
    GL_NEAREST,                 /* glvNEAREST */
    GL_LINEAR,                  /* glvLINEAR */
};

/* Possible texture wraps. */
static GLenum _TextureWrapNames[] =
{
    GL_CLAMP_TO_EDGE,           /* glvCLAMP */
    GL_REPEAT,                  /* glvREPEAT */
    GL_MIRRORED_REPEAT_OES,     /* gcvMIRROR */
};

/* Possible texture functions. */
static GLenum _TextureFunctionNames[] =
{
    GL_REPLACE,                 /* glvTEXREPLACE */
    GL_MODULATE,                /* glvTEXMODULATE */
    GL_DECAL,                   /* glvTEXDECAL */
    GL_BLEND,                   /* glvTEXBLEND */
    GL_ADD,                     /* glvTEXADD */
    GL_COMBINE,                 /* glvTEXCOMBINE */
};

/* Possible combine color texture functions. */
static GLenum _CombineColorTextureFunctionNames[] =
{
    GL_REPLACE,                 /* glvCOMBINEREPLACE */
    GL_MODULATE,                /* glvCOMBINEMODULATE */
    GL_ADD,                     /* glvCOMBINEADD */
    GL_ADD_SIGNED,              /* glvCOMBINEADDSIGNED */
    GL_INTERPOLATE,             /* glvCOMBINEINTERPOLATE */
    GL_SUBTRACT,                /* glvCOMBINESUBTRACT */
    GL_DOT3_RGB,                /* glvCOMBINEDOT3RGB */
    GL_DOT3_RGBA,               /* glvCOMBINEDOT3RGBA */
};

/* Possible combine alpha texture functions. */
static GLenum _CombineAlphaTextureFunctionNames[] =
{
    GL_REPLACE,                 /* glvCOMBINEREPLACE */
    GL_MODULATE,                /* glvCOMBINEMODULATE */
    GL_ADD,                     /* glvCOMBINEADD */
    GL_ADD_SIGNED,              /* glvCOMBINEADDSIGNED */
    GL_INTERPOLATE,             /* glvCOMBINEINTERPOLATE */
    GL_SUBTRACT,                /* glvCOMBINESUBTRACT */
};

/* Possible combine texture function sources. */
static GLenum _CombineFunctionSourceNames[] =
{
    GL_TEXTURE,                 /* glvTEXTURE */
    GL_CONSTANT,                /* glvCONSTANT */
    GL_PRIMARY_COLOR,           /* glvCOLOR */
    GL_PREVIOUS,                /* glvPREVIOUS */
};

/* Possible combine texture function RGB operands. */
static GLenum _CombineFunctionColorOperandNames[] =
{
    GL_SRC_ALPHA,               /* glvSRCALPHA */
    GL_ONE_MINUS_SRC_ALPHA,     /* glvSRCALPHAINV */
    GL_SRC_COLOR,               /* glvSRCCOLOR */
    GL_ONE_MINUS_SRC_COLOR,     /* glvSRCCOLORINV */
};

/* Possible combine texture function alpha operands. */
static GLenum _CombineFunctionAlphaOperandNames[] =
{
    GL_SRC_ALPHA,               /* glvSRCALPHA */
    GL_ONE_MINUS_SRC_ALPHA,     /* glvSRCALPHAINV */
};

/* Possible texture coordinate generation modes. */
static GLenum _TextureGenModes[] =
{
    GL_NORMAL_MAP_OES,          /* glvTEXNORMAL */
    GL_REFLECTION_MAP_OES,      /* glvREFLECTION */
};


/******************************************************************************\
********************** Individual State Setting Functions **********************
\******************************************************************************/

static GLboolean _SetTextureFunction(
    glsCONTEXT_PTR Context,
    glsTEXTURESAMPLER_PTR Sampler,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    GLuint value;
    gcmHEADER_ARG("Context=0x%x Sampler=0x%x Value=0x%x Type=0x%04x",
                    Context, Sampler, Value, Type);

    result = glfConvertGLEnum(
        _TextureFunctionNames,
        gcmCOUNTOF(_TextureFunctionNames),
        Value, Type,
        &value
        );

    if (result)
    {
        glmSETHASH_3BITS(hashTextureFunction, value, Sampler->index);
        Sampler->function = (gleTEXTUREFUNCTION) value;
    }

    gcmFOOTER_ARG("return=%d", result);
    return result;
}

static GLboolean _SetCombineColorFunction(
    glsCONTEXT_PTR Context,
    glsTEXTURESAMPLER_PTR Sampler,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    GLuint value;
    gcmHEADER_ARG("Context=0x%x Sampler=0x%x Value=0x%x Type=0x%04x",
                    Context, Sampler, Value, Type);

    result = glfConvertGLEnum(
        _CombineColorTextureFunctionNames,
        gcmCOUNTOF(_CombineColorTextureFunctionNames),
        Value, Type,
        &value
        );

    if (result)
    {
        glmSETHASH_4BITS(hashTextureCombColorFunction, value, Sampler->index);
        Sampler->combColor.function = (gleTEXCOMBINEFUNCTION) value;

        if (value == glvCOMBINEDOT3RGBA)
        {
            Sampler->colorDataFlow.targetEnable = gcSL_ENABLE_XYZW;
            Sampler->colorDataFlow.tempEnable   = gcSL_ENABLE_XYZW;
            Sampler->colorDataFlow.tempSwizzle  = gcSL_SWIZZLE_XYZW;
            Sampler->colorDataFlow.argSwizzle   = gcSL_SWIZZLE_XYZW;
        }
        else
        {
            Sampler->colorDataFlow.targetEnable = gcSL_ENABLE_XYZ;
            Sampler->colorDataFlow.tempEnable   = gcSL_ENABLE_XYZ;
            Sampler->colorDataFlow.tempSwizzle  = gcSL_SWIZZLE_XYZZ;
            Sampler->colorDataFlow.argSwizzle   = gcSL_SWIZZLE_XYZZ;
        }
    }

    gcmFOOTER_ARG("return=%d", result);
    return result;
}

static GLboolean _SetCombineAlphaFunction(
    glsCONTEXT_PTR Context,
    glsTEXTURESAMPLER_PTR Sampler,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    GLuint value;
    gcmHEADER_ARG("Context=0x%x Sampler=0x%x Value=0x%x Type=0x%04x",
                    Context, Sampler, Value, Type);

    result = glfConvertGLEnum(
        _CombineAlphaTextureFunctionNames,
        gcmCOUNTOF(_CombineAlphaTextureFunctionNames),
        Value, Type,
        &value
        );

    if (result)
    {
        glmSETHASH_3BITS(hashTextureCombAlphaFunction, value, Sampler->index);
        Sampler->combAlpha.function = (gleTEXCOMBINEFUNCTION) value;
    }

    gcmFOOTER_ARG("return=%d", result);
    return result;
}

static GLboolean _SetCombineColorSource(
    glsCONTEXT_PTR Context,
    GLenum Source,
    glsTEXTURESAMPLER_PTR Sampler,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    GLuint value;
    gcmHEADER_ARG("Context=0x%x Source=0x%04x Sampler=0x%x Value=0x%x Type=0x%04x",
                    Context, Source, Sampler, Value, Type);

    result = glfConvertGLEnum(
        _CombineFunctionSourceNames,
        gcmCOUNTOF(_CombineFunctionSourceNames),
        Value, Type,
        &value
        );

    if (result)
    {
        gctUINT source = Source - GL_SRC0_RGB;

        switch (source)
        {
        case 0:
            glmSETHASH_2BITS(hashTextureCombColorSource0, value, Sampler->index);
            Sampler->combColor.source[0] = (gleCOMBINESOURCE) value;
            break;

        case 1:
            glmSETHASH_2BITS(hashTextureCombColorSource1, value, Sampler->index);
            Sampler->combColor.source[1] = (gleCOMBINESOURCE) value;
            break;

        case 2:
            glmSETHASH_2BITS(hashTextureCombColorSource2, value, Sampler->index);
            Sampler->combColor.source[2] = (gleCOMBINESOURCE) value;
            break;
        }
    }

    gcmFOOTER_ARG("return=%d", result);
    return result;
}

static GLboolean _SetCombineAlphaSource(
    glsCONTEXT_PTR Context,
    GLenum Source,
    glsTEXTURESAMPLER_PTR Sampler,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    GLuint value;
    gcmHEADER_ARG("Context=0x%x Source=0x%04x Sampler=0x%x Value=0x%x Type=0x%04x",
                    Context, Source, Sampler, Value, Type);

    result = glfConvertGLEnum(
        _CombineFunctionSourceNames,
        gcmCOUNTOF(_CombineFunctionSourceNames),
        Value, Type,
        &value
        );

    if (result)
    {
        gctUINT source = Source - GL_SRC0_ALPHA;

        switch (source)
        {
        case 0:
            glmSETHASH_2BITS(hashTextureCombAlphaSource0, value, Sampler->index);
            Sampler->combAlpha.source[0] = (gleCOMBINESOURCE) value;
            break;

        case 1:
            glmSETHASH_2BITS(hashTextureCombAlphaSource1, value, Sampler->index);
            Sampler->combAlpha.source[1] = (gleCOMBINESOURCE) value;
            break;

        case 2:
            glmSETHASH_2BITS(hashTextureCombAlphaSource2, value, Sampler->index);
            Sampler->combAlpha.source[2] = (gleCOMBINESOURCE) value;
            break;
        }
    }

    gcmFOOTER_ARG("return=%d", result);
    return result;
}

static GLboolean _SetCombineColorOperand(
    glsCONTEXT_PTR Context,
    GLenum Operand,
    glsTEXTURESAMPLER_PTR Sampler,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    GLuint value;
    gcmHEADER_ARG("Context=0x%x Operand=0x%04x Sampler=0x%x Value=0x%x Type=0x%04x",
                    Context, Operand, Sampler, Value, Type);

    result = glfConvertGLEnum(
        _CombineFunctionColorOperandNames,
        gcmCOUNTOF(_CombineFunctionColorOperandNames),
        Value, Type,
        &value
        );

    if (result)
    {
        GLuint operand = Operand - GL_OPERAND0_RGB;

        switch (operand)
        {
        case 0:
            glmSETHASH_2BITS(hashTextureCombColorOperand0, value, Sampler->index);
            Sampler->combColor.operand[0] = (gleCOMBINEOPERAND) value;
            break;

        case 1:
            glmSETHASH_2BITS(hashTextureCombColorOperand1, value, Sampler->index);
            Sampler->combColor.operand[1] = (gleCOMBINEOPERAND) value;
            break;

        case 2:
            glmSETHASH_2BITS(hashTextureCombColorOperand2, value, Sampler->index);
            Sampler->combColor.operand[2] = (gleCOMBINEOPERAND) value;
            break;
        }
    }

    gcmFOOTER_ARG("return=%d", result);
    return result;
}

static GLboolean _SetCombineAlphaOperand(
    glsCONTEXT_PTR Context,
    GLenum Operand,
    glsTEXTURESAMPLER_PTR Sampler,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    GLuint value;
    gcmHEADER_ARG("Context=0x%x Operand=0x%04x Sampler=0x%x Value=0x%x Type=0x%04x",
                    Context, Operand, Sampler, Value, Type);

    result = glfConvertGLEnum(
        _CombineFunctionAlphaOperandNames,
        gcmCOUNTOF(_CombineFunctionAlphaOperandNames),
        Value, Type,
        &value
        );

    if (result)
    {
        GLuint operand = Operand - GL_OPERAND0_ALPHA;

        switch (operand)
        {
        case 0:
            glmSETHASH_2BITS(hashTextureCombAlphaOperand0, value, Sampler->index);
            Sampler->combAlpha.operand[0] = (gleCOMBINEOPERAND) value;
            break;

        case 1:
            glmSETHASH_2BITS(hashTextureCombAlphaOperand1, value, Sampler->index);
            Sampler->combAlpha.operand[1] = (gleCOMBINEOPERAND) value;
            break;

        case 2:
            glmSETHASH_2BITS(hashTextureCombAlphaOperand2, value, Sampler->index);
            Sampler->combAlpha.operand[2] = (gleCOMBINEOPERAND) value;
            break;
        }
    }

    gcmFOOTER_ARG("return=%d", result);
    return result;
}

static GLboolean _SetCurrentColor(
    glsCONTEXT_PTR Context,
    glsTEXTURESAMPLER_PTR Sampler,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Context=0x%x Sampler=0x%x Value=0x%x Type=0x%04x",
                    Context, Sampler, Value, Type);

    glfSetVector4(
        &Sampler->constColor,
        Value,
        Type
        );

    gcmFOOTER_ARG("return=%d", GL_TRUE);

    return GL_TRUE;
}

static GLboolean _SetColorScale(
    glsCONTEXT_PTR Context,
    glsTEXTURESAMPLER_PTR Sampler,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLfloat scale;
    gcmHEADER_ARG("Context=0x%x Sampler=0x%x Value=0x%x Type=0x%04x",
                    Context, Sampler, Value, Type);

    scale = glfFloatFromRaw(Value, Type);

    if ((scale != 1.0f) && (scale != 2.0f) && (scale != 4.0f))
    {
        gcmFOOTER_ARG("return=%d", GL_FALSE);
        return GL_FALSE;
    }

    glfSetMutant(
        &Sampler->combColor.scale,
        Value,
        Type
        );

    glmSETHASH_1BIT(
        hashTexCombColorScaleOne,
        Sampler->combColor.scale.one,
        Sampler->index
        );

    /* Set uTexCombScale dirty. */
    Context->fsUniformDirty.uTexCombScaleDirty = gcvTRUE;

    gcmFOOTER_ARG("return=%d", GL_TRUE);

    return GL_TRUE;
}

static GLboolean _SetAlphaScale(
    glsCONTEXT_PTR Context,
    glsTEXTURESAMPLER_PTR Sampler,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLfloat scale;
    gcmHEADER_ARG("Context=0x%x Sampler=0x%x Value=0x%x Type=0x%04x",
                    Context, Sampler, Value, Type);

    scale = glfFloatFromRaw(Value, Type);

    if ((scale != 1.0f) && (scale != 2.0f) && (scale != 4.0f))
    {
        gcmFOOTER_ARG("return=%d", GL_FALSE);
        return GL_FALSE;
    }

    glfSetMutant(
        &Sampler->combAlpha.scale,
        Value,
        Type
        );

    glmSETHASH_1BIT(
        hashTexCombAlphaScaleOne,
        Sampler->combAlpha.scale.one,
        Sampler->index
        );

    /* Set uTexCombScale dirty. */
    Context->fsUniformDirty.uTexCombScaleDirty = gcvTRUE;

    gcmFOOTER_ARG("return=%d", GL_TRUE);

    return GL_TRUE;
}

static GLboolean _SetTexCoordGenMode(
    glsCONTEXT_PTR Context,
    glsTEXTURESAMPLER_PTR Sampler,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    gcmHEADER_ARG("Context=0x%x Sampler=0x%x Value=0x%x Type=0x%04x",
                    Context, Sampler, Value, Type);

    do
    {
        GLuint value;

        /* Convert the value. */
        result = glfConvertGLEnum(
            _TextureGenModes,
            gcmCOUNTOF(_TextureGenModes),
            Value, Type,
            &value
            );

        /* Error? */
        if (result)
        {
            /* Update the generation mode. */
            Sampler->genMode = (gleTEXTUREGEN) value;

            /* Update the hash key. */
            glmSETHASH_1BIT(hashTexCubeCoordGenMode, value, Sampler->index);
        }
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("return=%d", result);

    /* Return result. */
    return result;
}


/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

#define gcSENTINELTEXTURE ( (gcoTEXTURE) ~0 )

typedef struct _glsCOMPRESSEDTEXTURE * glsCOMPRESSEDTEXTURE_PTR;
typedef struct _glsCOMPRESSEDTEXTURE
{
    GLint       bits;
    GLint       bytes;
    GLenum      format;
    GLenum      type;
}
glsCOMPRESSEDTEXTURE;

static GLenum _compressedTextures[] =
{
    GL_PALETTE4_RGB8_OES,
    GL_PALETTE4_RGBA8_OES,
    GL_PALETTE4_R5_G6_B5_OES,
    GL_PALETTE4_RGBA4_OES,
    GL_PALETTE4_RGB5_A1_OES,
    GL_PALETTE8_RGB8_OES,
    GL_PALETTE8_RGBA8_OES,
    GL_PALETTE8_R5_G6_B5_OES,
    GL_PALETTE8_RGBA4_OES,
    GL_PALETTE8_RGB5_A1_OES,

    GL_COMPRESSED_RGB_S3TC_DXT1_EXT,
    GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,
    GL_COMPRESSED_RGBA_S3TC_DXT3_EXT,
    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,

    GL_ETC1_RGB8_OES,
};


/*******************************************************************************
**
**  _GetClosestPowerOfTwo
**
**  Find the closest power of two to the specified value.
**
**  INPUT:
**
**      Value
**          Specified value for which the power of two is to be found.
**
**      RoundUp
**          Specifies whether the smaller or greater power of two number
**          needs to be found.
**
**  OUTPUT:
**
**      Closest power of two number.
*/

static gctUINT _GetClosestPowerOfTwo(
    gctUINT Value,
    gctBOOL RoundUp
    )
{
    gcmHEADER_ARG("Value=%u RoundUp=%d", Value, RoundUp);
    /* Special case for zero. */
    if (Value == 0)
    {
        if (RoundUp)
        {
            gcmFOOTER_ARG("return=%u", 1);
            return 1;
        }
        else
        {
            gcmFOOTER_ARG("return=%u", 0);
            return 0;
        }
    }

    /* Already a power of two? */
    else if ((Value & (Value - 1)) == 0)
    {
        gcmFOOTER_ARG("return=%u", Value);
        return Value;
    }

    /* Find the closest power of two. */
    else
    {
        gctINT leadingZeroCount = 0;
        gctUINT mask = 1U << 31;
        gctUINT result;

        while ((Value & mask) == 0)
        {
            leadingZeroCount++;
            mask >>= 1;
        }

        if (RoundUp)
        {
            leadingZeroCount--;
        }

        gcmASSERT(leadingZeroCount >= 0);
        result = 1 << gcmMIN(31 - leadingZeroCount, 31);
        gcmFOOTER_ARG("return=%u", result);
        return result;
    }
}


/*******************************************************************************
**
**  _GetCompressedPalettedTextureDetails
**
**  Return Find the closest power of two to the specified value.
**
**  INPUT:
**
**      Name
**          Enumerated compressed paletted texture name.
**
**  OUTPUT:
**
**      Pointer to the format information.
*/

static glsCOMPRESSEDTEXTURE_PTR _GetCompressedPalettedTextureDetails(
    GLenum Name
    )
{
    static glsCOMPRESSEDTEXTURE _compressedTextureDetails[] =
    {
        { 4, 3, GL_RGB,  GL_UNSIGNED_BYTE },            /* GL_PALETTE4_RGB8_OES  */
        { 4, 4, GL_RGBA, GL_UNSIGNED_BYTE },            /* GL_PALETTE4_RGBA8_OES */
        { 4, 2, GL_RGB,  GL_UNSIGNED_SHORT_5_6_5 },     /* GL_PALETTE4_R5_G6_B5_OES */
        { 4, 2, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4 },   /* GL_PALETTE4_RGBA4_OES */
        { 4, 2, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1 },   /* GL_PALETTE4_RGB5_A1_OES */
        { 8, 3, GL_RGB,  GL_UNSIGNED_BYTE },            /* GL_PALETTE8_RGB8_OES */
        { 8, 4, GL_RGBA, GL_UNSIGNED_BYTE },            /* GL_PALETTE8_RGBA8_OES */
        { 8, 2, GL_RGB,  GL_UNSIGNED_SHORT_5_6_5 },     /* GL_PALETTE8_R5_G6_B5_OES */
        { 8, 2, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4 },   /* GL_PALETTE8_RGBA4_OES */
        { 8, 2, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1 },   /* GL_PALETTE8_RGB5_A1_OES */
    };

    gctINT index;

    gcmHEADER_ARG("Name=0x%04x", Name);

    /* Determine the info index. */
    index = Name - GL_PALETTE4_RGB8_OES;

    /* Out of ranage? */
    if ((index < 0) || (index > (gctINT) gcmCOUNTOF(_compressedTextureDetails)))
    {
        gcmFOOTER_ARG("return=0x%x", gcvNULL);
        return gcvNULL;
    }
    else
    {
        gcmFOOTER_ARG("return=0x%x", &_compressedTextureDetails[index]);
        return &_compressedTextureDetails[index];
    }
}


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
    IN GLsizei Width,
    IN GLsizei Height,
    IN GLsizei ImageSize,
    IN const void * Data,
    OUT gctUINT32 * Type
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
        glmERROR(GL_OUT_OF_MEMORY);
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

    *Type = GL_UNSIGNED_BYTE;

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
        r = (glmRED  (c0) + glmRED  (c1)) / 2;
        g = (glmGREEN(c0) + glmGREEN(c1)) / 2;
        b = (glmBLUE (c0) + glmBLUE (c1)) / 2;
        color[2] = 0x8000 | (r << 10) | ((g & 0x3E) << 4) | b;

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
    bpp = (Format == gcvSURF_R5G6B5) ? 2 : 4;

    /* Allocate the decompressed memory. */
    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL, Width * Height * bpp, &pointer)))
    {
        /* Error. */
        glmERROR(GL_OUT_OF_MEMORY);
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

/*******************************************************************************
**
**  _GenerateMipMap
**
**  Allocate the specified mipmap level and generate the image for it.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Texture
**          Texture object handle.
**
**      Format
**          Format of the mipmap to add.
**
**      Level
**          The level number after which the new level is to be added.
**
**      Width, Height
**          The size of the mipmap level speified by Level parameter.
**
**      Faces
**          Number of faces in the texture.
**
**  OUTPUT:
**
**      Pointer to the format information.
*/

static gceSTATUS _GenerateMipMap(
    IN glsCONTEXT_PTR Context,
    IN gcoTEXTURE Texture,
    IN gceSURF_FORMAT Format,
    IN GLint Level,
    IN GLsizei Width,
    IN GLsizei Height,
    IN GLuint Faces
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Texture=0x%x Format=0x%04x Level=%d Width=%d Height=%d Faces=%u",
                    Context, Texture, Format, Level, Width, Height, Faces);

    do
    {
        gcoSURF lod0, lod1;
        GLsizei newWidth, newHeight;

        /* Compute the new LOD size. */
        newWidth  = Width / 2;
        newHeight = Height / 2;

        /* Reached the smallest level? */
        if ((newWidth == Width) && (newHeight == Height))
        {
            status = gcvSTATUS_MIPMAP_TOO_SMALL;
            break;
        }

        if ((newWidth == 0) && (newHeight == 0))
        {
            status = gcvSTATUS_MIPMAP_TOO_SMALL;
            break;
        }

        if (newWidth == 0)
            newWidth = 1;
        else if (newHeight == 0)
            newHeight = 1;

        /* Get the texture surface. */
        gcmERR_BREAK(gcoTEXTURE_GetMipMap(
            Texture, Level, &lod0
            ));

        /* Create a new level. */
        gcmERR_BREAK(gcoTEXTURE_AddMipMap(
            Texture,
            Level + 1,
            Format,
            newWidth, newHeight,
            0, Faces,
            gcvPOOL_DEFAULT,
            &lod1
            ));

        /* Resample to create the lower level. */
        gcmERR_BREAK(gcoSURF_Resample(
            lod0, lod1
            ));

#ifdef _DUMP_FILE
        {
            gcoDUMP dump;
            gctUINT width, height;
            gctINT stride;
            gctPOINTER memory[3] = {0};
            gctUINT32 address[3] = {gcvNULL};

            gcmVERIFY_OK(gcoSURF_Lock(
                lod1, address, memory
                ));

            gcmVERIFY_OK(gcoSURF_GetAlignedSize(
                lod1, &width, &height, &stride
                ));

            gcmVERIFY_OK(gcoHAL_GetDump(
                Context->hal, &dump
                ));

            gcmVERIFY_OK(gcoDUMP_DumpData(
                dump, gcvTAG_TEXTURE, address[0], height * stride, (char *) memory[0]
                ));

            gcmVERIFY_OK(gcoSURF_Unlock(
                lod1, memory[0]
                ));
        }
#endif
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  glfGenerateMipMaps
**
**  Generate mipmap levels for the specified texture.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Texture
**          Pointer to the texture wrapper object.
**
**      Format
**          Format of the mipmap to add.
**
**      Level
**          The level number after which the new level is to be added.
**
**      Width, Height
**          The size of the mipmap level speified by Level parameter.
**
**      Faces
**          Number of faces in the texture.
**
**  OUTPUT:
**
**      Pointer to the format information.
*/

gceSTATUS glfGenerateMipMaps(
    IN glsCONTEXT_PTR Context,
    IN glsTEXTUREWRAPPER_PTR Texture,
    IN gceSURF_FORMAT Format,
    IN GLint Level,
    IN GLsizei Width,
    IN GLsizei Height,
    IN GLuint Faces
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Texture=0x%x Format=0x%04x Level=%d Width=%d Height=%d Faces=%u",
                    Context, Texture, Format, Level, Width, Height, Faces);

    do
    {
        GLint level = Level;

        while (gcvTRUE)
        {
            gcmERR_BREAK(_GenerateMipMap(
                Context, Texture->object, Format,
                level, Width, Height, Faces
                ));

            if (status == gcvSTATUS_MIPMAP_TOO_SMALL)
            {
                break;
            }

            level += 1;

            Width  = Width / 2;
            Height = Height / 2;
        }

        /* Mark texture as dirty. */
        if (Level != level)
        {
            /* Invlaidate the texture. */
            Texture->dirty = gcvTRUE;
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  glfGetMaxLOD
**
**  Determine the maximum LOD number.
**
**  INPUT:
**
**      Width, Height
**          The size of LOD 0.
**
**  OUTPUT:
**
**      Maximum level number.
*/

gctINT glfGetMaxLOD(
    gctINT Width,
    gctINT Height
    )
{
    gctINT lod = 0;
    gcmHEADER_ARG("Width=%d Height=%d", Width, Height);

    while ((Width > 1) || (Height > 1))
    {
        /* Update the maximum LOD. */
        lod += 1;

        /* Compute the size of the next level. */
        Width  = gcmMAX(Width  / 2, 1);
        Height = gcmMAX(Height / 2, 1);
    }

    gcmFOOTER_ARG("return=%d", lod);
    /* Return the maximum LOD. */
    return lod;
}


/*******************************************************************************
**
**  _SetTextureWrapperFormat
**
**  Set texture format and associated fields.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Texture
**          Pointer to the texture wrapper object.
**
**      Format
**          Specifies the number of color components in the texture. Must be
**          GL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, or GL_LUMINANCE_ALPHA.
**
**  OUTPUT:
**
**      Nothing.
*/

static void _SetTextureWrapperFormat(
    glsCONTEXT_PTR Context,
    glsTEXTUREWRAPPER_PTR Texture,
    GLenum Format
    )
{
    gcmHEADER_ARG("Context=0x%x Texture=0x%x Format=0x%04x", Context, Texture, Format);

    /* Set the format. */
    Texture->format = Format;

    /* Set target enable for FS. */
    switch (Format)
    {
    case GL_ALPHA:
        Texture->combineFlow.targetEnable = gcSL_ENABLE_W;
        Texture->combineFlow.tempEnable   = gcSL_ENABLE_X;
        Texture->combineFlow.tempSwizzle  = gcSL_SWIZZLE_XXXX;
        Texture->combineFlow.argSwizzle   = gcSL_SWIZZLE_WWWW;
        break;

    case GL_LUMINANCE:
    case GL_RGB:
        Texture->combineFlow.targetEnable = gcSL_ENABLE_XYZ;
        Texture->combineFlow.tempEnable   = gcSL_ENABLE_XYZ;
        Texture->combineFlow.tempSwizzle  = gcSL_SWIZZLE_XYZZ;
        Texture->combineFlow.argSwizzle   = gcSL_SWIZZLE_XYZZ;
        break;

    case GL_LUMINANCE_ALPHA:
    case GL_RGBA:
    case GL_BGRA_EXT:
        Texture->combineFlow.targetEnable = gcSL_ENABLE_XYZW;
        Texture->combineFlow.tempEnable   = gcSL_ENABLE_XYZW;
        Texture->combineFlow.tempSwizzle  = gcSL_SWIZZLE_XYZW;
        Texture->combineFlow.argSwizzle   = gcSL_SWIZZLE_XYZW;
        break;

    default:
        /* Invalid request. */
        gcmASSERT(gcvFALSE);
    }
    gcmFOOTER_NO();
    return;
}


/*******************************************************************************
**
**  _UpdateStageEnable
**
**  Update stage enable state.
**
**  INPUT:
**
**      Context
**          Pointer to the glsCONTEXT structure.
**
**      Sampler
**          Pointer to the texture sampler descriptor.
**
**  OUTPUT:
**
**      Nothing.
*/

static void _UpdateStageEnable(
    glsCONTEXT_PTR Context,
    glsTEXTURESAMPLER_PTR Sampler
    )
{
    gctUINT formatIndex;
    gctBOOL mipmapRequired;
    gctUINT index;

    gcmHEADER_ARG("Context=0x%x Sampler=0x%x", Context, Sampler);

    Sampler->stageEnabled = GL_FALSE;

    if ((Sampler->enableTexturing
      || Sampler->enableCubeTexturing
      || Sampler->enableExternalTexturing)
    &&  (Sampler->binding->object != gcvNULL)
    )
    {
        /* Determine whether mipmaps are required. */
        mipmapRequired
            =  (Sampler->binding->minFilter != glvNEAREST)
            && (Sampler->binding->minFilter != glvLINEAR);

        /* Determine maximum LOD. */
        Sampler->binding->maxLOD = mipmapRequired
            ? Sampler->binding->maxLevel
            : 0;

        /* Determine the current stage enable flag. */
        Sampler->stageEnabled = gcmIS_SUCCESS(gcoTEXTURE_IsComplete(
                                        Sampler->binding->object,
                                        Sampler->binding->maxLOD));
    }

    /* Get the sampler index. */
    index = Sampler->index;

    if (Sampler->stageEnabled)
    {
        /* Determine the format. */
        switch (Sampler->binding->format)
        {
        case GL_ALPHA:
        case GL_RGB:
        case GL_RGBA:
        case GL_LUMINANCE:
        case GL_LUMINANCE_ALPHA:
            formatIndex = Sampler->binding->format - GL_ALPHA;
            break;
        case GL_BGRA_EXT:
            formatIndex = 5;
            break;
        default:
            /* Invalid request. */
            gcmFOOTER_NO();
            gcmASSERT(gcvFALSE);
            return;
        }

        /* Update the hash key. */
        glmSETHASH_1BIT (hashStageEnabled,  Sampler->stageEnabled, index);
        glmSETHASH_3BITS(hashTextureFormat, formatIndex,  index);
    }
    else
    {
        /* Update the hash key. */
        glmSETHASH_1BIT(hashStageEnabled,  Sampler->stageEnabled, index);

        /* Set to an invalid format. */
        glmCLEARHASH_3BITS(hashTextureFormat, index);
    }
    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  _InitTextureWrapper
**
**  Sets default values to specified texture wrapper object.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Texture
**          Pointer to the texture wrapper object.
**
**  OUTPUT:
**
**      Nothing.
*/

static void _InitTextureWrapper(
    glsCONTEXT_PTR Context,
    glsTEXTUREWRAPPER_PTR Texture
    )
{
    gcmHEADER_ARG("Context=0x%x Texture=0x%x", Context, Texture);
    /*
    ** Init everything to zero.
    */
    gcoOS_ZeroMemory(Texture, gcmSIZEOF(glsTEXTUREWRAPPER));

    /*
    ** Set binding.
    */
    Texture->binding          = gcvNULL;
    Texture->boundAtLeastOnce = gcvFALSE;

    /*
    ** Default format.
    */
    _SetTextureWrapperFormat(Context, Texture, GL_RGBA);

    /*
    ** Init states.
    */
    Texture->maxLevel  = 1000;
    Texture->minFilter = glvNEAREST_MIPMAP_LINEAR;
    Texture->magFilter = glvLINEAR;
    Texture->wrapS     = glvREPEAT;
    Texture->wrapT     = glvREPEAT;
    Texture->genMipmap = GL_FALSE;
    Texture->anisoFilter = 1;

    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  _ConstructWrapper
**
**  Constructs a new texture wrapper object.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      InsertAfter
**          Pointer to the wrapper that will preceed the new one.
**
**      Name
**          Name of the new texture wrapper object.
**
**  OUTPUT:
**
**      Pointer to the new texture wrapper object.
*/

static glsTEXTUREWRAPPER_PTR _ConstructWrapper(
    glsCONTEXT_PTR Context,
    glsTEXTUREWRAPPER_PTR InsertAfter,
    GLuint Name
    )
{
    glsTEXTUREWRAPPER_PTR texture;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Context=0x%x InsertAfter=0x%x Name=%u", Context, InsertAfter, Name);

    /* Allocate new texture descriptor. */
    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                   gcmSIZEOF(glsTEXTUREWRAPPER),
                                   &pointer)))
    {
        gcmFOOTER_NO();
        return gcvNULL;
    }

    texture = pointer;

    /* Init texture defaults. */
    _InitTextureWrapper(Context, texture);

    /* Set the name. */
    texture->name = Name;

    /* Append the new texture node. */
    texture->prev = InsertAfter;
    texture->next = InsertAfter->next;
    texture->prev->next =
    texture->next->prev = texture;

    gcmFOOTER_ARG("texture=0x%x", texture);

    /* Return result. */
    return texture;
}


/*******************************************************************************
**
**  _ResetTextureWrapper
**
**  Delete objects associated with the wrapper.
**
**  INPUT:
**
**      Context
**          Pointer to the glsCONTEXT structure.
**
**      Texture
**          Pointer to the glsTEXTUREWRAPPER texture wrapper.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _ResetTextureWrapper(
    glsCONTEXT_PTR Context,
    glsTEXTUREWRAPPER_PTR Texture
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=0x%x Texture=0x%x", Context, Texture);

    do
    {
        /* Reset the number of levels. */
        Texture->maxLevel = 1000;

        Texture->uploaded   = gcvFALSE;

        /* Destroy the texture object. */
        if (Texture->object != gcvNULL)
        {
            gcmERR_BREAK(gcoTEXTURE_Destroy(Texture->object));
            Texture->object = gcvNULL;
        }

        /* Remove existing YUV direct structure. */
        if (Texture->direct.source != gcvNULL)
        {
#if defined(ANDROID)
            if (Texture->privHandle != gcvNULL)
            {
#endif
            /* Unlock the source surface. */
            gcmERR_BREAK(gcoSURF_Unlock(
                Texture->direct.source,
                gcvNULL
                ));

            /* Destroy the source surface. */
            gcmERR_BREAK(gcoSURF_Destroy(
                Texture->direct.source
                ));

#if defined(ANDROID)
            }
#endif

            /* Destroy the temporary surface. */
            if (Texture->direct.temp != gcvNULL)
            {
                gcmERR_BREAK(gcoSURF_Destroy(
                    Texture->direct.temp
                    ));

                Texture->direct.temp = gcvNULL;
            }

            /* Clear the LOD array. */
            gcmVERIFY_OK(gcoOS_ZeroMemory(
                Texture->direct.texture, gcmSIZEOF(gcoSURF) * 16
                ));
        }

#if defined(ANDROID)
        /* Reset the source surface will be uploaded from. */
        Texture->source     = gcvNULL;
        Texture->privHandle = gcvNULL;
#endif
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  glfInitializeTexture
**
**  Constructs texture management object.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS glfInitializeTexture(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status;

    static GLenum textureFunction   = GL_MODULATE;
    static GLenum combColorFunction = GL_MODULATE;
    static GLenum combAlphaFunction = GL_MODULATE;

    static GLenum combColorSource0 = GL_TEXTURE;
    static GLenum combColorSource1 = GL_PREVIOUS;
    static GLenum combColorSource2 = GL_CONSTANT;

    static GLenum combColorOperand0 = GL_SRC_COLOR;
    static GLenum combColorOperand1 = GL_SRC_COLOR;
    static GLenum combColorOperand2 = GL_SRC_ALPHA;

    static GLenum combAlphaSource0 = GL_TEXTURE;
    static GLenum combAlphaSource1 = GL_PREVIOUS;
    static GLenum combAlphaSource2 = GL_CONSTANT;

    static GLenum combAlphaOperand0 = GL_SRC_ALPHA;
    static GLenum combAlphaOperand1 = GL_SRC_ALPHA;
    static GLenum combAlphaOperand2 = GL_SRC_ALPHA;

    static GLenum textureCoordGenMode = GL_REFLECTION_MAP_OES;

    static gltFRACTYPE vec0000[] =
    {
        glvFRACZERO, glvFRACZERO, glvFRACZERO, glvFRACZERO
    };

    static gltFRACTYPE value1 = glvFRACONE;

    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        gctUINT texMaxWidth;
        gctUINT texMaxHeight;
        gctUINT texMaxDepth;
        gctBOOL texCubic;
        gctBOOL texNonPowerOfTwo;
        gctUINT texPixelSamplers;
        GLuint samplerSize, j;
        GLint i;
        gctPOINTER pointer = gcvNULL;

        /* Query the texture caps. */
        gcmERR_BREAK(gcoTEXTURE_QueryCaps(
            Context->hal,
            &texMaxWidth,
            &texMaxHeight,
            &texMaxDepth,
            &texCubic,
            &texNonPowerOfTwo,
            gcvNULL,
            &texPixelSamplers
            ));

        /* Limit to API defined value. */
        if (texPixelSamplers > glvMAX_TEXTURES)
        {
            texPixelSamplers = glvMAX_TEXTURES;
        }

        /* Make sure we have samplers. */
        if (texPixelSamplers == 0)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        /* Enable DrawTex position stream. */
        Context->aPositionDrawTexInfo.streamEnabled = GL_TRUE;

        Context->hwPointSprite = gcvFALSE;

        /* Init the texture sentinel. */
        Context->texture.sentinel.object  = gcSENTINELTEXTURE;
        Context->texture.sentinel.binding = gcvNULL;
        Context->texture.sentinel.prev    =
        Context->texture.sentinel.next    = &Context->texture.sentinel;

        /* Allocate and init texture sampler structures. */
        samplerSize = texPixelSamplers * gcmSIZEOF(glsTEXTURESAMPLER);
        gcmERR_BREAK(gcoOS_Allocate(
            gcvNULL,
            samplerSize,
            &pointer
            ));

        Context->texture.sampler = pointer;

        /* Reset to zero. */
        gcoOS_ZeroMemory(Context->texture.sampler, samplerSize);

        /* Init default textures. */
        for (j = 0; j < gcmCOUNTOF(Context->texture.defaultTexture); j += 1)
        {
            _InitTextureWrapper(Context, &Context->texture.defaultTexture[j]);
        }

        /* Init active samplers. */
        Context->texture.activeSampler = &Context->texture.sampler[0];
        Context->texture.activeSamplerIndex = 0;
        Context->texture.activeClientSampler = &Context->texture.sampler[0];
        Context->texture.activeClientSamplerIndex = 0;

        /* Init texture caps. */
        Context->texture.maxWidth           = (GLuint) texMaxWidth;
        Context->texture.maxHeight          = (GLuint) texMaxHeight;
        Context->texture.maxDepth           = (GLuint) texMaxDepth;
        Context->texture.cubic              = (GLboolean) texCubic;
        Context->texture.nonPowerOfTwo      = (GLboolean) texNonPowerOfTwo;
        Context->texture.pixelSamplers      = (GLint) texPixelSamplers;
        Context->texture.generateMipmapHint = GL_DONT_CARE;
        Context->texture.matrixDirty        = GL_FALSE;

        /* Init the samplers. */
        for (i = 0; i < Context->texture.pixelSamplers; i++)
        {
            /* Get a shortcut to the current sampler. */
            glsTEXTURESAMPLER_PTR sampler = &Context->texture.sampler[i];

            /* Set the index. */
            sampler->index = i;

            /* Initialize default bindings. */
            for (j = 0; j < gcmCOUNTOF(Context->texture.defaultTexture); j += 1)
            {
                sampler->bindings[j] = &Context->texture.defaultTexture[j];
            }

            /* Bind to default 2D texture. */
            sampler->binding = sampler->bindings[glvTEXTURE2D];

            /* Set data flow structute pointers. */
            sampler->combColor.combineFlow = &sampler->colorDataFlow;
            sampler->combAlpha.combineFlow = &sampler->alphaDataFlow;

            /* Set defaults for alapha data flow. */
            sampler->alphaDataFlow.targetEnable = gcSL_ENABLE_W;
            sampler->alphaDataFlow.tempEnable   = gcSL_ENABLE_X;
            sampler->alphaDataFlow.tempSwizzle  = gcSL_SWIZZLE_XXXX;
            sampler->alphaDataFlow.argSwizzle   = gcSL_SWIZZLE_WWWW;

            /* Set default states. */
            sampler->coordReplace = GL_FALSE;

            /* Enable DrawTex texture coordinate stream. */
            sampler->aTexCoordDrawTexInfo.streamEnabled = GL_TRUE;

            gcmVERIFY(_SetTextureFunction(
                Context, sampler,
                &textureFunction, glvINT
                ));

            gcmVERIFY(_SetCombineColorFunction(
                Context, sampler,
                &combColorFunction, glvINT
                ));

            gcmVERIFY(_SetCombineAlphaFunction(
                Context, sampler,
                &combAlphaFunction, glvINT
                ));

            gcmVERIFY(_SetCombineColorSource(
                Context, GL_SRC0_RGB, sampler,
                &combColorSource0, glvINT
                ));

            gcmVERIFY(_SetCombineColorSource(
                Context, GL_SRC1_RGB, sampler,
                &combColorSource1, glvINT
                ));

            gcmVERIFY(_SetCombineColorSource(
                Context, GL_SRC2_RGB, sampler,
                &combColorSource2, glvINT
                ));

            gcmVERIFY(_SetCombineAlphaSource(
                Context, GL_SRC0_ALPHA, sampler,
                &combAlphaSource0, glvINT
                ));

            gcmVERIFY(_SetCombineAlphaSource(
                Context, GL_SRC1_ALPHA, sampler,
                &combAlphaSource1, glvINT
                ));

            gcmVERIFY(_SetCombineAlphaSource(
                Context, GL_SRC2_ALPHA, sampler,
                &combAlphaSource2, glvINT
                ));

            gcmVERIFY(_SetCombineColorOperand(
                Context, GL_OPERAND0_RGB, sampler,
                &combColorOperand0, glvINT
                ));

            gcmVERIFY(_SetCombineColorOperand(
                Context, GL_OPERAND1_RGB, sampler,
                &combColorOperand1, glvINT
                ));

            gcmVERIFY(_SetCombineColorOperand(
                Context, GL_OPERAND2_RGB, sampler,
                &combColorOperand2, glvINT
                ));

            gcmVERIFY(_SetCombineAlphaOperand(
                Context, GL_OPERAND0_ALPHA, sampler,
                &combAlphaOperand0, glvINT
                ));

            gcmVERIFY(_SetCombineAlphaOperand(
                Context, GL_OPERAND1_ALPHA, sampler,
                &combAlphaOperand1, glvINT
                ));

            gcmVERIFY(_SetCombineAlphaOperand(
                Context, GL_OPERAND2_ALPHA, sampler,
                &combAlphaOperand2, glvINT
                ));

            gcmVERIFY(_SetTexCoordGenMode(
                Context, sampler,
                &textureCoordGenMode, glvINT
                ));

            gcmVERIFY(_SetCurrentColor(
                Context, sampler,
                vec0000, glvFRACTYPEENUM
                ));

            gcmVERIFY(_SetColorScale(
                Context, sampler,
                &value1, glvFRACTYPEENUM
                ));

            gcmVERIFY(_SetAlphaScale(
                Context, sampler,
                &value1, glvFRACTYPEENUM
                ));
        }
    }
    while (GL_FALSE);

    gcmFOOTER();
    /* Return result. */
    return status;
}


/*******************************************************************************
**
**  glfDestroyTexture
**
**  Destructs texture management object.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS glfDestroyTexture(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        GLuint i;
        glsTEXTUREWRAPPER_PTR texture;

        /* Destroy the default textures. */
        for (i = 0; i < gcmCOUNTOF(Context->texture.defaultTexture); i += 1)
        {
            gcmERR_BREAK(_ResetTextureWrapper(
                Context,
                &Context->texture.defaultTexture[i]
                ));
        }

        /* Failed? */
        if (gcmIS_ERROR(status))
        {
            break;
        }

        /* Delete samplers. */
        if (Context->texture.sampler != gcvNULL)
        {
            gcmERR_BREAK(gcmOS_SAFE_FREE(
                gcvNULL, Context->texture.sampler
                ));
        }

        /* Delete textures. */
        if (Context->texture.sentinel.object == gcSENTINELTEXTURE)
        {
            for (texture = Context->texture.sentinel.next;
                 texture->object != gcSENTINELTEXTURE;)
            {
                /* Save the curent node. */
                glsTEXTUREWRAPPER_PTR node = texture;

                /* Advance to the next node. */
                texture = texture->next;

                /* Destroy the texture. */
                gcmERR_BREAK(_ResetTextureWrapper(Context, node));

                /* Destroy the wrapper. */
                gcmERR_BREAK(gcmOS_SAFE_FREE(gcvNULL, node));
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  glfFindTexture
**
**  Returns a pointer to the specified texture.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Texture
**          The name of the texture to find.
**
**  OUTPUT:
**
**      Pointer to the texture if found.
*/

glsTEXTUREWRAPPER_PTR glfFindTexture(
    glsCONTEXT_PTR Context,
    GLuint Texture
    )
{
    glsTEXTUREWRAPPER_PTR texture;
    glsCONTEXT_PTR shared;

    gcmHEADER_ARG("Context=0x%x Texture=0x%08x", Context, Texture);

    /* Map shared context. */
#if gcdRENDER_THREADS
    shared = (Context->shared != gcvNULL) ? Context->shared : Context;
#else
    shared = Context;
#endif

    /* Go through all existing textures one by one. */
    for (texture = shared->texture.sentinel.next;
         texture->object != gcSENTINELTEXTURE;
         texture = texture->next)
    {
        /* See if we found the texture. */
        if (texture->name == Texture)
        {
            gcmFOOTER_ARG("texture=0x%08x", texture);
            return texture;
        }
    }

    gcmFOOTER_ARG("texture=0x%x", gcvNULL);
    /* No such texture. */
    return gcvNULL;
}


/*******************************************************************************
**
**  glfInitializeTempBitmap
**
**  Initialize the temporary bitmap image.
**
**  INPUT:
**
**      Context
**          Pointer to the context.
**
**      Format
**          Format of the image.
**
**      Width, Height
**          The size of the image.
**
** OUTPUT:
**
**      Nothing.
*/

gceSTATUS glfInitializeTempBitmap(
    IN glsCONTEXT_PTR Context,
    IN gceSURF_FORMAT Format,
    IN gctUINT Width,
    IN gctUINT Height
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoSURF bitmap = gcvNULL;
    gcmHEADER_ARG("Context=0x%x Format=0x%04x Width=%u Height=%u",
                    Context, Format, Width, Height);

    do
    {
        /* See if the existing surface can be reused. */
        if ((Context->tempWidth  < Width)  ||
            (Context->tempHeight < Height) ||
            (Context->tempFormat != Format))
        {
            gctUINT width;
            gctUINT height;
            gctINT stride;
            gctPOINTER bits[3];
            gcsSURF_FORMAT_INFO_PTR info[2];

            /* Is there a surface allocated? */
            if (Context->tempBitmap != gcvNULL)
            {
                /* Unlock the surface. */
                if (Context->tempBits != gcvNULL)
                {
                    gcmERR_BREAK(gcoSURF_Unlock(
                        Context->tempBitmap, Context->tempBits
                        ));

                    Context->tempBits = gcvNULL;
                }

                /* Destroy the surface. */
                gcmERR_BREAK(gcoSURF_Destroy(Context->tempBitmap));

                /* Reset temporary surface. */
                Context->tempBitmap       = gcvNULL;
                Context->tempFormat       = gcvSURF_UNKNOWN;
                Context->tempBitsPerPixel = 0;
                Context->tempWidth        = 0;
                Context->tempHeight       = 0;
                Context->tempStride       = 0;
            }

            /* Valid surface requested? */
            if (Format != gcvSURF_UNKNOWN)
            {
                /* Round up the size. */
                width  = gcmALIGN(Width,  256);
                height = gcmALIGN(Height, 256);

                /* Allocate a new surface. */
                gcmONERROR(gcoSURF_Construct(
                    Context->hal,
                    width, height, 1,
                    gcvSURF_BITMAP, Format,
                    gcvPOOL_UNIFIED,
                    &bitmap
                    ));

                /* Get the pointer to the bits. */
                gcmONERROR(gcoSURF_Lock(
                    bitmap, gcvNULL, bits
                    ));

                /* Query the parameters back. */
                gcmONERROR(gcoSURF_GetAlignedSize(
                    bitmap, &width, &height, &stride
                    ));

                /* Query format specifics. */
                gcmONERROR(gcoSURF_QueryFormat(
                    Format, info
                    ));

                /* Set information. */
                Context->tempBitmap       = bitmap;
                Context->tempBits         = bits[0];
                Context->tempFormat       = Format;
                Context->tempBitsPerPixel = info[0]->bitsPerPixel;
                Context->tempWidth        = width;
                Context->tempHeight       = height;
                Context->tempStride       = stride;
            }
        }
        else
        {
            status = gcvSTATUS_OK;
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;

OnError:
    if (bitmap != gcvNULL)
    {
        gcoSURF_Destroy(bitmap);
        bitmap = gcvNULL;
    }
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  glfResolveDrawToTempBitmap
**
**  Resolve specified area of the drawing surface to the temporary bitmap.
**
**  INPUT:
**
**      Context
**          Pointer to the context.
**
**      SourceX, SourceY, Width, Height
**          The origin and the size of the image.
**
** OUTPUT:
**
**      Nothing.
*/

gceSTATUS glfResolveDrawToTempBitmap(
    IN glsCONTEXT_PTR Context,
    IN gctINT SourceX,
    IN gctINT SourceY,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x SourceX=%d SourceY=%d Width=%d Height=%d",
                    Context, SourceX, SourceY, Width, Height);

    do
    {
        gctUINT resX;
        gctUINT resY;
        gctUINT resW;
        gctUINT resH;

        gctUINT sourceX;
        gctUINT sourceY;

        gcsPOINT srcOrigin;
        gcsPOINT trgOrigin;
        gcsPOINT rectSize;

        /* Clamp coordinates. */
        gctINT left   = gcmMAX(SourceX, 0);
        gctINT top    = gcmMAX(SourceY, 0);
        gctINT right  = gcmMIN(SourceX + Width,  (GLint) Context->drawWidth);
        gctINT bottom = gcmMIN(SourceY + Height, (GLint) Context->drawHeight);

        if ((right <= 0) || (bottom <= 0))
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        /* Temp surface alignment restrictions. */
        if (gcoHAL_IsFeatureAvailable(Context->hal, gcvFEATURE_SUPER_TILED) == gcvSTATUS_TRUE)
        {
            resX = 64;
            resY = 64;
            resW = 64;
            resH = 64;
        }
        else
        {
            resX = 4;
            resY = 4;
            resW = 16;
            resH = 4;
        }

        /* Convert GL coordinates. */
        sourceX = left;
        sourceY = top;

        /* Determine the aligned source origin. */
        srcOrigin.x = sourceX & ~(resX - 1);
        srcOrigin.y = sourceY & ~(resY - 1);
        if ((srcOrigin.x + (gctINT) resW > (GLint) Context->drawWidth)
        &&  (srcOrigin.x > 0)
        )
        {
            srcOrigin.x = (Context->drawWidth - resW) & ~(resX - 1);
        }

        /* Determine the origin adjustment. */
        Context->tempX = sourceX - srcOrigin.x;
        Context->tempY = sourceY - srcOrigin.y;

        /* Determine the aligned area size. */
        rectSize.x = (gctUINT) gcmALIGN(right  - left + Context->tempX, resW);
        rectSize.y = (gctUINT) gcmALIGN(bottom - top  + Context->tempY, resH);

        /* Always resolve to 0,0. */
        trgOrigin.x = 0;
        trgOrigin.y = 0;

        /* Initialize the temporary surface. */
        /* FIXME: Bug #4219. */
        gcmERR_BREAK(glfInitializeTempBitmap(
            Context,
            Context->drawFormatInfo[0]->format,
            rectSize.x,
            rectSize.y
            ));

        /* Set orient as render target. */
        gcmERR_BREAK(gcoSURF_SetOrientation(
            Context->tempBitmap,
            gcvORIENTATION_BOTTOM_TOP
            ));

        /* Resolve the aligned area. */
        gcmERR_BREAK(gcoSURF_ResolveRect(
            Context->draw,
            Context->tempBitmap,
            &srcOrigin,
            &trgOrigin,
            &rectSize
            ));

        /* Make sure the operation is complete. */
        gcmERR_BREAK(gcoHAL_Commit(
            Context->hal, gcvTRUE
            ));

        /* Compute the pointer to the last line. */
        Context->tempLastLine
            =  Context->tempBits
            +  Context->tempStride *  Context->tempY
            + (Context->tempX      *  Context->tempBitsPerPixel) / 8;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _FindFirstHole
**
**  The list of textures is sorted by name in ascending order. This function
**  finds two textures whose names create a "hole" in the name sequence and
**  returns a pointer to the first of the two textures.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Pointer to the texture preceeding the name hole.
*/

static glsTEXTUREWRAPPER_PTR _FindFirstHole(
    glsCONTEXT_PTR Context
    )
{
    GLuint predictedName;
    glsTEXTUREWRAPPER_PTR texture;
    glsCONTEXT_PTR shared;

    gcmHEADER_ARG("Context=0x%x", Context);

    /* Map shared context. */
#if gcdRENDER_THREADS
    shared = (Context->shared != gcvNULL) ? Context->shared : Context;
#else
    shared = Context;
#endif

    /* Set predicted to the first available. */
    predictedName = 1;

    /* Go through all existing textures one by one. */
    for (texture = shared->texture.sentinel.next;
         texture->object != gcSENTINELTEXTURE;
         texture = texture->next)
    {
        /* See if we found the texture. */
        if (predictedName < texture->name)
        {
            break;
        }

        /* Predict the name of the next texture. */
        predictedName = texture->name + 1;

        /* Check for overflow. */
        if (predictedName == 0)
        {
            gcmFOOTER_ARG("result=0x%x", gcvNULL);
            /* Wow! We actually did overflow... */
            return gcvNULL;
        }
    }

    gcmFOOTER_ARG("result=0x%x", texture->prev);

    /* Return result. */
    return texture->prev;
}

/*******************************************************************************
**
**  glfLoadTexture / glfUnloadTexture
**
**  Texture state and binding/unbinding functions.
**
**  INPUT:
**
**      Context
**          Pointer to the glsCONTEXT structure.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS glfLoadTexture(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    static gceTEXTURE_FILTER halMipFilter[] =
    {
        gcvTEXTURE_NONE,    /* glvNEAREST */
        gcvTEXTURE_NONE,    /* glvLINEAR */
        gcvTEXTURE_POINT,   /* glvNEAREST_MIPMAP_NEAREST */
        gcvTEXTURE_POINT,   /* glvLINEAR_MIPMAP_NEAREST */
        gcvTEXTURE_LINEAR,  /* glvNEAREST_MIPMAP_LINEAR */
        gcvTEXTURE_LINEAR,  /* glvLINEAR_MIPMAP_LINEAR */
    };

    static gceTEXTURE_FILTER halMinFilter[] =
    {
        gcvTEXTURE_POINT,   /* glvNEAREST */
        gcvTEXTURE_LINEAR,  /* glvLINEAR */
        gcvTEXTURE_POINT,   /* glvNEAREST_MIPMAP_NEAREST */
        gcvTEXTURE_LINEAR,  /* glvLINEAR_MIPMAP_NEAREST */
        gcvTEXTURE_POINT,   /* glvNEAREST_MIPMAP_LINEAR */
        gcvTEXTURE_LINEAR,  /* glvLINEAR_MIPMAP_LINEAR */
    };

    static gceTEXTURE_FILTER halMagFilter[] =
    {
        gcvTEXTURE_POINT,   /* glvNEAREST */
        gcvTEXTURE_LINEAR,  /* glvLINEAR */
    };

    static gceTEXTURE_ADDRESSING halWrap[] =
    {
        gcvTEXTURE_CLAMP,   /* glvCLAMP */
        gcvTEXTURE_WRAP,    /* glvREPEAT */
        gcvTEXTURE_MIRROR,  /* glvMIRROR */
    };

    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        gctINT i;

        /* Make a shortcut to the texture attribute array. */
        glsUNIFORMWRAP_PTR* attrTexture = Context->currProgram->fs.texture;

        /* Iterate though the attributes. */
        for (i = 0; i < glvMAX_TEXTURES; i++)
        {
            gctUINT32 samplerNumber;
            glsTEXTURESAMPLER_PTR sampler;
            glsTEXTUREWRAPPER_PTR texture;
            gcsTEXTURE halTexture;

            /* Skip if the texture is not used. */
            if (attrTexture[i] == gcvNULL)
            {
                continue;
            }

            /* Get a shortcut to the current sampler. */
            sampler = &Context->texture.sampler[i];

            /* Make sure the stage is valid. */
            gcmASSERT(sampler->stageEnabled);
            gcmASSERT(sampler->binding != gcvNULL);
            gcmASSERT(sampler->binding->object != gcvNULL);

            /* Get a shortcut to the current sampler's bound texture. */
            texture = sampler->binding;


            /* Flush texture cache. */
            if (texture->dirty)
            {
                gcmERR_BREAK(gcoTEXTURE_Flush(texture->object));
                texture->dirty = gcvFALSE;
            }


            /* Program states. */
            halTexture.s = halWrap[texture->wrapS];
            halTexture.t = halWrap[texture->wrapT];
            halTexture.r = gcvTEXTURE_WRAP;


            /* Use same logic for all platforms*/
            if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_NON_POWER_OF_TWO) != gcvSTATUS_TRUE)
            {
                /* Set addressing mode to clamp for npot texture temp. */
                if ( (texture->width  & (texture->width  - 1))
                ||   (texture->height & (texture->height - 1))
                )
                {
                    halTexture.s = gcvTEXTURE_CLAMP;
                    halTexture.t = gcvTEXTURE_CLAMP;
                }
            }


            halTexture.border[0]  = 0;
            halTexture.border[1]  = 0;
            halTexture.border[2]  = 0;
            halTexture.border[3]  = 0;

            halTexture.minFilter = halMinFilter[texture->minFilter];
            halTexture.magFilter = halMagFilter[texture->magFilter];
            halTexture.mipFilter = halMipFilter[texture->minFilter];
            halTexture.anisoFilter = texture->anisoFilter;

            halTexture.lodMin  = 0;
            halTexture.lodMax  = texture->maxLOD << 16;
            halTexture.lodBias = (gctFIXED_POINT)(Context->texture.activeSampler->lodBias);

            /* Get the sampler number. */
            gcmERR_BREAK(gcUNIFORM_GetSampler(
                attrTexture[i]->uniform,
                &samplerNumber
                ));

            /* Bind to the sampler. */
            gcmERR_BREAK(gcoTEXTURE_BindTexture(
                texture->object,
                0,
                samplerNumber,
                &halTexture
                ));
        }
    }
    while (GL_FALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}

gceSTATUS glfUnloadTexture(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    GLint i;

    /* Make a shortcut to the texture attribute array. */
    glsUNIFORMWRAP_PTR* attrTexture = Context->currProgram->fs.texture;

    gcmHEADER_ARG("Context=0x%x", Context);

    /* Iterate though the attributes. */
    for (i = 0; i < glvMAX_TEXTURES; i++)
    {
        /* Skip if the texture is not used. */
        if (attrTexture[i] != gcvNULL)
        {
            glsTEXTURESAMPLER_PTR sampler;
            glsTEXTUREWRAPPER_PTR texture;

            /* Get a shortcut to the current sampler. */
            sampler = &Context->texture.sampler[i];

            /* Make sure the stage is valid. */
            gcmASSERT(sampler->stageEnabled);
            gcmASSERT(sampler->binding != gcvNULL);
            gcmASSERT(sampler->binding->object != gcvNULL);

            /* Get a shortcut to the current sampler's bound texture. */
            texture = sampler->binding;

            /* Unbind the texture. */
            gcmERR_BREAK(gcoTEXTURE_BindTexture(texture->object, 0, -1, gcvNULL));
        }
    }

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _SetTextureParameter/_GetTextureParameter
**
**  Texture parameter access.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Target
**          Specifies the target texture.
**
**      Name
**          Specifies the symbolic name of a single-valued texture parameter.
**
**      Value
**          Points to the value of 'Name'.
**
**      Type
**          Type of the value.
**
**  OUTPUT:
**
**      Nothing.
*/

static GLboolean _SetTextureParameter(
    glsCONTEXT_PTR Context,
    GLenum Target,
    GLenum Name,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    gcmHEADER_ARG("Context=0x%x Target=0x%04x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Target, Name, Value, Type);

    do
    {
        gleTARGETTYPE target;
        glsTEXTUREWRAPPER_PTR texture;
        GLuint value;

        /* Determine the target. */
        if (Target == GL_TEXTURE_2D)
        {
            /* 2D target. */
            target = glvTEXTURE2D;
        }
        else if (Target == GL_TEXTURE_CUBE_MAP_OES)
        {
            /* Cubemap target. */
            target = glvCUBEMAP;
        }
        else if (Target == GL_TEXTURE_EXTERNAL_OES)
        {
            target = glvTEXTUREEXTERNAL;
        }
        else
        {
            /* Invalid target. */
            result = GL_FALSE;
            break;
        }

        /* Get a shortcut to the active texture. */
        texture = Context->texture.activeSampler->bindings[target];
        gcmASSERT(texture != gcvNULL);

        /* Dispatch. */
        switch (Name)
        {
        case GL_TEXTURE_MIN_FILTER:
            if ((result = glfConvertGLEnum(
                    _TextureMinFilterNames,
                    gcmCOUNTOF(_TextureMinFilterNames),
                    Value, Type,
                    &value
                    )) != GL_FALSE)
            {
                texture->minFilter = (gleTEXTUREFILTER) value;
            }
            break;

        case GL_TEXTURE_MAG_FILTER:
            if ((result = glfConvertGLEnum(
                    _TextureMagFilterNames,
                    gcmCOUNTOF(_TextureMagFilterNames),
                    Value, Type,
                    &value
                    )) != GL_FALSE)
            {
                texture->magFilter = (gleTEXTUREFILTER) value;
            }
            break;

        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            {
                /* Convert the enum. */
                GLint value = (Type == glvFLOAT)
                    ? glmFLOAT2INT(* (GLfloat *) Value)
                    :             (* (GLint   *) Value);

                if (value < 1)
                {
                    result = GL_FALSE;
                }
                else
                {
                    /* Clamp the value to maxAniso supported. */
                    value = gcmMIN(value, (GLint)Context->maxAniso);

                    texture->anisoFilter = value;

                    result = GL_TRUE;
                }
            }
            break;

        case GL_TEXTURE_WRAP_S:
            if ((result = glfConvertGLEnum(
                    _TextureWrapNames,
                    gcmCOUNTOF(_TextureWrapNames),
                    Value, Type,
                    &value
                    )) != GL_FALSE)
            {
                texture->wrapS = (gleTEXTUREWRAP) value;
            }
            break;

        case GL_TEXTURE_WRAP_T:
            if ((result = glfConvertGLEnum(
                    _TextureWrapNames,
                    gcmCOUNTOF(_TextureWrapNames),
                    Value, Type,
                    &value
                    )) != GL_FALSE)
            {
                texture->wrapT = (gleTEXTUREWRAP) value;
            }
            break;

        case GL_GENERATE_MIPMAP:
            if ((result = glfConvertGLboolean(
                    Value, Type,
                    &value
                    )) != GL_FALSE)
            {
                texture->genMipmap = value != 0;
            }
            break;

        case GL_TEXTURE_CROP_RECT_OES:
            glfGetFromMutableArray(
                (gluMUTABLE_PTR) Value,
                Type,
                4,
                &texture->cropRect,
                glvINT
                );

            /* Invalidate normalized crop rectangle. */
            texture->dirtyCropRect = GL_TRUE;

            /* Success. */
            result = GL_TRUE;
            break;

        case GL_TEXTURE_MAX_LEVEL_APPLE:
            {
                GLint maxLevel;

                glfGetFromMutableArray(
                    (gluMUTABLE_PTR) Value,
                    Type,
                    1,
                    &maxLevel,
                    glvINT
                );

                if (maxLevel > 0)
                {
                    GLint tValue;
                    tValue = glfGetMaxLOD(texture->width, texture->height);
                    texture->maxLevel = gcmMIN(tValue, maxLevel);
                    result = GL_TRUE;
                }
                else
                {
                    result = GL_FALSE;
                }
                break;
            }

        default:
            result = GL_FALSE;
        }
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("result=%d", result);
    /* Return result. */
    return result;
}

static GLboolean _GetTextureParameter(
    glsCONTEXT_PTR Context,
    GLenum Target,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result = GL_TRUE;
    gcmHEADER_ARG("Context=0x%x Target=0x%04x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Target, Name, Value, Type);

    do
    {
        gleTARGETTYPE target;
        glsTEXTUREWRAPPER_PTR texture;

        /* Determine the target. */
        if (Target == GL_TEXTURE_2D)
        {
            /* 2D target. */
            target = glvTEXTURE2D;
        }
        else if (Target == GL_TEXTURE_CUBE_MAP_OES)
        {
            /* Cubemap target. */
            target = glvCUBEMAP;
        }
        else if (Target == GL_TEXTURE_EXTERNAL_OES)
        {
            target = glvTEXTUREEXTERNAL;
        }
        else
        {
            /* Invalid target. */
            result = GL_FALSE;
            break;
        }

        /* Get a shortcut to the active texture. */
        texture = Context->texture.activeSampler->bindings[target];
        gcmASSERT(texture != gcvNULL);

        /* Dispatch. */
        switch (Name)
        {
        case GL_TEXTURE_MIN_FILTER:
            glfGetFromEnum(
                _TextureMinFilterNames[texture->minFilter],
                Value,
                Type
                );
            break;

        case GL_TEXTURE_MAG_FILTER:
            glfGetFromEnum(
                _TextureMagFilterNames[texture->magFilter],
                Value,
                Type
                );
            break;

        case GL_TEXTURE_WRAP_S:
            glfGetFromEnum(
                _TextureWrapNames[texture->wrapS],
                Value,
                Type
                );
            break;

        case GL_TEXTURE_WRAP_T:
            glfGetFromEnum(
                _TextureWrapNames[texture->wrapT],
                Value,
                Type
                );
            break;

        case GL_GENERATE_MIPMAP:
            glfGetFromInt(
                texture->genMipmap,
                Value,
                Type
                );
            break;

        case GL_TEXTURE_CROP_RECT_OES:
            glfGetFromIntArray(
                (const GLint*) &texture->cropRect,
                4,
                Value,
                Type
                );
            break;

        case GL_TEXTURE_MAX_LEVEL_APPLE:
            glfGetFromInt(
                texture->maxLevel,
                Value,
                Type
                );
            break;

        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            glfGetFromInt(
                (GLint)texture->anisoFilter,
                Value,
                Type
                );
            break;

        default:
            result = GL_FALSE;
        }
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("result=%d", result);
    /* Return result. */
    return result;
}


/*******************************************************************************
**
**  _SetTextureEnvironment/_GetTextureEnvironment
**
**  Texture environment state access.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Name
**          Specifies the symbolic name of the state to set/get.
**
**      Value
**          Points to the data.
**
**      Type
**          Data format.
**
**  OUTPUT:
**
**      Nothing.
*/

static GLboolean _SetTextureEnvironment(
    glsCONTEXT_PTR Context,
    GLenum Name,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    /* Get a shortcut to the active sampler. */
    glsTEXTURESAMPLER_PTR sampler = Context->texture.activeSampler;
    GLboolean result;
    gcmHEADER_ARG("Context=0x%x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Name, Value, Type);
    gcmASSERT(sampler != gcvNULL);

    /* Set parameter. */
    switch (Name)
    {
    case GL_TEXTURE_ENV_MODE:
        result = _SetTextureFunction(Context, sampler, Value, Type);
        break;

    case GL_COMBINE_RGB:
        result = _SetCombineColorFunction(Context, sampler, Value, Type);
        break;

    case GL_COMBINE_ALPHA:
        result = _SetCombineAlphaFunction(Context, sampler, Value, Type);
        break;

    case GL_SRC0_RGB:
    case GL_SRC1_RGB:
    case GL_SRC2_RGB:
        result = _SetCombineColorSource(Context, Name, sampler, Value, Type);
        break;

    case GL_SRC0_ALPHA:
    case GL_SRC1_ALPHA:
    case GL_SRC2_ALPHA:
        result = _SetCombineAlphaSource(Context, Name, sampler, Value, Type);
        break;

    case GL_OPERAND0_RGB:
    case GL_OPERAND1_RGB:
    case GL_OPERAND2_RGB:
        result = _SetCombineColorOperand(Context, Name, sampler, Value, Type);
        break;

    case GL_OPERAND0_ALPHA:
    case GL_OPERAND1_ALPHA:
    case GL_OPERAND2_ALPHA:
        result = _SetCombineAlphaOperand(Context, Name, sampler, Value, Type);
        break;

    case GL_TEXTURE_ENV_COLOR:
        result = _SetCurrentColor(Context, sampler, Value, Type);
        break;

    case GL_RGB_SCALE:
        result = _SetColorScale(Context, sampler, Value, Type);
        break;

    case GL_ALPHA_SCALE:
        result = _SetAlphaScale(Context, sampler, Value, Type);
        break;
    default:
        result = GL_FALSE;
        break;
    }

    gcmFOOTER_ARG("result=%d", result);
    /* Invalid enum. */
    return result;
}

static GLboolean _GetTextureEnvironment(
    glsCONTEXT_PTR Context,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result = GL_TRUE;

    /* Get a shortcut to the active sampler. */
    glsTEXTURESAMPLER_PTR sampler = Context->texture.activeSampler;
    gcmHEADER_ARG("Context=0x%x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Name, Value, Type);
    gcmASSERT(sampler != gcvNULL);

    switch (Name)
    {
    case GL_TEXTURE_ENV_MODE:
        glfGetFromEnum(
            _TextureFunctionNames[sampler->function],
            Value,
            Type
            );
        break;

    case GL_COMBINE_RGB:
        glfGetFromEnum(
            _CombineColorTextureFunctionNames[sampler->combColor.function],
            Value,
            Type
            );
        break;

    case GL_COMBINE_ALPHA:
        glfGetFromEnum(
            _CombineAlphaTextureFunctionNames[sampler->combAlpha.function],
            Value,
            Type
            );
        break;

    case GL_SRC0_RGB:
        glfGetFromEnum(
            _CombineFunctionSourceNames[sampler->combColor.source[0]],
            Value,
            Type
            );
        break;

    case GL_SRC1_RGB:
        glfGetFromEnum(
            _CombineFunctionSourceNames[sampler->combColor.source[1]],
            Value,
            Type
            );
        break;

    case GL_SRC2_RGB:
        glfGetFromEnum(
            _CombineFunctionSourceNames[sampler->combColor.source[2]],
            Value,
            Type
            );
        break;

    case GL_SRC0_ALPHA:
        glfGetFromEnum(
            _CombineFunctionSourceNames[sampler->combAlpha.source[0]],
            Value,
            Type
            );
        break;

    case GL_SRC1_ALPHA:
        glfGetFromEnum(
            _CombineFunctionSourceNames[sampler->combAlpha.source[1]],
            Value,
            Type
            );
        break;

    case GL_SRC2_ALPHA:
        glfGetFromEnum(
            _CombineFunctionSourceNames[sampler->combAlpha.source[2]],
            Value,
            Type
            );
        break;

    case GL_OPERAND0_RGB:
        glfGetFromEnum(
            _CombineFunctionColorOperandNames[sampler->combColor.operand[0]],
            Value,
            Type
            );
        break;

    case GL_OPERAND1_RGB:
        glfGetFromEnum(
            _CombineFunctionColorOperandNames[sampler->combColor.operand[1]],
            Value,
            Type
            );
        break;

    case GL_OPERAND2_RGB:
        glfGetFromEnum(
            _CombineFunctionColorOperandNames[sampler->combColor.operand[2]],
            Value,
            Type
            );
        break;

    case GL_OPERAND0_ALPHA:
        glfGetFromEnum(
            _CombineFunctionAlphaOperandNames[sampler->combAlpha.operand[0]],
            Value,
            Type
            );
        break;

    case GL_OPERAND1_ALPHA:
        glfGetFromEnum(
            _CombineFunctionAlphaOperandNames[sampler->combAlpha.operand[1]],
            Value,
            Type
            );
        break;

    case GL_OPERAND2_ALPHA:
        glfGetFromEnum(
            _CombineFunctionAlphaOperandNames[sampler->combAlpha.operand[2]],
            Value,
            Type
            );
        break;

    case GL_TEXTURE_ENV_COLOR:
        glfGetFromVector4(
            &sampler->constColor,
            Value,
            Type
            );
        break;

    case GL_RGB_SCALE:
        glfGetFromMutant(
            &sampler->combColor.scale,
            Value,
            Type
            );
        break;

    case GL_ALPHA_SCALE:
        glfGetFromMutant(
            &sampler->combAlpha.scale,
            Value,
            Type
            );
        break;

    default:
        result = GL_FALSE;
    }

    gcmFOOTER_ARG("result=%d", result);
    /* Return result. */
    return result;
}


/*******************************************************************************
**
**  _SetTextureState/_GetTextureState
**
**  Texture state access main entry points.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Target
**          Specifies a texture environment.
**
**      Name
**          Specifies the symbolic name of the state to set/get.
**
**      Value
**          Points to the data.
**
**      Type
**          Data format.
**
**  OUTPUT:
**
**      Nothing.
*/

static GLboolean _SetTextureState(
    glsCONTEXT_PTR Context,
    GLenum Target,
    GLenum Name,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    gcmHEADER_ARG("Context=0x%x Target=0x%04x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Target, Name, Value, Type);
    switch (Target)
    {
    case GL_TEXTURE_ENV:
        result = _SetTextureEnvironment(
            Context,
            Name,
            (gluMUTABLE_PTR) Value,
            Type
            );
        gcmFOOTER_ARG("result=%d", result);
        return result;
        break;

    case GL_POINT_SPRITE_OES:
        {
            GLuint value;
            if ((Name == GL_COORD_REPLACE_OES) &&
                glfConvertGLboolean(Value, Type, &value))
            {
                /* Invalidate point sprite state. */
                Context->pointStates.spriteDirty = GL_TRUE;

                /* Set coordinate replacement mode for the unit. */
                Context->texture.activeSampler->coordReplace = value != 0;
                gcmFOOTER_ARG("result=%d", GL_TRUE);
                return GL_TRUE;
            }
        }
        break;
    case GL_TEXTURE_FILTER_CONTROL_EXT:
        if (Name == GL_TEXTURE_LOD_BIAS_EXT)
        {
            Context->texture.activeSampler->lodBias = * (GLfloat *) Value;
            gcmFOOTER_ARG("result=%d", GL_TRUE);
            return GL_TRUE;
        }
        break;
    }

    gcmFOOTER_ARG("result=%d", GL_FALSE);
    return GL_FALSE;
}

static GLboolean _GetTextureState(
    glsCONTEXT_PTR Context,
    GLenum Target,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    gcmHEADER_ARG("Context=0x%x Target=0x%04x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Target, Name, Value, Type);
    switch (Target)
    {
    case GL_TEXTURE_ENV:
        result = _GetTextureEnvironment(
            Context,
            Name,
            Value,
            Type
            );
        gcmFOOTER_ARG("result=%d", result);
        return result;

    case GL_POINT_SPRITE_OES:
        if (Name == GL_COORD_REPLACE_OES)
        {
            glfGetFromInt(
                Context->texture.activeSampler->coordReplace,
                Value,
                Type
                );

            gcmFOOTER_ARG("result=%d", GL_TRUE);
            return GL_TRUE;
        }
        break;
    case GL_TEXTURE_FILTER_CONTROL_EXT:
        if (Name == GL_TEXTURE_LOD_BIAS_EXT)
        {
            glfGetFromMutableArray(
                (gluMUTABLE_PTR) (&(Context->texture.activeSampler->lodBias)),
                Type,
                1,
                Value,
                glvFLOAT
                );
            gcmFOOTER_ARG("result=%d", GL_TRUE);
            return GL_TRUE;
        }
        break;
    }

    gcmFOOTER_ARG("result=%d", GL_FALSE);
    return GL_FALSE;
}


/*******************************************************************************
**
**  _SetTexGen/_GetTexGen (GL_OES_texture_cube_map)
**
**  Texture coordinate generation mode.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Coord
**          Specifies the type of coordinate.
**
**      Name
**          Specifies the symbolic name of the state to set/get.
**
**      Value
**          Points to the data.
**
**      Type
**          Data format.
**
**  OUTPUT:
**
**      Nothing.
*/

static GLboolean _SetTexGen(
    glsCONTEXT_PTR Context,
    GLenum Coord,
    GLenum Name,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result;
    gcmHEADER_ARG("Context=0x%x Coord=0x%04x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Coord, Name, Value, Type);

    do
    {
        glsTEXTURESAMPLER_PTR sampler;

        /* Verify the target coordinate. */
        if (Coord != GL_TEXTURE_GEN_STR_OES)
        {
            result = GL_FALSE;
            break;
        }

        /* Verify the coordinate mode. */
        if (Name != GL_TEXTURE_GEN_MODE_OES)
        {
            result = GL_FALSE;
            break;
        }

        /* Get a shortcut to the active sampler. */
        sampler = Context->texture.activeSampler;
        gcmASSERT(sampler != gcvNULL);

        /* Set texture coordinate generation mode. */
        result = _SetTexCoordGenMode(
            Context, sampler, (gluMUTABLE_PTR) Value, Type
            );
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("result=%d", result);
    /* Return result. */
    return result;
}

static GLboolean _GetTexGen(
    glsCONTEXT_PTR Context,
    GLenum Coord,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result = GL_TRUE;
    gcmHEADER_ARG("Context=0x%x Coord=0x%04x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Coord, Name, Value, Type);

    do
    {
        glsTEXTURESAMPLER_PTR sampler;

        /* Verify the target coordinate. */
        if (Coord != GL_TEXTURE_GEN_STR_OES)
        {
            result = GL_FALSE;
            break;
        }

        /* Verify the coordinate mode. */
        if (Name != GL_TEXTURE_GEN_MODE_OES)
        {
            result = GL_FALSE;
            break;
        }

        /* Get a shortcut to the active sampler. */
        sampler = Context->texture.activeSampler;
        gcmASSERT(sampler != gcvNULL);

        /* Convert the value. */
        glfGetFromEnum(
            _TextureGenModes[sampler->genMode],
            Value,
            Type
            );
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("result=%d", result);
    /* Return result. */
    return result;
}


/*******************************************************************************
**
**  glfEnableTexturing
**
**  Enable texturing for the active sampler.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Enable
**          Enable flag.
**
**  OUTPUT:
**
**      Nothing.
*/
GLenum glfEnableTexturing(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    /* Make a shortcut to the sampler. */
    glsTEXTURESAMPLER_PTR sampler = Context->texture.activeSampler;
    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);

    /* Set texturing enable state. */
    sampler->enableTexturing = Enable;
    if (Enable)
    {
        sampler->binding = sampler->bindings[glvTEXTURE2D];
    }

    gcmFOOTER_ARG("result=0x%04x", GL_NO_ERROR);
    /* Success. */
    return GL_NO_ERROR;
}


/*******************************************************************************
**
**  glfEnableCubeTexturing
**
**  Enable cubemap texturing for the active sampler.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Enable
**          Enable flag.
**
**  OUTPUT:
**
**      Nothing.
*/

GLenum glfEnableCubeTexturing(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    /* Make a shortcut to the sampler. */
    glsTEXTURESAMPLER_PTR sampler = Context->texture.activeSampler;

    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);

    /* Set texturing enable state. */
    sampler->enableCubeTexturing = Enable;

    /* Update texture binding. */
    gcmASSERT((Enable == 0) || (Enable == 1));
    if (Enable)
    {
        sampler->binding = sampler->bindings[glvCUBEMAP];
    }

    gcmFOOTER_ARG("result=0x%04x", GL_NO_ERROR);
    /* Success. */
    return GL_NO_ERROR;
}

/*******************************************************************************
**
**  glfEnableExternalTexturing
**
**  Enable Image external texturing for the active sampler.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Enable
**          Enable flag.
**
**  OUTPUT:
**
**      Nothing.
*/

GLenum glfEnableExternalTexturing(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    /* Make a shortcut to the sampler. */
    glsTEXTURESAMPLER_PTR sampler = Context->texture.activeSampler;

    /* Set texturing enable state. */
    sampler->enableExternalTexturing = Enable;
    if (Enable)
    {
        sampler->binding = sampler->bindings[glvTEXTUREEXTERNAL];
    }

    /* Success. */
    return GL_NO_ERROR;
}


/*******************************************************************************
**
**  glfEnableCoordGen
**
**  Enable texture coordinate generation for the active sampler.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Enable
**          Enable flag.
**
**  OUTPUT:
**
**      Nothing.
*/

GLenum glfEnableCoordGen(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    /* Make a shortcut to the sampler. */
    glsTEXTURESAMPLER_PTR sampler = Context->texture.activeSampler;

    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);

    /* Update the state. */
    sampler->genEnable = Enable;

    /* Update the hash key. */
    glmSETHASH_1BIT(hashTexCubeCoordGenEnable, Enable, sampler->index);

    gcmFOOTER_ARG("result=0x%04x", GL_NO_ERROR);
    /* Success. */
    return GL_NO_ERROR;
}


/*******************************************************************************
**
**  glfUpdateTextureStates
**
**  Update texture states.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Nothing.
**
*/

gceSTATUS glfUpdateTextureStates(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    GLboolean coordReplace = GL_FALSE;
    GLint hashComponents;
    GLint i;

    gcmHEADER_ARG("Context=0x%x", Context);

    for (i = 0; i < Context->texture.pixelSamplers; i++)
    {
        glsTEXTURESAMPLER_PTR sampler = &Context->texture.sampler[i];

#if defined(ANDROID)
        if ((sampler->enableTexturing
          || sampler->enableExternalTexturing
          || sampler->enableCubeTexturing)
        &&  (sampler->binding->source != gcvNULL)
        )
        {
            /* Dynamically allocate mipmap level 0 for EGLImage case. */
            /* Get shortcuts. */
            glsTEXTUREWRAPPER_PTR texture = sampler->binding;
            gcoSURF mipmap = gcvNULL;

            struct gc_private_handle_t * handle =
               (struct gc_private_handle_t *) texture->privHandle;

            if (texture->object == gcvNULL)
            {
                gceSURF_FORMAT srcFormat;
                gceSURF_FORMAT dstFormat;

                gctUINT width;
                gctUINT height;

                /* Get source format. */
                if (gcmIS_ERROR(gcoSURF_GetFormat(texture->source,
                                                  gcvNULL,
                                                  &srcFormat)))
                {
                    glmERROR(GL_INVALID_VALUE);
                    break;
                }

                /* Translate to closest texture format. */
                if (gcmIS_ERROR(gcoTEXTURE_GetClosestFormat(Context->hal,
                                                            srcFormat,
                                                            &dstFormat)))
                {
                    glmERROR(GL_INVALID_VALUE);
                    break;
                }

                /* Get source size. */
                if (gcmIS_ERROR(gcoSURF_GetSize(texture->source,
                                                &width,
                                                &height,
                                                gcvNULL)))
                {
                    glmERROR(GL_INVALID_VALUE);
                    break;
                }

                /* Dynamically allocate texture object. */
                status = gcoTEXTURE_Construct(Context->hal, &texture->object);

                if (gcmIS_ERROR(status))
                {
                    glmERROR(GL_OUT_OF_MEMORY);
                    break;
                }

                /* Create mipmap level 0. */
                status = gcoTEXTURE_AddMipMap(
                    texture->object,
                    0,
                    dstFormat,
                    width,
                    height,
                    0,
                    gcvFACE_NONE,
                    gcvPOOL_DEFAULT,
                    &mipmap
                    );

                if (gcmIS_ERROR(status))
                {
                    gcmFATAL("%s: add mipmap fail", __FUNCTION__);

                    /* Destroy the texture. */
                    gcmVERIFY_OK(gcoTEXTURE_Destroy(texture->object));

                    texture->object = gcvNULL;

                    glmERROR(GL_INVALID_VALUE);
                    break;
                }
            }

            if (texture->uploaded == gcvFALSE
            /* Always upload for texture2D target. */
            ||  texture->targetType == glvTEXTURE2D
            )
            {
                if (mipmap == gcvNULL)
                {
                    /* Get mipmap surface. */
                    gcmVERIFY_OK(gcoTEXTURE_GetMipMap(
                        texture->object,
                        0,
                        &mipmap
                        ));
                }

                /* Check composition signal. */
                if (handle != gcvNULL && handle->hwDoneSignal != 0)
                {
                    gcmVERIFY_OK(gcoOS_Signal(
                        gcvNULL,
                        (gctSIGNAL) handle->hwDoneSignal,
                        gcvFALSE
                        ));
                }

                if (Context->dither)
                {
                    /*disable the dither for resolve*/
                    gcmERR_BREAK(gco3D_EnableDither(Context->hw, gcvFALSE));
                }

#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
                gctBOOL    needAdjust = gcvFALSE;

                if ((handle->allocUsage & GRALLOC_USAGE_HW_RENDER) &&
                    (handle->common.pid == Context->pid))
                {
                    gctUINT32  physical[3];
                    gctPOINTER logical[3];
                    gctINT     stride;
                    gctUINT32  height;
                    gctUINT32  alignedHeight;

                    /* Lock linear surface. */
                    gcmERR_BREAK(gcoSURF_Lock(
                        texture->source,
                        physical,
                        logical
                        ));

                    /* Unlock. */
                    gcmERR_BREAK(gcoSURF_Unlock(
                        texture->source,
                        logical[0]
                        ));

                    /* Get height. */
                    gcmERR_BREAK(gcoSURF_GetAlignedSize(
                        texture->source,
                        gcvNULL,
                        &height,
                        gcvNULL
                        ));

                    /* Get aligned height. */
                    gcmERR_BREAK(gcoSURF_GetAlignedSize(
                        texture->source,
                        gcvNULL,
                        &alignedHeight,
                        &stride
                        ));

                    /* Do we need to adjust start address? */
                    needAdjust = (height != alignedHeight);

                    /* Check adjust. */
                    if (needAdjust)
                    {
                        /* Compute adjustment. */
                        gctUINT32 offset = (alignedHeight - height) * stride;

                        /* Now adjust linear resolve address. */
                        gcmERR_BREAK(gcoSURF_SetLinearResolveAddress(
                            texture->source,
                            physical[0] + offset,
                            (gctPOINTER) ((gctUINT32) logical[0] + offset)
                            ));
                    }
                }
#   endif

                /* Upload texture here. */
                status = gcoSURF_Resolve(texture->source, mipmap);

#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
                if (needAdjust)
                {
                    /* Reset resolve address. */
                    gcmERR_BREAK(gcoSURF_SetLinearResolveAddress(
                        texture->source,
                        0U,
                        gcvNULL
                        ));
                }
#   endif

                if (gcmIS_ERROR(status))
                {
                    gcmFATAL("%s: upload texture failed", __FUNCTION__);

                    glmERROR(GL_INVALID_OPERATION);
                    break;
                }

                if (Context->dither)
                {
                    /* restore the dither */
                    gcmERR_BREAK(gco3D_EnableDither(Context->hw, gcvTRUE));
                }

                if (handle != gcvNULL && handle->hwDoneSignal != 0)
                {
                    /* Signal the signal, so CPU apps
                     * can lock again once resolve is done. */
                    gcsHAL_INTERFACE iface;

                    iface.command            = gcvHAL_SIGNAL;
                    iface.u.Signal.signal    = (gctSIGNAL) handle->hwDoneSignal;
                    iface.u.Signal.auxSignal = gcvNULL;
                    /* Stuff the client's PID. */
                    iface.u.Signal.process   = (gctHANDLE) handle->clientPID;
                    iface.u.Signal.fromWhere = gcvKERNEL_PIXEL;

                    /* Schedule the event. */
                    gcmVERIFY_OK(gcoHAL_ScheduleEvent(gcvNULL, &iface));
                }

                /* Wait all the pixels done. */
                gcmVERIFY_OK(gco3D_Semaphore(
                    Context->hw,
                    gcvWHERE_RASTER,
                    gcvWHERE_PIXEL,
                    gcvHOW_SEMAPHORE_STALL
                    ));

                /* Set dirty flag (for later flush). */
                texture->dirty    = gcvTRUE;

                /* Set uploaded flag. */
                texture->uploaded = gcvTRUE;
            }
        }
#endif

        if ((sampler->enableTexturing
          || sampler->enableExternalTexturing
          || sampler->enableCubeTexturing)
        &&  (sampler->binding->direct.source != gcvNULL)
        )
        {
            /* Dynamically allocate mipmap level 0 for EGLImage case. */
            /* Get shortcuts. */
            glsTEXTUREWRAPPER_PTR texture = sampler->binding;

#if defined(ANDROID)
            struct gc_private_handle_t * handle =
               (struct gc_private_handle_t *) texture->privHandle;
#endif

            if (texture->object == gcvNULL)
            {
                gceSURF_FORMAT srcFormat;
                gceSURF_FORMAT dstFormat;

                gctUINT width;
                gctUINT height;

                /* Get source format. */
                if (gcmIS_ERROR(gcoSURF_GetFormat(texture->direct.source,
                                                  gcvNULL,
                                                  &srcFormat)))
                {
                    glmERROR(GL_INVALID_VALUE);
                    break;
                }

                /* Translate format. */
                switch (srcFormat)
                {
                case gcvSURF_YV12:
                case gcvSURF_I420:
                case gcvSURF_NV12:
                case gcvSURF_NV21:
                    dstFormat = gcvSURF_YUY2;
                    break;

                case gcvSURF_YUY2:
                case gcvSURF_UYVY:
                    dstFormat = srcFormat;
                    break;

                default:
                    status = gcvSTATUS_INVALID_ARGUMENT;
                    break;
                }

                if (gcmIS_ERROR(status))
                {
                    glmERROR(GL_INVALID_VALUE);
                    break;
                }

                /* Get source size. */
                if (gcmIS_ERROR(gcoSURF_GetSize(texture->direct.source,
                                                &width,
                                                &height,
                                                gcvNULL)))
                {
                    glmERROR(GL_INVALID_VALUE);
                    break;
                }

                /* Dynamically allocate texture object. */
                status = gcoTEXTURE_Construct(Context->hal, &texture->object);

                if (gcmIS_ERROR(status))
                {
                    glmERROR(GL_OUT_OF_MEMORY);
                    break;
                }

                /* Create mipmap level 0. */
                status = gcoTEXTURE_AddMipMap(
                    texture->object,
                    0,
                    dstFormat,
                    width,
                    height,
                    0,
                    gcvFACE_NONE,
                    gcvPOOL_DEFAULT,
                    &texture->direct.texture[0]
                    );

                if (gcmIS_ERROR(status))
                {
                    gcmFATAL("%s: add mipmap fail", __FUNCTION__);

                    /* Destroy the texture. */
                    gcmVERIFY_OK(gcoTEXTURE_Destroy(texture->object));

                    texture->object = gcvNULL;

                    glmERROR(GL_INVALID_VALUE);
                    break;
                }

                /* Set max lod. */
                texture->direct.maxLod = 0;
            }

            if (texture->uploaded == gcvFALSE
            /* Always upload for texture2D target. */
            ||  texture->targetType == glvTEXTURE2D
            )
            {
                gctUINT lod;

#if defined(ANDROID)
                /* Check composition signal. */
                if (handle != gcvNULL && handle->hwDoneSignal != 0)
                {
                    gcmVERIFY_OK(gcoOS_Signal(
                        gcvNULL,
                        (gctSIGNAL) handle->hwDoneSignal,
                        gcvFALSE
                        ));
                }
#endif

                /* Filter blit to the temporary buffer to create
                   a power of two sized texture. */
                if (texture->direct.temp != gcvNULL)
                {
                    gcmERR_BREAK(gcoSURF_FilterBlit(
                        texture->direct.source,
                        texture->direct.temp,
                        gcvNULL, gcvNULL, gcvNULL
                        ));

                    /* Stall the hardware. */
                    gcmERR_BREAK(gcoHAL_Commit(Context->hal, gcvTRUE));

                    /* Create the first LOD. */
                    gcmERR_BREAK(gcoSURF_Resolve(
                        texture->direct.temp,
                        texture->direct.texture[0]
                        ));
                }
                else
                {
                    gcmERR_BREAK(gcoSURF_Flush(texture->direct.texture[0]));

                    gcmERR_BREAK(gco3D_Semaphore(
                        Context->hw,
                        gcvWHERE_RASTER,
                        gcvWHERE_PIXEL,
                        gcvHOW_SEMAPHORE_STALL
                        ));

                    if (Context->dither)
                    {
                        /*disable the dither for resolve*/
                        gcmERR_BREAK(gco3D_EnableDither(Context->hw, gcvFALSE));
                    }

                    /* Create the first LOD. */
                    gcmERR_BREAK(gcoSURF_Resolve(
                        texture->direct.source,
                        texture->direct.texture[0]
                        ));


                    if (Context->dither)
                    {
                        /* restore the dither */
                        gcmERR_BREAK(gco3D_EnableDither(Context->hw, gcvTRUE));
                    }
                }

                /* Generate LODs. */
                for (lod = 1; lod <= texture->direct.maxLod; lod++)
                {
                    gcmERR_BREAK(gcoSURF_Resample(
                        texture->direct.texture[lod - 1],
                        texture->direct.texture[lod]
                        ));
                }

                /* Check for errors. */
                gcmERR_BREAK(status);

                /* wait all the pixels done */
                gcmERR_BREAK(gco3D_Semaphore(
                    Context->hw,
                    gcvWHERE_RASTER,
                    gcvWHERE_PIXEL,
                    gcvHOW_SEMAPHORE_STALL
                    ));

#if defined(ANDROID)
                if (handle != gcvNULL && handle->hwDoneSignal != 0)
                {
                    /* Signal the signal, so CPU apps
                     * can lock again once resolve is done. */
                    gcsHAL_INTERFACE iface;

                    iface.command            = gcvHAL_SIGNAL;
                    iface.u.Signal.signal    = (gctSIGNAL) handle->hwDoneSignal;
                    iface.u.Signal.auxSignal = gcvNULL;
                    /* Stuff the client's PID. */
                    iface.u.Signal.process   = (gctHANDLE) handle->clientPID;
                    iface.u.Signal.fromWhere = gcvKERNEL_PIXEL;

                    /* Schedule the event. */
                    gcmERR_BREAK(gcoHAL_ScheduleEvent(
                        gcvNULL,
                        &iface
                        ));
                }
#endif

                /* Reset planar dirty flag. */
                texture->direct.dirty = gcvFALSE;

                /* Set uploaded flag. */
                texture->uploaded     = gcvTRUE;
            }
        }

        /* Update stage enable sgtate. */
        _UpdateStageEnable(Context, sampler);

        /* Optimization. */
        if (!sampler->stageEnabled)
        {
            glmCLEARHASH_2BITS(
                hashTexCoordComponentCount,
                i
                );
            continue;
        }

        /* Our hardware currently has only one global coordinate replacement
           state (not per unit as specified in OpenGL ES spec). Up until now
           it has not been a problem. Here we determine whether the state
           has been turned on for any of the enabled samplers and OR them
           into one state for later analysis. */
        if (Context->pointStates.spriteDirty && sampler->stageEnabled)
        {
            coordReplace |= sampler->coordReplace;
        }

        /* Determine the number of components in streamed texture coordinate. */
        if (Context->drawTexOESEnabled)
        {
            /* Always 2 components for DrawTex extension. */
            sampler->coordType    = gcSHADER_FLOAT_X2;
            sampler->coordSwizzle = gcSL_SWIZZLE_XYYY;

            /* Set hash key component count. */
            hashComponents = 2;
        }
        else if (sampler->aTexCoordInfo.streamEnabled)
        {
            /* Copy stream component count. */
            sampler->coordType    = sampler->aTexCoordInfo.varyingType;
            sampler->coordSwizzle = sampler->aTexCoordInfo.varyingSwizzle;

            /* Set hash key component count. */
            hashComponents = sampler->aTexCoordInfo.components;
        }
        else
        {
            /* Constant texture coordinate allways has 4 components. */
            sampler->coordType    = gcSHADER_FLOAT_X4;
            sampler->coordSwizzle = gcSL_SWIZZLE_XYZW;

            /* Set hash key component count. */
            hashComponents = 4;
        }

        /* Update the hash key; only values in 2..4 range are possible,
           use the count reduced by 2 to save (1 * 4) = 4 hash bits. */
        glmSETHASH_2BITS(
            hashTexCoordComponentCount,
            (hashComponents - 2),
            i
            );
    }

    /* Update point sprite state. */
    if (Context->pointStates.spriteDirty)
    {
        Context->pointStates.spriteActive
            =  coordReplace
            && Context->pointStates.pointPrimitive
            && Context->pointStates.spriteEnable;

        if (Context->hwPointSprite != Context->pointStates.spriteActive)
        {
            status = gco3D_SetPointSprite(
                Context->hw,
                Context->hwPointSprite = Context->pointStates.spriteActive
                );
        }

        Context->pointStates.spriteDirty = GL_FALSE;
    }

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  glfQueryTextureState
**
**  Queries texture state values.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Name
**          Specifies the symbolic name of the state to get.
**
**      Type
**          Data format.
**
**  OUTPUT:
**
**      Value
**          Points to the data.
*/

GLboolean glfQueryTextureState(
    glsCONTEXT_PTR Context,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result = GL_TRUE;

    gcmHEADER_ARG("Context=0x%x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Name, Value, Type);

    switch (Name)
    {
    case GL_ACTIVE_TEXTURE:
        glfGetFromEnum(
            GL_TEXTURE0 + Context->texture.activeSamplerIndex,
            Value,
            Type
            );
        break;

    case GL_CLIENT_ACTIVE_TEXTURE:
        glfGetFromEnum(
            GL_TEXTURE0 + Context->texture.activeClientSamplerIndex,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_2D:
        glfGetFromInt(
            Context->texture.activeSampler->enableTexturing,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_CUBE_MAP_OES:
        glfGetFromInt(
            Context->texture.activeSampler->enableCubeTexturing,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_EXTERNAL_OES:
        glfGetFromInt(
            Context->texture.activeSampler->enableExternalTexturing,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_GEN_STR_OES:
        glfGetFromInt(
            Context->texture.activeSampler->genEnable,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_BINDING_2D:
        glfGetFromInt(
            Context->texture.activeSampler->bindings[glvTEXTURE2D]->name,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_BINDING_CUBE_MAP_OES:
        glfGetFromInt(
            Context->texture.activeSampler->bindings[glvCUBEMAP]->name,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_BINDING_EXTERNAL_OES:
        glfGetFromInt(
            Context->texture.activeSampler->bindings[glvTEXTUREEXTERNAL]->name,
            Value,
            Type
            );
        break;

    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
        glfGetFromInt(
            gcmCOUNTOF(_compressedTextures),
            Value,
            Type
            );
        break;

    case GL_COMPRESSED_TEXTURE_FORMATS:
        glfGetFromEnumArray(
            _compressedTextures,
            gcmCOUNTOF(_compressedTextures),
            Value,
            Type
            );
        break;

    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE_OES:
        {
            glfGetFromInt(
                gcmMAX(Context->maxTextureWidth,
                       Context->maxTextureHeight),
                Value,
                Type
                );
        }
        break;

    case GL_MAX_TEXTURE_UNITS:
        glfGetFromInt(
            glvMAX_TEXTURES,
            Value,
            Type
            );
        break;

    case GL_ALPHA_SCALE:
        glfGetFromMutant(
            &Context->texture.activeSampler->combAlpha.scale,
            Value,
            Type
            );
        break;

    case GL_RGB_SCALE:
        glfGetFromMutant(
            &Context->texture.activeSampler->combColor.scale,
            Value,
            Type
            );
        break;

    case GL_GENERATE_MIPMAP_HINT:
        glfGetFromEnum(
            Context->texture.generateMipmapHint,
            Value,
            Type
            );
        break;

    case GL_MAX_TEXTURE_LOD_BIAS_EXT:
        glfGetFromMutableArray(
            (gluMUTABLE_PTR) (&(Context->texture.activeSampler->lodBias)),
            Type,
            1,
            Value,
            glvFLOAT
            );
        break;

    case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
        glfGetFromInt(
            (GLint)Context->maxAniso,
            Value,
            Type
            );
        break;

    default:
        result = GL_FALSE;
    }

    gcmFOOTER_ARG("result=%d", result);

    /* Return result. */
    return result;
}


/*******************************************************************************
**
**  _validateFormat
**
**  Verifies whether the format specified is one of the valid constants.
**
**  INPUT:
**
**      Format
**          Specifies the number of color components in the texture. Must be
**          GL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, or GL_LUMINANCE_ALPHA.
**
**  OUTPUT:
**
**      Returns GL_FALSE for unsupported/invalid formats.
*/

static GLboolean _validateFormat(
    GLenum Format
    )
{
    gcmHEADER_ARG("Format=0x%04x", Format);
    switch (Format)
    {
    case GL_ALPHA:
    case GL_RGB:
    case GL_RGBA:
    case GL_LUMINANCE:
    case GL_LUMINANCE_ALPHA:
    case GL_BGRA_EXT:
    case GL_DEPTH_STENCIL_OES:
        gcmFOOTER_ARG("result=%d", GL_TRUE);
        return GL_TRUE;
        break;
    default:
        gcmFOOTER_ARG("result=%d", GL_FALSE);
        return GL_FALSE;
    }
}


/*******************************************************************************
**
**  _validateType
**
**  Verifies whether the type specified is one of the valid constants.
**
**  INPUT:
**
**      Type
**          Specifies the data type of the pixel data. The following symbolic
**          values are accepted: GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5,
**          GL_UNSIGNED_SHORT_4_4_4_4, and GL_UNSIGNED_SHORT_5_5_5_1.
**
**  OUTPUT:
**
**      Returns GL_FALSE for unsupported/invalid types.
*/

static GLboolean _validateType(
    GLenum Type
    )
{
    gcmHEADER_ARG("Type=0x%04x", Type);
    switch (Type)
    {
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
    case GL_UNSIGNED_INT_24_8_OES:

    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_ETC1_RGB8_OES:
        gcmFOOTER_ARG("result=%d", GL_TRUE);
        return GL_TRUE;

    default:
        gcmFOOTER_ARG("result=%d", GL_FALSE);
        return GL_FALSE;
    }
}


/*******************************************************************************
**
**  _getFormat
**
**  Convert OpenGL texture format into HAL format.
**  The function fails if type is GL_UNSIGNED_SHORT_5_6_5 and format is not
**  GL_RGB. The function also fails if type is one of GL_UNSIGNED_SHORT_4_4_4_4,
**  or GL_UNSIGNED_SHORT_5_5_5_1 and formatis not GL_RGBA.
**
**  INPUT:
**
**      Format
**          Specifies the format of the pixel data. Must be same as
**          internalformat. The following symbolic values are accepted:
**          GL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, and GL_LUMINANCE_ALPHA.
**
**      Type
**          Specifies the data type of the pixel data. The following symbolic
**          values are accepted: GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5,
**          GL_UNSIGNED_SHORT_4_4_4_4, and GL_UNSIGNED_SHORT_5_5_5_1.
**
**      HalFormat
**          HAL format.
**
**  OUTPUT:
**
**      Returns GL_FALSE for unsupported formats.
*/

static GLboolean _getFormat(
    GLenum Format,
    GLenum Type,
    gceSURF_FORMAT* HalFormat
    )
{
    GLboolean result;

    gcmHEADER_ARG("Format=0x%04x Type=0x%04x HalFormat=0x%x",
                    Format, Type, HalFormat);

    /* Assume there is no equivalent. */
    *HalFormat = gcvSURF_UNKNOWN;

    /* Dispatch on the type. */
    switch (Type)
    {
    case GL_UNSIGNED_BYTE:

        /* Dispatch on format. */
        switch (Format)
        {
        case GL_ALPHA:
            *HalFormat = gcvSURF_A8;
            break;

        case GL_RGB:
            *HalFormat = gcvSURF_B8G8R8;
            break;

        case GL_RGBA:
            *HalFormat = gcvSURF_A8B8G8R8;
            break;

        case GL_LUMINANCE:
            *HalFormat = gcvSURF_L8;
            break;

        case GL_LUMINANCE_ALPHA:
            *HalFormat = gcvSURF_A8L8;
            break;

        case GL_BGRA_EXT:
            *HalFormat = gcvSURF_A8R8G8B8;
            break;
        }

        break;

    case GL_UNSIGNED_SHORT_5_6_5:
        if (Format == GL_RGB)
        {
            *HalFormat = gcvSURF_R5G6B5;
        }
        break;

    case GL_UNSIGNED_SHORT_4_4_4_4:
        if (Format == GL_RGBA)
        {
            *HalFormat = gcvSURF_R4G4B4A4;
        }
        break;

    case GL_UNSIGNED_SHORT_5_5_5_1:
        if (Format == GL_RGBA)
        {
            *HalFormat = gcvSURF_R5G5B5A1;
        }
        break;

    case GL_PALETTE4_RGBA4_OES:
    case GL_PALETTE8_RGBA4_OES:
        *HalFormat = gcvSURF_A4R4G4B4;
        break;

    case GL_PALETTE4_RGB5_A1_OES:
    case GL_PALETTE8_RGB5_A1_OES:
        *HalFormat = gcvSURF_A1R5G5B5;
        break;

    case GL_PALETTE4_R5_G6_B5_OES:
    case GL_PALETTE8_R5_G6_B5_OES:
        *HalFormat = gcvSURF_R5G6B5;
        break;

    case GL_PALETTE4_RGB8_OES:
    case GL_PALETTE8_RGB8_OES:
        *HalFormat = gcvSURF_X8R8G8B8;
        break;

    case GL_PALETTE4_RGBA8_OES:
    case GL_PALETTE8_RGBA8_OES:
        *HalFormat = gcvSURF_A8R8G8B8;
        break;

    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        *HalFormat = gcvSURF_DXT1;
        break;

    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        *HalFormat = gcvSURF_DXT3;
        break;

    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        *HalFormat = gcvSURF_DXT5;
        break;

    case GL_ETC1_RGB8_OES:
        *HalFormat = gcvSURF_ETC1;
        break;

    case GL_UNSIGNED_INT_24_8_OES:
        if (Format == GL_DEPTH_STENCIL_OES)
        {
            *HalFormat = gcvSURF_D24S8;
        }
        break;
    }

    result = (*HalFormat == gcvSURF_UNKNOWN) ? GL_FALSE : GL_TRUE;
    gcmFOOTER_ARG("result=%d", result);
    /* Return result. */
    return result;
}

static gceENDIAN_HINT _getEndianHint(
    GLenum Format,
    GLenum Type
    )
{
    gcmHEADER_ARG("Format=0x%04x Type=0x%04x", Format, Type);
    /* Dispatch on the type. */
    switch (Type)
    {
    case GL_UNSIGNED_BYTE:
        gcmFOOTER_ARG("result=0x%04x", gcvENDIAN_NO_SWAP);
        return gcvENDIAN_NO_SWAP;

    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
        gcmFOOTER_ARG("result=0x%04x", gcvENDIAN_SWAP_WORD);
        return gcvENDIAN_SWAP_WORD;

    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_ETC1_RGB8_OES:
        gcmFOOTER_ARG("result=0x%04x", gcvENDIAN_NO_SWAP);
        return gcvENDIAN_NO_SWAP;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("result=0x%04x", gcvENDIAN_NO_SWAP);
        return gcvENDIAN_NO_SWAP;
    }
}

/*******************************************************************************
**
**  _QueryImageBaseFormat
**
**  Get the texture's format of an EGL image.
**
**  INPUT:
**
**      Image
**          Specifies an eglImage.
**
**  OUTPUT
**
**      Format
**          Return format of the texture.
**
*/

static gceSTATUS _QueryImageBaseFormat(
    khrEGL_IMAGE_PTR Image,
    GLenum * Format
   )
{
    gcmHEADER_ARG("Image=0x%x Format=0x%x", Image, Format);
    switch (Image->type)
    {
    case KHR_IMAGE_TEXTURE_2D:
    case KHR_IMAGE_TEXTURE_CUBE:
        *Format = (GLenum) Image->u.texture.format;
        break;

#if defined(ANDROID)
    case KHR_IMAGE_ANDROID_NATIVE_BUFFER:
        {
        switch (((android_native_buffer_t*)Image->u.androidNativeBuffer.native)->format)
            {
            case HAL_PIXEL_FORMAT_RGB_565:
                *Format = GL_RGB;
                break;

            case HAL_PIXEL_FORMAT_RGBA_8888:
            case HAL_PIXEL_FORMAT_RGBX_8888:
            case HAL_PIXEL_FORMAT_RGBA_4444:
            case HAL_PIXEL_FORMAT_RGBA_5551:
                *Format = GL_RGBA;
                break;

            case HAL_PIXEL_FORMAT_BGRA_8888:
                *Format = GL_BGRA_EXT;
                break;

            default:
                *Format = GL_RGBA;
                gcmFOOTER_ARG("result=0x%04x", gcvFALSE);
                return gcvFALSE;
            }
        }
        break;
#endif

    default:
        *Format = GL_RGBA;
        break;
    }

    gcmFOOTER_ARG("result=%s", "gcvSTATUS_OK");
    return gcvSTATUS_OK;
}



/*******************************************************************************
**
**  _GetSourceStride
**
**  Compute the source texture's stride, when doing TexImage2D and TexSubImage2D.
**  The function fails if type is GL_UNSIGNED_SHORT_5_6_5 and format is not
**  GL_RGB. The function also fails if type is one of GL_UNSIGNED_SHORT_4_4_4_4,
**  or GL_UNSIGNED_SHORT_5_5_5_1 and formatis not GL_RGBA.
**
**  INPUT:
**
**      Format
**          Specifies the format of the pixel data. Must be same as
**          internalformat. The following symbolic values are accepted:
**          GL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, and GL_LUMINANCE_ALPHA.
**

**      Type
**          Specifies the data type of the pixel data. The following symbolic
**          values are accepted: GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5,
**          GL_UNSIGNED_SHORT_4_4_4_4, and GL_UNSIGNED_SHORT_5_5_5_1.
**
**
**      Width
**          Specifies the width of texture in user buffer, in pixels.
**
**      UnpackAlignment
**          Specifies the unpack aligment of texture in the user buffer, in GLbytes
**
**      Stride
**          The stride of texture in the user buffer, in GLbytes
**
**  OUTPUT:
**
**      Returns GL_FALSE for unsupported formats.
*/

static GLboolean _GetSourceStride(
    GLenum  Format,
    GLenum  Type,
    GLsizei Width,
    GLint   UnpackAlignment,
    gctINT* Stride
    )
{
    GLint pixelSize = -1;
    gcmHEADER_ARG("Format=0x%04x Type=0x%04x Width=%d UnpackAlignment=%d Stride=0x%x",
                    Format, Type, Width, UnpackAlignment, Stride);

    switch (Type)
    {
    case GL_UNSIGNED_BYTE:

        switch (Format)
        {
        case GL_ALPHA:
        case GL_LUMINANCE:
            pixelSize = 1;
            break;

        case GL_LUMINANCE_ALPHA:
            pixelSize = 2;
            break;

        case GL_RGB:
            pixelSize = 3;
            break;

        case GL_RGBA:
        case GL_BGRA_EXT:
            pixelSize = 4;
            break;

        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_ETC1_RGB8_OES:
            pixelSize = 0;
            break;
        }

        break;

    case GL_UNSIGNED_SHORT_5_6_5:
    case GL_UNSIGNED_SHORT_4_4_4_4:
    case GL_UNSIGNED_SHORT_5_5_5_1:
        pixelSize = 2;
        break;
    case GL_UNSIGNED_INT_24_8_OES:
        pixelSize = 4;
        break;
    }

    if (pixelSize != -1)
    {
        *Stride
            = (Width * pixelSize + UnpackAlignment - 1)
            & (~(UnpackAlignment-1));

        gcmFOOTER_ARG("result=%d", GL_TRUE);
        return GL_TRUE;
    }

    gcmFOOTER_ARG("result=%d", GL_FALSE);
    return GL_FALSE;
}

/*******************************************************************************
**
**  _DrawTexOES
**
**  Main entry function for GL_OES_draw_texture extension support.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Xs, Ys, Zs
**          Position of the affected screen rectangle.
**
**      Ws, Hs
**          The width and height of the affected screen rectangle in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/

static GLboolean _DrawTexOES(
    glsCONTEXT_PTR Context,
    gltFRACTYPE Xs,
    gltFRACTYPE Ys,
    gltFRACTYPE Zs,
    gltFRACTYPE Ws,
    gltFRACTYPE Hs
    )
{
    /* Define result. */
    GLboolean result = GL_TRUE;

    do
    {
        GLint i;

        /* Viewport bounding box size. */
        gltFRACTYPE widthViewport, heightViewport;

        /* Normalized coordinates. */
        gltFRACTYPE normLeft, normTop, normWidth, normHeight;

        /* Rectangle coordinates. */
        gltFRACTYPE left, top, right, bottom, width, height;

        /* Define depth. */
        gltFRACTYPE depth;

        /* 4 vertices for two triangles with three coordinates each. */
        gltFRACTYPE vertexBuffer[4 * 3];

        /* Validate the render area size. */
        if ((Ws <= glvFRACZERO) || (Hs <= glvFRACZERO))
        {
            result = GL_FALSE;
            break;
        }

        /* Determine the viewport bounding box size. */
        widthViewport  = glmINT2FRAC(Context->viewportStates.viewportBox[2]);
        heightViewport = glmINT2FRAC(Context->viewportStates.viewportBox[3]);

        /* Normalize coordinates. */
        normLeft   = glmFRACDIVIDE(Xs, widthViewport);
        normTop    = glmFRACDIVIDE(Ys, heightViewport);
        normWidth  = glmFRACDIVIDE(Ws, widthViewport);
        normHeight = glmFRACDIVIDE(Hs, heightViewport);

        /* Transform coordinates. */
        left   = glmFRACMULTIPLY(normLeft,   glvFRACTWO) - glvFRACONE;
        top    = glmFRACMULTIPLY(normTop,    glvFRACTWO) - glvFRACONE;
        right  = glmFRACMULTIPLY(normWidth,  glvFRACTWO) + left;
        bottom = glmFRACMULTIPLY(normHeight, glvFRACTWO) + top;

        /* Compute the depth value. */
        if (Zs <= glvFRACZERO)
        {
            /* Get the near value. */
            depth = glfFracFromMutant(&Context->depthStates.depthRange[0]);
        }

        else if (Zs >= glvFRACONE)
        {
            /* Get the far value. */
            depth = glfFracFromMutant(&Context->depthStates.depthRange[1]);
        }

        else
        {
            gltFRACTYPE zNear = glfFracFromMutant(
                &Context->depthStates.depthRange[0]
                );

            gltFRACTYPE zFar  = glfFracFromMutant(
                &Context->depthStates.depthRange[1]
                );

            depth = zNear + glmFRACMULTIPLY(Zs, zFar - zNear);
        }

        /* Convert to Vivante space. */
        if (Context->chipModel < gcv1000 && Context->chipModel != gcv880)
        {
            depth = glmFRACMULTIPLY(depth + glvFRACONE, glvFRACHALF);
        }

        /* Create two triangles. */
        vertexBuffer[ 0] = left;
        vertexBuffer[ 1] = top;
        vertexBuffer[ 2] = depth;

        vertexBuffer[ 3] = right;
        vertexBuffer[ 4] = top;
        vertexBuffer[ 5] = depth;

        vertexBuffer[ 6] = right;
        vertexBuffer[ 7] = bottom;
        vertexBuffer[ 8] = depth;

        vertexBuffer[ 9] = left;
        vertexBuffer[10] = bottom;
        vertexBuffer[11] = depth;

        /* Update the extension flags. */
        Context->hashKey.hashDrawTextureOES = 1;
        Context->drawTexOESEnabled = GL_TRUE;

        /* Set stream parameters. */
        glfSetStreamParameters(
            Context,
            &Context->aPositionDrawTexInfo,
            glvFRACGLTYPEENUM,
            3,
            3 * sizeof(gltFRACTYPE),
            gcvFALSE,
            &vertexBuffer,
            gcvNULL, glvTOTALBINDINGS       /* No buffer support here. */
            );

        /* Create texture coordinates as necessary. */
        for (i = 0; i < Context->texture.pixelSamplers; i++)
        {
            glsTEXTURESAMPLER_PTR sampler = &Context->texture.sampler[i];

            /* Update stage enable state. */
            _UpdateStageEnable(Context, sampler);

            if (sampler->stageEnabled)
            {
                /* Make a shortcut to the bound texture. */
                glsTEXTUREWRAPPER_PTR texture = sampler->binding;

                if (texture->dirtyCropRect)
                {
                    gcoSURF surface = gcvNULL;
                    gceORIENTATION orientation = gcvORIENTATION_TOP_BOTTOM;

                    /* Convert texture size to fractional values. */
                    gltFRACTYPE textureWidth  = glmINT2FRAC(texture->width);
                    gltFRACTYPE textureHeight = glmINT2FRAC(texture->height);

                    /* Get texture orientation. */
                    gcmVERIFY_OK(gcoTEXTURE_GetMipMap(texture->object, 0, &surface));
                    gcmVERIFY_OK(gcoSURF_QueryOrientation(surface, &orientation));

                    /* Convert crop rectangle to fractional values. */
                    left   = glmINT2FRAC(texture->cropRect[0]);
                    top    = glmINT2FRAC(texture->cropRect[1]);
                    width  = glmINT2FRAC(texture->cropRect[2]);
                    height = glmINT2FRAC(texture->cropRect[3]);

                    /* Flip bottom-top texture. */
                    if (orientation == gcvORIENTATION_BOTTOM_TOP)
                    {
                        gcmTRACE(gcvLEVEL_VERBOSE, "texture %d is bottom-up", i);

                        top    = textureHeight - top;
                        height = -height;
                    }

                    /* Normalize crop rectangle. */
                    normLeft   = glmFRACDIVIDE(left,   textureWidth);
                    normTop    = glmFRACDIVIDE(top,    textureHeight);
                    normWidth  = glmFRACDIVIDE(width,  textureWidth);
                    normHeight = glmFRACDIVIDE(height, textureHeight);

                    /* Construct two triangles. */
                    texture->texCoordBuffer[0] = normLeft;
                    texture->texCoordBuffer[1] = normTop;

                    texture->texCoordBuffer[2] = normLeft + normWidth;
                    texture->texCoordBuffer[3] = normTop;

                    texture->texCoordBuffer[4] = normLeft + normWidth;
                    texture->texCoordBuffer[5] = normTop  + normHeight;

                    texture->texCoordBuffer[6] = normLeft;
                    texture->texCoordBuffer[7] = normTop  + normHeight;

                    texture->dirtyCropRect = GL_FALSE;
                }

                /* Set stream parameters. */
                glfSetStreamParameters(
                    Context,
                    &sampler->aTexCoordDrawTexInfo,
                    glvFRACGLTYPEENUM,
                    2,
                    2 * sizeof(gltFRACTYPE),
                    gcvFALSE,
                    &texture->texCoordBuffer,
                    gcvNULL,                       /* No buffer support here. */
                    glvTEX0COORDBUFFER_AUX + i
                    );
            }
        }

        /* Draw the texture. */
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        /* Restore the hash key. */
        Context->hashKey.hashDrawTextureOES = 0;
        Context->drawTexOESEnabled = GL_FALSE;
    }
    while (gcvFALSE);

    /* Return result. */
    return result;
}


/******************************************************************************\
**************************** Texture Management Code ***************************
\******************************************************************************/

/*******************************************************************************
**
**  glGenTextures
**
**  glGenTextures returns 'Count' texture names in 'Textures'. There is no
**  guarantee that the names form a contiguous set of integers. However, it is
**  guaranteed that none of the returned names was in use immediately before the
**  call to glGenTextures.
**
**  The generated textures have no dimensionality; they assume the
**  dimensionality of the texture target to which they are first bound
**  (see glBindTexture).
**
**  Texture names returned by a call to glGenTextures are not returned by
**  subsequent calls, unless they are first deleted with glDeleteTextures.
**
**  INPUT:
**
**      Count
**          Specifies the number of texture names to be generated.
**
**      Textures
**          Specifies an array in which the generated texture names are stored.
**
**  OUTPUT:
**
**      Textures
**          An array filled with generated texture names.
*/

GL_API void GL_APIENTRY glGenTextures(
    GLsizei Count,
    GLuint* Textures
    )
{
    glsTEXTUREWRAPPER_PTR texture;
    glsTEXTUREWRAPPER_PTR insertAfter;
    GLsizei i;

    glmENTER2(glmARGUINT, Count, glmARGPTR, Textures)
    {
        glmPROFILE(context, GLES1_GENTEXTURES, 0);
        /* Verify the arguments. */
        if (Count < 0)
        {
            /* The texture count is wrong. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        if (Textures == gcvNULL)
        {
            break;
        }

        /* Generate textures. */
        for (i = 0; i < Count; i++)
        {
            /* Set to default texture name in case of failure. */
            Textures[i] = 0;

            /* Find the insertion point. */
            insertAfter = _FindFirstHole(context);

            /* See if we still have available names. */
            if (insertAfter == gcvNULL)
            {
                continue;
            }

            /* Construct new object. */
            texture = _ConstructWrapper(context,
                                        insertAfter,
                                        insertAfter->name + 1);

            /* Make sure the allocation succeeded. */
            if (texture == gcvNULL)
            {
                continue;
            }

            /* Set the name. */
            Textures[i] = texture->name;
        }

        gcmDUMP_API("${ES11 glGenTextures 0x%08X (0x%08X)", Count, Textures);
        gcmDUMP_API_ARRAY(Textures, Count);
        gcmDUMP_API("$}");
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glDeleteTextures
**
**  glDeleteTextures deletes 'Count' textures named by the elements of the array
**  'Textures'. After a texture is deleted, it has no contents or dimensionality,
**  and its name is free for reuse (for example by glGenTextures). If a texture
**  that is currently bound is deleted, the binding reverts to 0 (the default
**  texture).
**
**  glDeleteTextures silently ignores 0's and names that do not correspond to
**  existing textures.
**
**  INPUT:
**
**      Count
**          Specifies the number of textures to be deleted.
**
**      Textures
**          Specifies an array of textures to be deleted.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glDeleteTextures(
    GLsizei Count,
    const GLuint* Textures
    )
{
    glsTEXTUREWRAPPER * texture;
    glsTEXTURESAMPLER * sampler;
    GLsizei i;

    glmENTER2(glmARGUINT, Count, glmARGPTR, Textures)
    {
        gcmDUMP_API("${ES11 glDeleteTextures 0x%08X (0x%08X)", Count, Textures);
        gcmDUMP_API_ARRAY(Textures, Count);
        gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_DELETETEXTURES, 0);
        /* Verify the arguments. */
        if (Count < 0)
        {
            /* The texture count is wrong. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        if (Textures == gcvNULL)
        {
            break;
        }

        /* Iterate through the texture names. */
        for (i = 0; i < Count; i++)
        {
            GLsizei j;

            /* Skip default textures. */
            if (Textures[i] == 0)
            {
                continue;
            }

            /* Try to find the texture. */
            texture = glfFindTexture(context, Textures[i]);

            /* Skip if not found. */
            if (texture == gcvNULL)
            {
                continue;
            }

            if (gcvNULL != context->frameBuffer)
            {
                if (context->frameBuffer->color.object == texture)
                {
                    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, 0, 0);
                }

                if (context->frameBuffer->depth.object == texture)
                {
                    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_TEXTURE_2D, 0, 0);
                }

                if (context->frameBuffer->stencil.object == texture)
                {
                    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_STENCIL_ATTACHMENT_OES, GL_TEXTURE_2D, 0, 0);
                }
            }

            /* Unbind if bound. */
            for (j = 0; j < context->texture.pixelSamplers; j++)
            {
                /* Get a shortcut to the bound sampler. */
                sampler = &context->texture.sampler[j];

                /* Check binding. */
                if (sampler->bindings[texture->targetType] != texture)
                {
                    continue;
                }

                /* Bind to the default texture. */
                sampler->bindings[texture->targetType]
                    = &context->texture.defaultTexture[texture->targetType];

                /* Is the texture currently selected? */
                if (sampler->binding == texture)
                {
                    /* Select the default. */
                    sampler->binding = sampler->bindings[texture->targetType];
                }
            }

            /* Destroy the texture. */
            if (gcmIS_ERROR(_ResetTextureWrapper(context, texture)))
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }

            /* Remove the texture from the list. */
            texture->prev->next = texture->next;
            texture->next->prev = texture->prev;

            /* Destroy the texture descriptor. */
            if (gcmIS_ERROR(gcoOS_Free(gcvNULL, texture)))
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }
            else
            {
                texture = gcvNULL;
            }
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glIsTexture
**
**  glIsTexture returns GL_TRUE if texture is currently the name of a texture.
**  If texture is zero, or is a non-zero value that is not currently the name
**  of a texture, or if an error occurs, glIsTexture returns GL_FALSE.
**
**  INPUT:
**
**      Texture
**          Specifies a value that may be the name of a texture.
**
**  OUTPUT:
**
**      GL_TRUE or GL_FALSE (see above description.)
*/

GL_API GLboolean GL_APIENTRY glIsTexture(
    GLuint Texture
    )
{
    glsTEXTUREWRAPPER_PTR texture;
    GLboolean result = GL_FALSE;

    glmENTER1(glmARGHEX, Texture)
    {
        glmPROFILE(context, GLES1_ISTEXTURE, 0);
        /* Try to find the texture. */
        texture = glfFindTexture(context, Texture);

        /* Set the result. */
        result = (texture != gcvNULL)
               ? (texture->boundAtLeastOnce != gcvFALSE)
               : GL_FALSE;

        gcmDUMP_API("${ES11 glIsTexture 0x%08X := 0x%08X}", Texture, result);
    }
    glmLEAVE();

    /* Return result. */
    return result;
}


/*******************************************************************************
**
**  glActiveTexture
**
**  glActiveTexture selects which texture unit subsequent texture state calls
**  will affect. The number of texture units an implementation supports is
**  implementation dependent, it must be at least 1.
**
**  INPUT:
**
**      Texture
**          Specifies which texture unit to make active. The number of texture
**          units is implementation dependent, but must be at least one.
**          'Texture' must be one of GL_TEXTUREi, where
**          0 <= i < GL_MAX_TEXTURE_UNITS, which is an implementation-dependent
**          value. The intial value is GL_TEXTURE0.
**
**  OUTPUT:
**
**      Nothing
*/

GL_API void GL_APIENTRY glActiveTexture(
    GLenum Texture
    )
{
    glmENTER1(glmARGHEX, Texture)
    {
        /* Convert to 0..glvMAX_TEXTURES range. */
        GLint index = Texture - GL_TEXTURE0;

        gcmDUMP_API("${ES11 glActiveTexture 0x%08X}", Texture);

        glmPROFILE(context, GLES1_ACTIVETEXTURE, 0);
        /* Validate. */
        if ( (index < 0) || (index >= context->texture.pixelSamplers) )
        {
            /* Invalid texture unit. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Set the texture unit. */
        context->texture.activeSampler = &context->texture.sampler[index];
        context->texture.activeSamplerIndex = index;

        /* Update matrix mode if necessary. */
        if ((context->matrixMode >= glvTEXTURE_MATRIX_0) &&
            (context->matrixMode <= glvTEXTURE_MATRIX_3))
        {
            glfSetMatrixMode(context, GL_TEXTURE);
        }

        /* Update current texture matrix. */
        (*context->matrixStackArray[glvTEXTURE_MATRIX_0 + index].currChanged) (
            context
            );
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glClientActiveTexture
**
**  glClientActiveTexture selects the vertex array client state parameters to
**  be modified by glTexCoordPointer, and enabled or disabled with
**  glEnableClientState or glDisableClientState, respectively, when called with
**  a parameter of GL_TEXTURE_COORD_ARRAY.
**
**  INPUT:
**
**      Texture
**          Specifies which texture unit to make active. The number of texture
**          units is implementation dependent, but must be at least one.
**          'Texture' must be one of GL_TEXTUREi, where
**          0 <= i < GL_MAX_TEXTURE_UNITS, which is an implementation-dependent
**          value. The intial value is GL_TEXTURE0.
**
**  OUTPUT:
**
**      Nothing
*/

GL_API void GL_APIENTRY glClientActiveTexture(
    GLenum Texture
    )
{
    GLint index;

    glmENTER1(glmARGHEX, Texture)
    {
        gcmDUMP_API("${ES11 glClientActiveTexture 0x%08X}", Texture);

        /* Convert to 0..glvMAX_TEXTURES range. */
        index = Texture - GL_TEXTURE0;

        glmPROFILE(context, GLES1_CLIENTACTIVETEXTURE, 0);
        /* Validate. */
        if ( (index < 0) || (index >= context->texture.pixelSamplers) )
        {
            /* Invalid texture unit. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Set the texture unit. */
        context->texture.activeClientSampler = &context->texture.sampler[index];
        context->texture.activeClientSamplerIndex = index;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glBindTexture
**
**  glBindTexture lets you create or use a named texture. Calling glBindTexture
**  with target set to GL_TEXTURE_2D, and texture set to the name of the new
**  texture binds the texture name to the target. When a texture is bound to a
**  target, the previous binding for that target is automatically broken.
**  Texture names are unsigned integers. The value 0 is reserved to represent
**  the default texture for each texture target. Texture names and the
**  corresponding texture contents are local to the shared texture-object space
**  (see eglCreateContext) of the current GL rendering context.
**
**  You may use glGenTextures to generate a set of new texture names.
**
**  While a texture is bound, GL operations on the target to which it is bound
**  affect the bound texture. If texture mapping of the dimensionality of the
**  target to which a texture is bound is active, the bound texture is used.
**  In effect, the texture targets become aliases for the textures currently
**  bound to them, and the texture name 0 refers to the default textures that
**  were bound to them at initialization.
**
**  A texture binding created with glBindTexture remains active until a
**  different texture is bound to the same target, or until the bound texture
**  is deleted with glDeleteTextures.
**
**  Once created, a named texture may be re-bound to the target of the matching
**  dimensionality as often as needed. It is usually much faster to use
**  glBindTexture to bind an existing named texture to one of the texture
**  targets than it is to reload the texture image using glTexImage2D.
**
**  INPUT:
**
**      Target
**          Specifies the target to which the texture is bound.
**          Must be GL_TEXTURE_2D or GL_TEXTURE_CUBE_MAP_OES per
**          GL_OES_texture_cube_map extension.
**
**      Texture
**          Specifies the name of a texture.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glBindTexture(
    GLenum Target,
    GLuint Texture
    )
{
    glsTEXTURESAMPLER_PTR sampler;
    glsTEXTUREWRAPPER_PTR texture;

    glmENTER2(glmARGUINT, Target, glmARGHEX, Texture)
    {
        gleTARGETTYPE target;
        glsCONTEXT_PTR shared;

        gcmDUMP_API("${ES11 glBindTexture 0x%08X 0x%08X}", Target, Texture);

        glmPROFILE(context, GLES1_BINDTEXTURE, 0);

        /* Map shared context. */
#if gcdRENDER_THREADS
        shared = (context->shared != gcvNULL) ? context->shared : context;
#else
        shared = context;
#endif

        /* Determine the target. */
        if (Target == GL_TEXTURE_2D)
        {
            /* 2D target. */
            target = glvTEXTURE2D;
        }
        else if (Target == GL_TEXTURE_CUBE_MAP_OES)
        {
            /* Cubemap target. */
            target = glvCUBEMAP;
        }
        else if (Target == GL_TEXTURE_EXTERNAL_OES)
        {
            /*External texture target*/
            target = glvTEXTUREEXTERNAL;
        }
        else
        {
            /* Invalid target. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Get a shortcut to the active sampler. */
        sampler = context->texture.activeSampler;

        /* Bind to default? */
        if (Texture == 0)
        {
            texture = &context->texture.defaultTexture[target];
        }

        /* Try to find the texture; create it if does not exist. */
        else
        {
            /* Start from the first item; */
            /* this could also be the sentinel itself as well. */
            texture = shared->texture.sentinel.next;

            while (GL_TRUE)
            {
                /* Found the texture? */
                if (texture->name == Texture)
                {
                    break;
                }

                /* Found the place of insertion? */
                if ( (texture->name == 0) || (Texture < texture->name) )
                {
                    /* Construct new object. */
                    texture = _ConstructWrapper(context,
                                                texture->prev,
                                                Texture);
                    break;
                }

                /* Move to the next texture. */
                texture = texture->next;
            }
        }

        /* Make sure we have a texture. */
        if (texture == gcvNULL)
        {
            break;
        }

        /* Set default values for external texture. */
        if (Target == GL_TEXTURE_EXTERNAL_OES)
        {
            texture->minFilter        = glvLINEAR;
            texture->magFilter        = glvLINEAR;
            texture->wrapS            = glvCLAMP;
            texture->wrapT            = glvCLAMP;
        }

        /* Don't rebind the same thing again. */
        if (sampler->bindings[target] == texture)
        {
            break;
        }

        /* Been bound before? */
        if (texture->boundAtLeastOnce)
        {
            /* Types have to match. */
            if (texture->targetType != target)
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }
        }
        else
        {
            /* Assign the type. */
            texture->targetType = target;
        }

        /* Unbind currently bound texture. */
        sampler->bindings[target]->binding = gcvNULL;

        /* Is this the current system target? */
        if (sampler->binding == sampler->bindings[target])
        {
            /* Update the current system target as well. */
            sampler->binding = texture;
        }

        /* Update the sampler binding for the selected target. */
        sampler->bindings[target] = texture;

        /* Bind the texture to the sampler. */
        texture->binding          = sampler;
        texture->boundAtLeastOnce = gcvTRUE;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glTexParameter
**
**  glTexParameter assigns the value in 'Value' to the texture parameter
**  specified in 'Name'. 'Target' defines the target texture.
**
**  The following 'Name'/'Value' combinations are accepted:
**
**      GL_TEXTURE_MIN_FILTER
**          GL_NEAREST
**          GL_LINEAR
**          GL_NEAREST_MIPMAP_NEAREST
**          GL_LINEAR_MIPMAP_NEAREST
**          GL_NEAREST_MIPMAP_LINEAR (default)
**          GL_LINEAR_MIPMAP_LINEAR
**
**      GL_TEXTURE_MAG_FILTER
**          GL_NEAREST
**          GL_LINEAR (default)
**
**      GL_TEXTURE_WRAP_S
**          GL_CLAMP_TO_EDGE
**          GL_REPEAT (default)
**
**      GL_TEXTURE_WRAP_T
**          GL_CLAMP_TO_EDGE
**          GL_REPEAT (default)
**
**      GL_GENERATE_MIPMAP
**          GL_TRUE
**          GL_FALSE (default)
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D or
**          GL_TEXTURE_CUBE_MAP_OES per GL_OES_texture_cube_map extension.
**
**      Name
**          Specifies the symbolic name of a single-valued texture parameter.
**          'Name' can be one of the following: GL_TEXTURE_MIN_FILTER,
**          GL_TEXTURE_MAG_FILTER, GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T
**          or GL_GENERATE_MIPMAP.
**
**      Value
**          Specifies the value of 'Name'.
**
**  OUTPUT:
**
**      Nothing.
*/

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glTexParameterf(
    GLenum Target,
    GLenum Name,
    GLfloat Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGFLOAT, Value)
    {
        glmPROFILE(context, GLES1_TEXPARAMETERF, 0);
        if (Name == GL_TEXTURE_CROP_RECT_OES)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (!_SetTextureParameter(context, Target, Name,
                                  &Value, glvFLOAT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        gcmDUMP_API("${ES11 glTexParameterf 0x%08X 0x%08X 0x%08X}", Target, Name, *(GLuint*)&Value);
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexParameterfv(
    GLenum Target,
    GLenum Name,
    const GLfloat* Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Value)
    {
        gcmDUMP_API("${ES11 glTexParameterfv 0x%08X 0x%08X (0x%08X)", Target, Name, Value);
        gcmDUMP_API_ARRAY(Value, 1);
        gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_TEXPARAMETERFV, 0);
        if (!_SetTextureParameter(context, Target, Name,
                                  Value, glvFLOAT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glTexParameterx(
    GLenum Target,
    GLenum Name,
    GLfixed Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGFIXED, Value)
    {
        gcmDUMP_API("${ES11 glTexParameterx 0x%08X 0x%08X 0x%08X}", Target, Name, Value);

        glmPROFILE(context, GLES1_TEXPARAMETERX, 0);
        if (Name == GL_TEXTURE_CROP_RECT_OES)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (!_SetTextureParameter(context, Target, Name,
                                  &Value, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexParameterxOES(
    GLenum Target,
    GLenum Name,
    GLfixed Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGFIXED, Value)
    {
        gcmDUMP_API("${ES11 glTexParameterxOES 0x%08X 0x%08X 0x%08X}", Target, Name, Value);

        if (Name == GL_TEXTURE_CROP_RECT_OES)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (!_SetTextureParameter(context, Target, Name,
                                  &Value, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexParameterxv(
    GLenum Target,
    GLenum Name,
    const GLfixed* Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Value)
    {
        gcmDUMP_API("${ES11 glTexParameterxv 0x%08X 0x%08X (0x%08X)", Target, Name, Value);
        gcmDUMP_API_ARRAY(Value, 1);
        gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_TEXPARAMETERXV, 0);
        if (!_SetTextureParameter(context, Target, Name,
                                  Value, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexParameterxvOES(
    GLenum Target,
    GLenum Name,
    const GLfixed* Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Value)
    {
        gcmDUMP_API("${ES11 glTexParameterxvOES 0x%08X 0x%08X (0x%08X)", Target, Name, Value);
        gcmDUMP_API_ARRAY(Value, 1);
        gcmDUMP_API("$}");

        if (!_SetTextureParameter(context, Target, Name,
                                  Value, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexParameteri(
    GLenum Target,
    GLenum Name,
    GLint Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGINT, Value)
    {
        gcmDUMP_API("${ES11 glTexParameteri 0x%08X 0x%08X 0x%08X}", Target, Name, Value);

        glmPROFILE(context, GLES1_TEXPARAMETERI, 0);
        if (Name == GL_TEXTURE_CROP_RECT_OES)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (!_SetTextureParameter(context, Target, Name,
                                  &Value, glvINT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexParameteriv(
    GLenum Target,
    GLenum Name,
    const GLint* Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Value)
    {
        gcmDUMP_API("${ES11 glTexParameteriv 0x%08X 0x%08X (0x%08X)", Target, Name, Value);
        gcmDUMP_API_ARRAY(Value, (Name == GL_TEXTURE_CROP_RECT_OES) ? 4 : 1);
        gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_TEXPARAMETERIV, 0);
        if (!_SetTextureParameter(context, Target, Name,
                                  Value, glvINT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glGetTexParameter
**
**  glGetTexParameter returns in 'Value' the value of the texture parameter
**  specified in 'Name'. 'Target' defines the target texture. 'Name' accepts
**  the same symbols as glTexParameter, with the same interpretations.
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D or
**          GL_TEXTURE_CUBE_MAP_OES per GL_OES_texture_cube_map extension.
**
**      Name
**          Specifies the symbolic name of a single-valued texture parameter.
**          'Name' can be one of the following: GL_TEXTURE_MIN_FILTER,
**          GL_TEXTURE_MAG_FILTER, GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T
**          or GL_GENERATE_MIPMAP.
**
**      Value
**          Returns the texture parameters.
**
**  OUTPUT:
**
**      See above.
*/

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glGetTexParameterfv(
    GLenum Target,
    GLenum Name,
    GLfloat* Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Value)
    {
        glmPROFILE(context, GLES1_GETTEXPARAMETERFV, 0);
        if (!_GetTextureParameter(context, Target, Name,
                                  Value, glvFLOAT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* TODO: Is the size fixed to 1. */
        gcmDUMP_API("${ES11 glGetTexParameterfv 0x%08X 0x%08X (0x%08X)", Target, Name, Value);
        gcmDUMP_API_ARRAY(Value, 1);
        gcmDUMP_API("$}");
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glGetTexParameterxv(
    GLenum Target,
    GLenum Name,
    GLfixed* Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Value)
    {
        glmPROFILE(context, GLES1_GETTEXPARAMETERXV, 0);
        if (!_GetTextureParameter(context, Target, Name,
                                  Value, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        gcmDUMP_API("${ES11 glGetTexParameterxv 0x%08X 0x%08X (0x%08X)", Target, Name, Value);
        gcmDUMP_API_ARRAY(Value, 1);
        gcmDUMP_API("$}");
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glGetTexParameterxvOES(
    GLenum Target,
    GLenum Name,
    GLfixed* Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Value)
    {
        if (!_GetTextureParameter(context, Target, Name,
                                  Value, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        gcmDUMP_API("${ES11 glGetTexParameterxvOES 0x%08X 0x%08X (0x%08X)", Target, Name, Value);
        gcmDUMP_API_ARRAY(Value, 1);
        gcmDUMP_API("$}");
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glGetTexParameteriv(
    GLenum Target,
    GLenum Name,
    GLint* Value
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Value)
    {
        glmPROFILE(context, GLES1_GETTEXPARAMETERIV, 0);
        if (!_GetTextureParameter(context, Target, Name,
                                  Value, glvINT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        gcmDUMP_API("${ES11 glGetTexParameteriv 0x%08X 0x%08X (0x%08X)", Target, Name, Value);
        gcmDUMP_API_ARRAY(Value, 1);
        gcmDUMP_API("$}");
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glTexEnv
**
**  A texture environment specifies how texture values are interpreted when a
**  fragment is textured. 'Target' must be GL_TEXTURE_ENV.
**
**  If 'Name' is GL_TEXTURE_ENV_MODE, then 'Param' or 'Params' is
**  (or points to) the symbolic name of a texture function.
**  Four texture functions may be specified:
**  GL_MODULATE, GL_DECAL, GL_BLEND, and GL_REPLACE.
**
**  A texture function acts on the fragment to be textured using the texture
**  image value that applies to the fragment (see glTexParameter) and produces
**  an RGBA color for that fragment. The following table shows how the RGBA
**  color is produced for each of the three texture functions that can be
**  chosen. C is a triple of color values (RGB) and A is the associated alpha
**  value. RGBA values extracted from a texture image are in the range [0, 1].
**  The subscript f refers to the incoming fragment, the subscript t to the
**  texture image, the subscript c to the texture environment color, and
**  subscript v indicates a value produced by the texture function.
**
**  INPUT:
**
**      Target
**          Specifies a texture environment.
**
**      Name
**          Specifies the symbolic name of a texture environment parameter.
**
**      Param
**          Specifies a parameter or a pointer to a parameter array that
**          contains either a single symbolic constant or an RGBA color.
**
**  OUTPUT:
**
**      Nothing.
*/

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glTexEnvf(
    GLenum Target,
    GLenum Name,
    GLfloat Param
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGFLOAT, Param)
    {
        gcmDUMP_API("${ES11 glTexEnvf 0x%08X 0x%08X 0x%08X}", Target, Name, *(GLuint*)&Param);

        glmPROFILE(context, GLES1_TEXENVF, 0);
        if (Name == GL_TEXTURE_ENV_COLOR)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (!_SetTextureState(context, Target, Name, &Param, glvFLOAT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexEnvfv(
    GLenum Target,
    GLenum Name,
    const GLfloat* Params
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Params)
    {
        gcmDUMP_API("${ES11 glTexEnvfv 0x%08X 0x%08X (0x%08X)", Target, Name, Params);
        gcmDUMP_API_ARRAY(Params, 1);
        gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_TEXENVFV, 0);
        if (!_SetTextureState(context, Target, Name, Params, glvFLOAT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glTexEnvx(
    GLenum Target,
    GLenum Name,
    GLfixed Param
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGFIXED, Param)
    {
        gcmDUMP_API("${ES11 glTexEnvx 0x%08X 0x%08X 0x%08X}", Target, Name, Param);

        glmPROFILE(context, GLES1_TEXENVX, 0);
        if (Name == GL_TEXTURE_ENV_COLOR)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (!_SetTextureState(context, Target, Name, &Param, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexEnvxOES(
    GLenum Target,
    GLenum Name,
    GLfixed Param
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGFIXED, Param)
    {
        gcmDUMP_API("${ES11 glTexEnvxOES 0x%08X 0x%08X 0x%08X}", Target, Name, Param);

        if (Name == GL_TEXTURE_ENV_COLOR)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (!_SetTextureState(context, Target, Name, &Param, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexEnvxv(
    GLenum Target,
    GLenum Name,
    const GLfixed* Params
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Params)
    {
        gcmDUMP_API("${ES11 glTexEnvxv 0x%08X 0x%08X (0x%08X)", Target, Name, Params);
        gcmDUMP_API_ARRAY(Params, 1);
        gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_TEXENVXV, 0);
        if (!_SetTextureState(context, Target, Name, Params, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexEnvxvOES(
    GLenum Target,
    GLenum Name,
    const GLfixed* Params
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Params)
    {
        gcmDUMP_API("${ES11 glTexEnvxvOES 0x%08X 0x%08X (0x%08X)", Target, Name, Params);
        gcmDUMP_API_ARRAY(Params, 1);
        gcmDUMP_API("$}");

        if (!_SetTextureState(context, Target, Name, Params, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexEnvi(
    GLenum Target,
    GLenum Name,
    GLint Param
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGUINT, Param)
    {
        gcmDUMP_API("${ES11 glTexEnvi 0x%08X 0x%08X 0x%08X}", Target, Name, Param);

        glmPROFILE(context, GLES1_TEXENVI, 0);
        if (Name == GL_TEXTURE_ENV_COLOR)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (!_SetTextureState(context, Target, Name, &Param, glvINT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexEnviv(
    GLenum Target,
    GLenum Name,
    const GLint* Params
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Params)
    {
        gcmDUMP_API("${ES11 glTexEnviv 0x%08X 0x%08X (0x%08X)", Target, Name, Params);
        gcmDUMP_API_ARRAY(Params, 1);
        gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_TEXENVIV, 0);
        if (!_SetTextureState(context, Target, Name, Params, glvINT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glGetTexEnv
**
**  Returns a specified value from the texture environment.
**
**  INPUT:
**
**      Target
**          Specifies a texture environment. Must be GL_TEXTURE_ENV.
**
**      Name
**          Specifies the symbolic name of a texture environment parameter.
**
**      Param
**          Specifies a parameter or a pointer to a parameter array that
**          contains either a single symbolic constant or an RGBA color.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glGetTexEnviv(
    GLenum Target,
    GLenum Name,
    GLint* Params
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Params)
    {
        glmPROFILE(context, GLES1_GETTEXENVIV, 0);
        if (!_GetTextureState(context, Target, Name, Params, glvINT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        gcmDUMP_API("${ES11 glGetTexEnviv 0x%08X 0x%08X (0x%08X)", Target, Name, Params);
        gcmDUMP_API_ARRAY(Params, 1);
        gcmDUMP_API("$}");
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glGetTexEnvfv(
    GLenum Target,
    GLenum Name,
    GLfloat* Params
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Params)
    {
        glmPROFILE(context, GLES1_GETTEXENVFV, 0);
        if (!_GetTextureState(context, Target, Name, Params, glvFLOAT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        gcmDUMP_API("${ES11 glGetTexEnvfv 0x%08X 0x%08X (0x%08X)", Target, Name, Params);
        gcmDUMP_API_ARRAY(Params, 1);
        gcmDUMP_API("$}");
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glGetTexEnvxv(
    GLenum Target,
    GLenum Name,
    GLfixed* Params
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Params)
    {
        glmPROFILE(context, GLES1_GETTEXENVXV, 0);
        if (!_GetTextureState(context, Target, Name, Params, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        gcmDUMP_API("${ES11 glGetTexEnvxv 0x%08X 0x%08X (0x%08X)", Target, Name, Params);
        gcmDUMP_API_ARRAY(Params, 1);
        gcmDUMP_API("$}");
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glGetTexEnvxvOES(
    GLenum Target,
    GLenum Name,
    GLfixed* Params
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Params)
    {
        if (!_GetTextureState(context, Target, Name, Params, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        gcmDUMP_API("${ES11 glGetTexEnvxvOES 0x%08X 0x%08X (0x%08X)", Target, Name, Params);
        gcmDUMP_API_ARRAY(Params, 1);
        gcmDUMP_API("$}");
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glTexImage2D
**
**  Texturing maps a portion of a specified texture image onto each graphical
**  primitive for which texturing is enabled. To enable and disable
**  two-dimensional texturing, call glEnable and glDisable with argument
**  GL_TEXTURE_2D. Two-dimensional texturing is initially disabled.
**
**  To define texture images, call glTexImage2D. The arguments describe the
**  parameters of the texture image, such as height, width, width of the border,
**  level-of-detail number (see glTexParameter), and number of color components
**  provided. The last three arguments describe how the image is represented in
**  memory.
**
**  Data is read from pixels as a sequence of unsigned bytes or shorts,
**  depending on type. These values are grouped into sets of one, two, three,
**  or four values, depending on format, to form elements.
**
**  When type is GL_UNSIGNED_BYTE, each of these bytes is interpreted as one
**  color component, depending on format. When type is one of
**  GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4,
**  GL_UNSIGNED_SHORT_5_5_5_1, each unsigned value is interpreted as containing
**  all the components for a single pixel, with the color components arranged
**  according to format.
**
**  The first element corresponds to the lower left corner of the texture image.
**  Subsequent elements progress left-to-right through the remaining texels in
**  the lowest row of the texture image, and then in successively higher rows of
**  the texture image. The final element corresponds to the upper right corner
**  of the texture image.
**
**  By default, adjacent pixels are taken from adjacent memory locations, except
**  that after all width pixels are read, the read pointer is advanced to the
**  next four-byte boundary. The four-byte row alignment is specified by
**  glPixelStore with argument GL_UNPACK_ALIGNMENT, and it can be set to one,
**  two, four, or eight bytes.
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D or
**          GL_TEXTURE_CUBE_MAP_xxx_OES per GL_OES_texture_cube_map extension.
**
**      Level
**          Specifies the level-of-detail number. Level 0 is the base image
**          level. Level n is the nth mipmap reduction image. Must be greater
**          or equal 0.
**
**      InternalFormat
**          Specifies the number of color components in the texture. Must be
**          GL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, or GL_LUMINANCE_ALPHA.
**
**      Width
**          Specifies the width of the texture image. Must be 2**n for some
**          integer n. All implementations support texture images that are
**          at least 64 texels wide.
**
**      Height
**          Specifies the height of the texture image. Must be 2**m for some
**          integer m. All implementations support texture images that are
**          at least 64 texels high.
**
**      Border
**          Specifies the width of the border. Must be 0.
**
**      Format
**          Specifies the format of the pixel data. Must be same as
**          internalformat. The following symbolic values are accepted:
**          GL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, and GL_LUMINANCE_ALPHA.
**
**      Type
**          Specifies the data type of the pixel data. The following symbolic
**          values are accepted: GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5,
**          GL_UNSIGNED_SHORT_4_4_4_4, and GL_UNSIGNED_SHORT_5_5_5_1.
**
**      Pixels
**          Specifies a pointer to the image data in memory.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glTexImage2D(
    GLenum Target,
    GLint Level,
    GLint InternalFormat,
    GLsizei Width,
    GLsizei Height,
    GLint Border,
    GLenum Format,
    GLenum Type,
    const GLvoid* Pixels
    )
{
    gceSTATUS status;

    glmENTER8(glmARGENUM, Target, glmARGINT, Level, glmARGENUM, InternalFormat,
              glmARGINT, Width, glmARGINT, Height, glmARGENUM, Format,
              glmARGENUM, Type, glmARGPTR, Pixels)
    {
        gctINT faces;
        gceTEXTURE_FACE face;
        gceSURF_FORMAT srcFormat;
        gceSURF_FORMAT dstFormat;
        glsTEXTURESAMPLER_PTR sampler;
        glsTEXTUREWRAPPER_PTR texture;
        gctINT stride;

        glmPROFILE(context, GLES1_TEXIMAGE2D, 0);

        /* Get a shortcut to the active sampler. */
        sampler = context->texture.activeSampler;

        /* Determine the target and the texture. */
        switch (Target)
        {
        case GL_TEXTURE_2D:
            texture = sampler->bindings[glvTEXTURE2D];
            face    = gcvFACE_NONE;
            faces   = 0;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_POSITIVE_X;
            faces   = 6;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_NEGATIVE_X;
            faces   = 6;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_POSITIVE_Y;
            faces   = 6;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_NEGATIVE_Y;
            faces   = 6;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_POSITIVE_Z;
            faces   = 6;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_NEGATIVE_Z;
            faces   = 6;
            break;

        default:
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Texture has to be something. */
        gcmASSERT(texture != gcvNULL);

        /* Validate arguments. */
        if (!_validateFormat(Format) || !_validateType(Type))

        {
            glmERROR(GL_INVALID_ENUM);
            goto OnError;
        }

        if ((Level < 0) || (Border != 0) ||
            /* Valid Width and Height. */
            (Width < 0) || (Height < 0)  ||
            (Width  > (GLsizei)context->maxTextureWidth)  ||
            (Height > (GLsizei)context->maxTextureHeight) ||
            /* Valid level. */
            (Level  > (GLint)gcoMATH_Ceiling(
                                 gcoMATH_Log2(
                                    (gctFLOAT)context->maxTextureWidth))) ||
            !_validateFormat(InternalFormat))
        {
            glmERROR(GL_INVALID_VALUE);
            goto OnError;
        }

        /* Check internal format and format conformance. */
        if (((GLenum) InternalFormat != Format) &&
            !((InternalFormat == GL_RGBA) && (Format == GL_BGRA_EXT))
           )
        {
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        if (!_getFormat(Format, Type, &srcFormat))
        {
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Get closest supported destination. */
        status = gcoTEXTURE_GetClosestFormat(
            context->hal, srcFormat, &dstFormat
            );

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_VALUE);
            goto OnError;
        }

        /* Null texture? */
        if ((Width == 0) || (Height == 0))
        {
            /* Destroy the texture. */
            gcmVERIFY_OK(_ResetTextureWrapper(context, texture));
            break;
        }

        /* Reset the texture if planar object is defined. */
        if (texture->direct.source != gcvNULL)
        {
            /* Destroy the texture. */
            gcmVERIFY_OK(_ResetTextureWrapper(context, texture));
        }

        /* Construct texture object. */
        if (texture->object == gcvNULL)
        {
            /* Construct the gcoTEXTURE object. */
            status = gcoTEXTURE_Construct(context->hal, &texture->object);

            if (gcmIS_ERROR(status))
            {
                /* Error. */
                glmERROR(GL_OUT_OF_MEMORY);
                goto OnError;
            }
        }

        /* Add the mipmap. If it already exists, the call will be ignored. */
        status = gcoTEXTURE_AddMipMap(
            texture->object,
            Level,
            dstFormat,
            Width, Height, 0,
            faces,
            gcvPOOL_DEFAULT,
            gcvNULL
            );

        if (gcmIS_ERROR(status))
        {
            /* Destroy the texture. */
            gcmVERIFY_OK(_ResetTextureWrapper(context, texture));

            glmERROR(GL_OUT_OF_MEMORY);
            goto OnError;
        }

        /* Get Stride of source texture */
        stride = 0;
        _GetSourceStride(
            InternalFormat, Type, Width, context->unpackAlignment, &stride
            );

        /* Upload the texture. */
        if (Pixels != gcvNULL)
        {
            do
            {
                if (srcFormat == gcvSURF_ETC1)
                {
                    status = gcoTEXTURE_UploadCompressed(texture->object,
                                                         face,
                                                         Width, Height, 0,
                                                         Pixels,
                                                         ((Width + 3) / 4) * ((Height + 3) / 4) * 8);
                }
                else if(srcFormat == gcvSURF_DXT3 || srcFormat == gcvSURF_DXT5)
                {
                    /* size must be 4x4 block aligned. */
                    gctSIZE_T size = ((Width + 3) & (~0x3))
                                   * ((Height + 3) & (~0x3));

                    status = gcoTEXTURE_UploadCompressed(texture->object,
                                                         face,
                                                         Width, Height, 0,
                                                         Pixels,
                                                         size
                                                         );
                }
                else
                {
                    status = gcoTEXTURE_Upload(texture->object,
                                               face,
                                               Width, Height, 0,
                                               Pixels,
                                               stride,
                                               srcFormat);
                }
            }
            while (gcvFALSE);

            if (status == gcvSTATUS_NOT_SUPPORTED)
            {
                glmERROR(GL_INVALID_OPERATION);
                goto OnError;
            }

            /* Mark texture as dirty. */
            texture->dirty = gcvTRUE;
        }

        /* Verify the result. */
        gcmASSERT(!gcmIS_ERROR(status));

        /* Level 0 is special. */
        if (Level == 0)
        {
            GLint tValue;
            /* Set endian hint */
            gcmVERIFY_OK(gcoTEXTURE_SetEndianHint(
                texture->object, _getEndianHint(Format, Type)
                ));

            /* Invalidate normalized crop rectangle. */
            texture->dirtyCropRect = GL_TRUE;

            /* Set texture parameters. */
            texture->width  = Width;
            texture->height = Height;

            /* Determine the number of mipmaps. */
            tValue = glfGetMaxLOD(texture->width, texture->height);
            texture->maxLevel = gcmMIN(tValue, texture->maxLevel);

            /* Update texture format. */
            _SetTextureWrapperFormat(context, texture, Format);

            /* Auto generate mipmaps. */
            if (texture->genMipmap)
            {
                gcmERR_BREAK(glfGenerateMipMaps(
                    context, texture, dstFormat, 0, Width, Height, faces
                    ));
            }
        }


            gcmDUMP_API("${ES11 glTexImage2D 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X (0x%08X)",
                Target, Level, InternalFormat, Width, Height, Border, Format, Type, Pixels);
            gcmDUMP_API_DATA(Pixels, stride * Height);
            gcmDUMP_API("$}");

        /* Easy return on error. */
OnError:;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glTexSubImage2D
**
**  Texturing maps a portion of a specified texture image onto each graphical
**  primitive for which texturing is enabled. To enable and disable
**  two-dimensional texturing, call glEnable and glDisable with argument
**  GL_TEXTURE_2D. Two-dimensional texturing is initially disabled.
**
**  glTexSubImage2D redefines a contiguous subregion of an existing
**  two-dimensional texture image. The texels referenced by pixels replace the
**  portion of the existing texture array with x indices xoffset and
**  xoffset + width - 1, inclusive, and y indices yoffset and
**  yoffset + height - 1, inclusive. This region may not include any texels
**  outside the range of the texture array as it was originally specified.
**  It is not an error to specify a subtexture with zero width or height,
**  but such a specification has no effect.
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D or
**          GL_TEXTURE_CUBE_MAP_xxx_OES per GL_OES_texture_cube_map extension.
**
**      Level
**          Specifies the level-of-detail number. Level 0 is the base image
**          level. Level n is the nth mipmap reduction image. Must be greater
**          or equal 0.
**
**      XOffset
**      YOffset
**          Specifies a texel offset within the texture array.
**
**      Width
**      Height
**          Specifies the size of the texture subimage.
**
**      Format
**          Specifies the format of the pixel data. Must be same as
**          internalformat. The following symbolic values are accepted:
**          GL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, and GL_LUMINANCE_ALPHA.
**
**      Type
**          Specifies the data type of the pixel data. The following symbolic
**          values are accepted: GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5,
**          GL_UNSIGNED_SHORT_4_4_4_4, and GL_UNSIGNED_SHORT_5_5_5_1.
**
**      Pixels
**          Specifies a pointer to the image data in memory.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glTexSubImage2D(
    GLenum Target,
    GLint Level,
    GLint XOffset,
    GLint YOffset,
    GLsizei Width,
    GLsizei Height,
    GLenum Format,
    GLenum Type,
    const GLvoid* Pixels
    )
{
    glmENTER9(glmARGENUM, Target, glmARGINT, Level, glmARGINT, XOffset,
              glmARGINT, YOffset, glmARGINT, Width, glmARGINT, Height,
              glmARGENUM, Format, glmARGENUM, Type, glmARGPTR, Pixels)
    {
        gceTEXTURE_FACE face;
        glsTEXTURESAMPLER_PTR sampler;
        glsTEXTUREWRAPPER_PTR texture;
        gceSURF_FORMAT srcFormat, dstFormat;
        gceSTATUS status;
        gcoSURF map;
        gctUINT height, width;
        gctINT stride;

        glmPROFILE(context, GLES1_TEXSUBIMAGE2D, 0);
        /* Get a shortcut to the active sampler. */
        sampler = context->texture.activeSampler;

        /* Determine the target and the texture. */
        switch (Target)
        {
        case GL_TEXTURE_2D:
            texture = sampler->bindings[glvTEXTURE2D];
            face    = gcvFACE_NONE;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_POSITIVE_X;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_NEGATIVE_X;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_POSITIVE_Y;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_NEGATIVE_Y;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_POSITIVE_Z;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_NEGATIVE_Z;
            break;

        default:
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Texture has to be something. */
        gcmASSERT(texture != gcvNULL);

        /* Validate arguments. */
        if (!_validateFormat(Format) || !_validateType(Type))
        {
            glmERROR(GL_INVALID_ENUM);
            goto OnError;
        }

        if ((Pixels == gcvNULL) ||
            (Level < 0) ||
            (XOffset < 0) || (YOffset < 0) ||
            /* Valid Width and Height. */
            (Width < 0) || (Height < 0)  ||
            (Width  > (GLsizei)context->maxTextureWidth)  ||
            (Height > (GLsizei)context->maxTextureHeight) ||
            /* Valid level. */
            (Level  > (GLint)gcoMATH_Ceiling(
                                gcoMATH_Log2(
                                    (gctFLOAT)context->maxTextureWidth)))
            )
        {
            glmERROR(GL_INVALID_VALUE);
            goto OnError;
        }

        if (!_getFormat(Format, Type, &srcFormat))
        {
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Get closest supported destination. */
        status = gcoTEXTURE_GetClosestFormat(
            context->hal, srcFormat, &dstFormat
            );

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_VALUE);
            goto OnError;
        }

        /* Make sure the texture is not empty. */
        if (texture->object == gcvNULL)
        {
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Make sure the format matches the internal format. */
        if ((Format != texture->format) &&
            !((Format == GL_BGRA_EXT) && (texture->format == GL_RGBA))
           )
        {
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Get mip map for specified level. */
        status = gcoTEXTURE_GetMipMap(texture->object, Level, &map);

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Get mip map height. */
        status = gcoSURF_GetSize(map, &width, &height, gcvNULL);

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Check the size validation. */
        if ((XOffset + Width  > (GLint) width) ||
            (YOffset + Height > (GLint) height))
        {
            glmERROR(GL_INVALID_VALUE);
            goto OnError;
        }

        /* Get Stride of source texture */
        stride = 0;
        _GetSourceStride(
            Format, Type, Width, context->unpackAlignment, &stride
            );

        /* Upload the texture. */
        status = gcoTEXTURE_UploadSub(
            texture->object,
            Level, face,
            XOffset, YOffset,
            Width, Height, 0,
            Pixels, stride,
            srcFormat
            );

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Regenerate mipmap if necessary. */
        if ((texture->genMipmap) && (Level == 0))
        {
            gcmERR_BREAK(glfGenerateMipMaps(
                context, texture, dstFormat, 0, width, height, face
                ));
        }

        /* Mark texture as dirty. */
        texture->dirty = gcvTRUE;

        gcmDUMP_API("${ES11 glTexSubImage2D 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X (0x%08X)",
            Target, Level, XOffset, YOffset, Width, Height, Format, Type, Pixels);
        gcmDUMP_API_DATA(Pixels, stride * Height);
        gcmDUMP_API("$}");

        /* Easy return on error. */
OnError:;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glCopyTexImage2D
**
**  glCopyTexImage2D defines a two-dimensional texture image with pixels from
**  the color buffer.
**
**  The screen-aligned pixel rectangle with lower left corner at (x, y) and with
**  a width of width + 2border and a height of height + 2border defines the
**  texture array at the mipmap level specified by level. InternalFormat
**  specifies the color components of the texture.
**
**  The red, green, blue, and alpha components of each pixel that is read are
**  converted to an internal fixed-point or floating-point format with
**  unspecified precision. The conversion maps the largest representable
**  component value to 1.0, and component value 0 to 0.0. The values are then
**  converted to the texture's internal format for storage in the texel array.
**
**  InternalFormat must be chosen such that color buffer components can be
**  dropped during conversion to the internal format, but new components cannot
**  be added. For example, an RGB color buffer can be used to create LUMINANCE
**  or RGB textures, but not ALPHA, LUMINANCE_ALPHA or RGBA textures.
**
**  Pixel ordering is such that lower x and y screen coordinates correspond to
**  lower s and t texture coordinates.
**
**  If any of the pixels within the specified rectangle of the color buffer are
**  outside the window associated with the current rendering context, then the
**  values obtained for those pixels are undefined.
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D or
**          GL_TEXTURE_CUBE_MAP_xxx_OES per GL_OES_texture_cube_map extension.
**
**      Level
**          Specifies the level-of-detail number. Level 0 is the base image
**          level. Level n is the nth mipmap reduction image. Must be greater
**          or equal 0.
**
**      InternalFormat
**          Specifies the color components of the texture. The following
**          symbolic values are accepted: GL_ALPHA, GL_RGB, GL_RGBA,
**          GL_LUMINANCE, and GL_LUMINANCE_ALPHA.
**
**      X
**      Y
**          Specify the window coordinates of the lower left corner of the
**          rectangular region of pixels to be copied.
**
**      Width
**      Height
**          Specifies the size of the texture image. Must be 0 or
**          2**n + 2*border for some integer n.
**
**      Border
**          Specifies the width of the border. Must be 0.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glCopyTexImage2D(
    GLenum Target,
    GLint Level,
    GLenum InternalFormat,
    GLint X,
    GLint Y,
    GLsizei Width,
    GLsizei Height,
    GLint Border
    )
{
    glmENTER7(glmARGENUM, Target, glmARGINT, Level, glmARGENUM, InternalFormat,
              glmARGINT, X, glmARGINT, Y, glmARGINT, Width, glmARGINT, Height)
    {
        gceSTATUS status;
        gctINT faces;
        gceTEXTURE_FACE face;
        glsTEXTURESAMPLER_PTR sampler;
        glsTEXTUREWRAPPER_PTR texture;
        gceSURF_FORMAT format, textureFormat;

        gcmDUMP_API("${ES11 glCopyTexImage2D 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
            Target, Level, InternalFormat, X, Y, Width, Height, Border);

        glmPROFILE(context, GLES1_COPYTEXIMAGE2D, 0);
        /* Get a shortcut to the active sampler. */
        sampler = context->texture.activeSampler;

        /* Determine the target and the texture. */
        switch (Target)
        {
        case GL_TEXTURE_2D:
            texture = sampler->bindings[glvTEXTURE2D];
            faces   = 0;
            face    = gcvFACE_NONE;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            faces   = 6;
            face    = gcvFACE_POSITIVE_X;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            faces   = 6;
            face    = gcvFACE_NEGATIVE_X;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            faces   = 6;
            face    = gcvFACE_POSITIVE_Y;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            faces   = 6;
            face    = gcvFACE_NEGATIVE_Y;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            faces   = 6;
            face    = gcvFACE_POSITIVE_Z;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            faces   = 6;
            face    = gcvFACE_NEGATIVE_Z;
            break;

        default:
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Texture has to be something. */
        gcmASSERT(texture != gcvNULL);

        if ((Level < 0) || (Border != 0) ||
            /* Valid Width and Height. */
            (Width < 0) || (Height < 0)  ||
            (Width  > (GLsizei)context->maxTextureWidth)  ||
            (Height > (GLsizei)context->maxTextureHeight) ||
            /* Valid level. */
            (Level  > (GLint)gcoMATH_Ceiling(
                                gcoMATH_Log2
                                    ((gctFLOAT)context->maxTextureWidth)))
            )
        {
            glmERROR(GL_INVALID_VALUE);
            goto OnError;
        }

        /* Get frame buffer format. */
        gcmVERIFY_OK(gcoSURF_GetFormat(context->draw, gcvNULL, &format));

        switch (InternalFormat)
        {
        case GL_ALPHA:
            textureFormat = gcvSURF_A8;
            break;

        case GL_RGB:
            textureFormat = gcvSURF_X8R8G8B8;
            break;

        case GL_RGBA:
            textureFormat = gcvSURF_A8R8G8B8;
            break;

        case GL_LUMINANCE:
            textureFormat = gcvSURF_L8;
            break;

        case GL_LUMINANCE_ALPHA:
            textureFormat = gcvSURF_A8L8;
            break;

        default:
            /* Unknown format. */
            glmERROR(GL_INVALID_ENUM);
            goto OnError;
        }

        /* Get closest texture format hardware supports. */
        status = gcoTEXTURE_GetClosestFormat(
            context->hal, textureFormat, &textureFormat
            );

        if (gcmIS_ERROR(status))
        {
            /* Invalid format. */
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Reset the texture if planar object is defined. */
        if (texture->direct.source != gcvNULL)
        {
            /* Destroy the texture. */
            gcmVERIFY_OK(_ResetTextureWrapper(context, texture));
        }

        /* Test if we need to create a new gcoTEXTURE object. */
        if (texture->object == gcvNULL)
        {
            /* Construct the gcoTEXTURE object. */
            status = gcoTEXTURE_Construct(context->hal, &texture->object);

            if (gcmIS_ERROR(status))
            {
                /* Error. */
                glmERROR(GL_OUT_OF_MEMORY);
                goto OnError;
            }
        }

        /* Add the mipmap. If it already exists, the call will be ignored. */
        status = gcoTEXTURE_AddMipMap(
            texture->object,
            Level,
            textureFormat,
            Width, Height, 0,
            faces,
            gcvPOOL_DEFAULT,
            gcvNULL
            );

        if (gcmIS_ERROR(status))
        {
            /* Destroy the texture. */
            gcmVERIFY_OK(_ResetTextureWrapper(context, texture));

            glmERROR(GL_OUT_OF_MEMORY);
            goto OnError;
        }

        /* Update frame buffer. */
        gcmERR_BREAK(glfUpdateFrameBuffer(context));

        /* Resolve the rectangle to the temporary surface. */
        status = glfResolveDrawToTempBitmap(context, X, Y, Width, Height);

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_OUT_OF_MEMORY);
            goto OnError;
        }

        /* Invalidate the cache. */
        gcmVERIFY_OK(
            gcoSURF_CPUCacheOperation(context->tempBitmap,
                                      gcvCACHE_INVALIDATE));

        /* Upload the texture. */
        status = gcoTEXTURE_Upload(
            texture->object,
            face,
            Width, Height, 0,
            context->tempLastLine,
            context->tempStride,
            context->tempFormat
            );

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Mark texture as dirty. */
        texture->dirty = gcvTRUE;

        /* Level 0 is special. */
        if (Level == 0)
        {
            GLint tValue;
            /* Invalidate normalized crop rectangle. */
            texture->dirtyCropRect = GL_TRUE;

            /* Set texture parameters. */
            texture->width  = Width;
            texture->height = Height;

            /* Determine the number of mipmaps. */
            tValue = glfGetMaxLOD(texture->width, texture->height);
            texture->maxLevel = gcmMIN(tValue, texture->maxLevel);

            /* Update texture format. */
            _SetTextureWrapperFormat(context, texture, InternalFormat);
        }

        /* Easy return on error. */
OnError:;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glCopyTexSubImage2D
**
**  glCopyTexSubImage2D replaces a rectangular portion of a two-dimensional
**  texture image with pixels from the color buffer.
**
**  The screen-aligned pixel rectangle with lower left corner at ( x, y) and
**  with width width and height height replaces the portion of the texture array
**  with x indices xoffset through xoffset + width - 1, inclusive, and y indices
**  yoffset through yoffset + height - 1, inclusive, at the mipmap level
**  specified by level.
**
**  The pixels in the rectangle are processed the same way as with
**  glCopyTexImage2D.
**
**  glCopyTexSubImage2D requires that the internal format of the currently
**  bound texture is such that color buffer components can be dropped during
**  conversion to the internal format, but new components cannot be added.
**  For example, an RGB color buffer can be used to create LUMINANCE or RGB
**  textures, but not ALPHA, LUMINANCE_ALPHA or RGBA textures.
**
**  The destination rectangle in the texture array may not include any texels
**  outside the texture array as it was originally specified. It is not an error
**  to specify a subtexture with zero width or height, but such a specification
**  has no effect.
**
**  If any of the pixels within the specified rectangle of the current color
**  buffer are outside the read window associated with the current rendering
**  context, then the values obtained for those pixels are undefined.
**
**  No change is made to the internalformat, width, height, or border
**  parameters of the specified texture array or to texel values outside
**  the specified subregion.
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D or
**          GL_TEXTURE_CUBE_MAP_xxx_OES per GL_OES_texture_cube_map extension.
**
**      Level
**          Specifies the level-of-detail number. Level 0 is the base image
**          level. Level n is the nth mipmap reduction image. Must be greater
**          or equal 0.
**
**      XOffset
**      YOffset
**          Specifies a texel offset within the texture array.
**
**      X
**      Y
**          Specify the window coordinates of the lower left corner of the
**          rectangular region of pixels to be copied.
**
**      Width
**      Height
**          Specifies the size of the texture subimage.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glCopyTexSubImage2D(
    GLenum Target,
    GLint Level,
    GLint XOffset,
    GLint YOffset,
    GLint X,
    GLint Y,
    GLsizei Width,
    GLsizei Height
    )
{
    glmENTER8(glmARGENUM, Target, glmARGINT, Level, glmARGINT, XOffset,
              glmARGINT, YOffset, glmARGINT, X, glmARGINT, Y, glmARGINT, Width,
              glmARGINT, Height)
    {
        gceSTATUS status;
        gceTEXTURE_FACE face;
        gctUINT mapHeight, mapWidth;
        glsTEXTURESAMPLER_PTR sampler;
        glsTEXTUREWRAPPER_PTR texture;
        gcoSURF map;

        gcmDUMP_API("${ES11 glCopyTexSubImage2D 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
            Target, Level, XOffset, YOffset, X, Y, Width, Height);

        glmPROFILE(context, GLES1_COPYTEXSUBIMAGE2D, 0);
        /* Get a shortcut to the active sampler. */
        sampler = context->texture.activeSampler;

        /* Determine the target and the texture. */
        switch (Target)
        {
        case GL_TEXTURE_2D:
            texture = sampler->bindings[glvTEXTURE2D];
            face    = gcvFACE_NONE;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_POSITIVE_X;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_NEGATIVE_X;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_POSITIVE_Y;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_NEGATIVE_Y;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_POSITIVE_Z;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_OES:
            if (Width != Height)
            {
                glmERROR(GL_INVALID_VALUE);
                goto OnError;
            }
            texture = sampler->bindings[glvCUBEMAP];
            face    = gcvFACE_NEGATIVE_Z;
            break;

        default:
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Texture has to be something. */
        gcmASSERT(texture != gcvNULL);

        /* Verify arguments. */
        if (/* Valid Width and Height. */
            (Width < 0) || (Height < 0) ||
            (Width  > (GLsizei)context->maxTextureWidth)  ||
            (Height > (GLsizei)context->maxTextureHeight) ||
            (XOffset < 0) || (YOffset < 0) ||
            /* Valid level. */
            (Level < 0) ||
            (Level  > (GLint)gcoMATH_Ceiling(
                                gcoMATH_Log2
                                    ((gctFLOAT)context->maxTextureWidth)))
            )
        {
            glmERROR(GL_INVALID_VALUE);
            goto OnError;
        }

        /* Texture object has to exist. */
        if (texture->object == gcvNULL)
        {
            /* No bound texture object. */
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Get requested mip map surface. */
        status = gcoTEXTURE_GetMipMap(texture->object, Level, &map);

        if (gcmIS_ERROR(status))
        {
            /* Mip map has to exist. */
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Get size of mip map. */
        status = gcoSURF_GetSize(map, &mapWidth, &mapHeight, gcvNULL);
        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Check for value's validation. */
        if (((XOffset + Width)  > (GLint) mapWidth) ||
            ((YOffset + Height) > (GLint) mapHeight))
        {
            glmERROR(GL_INVALID_VALUE);
            goto OnError;
        }

        /* Update frame buffer. */
        gcmERR_BREAK(glfUpdateFrameBuffer(context));

        /* Resolve the rectangle to the temporary surface. */
        status = glfResolveDrawToTempBitmap(context, X, Y, Width, Height);

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_OUT_OF_MEMORY);
            goto OnError;
        }

        /* Invalidate the cache. */
        gcmVERIFY_OK(
            gcoSURF_CPUCacheOperation(context->tempBitmap,
                                      gcvCACHE_INVALIDATE));

        /* Upload the texture. */
        status = gcoTEXTURE_UploadSub(
            texture->object,
            Level,
            face,
            XOffset, YOffset,
            Width, Height, 0,
            context->tempLastLine,
            context->tempStride,
            context->tempFormat
            );

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        /* Mark texture as dirty. */
        texture->dirty = gcvTRUE;

        /* Easy return on error. */
OnError:;
    }
    glmLEAVE();
}

/*******************************************************************************
**
**  glCompressedTexImage2D
**
**  glCompressedTexImage2D defines a two-dimensional texture image in compressed
**  format.
**
**  The supported compressed formats are paletted textures. The layout of the
**  compressed image is a palette followed by multiple mip-levels of texture
**  indices used for lookup into the palette. The palette format can be one of
**  R5_G6_B5, RGBA4, RGB5_A1, RGB8, or RGBA8. The texture indices can have a
**  resolution of 4 or 8 bits. As a result, the number of palette entries is
**  either 16 or 256.
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D.
**
**      Level
**          Specifies the level-of-detail number. Level 0 is the base image
**          level. Level n is the nth mipmap reduction image. Must be greater
**          or equal 0.
**
**      InternalFormat
**          Specifies the color components in the texture. The following
**          symbolic constants are accepted:
**              GL_PALETTE4_RGB8_OES
**              GL_PALETTE4_RGBA8_OES
**              GL_PALETTE4_R5_G6_B5_OES
**              GL_PALETTE4_RGBA4_OES
**              GL_PALETTE4_RGB5_A1_OES
**              GL_PALETTE8_RGB8_OES
**              GL_PALETTE8_RGBA8_OES
**              GL_PALETTE8_R5_G6_B5_OES
**              GL_PALETTE8_RGBA4_OES
**              GL_PALETTE8_RGB5_A1_OES
**          also:
**              GL_COMPRESSED_RGB_S3TC_DXT1_EXT
**              GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
**              GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
**              GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
**              GL_ETC1_RGB8_OES
**
**      Width
**          Specifies the width of the texture image. Must be 2**n for some
**          integer n. All implementations support texture images that are
**          at least 64 texels wide.
**
**      Height
**          Specifies the height of the texture image. Must be 2**m for some
**          integer m. All implementations support texture images that are
**          at least 64 texels high.
**
**      Border
**          Specifies the width of the border. Must be 0.
**
**      ImageSize
**          Specifies the size of the compressed image data in bytes.
**
**      Data
**          Specifies a pointer to the compressed image data in memory.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glCompressedTexImage2D(
    GLenum Target,
    GLint Level,
    GLenum InternalFormat,
    GLsizei Width,
    GLsizei Height,
    GLint Border,
    GLsizei ImageSize,
    const GLvoid* Data
    )
{
    glmENTER7(glmARGENUM, Target, glmARGINT, Level, glmARGENUM, InternalFormat,
              glmARGINT, Width, glmARGINT, Height, glmARGUINT, ImageSize,
              glmARGPTR, Data)
    {
        gceSTATUS status;
        GLubyte * pixels, p;
        GLvoid * buffer;
        GLubyte * b;
        GLint i, shift;
        GLint pixelCount;
        GLint paletteSize;
        GLint compressedSize;
        GLint uncompressedSize;
        glsCOMPRESSEDTEXTURE_PTR formatInfo;
        gceSURF_FORMAT srcFormat, dstFormat;

        glmPROFILE(context, GLES1_COMPRESSEDTEXIMAGE2D, 0);

        /* Validate the arguments. */
        if (Target != GL_TEXTURE_2D)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (!_getFormat(GL_RGB, InternalFormat, &srcFormat))
        {
            glmERROR(GL_INVALID_ENUM);
            goto OnError;
        }

        /* Get closest supported destination. */
        status = gcoTEXTURE_GetClosestFormat(
            context->hal, srcFormat, &dstFormat
            );

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_VALUE);
            goto OnError;
        }

        if ((InternalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT)
        ||  (InternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT)
        ||  (InternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT)
        ||  (InternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
        )
        {
            if ((dstFormat < gcvSURF_DXT1) || (dstFormat > gcvSURF_DXT5))
            {
                gceSURF_FORMAT imageFormat = gcvSURF_UNKNOWN;
                gctUINT32 rgbFormat = (InternalFormat == GL_COMPRESSED_RGB_S3TC_DXT1_EXT) ?
                    GL_RGB : GL_RGBA;

                /* Decompress DXT texture since hardware doesn't support
                ** it. */
                pixels = _DecompressDXT(Width, Height,
                                         ImageSize, Data,
                                         InternalFormat,
                                         imageFormat = dstFormat);

                if (pixels == gcvNULL)
                {
                    /* Could not decompress the DXT texture. */
                    gcmFOOTER_NO();
                    return;
                }

                glTexImage2D(
                    Target, Level, rgbFormat, Width, Height,
                    Border, rgbFormat, imageFormat, pixels
                    );
            }
            else
            {
                glTexImage2D(
                    Target, Level, GL_RGBA,  Width, Height,
                    Border, GL_RGBA, InternalFormat, Data
                    );
            }
            break;
        }

        if (InternalFormat == GL_ETC1_RGB8_OES)
        {
            if (dstFormat != gcvSURF_ETC1)
            {
                gctUINT32 imageType = 0;

                /* Decompress ETC1 texture since hardware doesn't support
                ** it. */
                pixels = _DecompressETC1(Width, Height,
                                         ImageSize, Data,
                                         &imageType);

                if (pixels == gcvNULL)
                {
                    /* Could not decompress the ETC1 texture. */
                    gcmFOOTER_NO();
                    return;
                }

                glTexImage2D(
                    Target, Level, GL_RGB, Width, Height,
                    Border, GL_RGB, imageType, pixels
                    );
            }
            else
            {
                glTexImage2D(
                    Target, Level, GL_RGB, Width, Height,
                    Border, GL_RGB, InternalFormat, Data
                    );
            }
            break;
        }


        /* Decode the paletted texture format. */
        formatInfo = _GetCompressedPalettedTextureDetails(InternalFormat);

        if (formatInfo == gcvNULL)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if ((Level < 0) || (Border != 0) ||
            /* Valid Width and Height. */
            (Width < 0) || (Height < 0)  ||
            (Width  > (GLsizei)context->maxTextureWidth)  ||
            (Height > (GLsizei)context->maxTextureHeight) ||
            /* Valid level. */
            (Level  > (GLint)gcoMATH_Ceiling(
                                 gcoMATH_Log2(
                                     (gctFLOAT)context->maxTextureWidth))) ||
            (Data   == gcvNULL)
            )
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Calcuate the input texture sizes. */
        pixelCount     = Width * Height;
        paletteSize    = (1 << formatInfo->bits) * formatInfo->bytes;
        compressedSize = (pixelCount * formatInfo->bits + 7) / 8;

        /* Verify the size of the compressed texture. */
        if (ImageSize != (paletteSize + compressedSize))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Calculate the uncompressed texture size. */
        uncompressedSize = pixelCount * formatInfo->bytes;

        /* Allocate uncompressed pixel buffer. */
        status = gcoOS_Allocate(
            gcvNULL,
            uncompressedSize,
            (gctPOINTER *) &buffer
            );

        if (gcmIS_ERROR(status))
        {
            /* Out of memory. */
            glmERROR(GL_OUT_OF_MEMORY);
            break;
        }

        /* Point to pixel data. */
        shift  = (formatInfo->bits == 4) ? 4 : 0;
        pixels = (GLubyte *) Data + paletteSize;

        /* Upload each level of mipmap */
        b = buffer;

        for (i = 0; i < pixelCount; i++)
        {
            /* Fetch compressed pixel. */
            p = (formatInfo->bits == 4)
                ? (*pixels >> shift) & 0xF
                :  *pixels;

            /* Move to next compressed pixel. */
            if (shift == 4)
            {
                shift = 0;
            }
            else
            {
                pixels += 1;
                shift = (formatInfo->bits == 4) ? 4 : 0;
            }

            /* Copy data from palette to uncompressed pixel buffer. */
            gcoOS_MemCopy(
                b, (GLubyte *) Data + p * formatInfo->bytes, formatInfo->bytes
                );

            b += formatInfo->bytes;
        }

        /* Perform a texture load on the uncompressed data. */
        glTexImage2D(
            Target,
            Level,
            formatInfo->format,
            Width, Height,
            Border,
            formatInfo->format, formatInfo->type,
            buffer
            );

        /* Free the uncompressed pixel buffer. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, buffer));

        gcmDUMP_API("${ES11 glCompressedTexImage2D 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X (0x%08X)",
            Target, Level, InternalFormat, Width, Height, Border, ImageSize, Data);
        gcmDUMP_API_DATA(Data, compressedSize);
        gcmDUMP_API("$}");

OnError:;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glCompressedTexSubImage2D
**
**  glCompressedTexSubImage2D redefines a contiguous subregion of an existing
**  two-dimensional compressed texture image. The texels referenced by pixels
**  replace the portion of the existing texture array with x indices xoffset
**  and xoffset + width - 1, inclusive, and y indices yoffset and
**  yoffset + height - 1, inclusive. This region may not include any texels
**  outside the range of the texture array as it was originally specified.
**  It is not an error to specify a subtexture with zero width or height,
**  but such a specification has no effect.
**
**  Currently, there is no supported compressed format for this function.
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D.
**
**      Level
**          Specifies the level-of-detail number.
**
**      XOffset
**      YOffset
**          Specifies a texel offset within the texture array.
**
**      Width
**      Height
**          Specifies the size of the texture subimage.
**
**      Format
**          Specifies the format of the pixel data. Currently, there is no
**          supported format.
**
**      ImageSize
**          Specifies the size of the compressed image data in bytes.
**
**      Data
**          Specifies a pointer to the compressed image data in memory.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glCompressedTexSubImage2D(
    GLenum Target,
    GLint Level,
    GLint XOffset,
    GLint YOffset,
    GLsizei Width,
    GLsizei Height,
    GLenum Format,
    GLsizei ImageSize,
    const GLvoid* Data
    )
{
    glmENTER9(glmARGENUM, Target, glmARGINT, Level,
              glmARGINT, XOffset, glmARGINT, YOffset,
              glmARGINT, Width, glmARGINT, Height,
              glmARGENUM, Format, glmARGUINT, ImageSize,
              glmARGPTR, Data)
    {
        glsCOMPRESSEDTEXTURE_PTR formatInfo;

        gcmDUMP_API("${ES11 glCompressedTexImage2D 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
            Target, Level, XOffset, YOffset, Width, Height, Format, ImageSize, Data);

        glmPROFILE(context, GLES1_COMPRESSEDTEXSUBIMAGE2D, 0);
        /* Validate the arguments. */
        if (Target != GL_TEXTURE_2D)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Decode the compressed texture format. */
        formatInfo = _GetCompressedPalettedTextureDetails(Format);

        if (formatInfo == gcvNULL)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if ((Level < 0) ||
            /* Valid Width and Height. */
            (Width < 0) || (Height < 0)  ||
            (Width  > (GLsizei)context->maxTextureWidth)  ||
            (Height > (GLsizei)context->maxTextureHeight) ||
            (XOffset < 0) || (YOffset < 0) ||
            /* Valid level. */
            (Level  > (GLint)gcoMATH_Ceiling(
                                 gcoMATH_Log2(
                                     (gctFLOAT)context->maxTextureWidth))) ||
            (Data   == gcvNULL)
            )
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Per openGL ES11 spec, there is not supported compress format for this
           function. Return GL_INVALID_OPERATION if no other errors occured. */
        glmERROR(GL_INVALID_OPERATION);
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glDrawTex (GL_OES_draw_texture)
**
**  OpenGL supports drawing sub-regions of a texture to rectangular regions of
**  the screen using the texturing pipeline. Source region size and content are
**  determined by the texture crop rectangle(s) of the enabled texture(s).
**
**  The functions
**
**      void DrawTex{sifx}OES(T Xs, T Ys, T Zs, T Ws, T Hs);
**      void DrawTex{sifx}vOES(T *coords);
**
**  draw a texture rectangle to the screen.  Xs, Ys, and Zs specify the
**  position of the affected screen rectangle. Xs and Ys are given directly in
**  window (viewport) coordinates.  Zs is mapped to window depth Zw as follows:
**
**                 { n,                 if z <= 0
**            Zw = { f,                 if z >= 1
**                 { n + z * (f - n),   otherwise
**
**  where <n> and <f> are the near and far values of DEPTH_RANGE.  Ws and Hs
**  specify the width and height of the affected screen rectangle in pixels.
**  These values may be positive or negative; however, if either (Ws <= 0) or
**  (Hs <= 0), the INVALID_VALUE error is generated.
**
**  Calling one of the DrawTex functions generates a fragment for each pixel
**  that overlaps the screen rectangle bounded by (Xs, Ys) and (Xs + Ws),
**  (Ys + Hs). For each generated fragment, the depth is given by Zw as defined
**  above, and the color by the current color.
**
**  Texture coordinates for each texture unit are computed as follows:
**
**  Let X and Y be the screen x and y coordinates of each sample point
**  associated with the fragment.  Let Wt and Ht be the width and height in
**  texels of the texture currently bound to the texture unit. Let Ucr, Vcr,
**  Wcr and Hcr be (respectively) the four integers that make up the texture
**  crop rectangle parameter for the currently bound texture. The fragment
**  texture coordinates (s, t, r, q) are given by
**
**      s = (Ucr + (X - Xs)*(Wcr/Ws)) / Wt
**      t = (Vcr + (Y - Ys)*(Hcr/Hs)) / Ht
**      r = 0
**      q = 1
**
**  In the specific case where X, Y, Xs and Ys are all integers, Wcr/Ws and
**  Hcr/Hs are both equal to one, the base level is used for the texture read,
**  and fragments are sampled at pixel centers, implementations are required
**  to ensure that the resulting u, v texture indices are also integers.
**  This results in a one-to-one mapping of texels to fragments.
**
**  Note that Wcr and/or Hcr can be negative.  The formulas given above for
**  s and t still apply in this case. The result is that if Wcr is negative,
**  the source rectangle for DrawTex operations lies to the left of the
**  reference point (Ucr, Vcr) rather than to the right of it, and appears
**  right-to-left reversed on the screen after a call to DrawTex.  Similarly,
**  if Hcr is negative, the source rectangle lies below the reference point
**  (Ucr, Vcr) rather than above it, and appears upside-down on the screen.
**
**  Note also that s, t, r, and q are computed for each fragment as part of
**  DrawTex rendering. This implies that the texture matrix is ignored and
**  has no effect on the rendered result.
**
**  INPUT:
**
**      Xs, Ys, Zs
**          Position of the affected screen rectangle.
**
**      Ws, Hs
**          The width and height of the affected screen rectangle in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_EXTENTION

GL_API void GL_APIENTRY glDrawTexsOES(
    GLshort Xs,
    GLshort Ys,
    GLshort Zs,
    GLshort Ws,
    GLshort Hs
    )
{
    glmENTER5(glmARGINT, Xs, glmARGINT, Ys, glmARGINT, Zs, glmARGINT, Ws,
              glmARGINT, Hs)
    {
        gcmDUMP_API("${ES11 glDrawTexsOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}", Xs, Ys, Zs, Ws, Hs);

        if (!_DrawTexOES(
            context,
            glmINT2FRAC(Xs),
            glmINT2FRAC(Ys),
            glmINT2FRAC(Zs),
            glmINT2FRAC(Ws),
            glmINT2FRAC(Hs)
            ))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glDrawTexiOES(
    GLint Xs,
    GLint Ys,
    GLint Zs,
    GLint Ws,
    GLint Hs
    )
{
    glmENTER5(glmARGINT, Xs, glmARGINT, Ys, glmARGINT, Zs, glmARGINT, Ws,
              glmARGINT, Hs)
    {
        gcmDUMP_API("${ES11 glDrawTexiOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}", Xs, Ys, Zs, Ws, Hs);

#ifdef ANDROID
        /* Disable dither for android composition. */
        gco3D_EnableDither(context->hw, gcvFALSE);
#endif

        if (!_DrawTexOES(
            context,
            glmINT2FRAC(Xs),
            glmINT2FRAC(Ys),
            glmINT2FRAC(Zs),
            glmINT2FRAC(Ws),
            glmINT2FRAC(Hs)
            ))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glDrawTexfOES(
    GLfloat Xs,
    GLfloat Ys,
    GLfloat Zs,
    GLfloat Ws,
    GLfloat Hs
    )
{
    glmENTER5(glmARGFLOAT, Xs, glmARGFLOAT, Ys, glmARGFLOAT, Zs,
              glmARGFLOAT, Ws, glmARGFLOAT, Hs)
    {
        gcmDUMP_API("${ES11 glDrawTexiOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
            *(GLuint*)&Xs, *(GLuint*)&Ys, *(GLuint*)&Zs, *(GLuint*)&Ws, *(GLuint*)&Hs);

        if (!_DrawTexOES(
            context,
            glmFLOAT2FRAC(Xs),
            glmFLOAT2FRAC(Ys),
            glmFLOAT2FRAC(Zs),
            glmFLOAT2FRAC(Ws),
            glmFLOAT2FRAC(Hs)
            ))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glDrawTexxOES(
    GLfixed Xs,
    GLfixed Ys,
    GLfixed Zs,
    GLfixed Ws,
    GLfixed Hs
    )
{
    glmENTER5(glmARGFIXED, Xs, glmARGFIXED, Ys, glmARGFIXED, Zs,
              glmARGFIXED, Ws, glmARGFIXED, Hs)
    {
        gcmDUMP_API("${ES11 glDrawTexxOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}", Xs, Ys, Zs, Ws, Hs);

        if (!_DrawTexOES(
            context,
            glmFIXED2FRAC(Xs),
            glmFIXED2FRAC(Ys),
            glmFIXED2FRAC(Zs),
            glmFIXED2FRAC(Ws),
            glmFIXED2FRAC(Hs)
            ))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glDrawTexsvOES(
    const GLshort* Coords
    )
{
    glmENTER1(glmARGPTR, Coords)
    {
        gcmDUMP_API("${ES11 glDrawTexsvOES (0x%08X)", Coords);
        gcmDUMP_API_DATA(Coords, sizeof(GLshort) * 5);
        gcmDUMP_API("$}");

        if (Coords != gcvNULL)
        {
            if (!_DrawTexOES(
                context,
                glmINT2FRAC(Coords[0]),
                glmINT2FRAC(Coords[1]),
                glmINT2FRAC(Coords[2]),
                glmINT2FRAC(Coords[3]),
                glmINT2FRAC(Coords[4])
                ))
            {
                glmERROR(GL_INVALID_VALUE);
                break;
            }
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glDrawTexivOES(
    const GLint* Coords
    )
{
    glmENTER1(glmARGPTR, Coords)
    {
        gcmDUMP_API("${ES11 glDrawTexivOES (0x%08X)", Coords);
        gcmDUMP_API_ARRAY(Coords, 5);
        gcmDUMP_API("$}");

        if (Coords != gcvNULL)
        {
            if (!_DrawTexOES(
                context,
                glmINT2FRAC(Coords[0]),
                glmINT2FRAC(Coords[1]),
                glmINT2FRAC(Coords[2]),
                glmINT2FRAC(Coords[3]),
                glmINT2FRAC(Coords[4])
                ))
            {
                glmERROR(GL_INVALID_VALUE);
                break;
            }
        }
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glDrawTexfvOES(
    const GLfloat* Coords
    )
{
    glmENTER1(glmARGPTR, Coords)
    {
        gcmDUMP_API("${ES11 glDrawTexfvOES (0x%08X)", Coords);
        gcmDUMP_API_ARRAY(Coords, 5);
        gcmDUMP_API("$}");

        if (Coords != gcvNULL)
        {
            if (!_DrawTexOES(
                context,
                glmFLOAT2FRAC(Coords[0]),
                glmFLOAT2FRAC(Coords[1]),
                glmFLOAT2FRAC(Coords[2]),
                glmFLOAT2FRAC(Coords[3]),
                glmFLOAT2FRAC(Coords[4])
                ))
            {
                glmERROR(GL_INVALID_VALUE);
                break;
            }
        }
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glDrawTexxvOES(
    const GLfixed* Coords
    )
{
    glmENTER1(glmARGPTR, Coords)
    {
        gcmDUMP_API("${ES11 glDrawTexxvOES (0x%08X)", Coords);
        gcmDUMP_API_ARRAY(Coords, 5);
        gcmDUMP_API("$}");

        if (Coords != gcvNULL)
        {
            if (!_DrawTexOES(
                context,
                glmFIXED2FRAC(Coords[0]),
                glmFIXED2FRAC(Coords[1]),
                glmFIXED2FRAC(Coords[2]),
                glmFIXED2FRAC(Coords[3]),
                glmFIXED2FRAC(Coords[4])
                ))
            {
                glmERROR(GL_INVALID_VALUE);
                break;
            }
        }
    }
    glmLEAVE();
}

/******************************************************************************\
*************************** eglBindTexImage support ****************************
\******************************************************************************/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_TEXTURE

EGLenum
glfBindTexImage(
    gcoSURF Surface,
    EGLenum Format,
    EGLenum Target,
    EGLBoolean Mipmap,
    EGLint Level,
    gcoSURF *Binder
    )
{
    EGLenum error = EGL_BAD_ACCESS;

    glmENTER3(glmARGPTR, Surface,
              glmARGENUM, Format,
              glmARGENUM, Target)
    {
        gceSTATUS status;
        gctUINT width;
        gctUINT height;
        glsTEXTURESAMPLER_PTR sampler;
        glsTEXTUREWRAPPER_PTR texture;
        gleTARGETTYPE target;
        GLenum format;

        /* Verify target. */
        if (Target != EGL_TEXTURE_2D)
        {
            error = EGL_BAD_PARAMETER;
            break;
        }

        /* Always use 2D target. */
        target = glvTEXTURE2D;

        /* Verify format.*/
        if (Format == EGL_TEXTURE_RGB)
        {
            format = GL_RGB;
        }
        else if (Format == EGL_TEXTURE_RGBA)
        {
            format = GL_RGBA;
        }
        else
        {
            error  = EGL_BAD_PARAMETER;
            break;
        }

        /* Get the current sampler. */
        sampler = context->texture.activeSampler;

        /* Get the current texture object. */
        texture = sampler->bindings[target]->name > 0 ? &sampler->binding[target]
                                                      : &context->texture.defaultTexture[target];

        /* Destroy the texture. */
        gcmVERIFY_OK(_ResetTextureWrapper(context, texture));

        /* Done when we are relaseing the texture. */
        if (Surface == gcvNULL)
        {
            if (Binder != gcvNULL)
            {
                *Binder = gcvNULL;
            }
            /* Success. */
            error = EGL_SUCCESS;
            break;
        }

        /* Query surface dimensions. */
        gcmERR_BREAK(gcoSURF_GetSize(Surface, &width, &height, gcvNULL));

        /* Create a new texture object. */
        gcmERR_BREAK(gcoTEXTURE_Construct(context->hal, &texture->object));


        /* Create the mipmap that the render target will resolve to.
           We should not bind the incoming surface directly to the texture
           but do a resolve when necessary to sync.
        */
        gcmERR_BREAK(gcoTEXTURE_AddMipMap(texture->object, 0, gcvSURF_A8R8G8B8, width, height, 0, 0, gcvPOOL_DEFAULT, &Surface));
        gcoSURF_SetOrientation(Surface, gcvORIENTATION_TOP_BOTTOM);

        if (Binder != gcvNULL)
        {
            *Binder = Surface;
        }


        /* See if we need to do any binding. */
        if (sampler->bindings[target] != texture)
        {
            /* Unbind currently bound texture. */
            sampler->bindings[target]->binding = gcvNULL;

            /* Is this the current system target? */
            if (sampler->binding == sampler->bindings[target])
            {
                /* Update the current system target as well. */
                sampler->binding = texture;
            }

            /* Update the sampler binding for the selected target. */
            sampler->bindings[target] = texture;

            /* Bind the texture to the sampler. */
            texture->binding          = sampler;
            texture->boundAtLeastOnce = gcvTRUE;
        }

        /* Level 0 is special. */
        if (Level == 0)
        {
            GLint tValue;
            /* Invalidate normalized crop rectangle. */
            texture->dirtyCropRect = GL_TRUE;

            /* Set texture parameters. */
            texture->width  = width;
            texture->height = height;

            /* Determine the number of mipmaps. */
            tValue = glfGetMaxLOD(texture->width, texture->height);
            texture->maxLevel = gcmMIN(tValue, texture->maxLevel);

            /* Update texture format. */
            _SetTextureWrapperFormat(context, texture, format);

            /* Auto generate mipmaps. */
            if (Mipmap)
            {
                gceSURF_FORMAT surfFormat;

                /* Query the surface format. */
                gcmERR_BREAK(gcoSURF_GetFormat(
                    Surface, gcvNULL, &surfFormat
                    ));

                gcmERR_BREAK(glfGenerateMipMaps(
                    context, texture, surfFormat, 0, width, height, 0
                    ));
            }
        }

        /* Mark texture as dirty. */
        texture->dirty = gcvTRUE;

        /* Success. */
        error = EGL_SUCCESS;
    }
    glmLEAVE();

    return error;
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
**  The optional mimmap generation may be enabled or disabled through the
**  glTexParameter call with GL_GENERATE_MIPMAP parameter.
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
**          format.
**
**  OUTPUT:
**
**      Pixels array initialized with texture pointers.
*/

GL_API void GL_APIENTRY glTexDirectVIV(
    GLenum Target,
    GLsizei Width,
    GLsizei Height,
    GLenum Format,
    GLvoid ** Pixels
    )
{
    glmENTER5(glmARGENUM, Target, glmARGINT, Width,
              glmARGINT, Height, glmARGENUM, Format, glmARGPTR, Pixels)
    {
        gceSTATUS status;
        gctBOOL powerOfTwo;
        gctBOOL scalerAvailable;
        gctBOOL tilerAvailable;
        gctBOOL yuy2AveragingAvailable;
        gctBOOL generateMipmap;
        gctBOOL packedSource;
        gceSURF_FORMAT sourceFormat;
        gceSURF_FORMAT textureFormat;
        glsTEXTURESAMPLER_PTR sampler;
        glsTEXTUREWRAPPER_PTR texture;
        gctUINT powerOfTwoWidth;
        gctUINT powerOfTwoHeight;
        gctUINT lodWidth;
        gctUINT lodHeight;
        gctINT maxLevel;
        gctBOOL needTemporary;
        gctINT i;

        /***********************************************************************
        ** Validate parameters.
        */

        /* Validate the target. */
        if (Target != GL_TEXTURE_2D)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Simple parameter validation. */
        if ((Width < 1) || (Height < 1) || (Pixels == gcvNULL))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Determine whether the texture is a power of two. */
        powerOfTwo
            =  ((Width & (Width - 1)) == 0)
            && ((Height & (Height - 1)) == 0);

        /* Determine power of two dimensions. */
        powerOfTwoWidth  = _GetClosestPowerOfTwo(Width,  gcvTRUE);
        powerOfTwoHeight = _GetClosestPowerOfTwo(Height, gcvTRUE);

        /***********************************************************************
        ** Get the bound texture.
        */

        /* Get a shortcut to the active sampler. */
        sampler = context->texture.activeSampler;

        /* Get a shortcut to the bound texture. */
        texture = sampler->binding;

        /* A texture has to be bound. */
        if (texture == gcvNULL)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
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
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /***********************************************************************
        ** Check whether the source can be handled.
        */

        if (!packedSource && !scalerAvailable && !tilerAvailable)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /***********************************************************************
        ** Determine whether we are going to generate mipmaps.
        */

        if (texture->genMipmap)
        {
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
                generateMipmap

                    /* We need the scaler to take 4:2:0 data in and scale
                       to a closest power of 2. */
                    = scalerAvailable

                    /* We need YUY2 averaging to generate mipmaps. */
                    && yuy2AveragingAvailable;
            }
        }
        else
        {
            generateMipmap = gcvFALSE;
        }

        /***********************************************************************
        ** Reset the bound texture.
        */

        /* Remove the existing texture. */
        status = _ResetTextureWrapper(context, texture);

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /***********************************************************************
        ** Construct new texture.
        */

        /* Construct texture object. */
        status = gcoTEXTURE_Construct(context->hal, &texture->object);

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_OUT_OF_MEMORY);
            break;
        }

        /* Invalidate normalized crop rectangle. */
        texture->dirtyCropRect = GL_TRUE;

        /* Set texture parameters. */
        texture->width  = generateMipmap ? (GLsizei) powerOfTwoWidth  : Width;
        texture->height = generateMipmap ? (GLsizei) powerOfTwoHeight : Height;

        /* Reset the dirty flag. */
        texture->direct.dirty = gcvFALSE;

        /* YUV texture reaches the shader in the RGB form. */
        _SetTextureWrapperFormat(context, texture, GL_RGB);

        /***********************************************************************
        ** Determine the maximum LOD.
        */

        /* Initialize the maximim LOD to zero. */
        maxLevel = 0;

        /* Are we generating a mipmap? */
        if (generateMipmap)
        {
            /* Start from level 0. */
            lodWidth  = powerOfTwoWidth;
            lodHeight = powerOfTwoHeight;

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

        /* Determine whether we need the temporary surface. */
        needTemporary =

            /* If we are not generating mipmap, temp surfaces is not needed. */
            generateMipmap &&

            (
                /* Scaler needs the temp surface. */
                !powerOfTwo ||

                /* This case is for chips with no 4:2:0 tiler support. */
                (packedSource && !tilerAvailable)
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
            glmERROR(GL_OUT_OF_MEMORY);
            break;
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
                powerOfTwoWidth,
                powerOfTwoHeight,
                1,
                gcvSURF_BITMAP,
                gcvSURF_A8R8G8B8,
                gcvPOOL_DEFAULT,
                &texture->direct.temp
                );

            if (gcmIS_ERROR(status))
            {
                glmERROR(GL_OUT_OF_MEMORY);
                break;
            }

            /* Overwrite the texture format. */
            textureFormat = gcvSURF_A8R8G8B8;
        }

        /***********************************************************************
        ** Clear the direct texture array.
        */

        /* Reset the array. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(
            texture->direct.texture, gcmSIZEOF(gcoSURF) * 16
            ));

        /***********************************************************************
        ** Create the levels of detail.
        */

        /* Start from level 0. */
        lodWidth  = texture->width;
        lodHeight = texture->height;

        for (i = 0; i <= maxLevel; i++)
        {
            /* Allocate the LOD surface. */
            gcmERR_BREAK(gcoTEXTURE_AddMipMap(
                texture->object,
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

        /* Roll back if error has occurred. */
        if (gcmIS_ERROR(status))
        {
            /* Remove the existing texture. */
            gcmVERIFY_OK(_ResetTextureWrapper(context, texture));

            glmERROR(GL_OUT_OF_MEMORY);
            break;
        }

        gcmDUMP_API("${ES11 glTexDirectVIV 0x%08X 0x%08X 0x%08X 0x%08X (0x%08X)", Target, Width, Height, Format, Pixels);
        gcmDUMP_API_ARRAY(*Pixels, 1);
        gcmDUMP_API("$}");
    }
    glmLEAVE();
}

/*******************************************************************************
**
**  glTexDirectVIVMap
**
**  glTexDirectVIVMap function creates a texture with direct access support.
**  This function is similar to glTexDirectVIV, the only difference is that,
**  glTexDirectVIVMap has two input "Logical" and "Physical", which support mapping
**  a user space momory or a physical address into the texture surface.
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
**  The optional mimmap generation may be enabled or disabled through the
**  glTexParameter call with GL_GENERATE_MIPMAP parameter.
**
**  Format parameter supports the following formats: GL_VIV_YV12, GL_VIV_NV12,
**  GL_VIV_YUY2 and GL_VIV_UYVY.
**
**  If Format is GL_VIV_YV12, glTexDirectVIVMap creates a planar YV12 4:2:0 texture
**  and the format of Pixels array is as follows: (Yplane, Uplane, Vplane).
**
**  If Format is GL_VIV_NV12, glTexDirectVIVMap creates a planar NV12 4:2:0 texture
**  and the format of Pixels array is as follows: (Yplane, UVplane).
**
**  If Format is GL_VIV_YUY2 or GL_VIV_UYVY, glTexDirectVIVMap creates a packed
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
            - if Width or Height are not aligned.
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
**          One of the following: GL_VIV_YV12, GL_VIV_NV12, GL_VIV_YUY2,
**          GL_VIV_UYVY.
**
**      Logical
**          Pointer to the logic address of the application-defined buffer
**          to the texture. or NULL if no logical pointer has been provided.
**
**      Physical
**          Physical address of the application-defined buffer
**          to the texture. or ~0 if no physical pointer has been provided.
**
**
*/

GL_API void GL_APIENTRY glTexDirectVIVMap(
    GLenum Target,
    GLsizei Width,
    GLsizei Height,
    GLenum Format,
    GLvoid ** Logical,
    const GLuint * Physical
    )
{
    glmENTER6(glmARGENUM, Target, glmARGINT, Width,
              glmARGINT, Height, glmARGENUM, Format, glmARGPTR, Logical, glmARGPTR, Physical)
    {
        gceSTATUS status;
        gctBOOL powerOfTwo;
        gctBOOL scalerAvailable;
        gctBOOL tilerAvailable;
        gctBOOL yuy2AveragingAvailable;
        gctBOOL generateMipmap;
        gctBOOL packedSource;
        gceSURF_FORMAT sourceFormat;
        gceSURF_FORMAT textureFormat;
        glsTEXTURESAMPLER_PTR sampler;
        glsTEXTUREWRAPPER_PTR texture;
        gctUINT powerOfTwoWidth;
        gctUINT powerOfTwoHeight;
        gctUINT lodWidth;
        gctUINT lodHeight;
        gctINT maxLevel;
        gctBOOL needTemporary;
        gctINT i;
        gctINT32 tileWidth, tileHeight;

        /***********************************************************************
        ** Validate parameters.
        */

        /* Validate the target. */
        if (Target != GL_TEXTURE_2D)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Simple parameter validation. */
        if ((Width < 1) || (Height < 1) || (Logical == gcvNULL))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        if (((*Logical) == gcvNULL) || (((gctUINT)(*Logical) & 0x3F) != 0) || ((*Physical) == ~0U))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        gcoHAL_QueryTiled(context->hal,
                          gcvNULL, gcvNULL,
                          &tileWidth, &tileHeight);

        /* Currently hardware only supprot aligned Width and Height */
        if (Width & (tileWidth * 4 - 1))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        if (Height & (tileHeight - 1))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Determine whether the texture is a power of two. */
        powerOfTwo
            =  ((Width & (Width - 1)) == 0)
            && ((Height & (Height - 1)) == 0);

        /* Determine power of two dimensions. */
        powerOfTwoWidth  = _GetClosestPowerOfTwo(Width,  gcvTRUE);
        powerOfTwoHeight = _GetClosestPowerOfTwo(Height, gcvTRUE);

        /***********************************************************************
        ** Get the bound texture.
        */

        /* Get a shortcut to the active sampler. */
        sampler = context->texture.activeSampler;

        /* Get a shortcut to the bound texture. */
        texture = sampler->binding;

        /* A texture has to be bound. */
        if (texture == gcvNULL)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
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
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /***********************************************************************
        ** Check whether the source can be handled.
        */

        if (!packedSource && !scalerAvailable && !tilerAvailable)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /***********************************************************************
        ** Determine whether we are going to generate mipmaps.
        */

        if (texture->genMipmap)
        {
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
                generateMipmap

                    /* We need the scaler to take 4:2:0 data in and scale
                       to a closest power of 2. */
                    = scalerAvailable

                    /* We need YUY2 averaging to generate mipmaps. */
                    && yuy2AveragingAvailable;
            }
        }
        else
        {
            generateMipmap = gcvFALSE;
        }

        /***********************************************************************
        ** Reset the bound texture.
        */

        /* Remove the existing texture. */
        status = _ResetTextureWrapper(context, texture);

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /***********************************************************************
        ** Construct new texture.
        */

        /* Construct texture object. */
        status = gcoTEXTURE_Construct(context->hal, &texture->object);

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_OUT_OF_MEMORY);
            break;
        }

        /* Invalidate normalized crop rectangle. */
        texture->dirtyCropRect = GL_TRUE;

        /* Set texture parameters. */
        texture->width  = generateMipmap ? (GLsizei) powerOfTwoWidth  : Width;
        texture->height = generateMipmap ? (GLsizei) powerOfTwoHeight : Height;

        /* Reset the dirty flag. */
        texture->direct.dirty = gcvFALSE;

        /* YUV texture reaches the shader in the RGB form. */
        _SetTextureWrapperFormat(context, texture, GL_RGB);

        /***********************************************************************
        ** Determine the maximum LOD.
        */

        /* Initialize the maximim LOD to zero. */
        maxLevel = 0;

        /* Are we generating a mipmap? */
        if (generateMipmap)
        {
            /* Start from level 0. */
            lodWidth  = powerOfTwoWidth;
            lodHeight = powerOfTwoHeight;

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

        /* Determine whether we need the temporary surface. */
        needTemporary =

            /* If we are not generating mipmap, temp surfaces is not needed. */
            generateMipmap &&

            (
                /* Scaler needs the temp surface. */
                !powerOfTwo ||

                /* This case is for chips with no 4:2:0 tiler support. */
                (packedSource && !tilerAvailable)
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
            glmERROR(GL_OUT_OF_MEMORY);
            break;
        }

        /* Set the user buffer to the surface. */
        gcmERR_BREAK(gcoSURF_MapUserSurface(
            texture->direct.source,
            0,
            (GLvoid*)(*Logical),
            (gctUINT32)(*Physical)
            ));


        /* Construct the temporary surface. */
        if (needTemporary)
        {
            status = gcoSURF_Construct(
                context->hal,
                powerOfTwoWidth,
                powerOfTwoHeight,
                1,
                gcvSURF_BITMAP,
                gcvSURF_A8R8G8B8,
                gcvPOOL_DEFAULT,
                &texture->direct.temp
                );

            if (gcmIS_ERROR(status))
            {
                glmERROR(GL_OUT_OF_MEMORY);
                break;
            }

            /* Overwrite the texture format. */
            textureFormat = gcvSURF_A8R8G8B8;
        }

        /***********************************************************************
        ** Clear the direct texture array.
        */

        gcmVERIFY_OK(gcoOS_ZeroMemory(
            texture->direct.texture, gcmSIZEOF(gcoSURF) * 16
            ));

        /***********************************************************************
        ** Create the levels of detail.
        */

        /* Start from level 0. */
        lodWidth  = texture->width;
        lodHeight = texture->height;

        for (i = 0; i <= maxLevel; i++)
        {
            /* Allocate the LOD surface. */
            gcmERR_BREAK(gcoTEXTURE_AddMipMap(
                texture->object,
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

        /* Roll back if error has occurred. */
        if (gcmIS_ERROR(status))
        {
            /* Remove the existing texture. */
            gcmVERIFY_OK(_ResetTextureWrapper(context, texture));

            glmERROR(GL_OUT_OF_MEMORY);
            break;
        }
    }
    glmLEAVE();
}



/*******************************************************************************
**
**  glTexDirectInvalidateVIV
**
**  glTexDirectInvalidateVIV function should be used by the applcations to
**  signal that the direct texture data has changed to allow the OpenGL driver
**  and hardware to perform any transformations on the data if necessary.
**
**  ERRORS:
**
**      GL_INVALID_ENUM
**          - if Target is not GL_TEXTURE_2D.
**
**      GL_INVALID_OPERATION
**          - if no texture is bound to the active texture unit;
**          - if the bound texture is not a direct texture.
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glTexDirectInvalidateVIV(
    GLenum Target
    )
{
    glmENTER1(glmARGENUM, Target)
    {
        glsTEXTURESAMPLER_PTR sampler;
        glsTEXTUREWRAPPER_PTR texture;

        /* Validate arguments. */
        if (Target != GL_TEXTURE_2D)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Get a shortcut to the active sampler. */
        sampler = context->texture.activeSampler;

        /* Get a shortcut to the bound texture. */
        texture = sampler->binding;

        /* A texture has to be bound. */
        if (texture == gcvNULL)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /* Has to be a planar-sourced texture. */
        if (texture->direct.source == gcvNULL)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /* Mark texture as dirty to be flushed later. */
        texture->dirty = gcvTRUE;

        /* Need upload. */
        texture->uploaded = gcvFALSE;

        /* Set the quick-reference dirty flag. */
        texture->direct.dirty = gcvTRUE;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  veglCreateImageTexture
**  (EGL_KHR_gl_texture_2D_image, KHR_gl_texture_cubemap_image)
**
**  veglCreateImageTexture function create an EGLimage from given args.
**
**
**  ERRORS:
**
**      GL_INVALID_VALUE
**          -if cann't find a texture object or sub-object from the given args
**          -if Level isn't 0 and the texture isn't complete
**
**
**  INPUT:
**
**      Target
**          Specifies the target texture.
**
**      Texture
**          Specifies the name of a texture ojbect.
**
**      Level
**          Specifies the texture level of a mipmap.
**
**      Face
**          Specifies the face of a cube map.
**
**      Depth
**          Specifies the depeth of a 3D map.
**
**  OUTPUT:
**
**      Image
**          return an EGLImage object.
*/

EGLenum
glfCreateImageTexture(
    EGLenum Target,
    gctINT Texture,
    gctINT Level,
    gctINT Depth,
    gctPOINTER Image
    )
{
    EGLenum status = EGL_BAD_PARAMETER;

    glmENTER5(glmARGENUM, Target, glmARGHEX, Texture,
              glmARGINT, Level, glmARGINT, Depth,
              glmARGPTR, Image)
    {
        gceTEXTURE_FACE face;
        glsTEXTUREWRAPPER_PTR texture;
        khrEGL_IMAGE_PTR image = gcvNULL;
        khrIMAGE_TYPE imageType;
        gctINT32 referenceCount = 0;
        gcoSURF surface;

        /* Determine the target and the texture. */
        switch (Target)
        {
        case EGL_GL_TEXTURE_2D_KHR:
            face      = gcvFACE_NONE;
            imageType = KHR_IMAGE_TEXTURE_2D;
            break;

        case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:
            face      = gcvFACE_POSITIVE_X;
            imageType = KHR_IMAGE_TEXTURE_CUBE;
            break;

        case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:
            face      = gcvFACE_NEGATIVE_X;
            imageType = KHR_IMAGE_TEXTURE_CUBE;
            break;

        case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:
            face      = gcvFACE_POSITIVE_Y;
            imageType = KHR_IMAGE_TEXTURE_CUBE;
            break;

        case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:
            face      = gcvFACE_NEGATIVE_Y;
            imageType = KHR_IMAGE_TEXTURE_CUBE;
            break;

        case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:
            face      = gcvFACE_POSITIVE_Z;
            imageType = KHR_IMAGE_TEXTURE_CUBE;
            break;

        case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:
            face      = gcvFACE_NEGATIVE_Z;
            imageType = KHR_IMAGE_TEXTURE_CUBE;
            break;

        default:
            status = EGL_BAD_PARAMETER;
            goto OnError;
        }

        /* Create eglImage from default texture object isn't permitted. */
        if (Texture == 0)
        {
            status = EGL_BAD_PARAMETER;
            goto OnError;
        }

        /* Find the texture object by name. */
        texture = glfFindTexture(context, Texture);

        /* Test texture object. */
        if ((texture == gcvNULL) || (texture->object == gcvNULL))
        {
            status = EGL_BAD_PARAMETER;
            goto OnError;
        }

        /* Get the surface of a level */
        if (gcmIS_ERROR(gcoTEXTURE_GetMipMap(texture->object, Level, &surface)))
        {
            status = EGL_BAD_PARAMETER;
            goto OnError;
        }

        if (surface == gcvNULL)
        {
            status = EGL_BAD_PARAMETER;
            goto OnError;
        }

#if defined(ANDROID)
        /* Test if the texture is from EGLImage. */
        if (texture->source != gcvNULL)
        {
            status = EGL_BAD_ACCESS;
            goto OnError;
        }
#endif

        /* Test if texture is yuv format. */
        if (texture->direct.source != gcvNULL)
        {
            status = EGL_BAD_ACCESS;
            goto OnError;
        }

        /* Get source surface reference count. */
        gcmVERIFY_OK(gcoSURF_QueryReferenceCount(surface, &referenceCount));

        /* Test if surface is a sibling of any eglImage. */
        if (referenceCount > 1)
        {
            status = EGL_BAD_ACCESS;
            goto OnError;
        }

        image = (khrEGL_IMAGE_PTR) Image;

        /* Set EGL Image info. */
        image->magic    = KHR_EGL_IMAGE_MAGIC_NUM;
        image->type     = imageType;
        image->surface  = surface;

        image->u.texture.format  = (gctUINT) texture->format;
        image->u.texture.level   = Level;
        image->u.texture.face    = face;
        image->u.texture.depth   = Depth;
        image->u.texture.texture = Texture;
        image->u.texture.object  = texture->object;

        /* Success. */
        status = EGL_SUCCESS;

        /* Easy return on error. */
OnError:;
    }
    glmLEAVE();

    return status;
}


/*******************************************************************************
**
**  glEGLImageTargetTexture2DOES (GL_OES_EGL_image)
**
**  Defines an entire two-dimensional texture array.  All properties of the
**  texture images (including width, height, format, border, mipmap levels of
**  detail, and image data) are taken from the specified eglImageOES <image>,
**  rather than from the client or the framebuffer. Any existing image arrays
**  associated with any mipmap levels in the texture object are freed (as if
**  TexImage was called for each, with an image of zero size).  As a result of
**  this referencing operation, all of the pixel data in the <buffer> used as
**  the EGLImage source resource (i.e., the <buffer> parameter passed to the
**  CreateImageOES command that returned <image>) will become undefined.
**
**  Currently, <target> must be TEXTURE_2D. <image> must be the handle of a
**  valid EGLImage resource, cast into the type eglImageOES. Assuming no errors
**  are generated in EGLImageTargetTexture2DOES, the newly specified texture
**  object will be an EGLImage target of the specified eglImageOES. If an
**  application later respecifies any image array in the texture object
**  (through mechanisms such as calls to TexImage2D and/or GenerateMipmapOES,
**  or setting the SGIS_GENERATE_MIPMAP parameter to TRUE), implementations
**  should allocate additional space for all specified (and respecified) image
**  arrays, and copy any existing image data to the newly (re)specified texture
**  object (as if TexImage was called for every level-of-detail in the texture
**  object). The respecified texture object will not be an EGLImage target.
**
**  If the GL is unable to specify a texture object using the supplied
**  eglImageOES <image> (if, for example, <image> refers to a multisampled
**  eglImageOES), the error INVALID_OPERATION is generated.
**
**  If <target> is not TEXTURE_2D, the error INVALID_ENUM is generated.
**
**  ERRORS:
**
**      GL_INVALID_ENUM
**          - if Target is not GL_TEXTURE_2D.
**
**      GL_INVALID_OPERATION
**          - if the specified image cannot be operated on;
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D.
**
**      Image
**          Specifies the source eglImage handle, cast to GLeglImageOES.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glEGLImageTargetTexture2DOES(
    GLenum Target,
    GLeglImageOES Image
    )
{
    glmENTER2(glmARGENUM, Target, glmARGPTR, Image)
    {
        gceSTATUS status;
        khrEGL_IMAGE_PTR image;
        glsEGL_IMAGE_ATTRIBUTES attributes;
        glsTEXTURESAMPLER_PTR sampler;
        glsTEXTUREWRAPPER_PTR texture;
        gceSURF_FORMAT dstFormat;
        gctBOOL yuvFormat = gcvFALSE;

        /* Validate arguments. */
        if ((Target != GL_TEXTURE_2D) && (Target != GL_TEXTURE_EXTERNAL_OES))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Get the eglImage. */
        image = (khrEGL_IMAGE_PTR) Image;

        /* Get texture attributes from eglImage. */
        status = glfGetEGLImageAttributes(image, &attributes);

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /* Check the texture size. */
        if ((attributes.width == 0) || (attributes.height == 0))
        {
            break;
        }

        /* Check if width and height is power of 2. */
        /* Here not fully follow GLES spec for gnash, which may use npot texture. */
        if (((attributes.width  & (attributes.width  - 1))  ||
             (attributes.height & (attributes.height - 1))) &&
             (attributes.level != 0))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Get a shortcut to the active sampler. */
        sampler = context->texture.activeSampler;


        if (Target == GL_TEXTURE_2D)
        {
            /* Get closest supported destination. */
            if (gcmIS_ERROR(gcoTEXTURE_GetClosestFormat(context->hal,
                                                        attributes.format,
                                                        &dstFormat)))
            {
                glmERROR(GL_INVALID_ENUM);
                break;
            }

            /* Get a shortcut to the bound texture. */
            texture = sampler->bindings[glvTEXTURE2D];
        }
        else
        {
            /**********************************************************************
            ** Validate the format.
            */
            switch (attributes.format)
            {
            case gcvSURF_YV12:
            case gcvSURF_I420:
            case gcvSURF_NV12:
            case gcvSURF_NV21:
                dstFormat = gcvSURF_YUY2;
                yuvFormat = gcvTRUE;
                break;

            case gcvSURF_YUY2:
            case gcvSURF_UYVY:
                dstFormat = attributes.format;
                yuvFormat = gcvTRUE;
                break;

            default:
                /* Get closest supported destination. */
                status = gcoTEXTURE_GetClosestFormat(context->hal,
                                                     attributes.format,
                                                     &dstFormat);

                break;
            }

            if (gcmIS_ERROR(status))
            {
                glmERROR(GL_INVALID_ENUM);
                break;
            }

            /* Get a shortcut to the bound texture. */
            texture = sampler->bindings[glvTEXTUREEXTERNAL];
        }

        if (texture == gcvNULL)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

#if defined(ANDROID) && (ANDROID_SDK_VERSION >= 7)
        /* Handle linear Android native buffer case. */
        if (image->type == KHR_IMAGE_ANDROID_NATIVE_BUFFER
        &&  yuvFormat == gcvFALSE
        )
        {
            gceSURF_TYPE type     = gcvSURF_TYPE_UNKNOWN;

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
            gcmERR_BREAK(gcoSURF_GetFormat(attributes.surface, &type, gcvNULL));

            if (type != gcvSURF_TEXTURE)
            {
                /* RGB linear surface. */
                gctBOOL reset  = gcvTRUE;

                do
                {
                    gcoSURF mipmap = gcvNULL;

                    gctUINT width;
                    gctUINT height;
                    gctUINT depth;
                    gceSURF_FORMAT format;
                    gceSURF_TYPE type;

                    if (texture->direct.source != gcvNULL)
                    {
                        /* Formerly bound a yuv texture. */
                        break;
                    }

                    if (texture->object == gcvNULL)
                    {
                        /* No texture object, do nothing here. */
                        reset = gcvFALSE;

                        break;
                    }

                    if (texture->uploaded == gcvFALSE)
                    {
                        /* Memory reduction: last composition is not done by
                         * 3D. So we will destroy the texture object. */
                        break;
                    }

                    /* Get mipmap level 0. */
                    if (gcmIS_ERROR(
                            gcoTEXTURE_GetMipMap(texture->object, 0, &mipmap)))
                    {
                        /* Can not get mipmap level 0, which means this texture
                         * object is clean. Destroy it. */
                        break;
                    }

                    /* Mipmap level 1 is available, can not skip reset. */
                    if (gcmIS_SUCCESS(
                            gcoTEXTURE_GetMipMap(texture->object, 1, &mipmap)))
                    {
                        /* This texture object has more than 1 levels, need destroy
                         * and re-create again. */
                        break;
                    }

                    /* Query size and format. */
                    gcmERR_BREAK(gcoSURF_GetSize(mipmap, &width, &height, &depth));

                    gcmERR_BREAK(gcoSURF_GetFormat(mipmap, &type, &format));

                    if ((width  == attributes.width)
                    &&  (height == attributes.height)
                    &&  (depth  == 1)
                    &&  (format == dstFormat)
                    &&  (gcvSURF_TEXTURE == type)
                    )
                    {
                        /* All parameters are matched, we do not need re-create
                         * another texture object. */
                        reset = gcvFALSE;
                    }
                }
                while (gcvFALSE);

                if (reset)
                {
                    /* Destroy the texture. */
                    gcmVERIFY_OK(_ResetTextureWrapper(context, texture));

                    /* Commit. */
                    gcmVERIFY_OK(gcoHAL_Commit(gcvNULL, gcvFALSE));
                }

                /* Save linear source surface. */
                texture->source = attributes.surface;

                /* Clear uploaded flag. */
                texture->uploaded = gcvFALSE;
            }
            else
            {
                /* RGB tile surface. */
                /* Destroy the texture. */
                if ((texture->object != gcvNULL)
                || (texture->direct.source != gcvNULL)
                )
                {
                    gcmVERIFY_OK(_ResetTextureWrapper(context, texture));
                }

                status = gcoTEXTURE_Construct(context->hal, &texture->object);

                /* Verify creation. */
                if (gcmIS_ERROR(status))
                {
                    glmERROR(GL_OUT_OF_MEMORY);
                    break;
                }

                /* Add mipmap from eglImage surface. Client 'owns' the surface. */
                status = gcoTEXTURE_AddMipMapFromClient(
                    texture->object, attributes.level, attributes.surface
                    );

                if (gcmIS_ERROR(status))
                {
                    gcmFATAL("%s: could not add mipmap surface.", __FUNCTION__);
                    glmERROR(GL_INVALID_VALUE);
                    break;
                }
            }

            texture->privHandle = attributes.privHandle;
        }
        else
#endif

        if (yuvFormat)
        {
            /* YUV surface path. */
            gctBOOL reset     = gcvTRUE;

            do
            {
                gcoSURF mipmap = gcvNULL;

                gctUINT width;
                gctUINT height;
                gctUINT depth;
                gceSURF_FORMAT format;
                gceSURF_TYPE type;

#if defined(ANDROID)
                if (texture->source != gcvNULL)
                {
                    /* Formerly bound a linear texture. */
                    break;
                }
#endif

                if (texture->object == gcvNULL)
                {
                    /* No texture object, do nothing here. */
                    reset = gcvFALSE;

                    break;
                }

                if (texture->uploaded == gcvFALSE)
                {
                    /* Memory reduction: last composition is not done by
                     * 3D. So we will destroy the texture object. */
                    break;
                }

                /* Get mipmap level 0. */
                if (gcmIS_ERROR(
                        gcoTEXTURE_GetMipMap(texture->object, 0, &mipmap)))
                {
                    /* Can not get mipmap level 0, which means this texture
                     * object is clean. Destroy it. */
                    break;
                }

                /* Mipmap level 1 is available, can not skip reset. */
                if (gcmIS_SUCCESS(
                        gcoTEXTURE_GetMipMap(texture->object, 1, &mipmap)))
                {
                    /* This texture object has more than 1 levels, need destroy
                     * and re-create again. */
                    break;
                }

                /* Query size and format. */
                gcmERR_BREAK(gcoSURF_GetSize(mipmap, &width, &height, &depth));

                gcmERR_BREAK(gcoSURF_GetFormat(mipmap, &type, &format));

                if ((width  == attributes.width)
                &&  (height == attributes.height)
                &&  (depth  == 1)
                &&  (format == dstFormat)
                &&  (gcvSURF_TEXTURE == type)
                )
                {
                    /* All parameters are matched, we do not need re-create
                     * another texture object. */
                    reset = gcvFALSE;
                }
            }
            while (gcvFALSE);

            if (reset)
            {
                /* Destroy the texture. */
                gcmVERIFY_OK(_ResetTextureWrapper(context, texture));

                /* Commit. */
                gcmVERIFY_OK(gcoHAL_Commit(gcvNULL, gcvFALSE));
            }

            /* Save linear source surface. */
            texture->direct.source = attributes.surface;

            /* Clear uploaded flag. */
            texture->uploaded = gcvFALSE;

#if defined(ANDROID)
            /* Save private handle. */
            texture->privHandle = attributes.privHandle;
#endif
        }

        else
        {
            /* Surface is RGB tile. */
            /* Destroy the texture. */
            if ((texture->object != gcvNULL) || (texture->direct.source != gcvNULL))
            {
                gcmVERIFY_OK(_ResetTextureWrapper(context, texture));
            }

            status = gcoTEXTURE_Construct(context->hal, &texture->object);

            /* Verify creation. */
            if (gcmIS_ERROR(status))
            {
                glmERROR(GL_OUT_OF_MEMORY);
                break;
            }

            /* Add mipmap from eglImage surface. */
            status = gcoTEXTURE_AddMipMapFromSurface(
                texture->object, attributes.level, attributes.surface
                );

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s: could not get mipmap surface.", __FUNCTION__);
                glmERROR(GL_INVALID_VALUE);
                break;
            }

            /* Add reference count. */
            status = gcoSURF_ReferenceSurface(attributes.surface);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s: could not increase surface reference.", __FUNCTION__);
                glmERROR(GL_INVALID_VALUE);
                break;
            }
        }

        /* Level 0 is special. */
        if (attributes.level == 0)
        {
            GLenum baseFormat;
            GLint tValue;

            /* Invalidate normalized crop rectangle. */
            texture->dirtyCropRect = GL_TRUE;

            /* Set texture parameters. */
            texture->width  = attributes.width;
            texture->height = attributes.height;

            /* Determine the number of mipmaps. */
            tValue=glfGetMaxLOD(texture->width, texture->height);
            texture->maxLevel = gcmMIN(tValue, texture->maxLevel);

            /* Update texture format. */
            _QueryImageBaseFormat(image, &baseFormat);
            _SetTextureWrapperFormat(context, texture, baseFormat);

            /* Auto generate mipmaps only for pot textures. */
            if ((texture->genMipmap) &&
                (texture->object != gcvNULL) &&
                ((attributes.width  & (attributes.width  - 1)) == 0) &&
                ((attributes.height & (attributes.height - 1)) == 0))
            {
                gcmERR_BREAK(glfGenerateMipMaps(
                    context, texture, dstFormat, 0,
                    attributes.width, attributes.height, 0
                    ));
            }
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glTexGenxOES (GL_OES_texture_cube_map)
**
**  Set of functions to configure the texture generation modes.
**
**  INPUT:
**
**      Coord
**          Specifies the type of coordinates for which the mode is to be
**          modified. Must be GL_TEXTURE_GEN_STR_OES (S, T, R coordinates).
**
**      Name
**          The name of the parameter that is being modified.
**          Must be GL_TEXTURE_GEN_MODE_OES.
**
**      Param, Params
**          The new value for the parameter. Can be either one of the following:
**              GL_NORMAL_MAP_OES
**              GL_REFLECTION_MAP_OES
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glTexGenfOES(
    GLenum Coord,
    GLenum Name,
    GLfloat Param
    )
{
    glmENTER3(glmARGENUM, Coord, glmARGENUM, Name,
              glmARGFLOAT, Param)
    {
        if (!_SetTexGen(context, Coord, Name, &Param, glvFLOAT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexGenfvOES(
    GLenum Coord,
    GLenum Name,
    const GLfloat * Params
    )
{
    glmENTER3(glmARGENUM, Coord, glmARGENUM, Name,
              glmARGPTR, Params)
    {
        if (!_SetTexGen(context, Coord, Name, Params, glvFLOAT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexGeniOES(
    GLenum Coord,
    GLenum Name,
    GLint Param
    )
{
    glmENTER3(glmARGENUM, Coord, glmARGENUM, Name,
              glmARGFLOAT, Param)
    {
        if (!_SetTexGen(context, Coord, Name, &Param, glvINT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexGenivOES(
    GLenum Coord,
    GLenum Name,
    const GLint * Params
    )
{
    glmENTER3(glmARGENUM, Coord, glmARGENUM, Name, glmARGPTR, Params)
    {
        if (!_SetTexGen(context, Coord, Name, Params, glvINT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexGenxOES(
    GLenum Coord,
    GLenum Name,
    GLfixed Param
    )
{
    glmENTER3(glmARGENUM, Coord, glmARGENUM, Name, glmARGFLOAT, Param)
    {
        if (!_SetTexGen(context, Coord, Name, &Param, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glTexGenxvOES(
    GLenum Coord,
    GLenum Name,
    const GLfixed * Params
    )
{
    glmENTER3(glmARGENUM, Coord, glmARGENUM, Name, glmARGPTR, Params)
    {
        if (!_SetTexGen(context, Coord, Name, Params, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glGetTexGenxOES (GL_OES_texture_cube_map)
**
**  Set of functions to retrieve the texture generation modes.
**
**  INPUT:
**
**      Coord
**          Specifies the type of coordinates for which the mode is to be
**          modified. Must be GL_TEXTURE_GEN_STR_OES (S, T, R coordinates).
**
**      Name
**          The name of the parameter that is being modified.
**          Must be GL_TEXTURE_GEN_MODE_OES.
**
**      Params
**          The pointer to the return value.
**
**  OUTPUT:
**
**      Params
**          The requested state value.
*/

GL_API void GL_APIENTRY glGetTexGenfvOES(
    GLenum Coord,
    GLenum Name,
    GLfloat * Params
    )
{
    glmENTER3(glmARGENUM, Coord, glmARGENUM, Name, glmARGPTR, Params)
    {
        if (!_GetTexGen(context, Coord, Name, Params, glvFLOAT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glGetTexGenivOES(
    GLenum Coord,
    GLenum Name,
    GLint * Params
    )
{
    glmENTER3(glmARGENUM, Coord, glmARGENUM, Name, glmARGPTR, Params)
    {
        if (!_GetTexGen(context, Coord, Name, Params, glvINT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glGetTexGenxvOES(
    GLenum Coord,
    GLenum Name,
    GLfixed * Params
    )
{
    glmENTER3(glmARGENUM, Coord, glmARGENUM, Name, glmARGPTR, Params)
    {
        if (!_GetTexGen(context, Coord, Name, Params, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glGenerateMipmapOES (OES_framebuffer_object)
**
**  Generate mipmap for spcific texture.
**
**  Arguments:
**
**      GLenum Target
**          Specifies the target texture.
**
**  OUTPUT:
**
**      Nothing.
**
*/

GL_API void GL_APIENTRY glGenerateMipmapOES(
    GLenum Target
    )
{
    gceSTATUS status;

    glmENTER1(glmARGENUM, Target)
    {
        gctINT faces;
        gceSURF_FORMAT format;
        glsTEXTURESAMPLER_PTR sampler;
        glsTEXTUREWRAPPER_PTR texture;
        gcoSURF surface;
        gctUINT width, height;

        gcmDUMP_API("${ES11 glGenerateMipmapOES 0x%08X}", Target);

        /* Get a shortcut to the active sampler. */
        sampler = context->texture.activeSampler;

        switch (Target)
        {
        case GL_TEXTURE_2D:
            texture = sampler->bindings[glvTEXTURE2D];
            faces   = 0;
            break;

        case GL_TEXTURE_CUBE_MAP_OES:
            texture = sampler->bindings[glvCUBEMAP];
            faces   = 6;
            break;

        default:
        gcmFATAL("glGenerateMipmap: Invalid target: %04X", Target);
        glmERROR(GL_INVALID_ENUM);
        goto OnError;
        }

        gcmASSERT(texture != gcvNULL);

        /* Make sure the texture is not empty. */
        if (texture->object == gcvNULL)
        {
        gcmTRACE(gcvLEVEL_WARNING,
                    "glGenerateMipMap: No texture object created for target %04X",
                    Target);

            glmERROR(GL_INVALID_OPERATION);
            goto OnError;
        }

        gcmERR_BREAK(gcoTEXTURE_GetMipMap(texture->object,
                                        0,
                                        &surface));

        gcmERR_BREAK(gcoSURF_GetFormat(surface, gcvNULL, &format));

        gcmERR_BREAK(gcoSURF_GetSize(surface, &width, &height, gcvNULL));

        gcmERR_BREAK(glfGenerateMipMaps(
                context, texture, format, 0, width, height, faces
                ));

        /* Easy return on error. */
OnError:;
    }
    glmLEAVE();
}
