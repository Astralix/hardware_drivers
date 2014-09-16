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
#include <EGL/egl.h>
#include "gc_hal_user_compiler.h"

#define _GC_OBJ_ZONE glvZONE_TRACE

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

typedef struct _glsFPFUNCTIONINFO * glsFPFUNCTIONINFO_PTR;
typedef struct _glsFPFUNCTIONINFO
{
    gceTEXTURE_FUNCTION function;
    gceTEXTURE_SOURCE   source0;
    gceTEXTURE_CHANNEL  channel0;
    gceTEXTURE_SOURCE   source1;
    gceTEXTURE_CHANNEL  channel1;
    gceTEXTURE_SOURCE   source2;
    gceTEXTURE_CHANNEL  channel2;
}
glsFPFUNCTIONINFO;

typedef struct _glsFPFUNCTIONPAIR * glsFPFUNCTIONPAIR_PTR;
typedef struct _glsFPFUNCTIONPAIR
{
    glsFPFUNCTIONINFO   color;
    glsFPFUNCTIONINFO   alpha;
}
glsFPFUNCTIONPAIR;

typedef struct _glsFPINFO * glsFPINFO_PTR;
typedef struct _glsFPINFO
{
    gctBOOL             writeColor;
    gctBOOL             writeAlpha;
    gctINT              scale;
}
glsFPINFO;

typedef struct _glsFPINFOPAIR * glsFPINFOPAIR_PTR;
typedef struct _glsFPINFOPAIR
{
    glsFPINFO           color;
    glsFPINFO           alpha;
}
glsFPINFOPAIR;


/*******************************************************************************
**
**  _GetTextureFunctionConfig
**
**  Determine texture function configuration for the specified sampler.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Sampler
**          Pointer to the sampler.
**
**  OUTPUT:
**
**      Information
**          Pointer to the information structure.
**
**      Configuration
**          Pointer to the configuration structure.
*/

static void _GetTextureFunctionConfig(
    glsCONTEXT_PTR Context,
    glsTEXTURESAMPLER_PTR Sampler,
    glsFPINFOPAIR_PTR Information,
    glsFPFUNCTIONPAIR_PTR Configuration
    )
{
    static glsFPFUNCTIONPAIR function[] =
    {
        /* glvTEXREPLACE: */
        {
            {
                gcvTEXTURE_REPLACE,
                    gcvCOLOR_FROM_TEXTURE,        gcvFROM_COLOR,
                    (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0,
                    (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0
            },
            {
                gcvTEXTURE_REPLACE,
                    gcvCOLOR_FROM_TEXTURE,        gcvFROM_ALPHA,
                    (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0,
                    (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0
            }
        },

        /* glvTEXMODULATE: */
        {
            {
                gcvTEXTURE_MODULATE,
                    gcvCOLOR_FROM_PREVIOUS_COLOR, gcvFROM_COLOR,
                    gcvCOLOR_FROM_TEXTURE,        gcvFROM_COLOR,
                    (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0
            },
            {
                gcvTEXTURE_MODULATE,
                    gcvCOLOR_FROM_PREVIOUS_COLOR, gcvFROM_ALPHA,
                    gcvCOLOR_FROM_TEXTURE,        gcvFROM_ALPHA,
                    (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0
            }
        },

        /* glvTEXDECAL: */
        {
            {
                gcvTEXTURE_REPLACE,
                    gcvCOLOR_FROM_TEXTURE,        gcvFROM_COLOR,
                    (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0,
                    (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0
            },
            {
                gcvTEXTURE_DUMMY,
                (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0,
                (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0,
                (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0
            }
        },

        /* glvTEXBLEND: */
        {
            {
                gcvTEXTURE_INTERPOLATE,
                    gcvCOLOR_FROM_CONSTANT_COLOR, gcvFROM_COLOR,
                    gcvCOLOR_FROM_PREVIOUS_COLOR, gcvFROM_COLOR,
                    gcvCOLOR_FROM_TEXTURE,        gcvFROM_COLOR
            },
            {
                gcvTEXTURE_MODULATE,
                    gcvCOLOR_FROM_PREVIOUS_COLOR, gcvFROM_ALPHA,
                    gcvCOLOR_FROM_TEXTURE,        gcvFROM_ALPHA,
                    (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0
            }
        },

        /* glvTEXADD: */
        {
            {
                gcvTEXTURE_ADD,
                    gcvCOLOR_FROM_PREVIOUS_COLOR, gcvFROM_COLOR,
                    gcvCOLOR_FROM_TEXTURE,        gcvFROM_COLOR,
                    (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0
            },
            {
                gcvTEXTURE_MODULATE,
                    gcvCOLOR_FROM_PREVIOUS_COLOR, gcvFROM_ALPHA,
                    gcvCOLOR_FROM_TEXTURE,        gcvFROM_ALPHA,
                    (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0
            }
        }
    };

    static glsFPFUNCTIONPAIR decalRGBA =
    {
        {
            gcvTEXTURE_INTERPOLATE,
                gcvCOLOR_FROM_TEXTURE,        gcvFROM_COLOR,
                gcvCOLOR_FROM_PREVIOUS_COLOR, gcvFROM_COLOR,
                gcvCOLOR_FROM_TEXTURE,        gcvFROM_ALPHA
        },
        {
            gcvTEXTURE_DUMMY,
            (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0,
            (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0,
            (gceTEXTURE_SOURCE) ~0, (gceTEXTURE_CHANNEL) ~0
        }
    };

    gcmHEADER_ARG("Context=0x%x Sampler=0x%04x Information=0x%x Configuration=0x%04x",
                    Context, Sampler, Information, Configuration);

    /* Set the functions. */
    if ((Sampler->function == glvTEXDECAL) &&
        ((Sampler->binding->format == GL_RGBA) ||
         (Sampler->binding->format == GL_BGRA_EXT)))
    {
        *Configuration = decalRGBA;
    }
    else
    {
        *Configuration = function[Sampler->function];
    }

    /* Set channel masks. */
    switch (Sampler->binding->format)
    {
    case GL_ALPHA:
        Information->color.writeColor = gcvFALSE;
        Information->color.writeAlpha = gcvFALSE;
        Information->alpha.writeColor = gcvFALSE;
        Information->alpha.writeAlpha = gcvTRUE;
        break;

    case GL_LUMINANCE:
    case GL_RGB:
        Information->color.writeColor = gcvTRUE;
        Information->color.writeAlpha = gcvFALSE;
        Information->alpha.writeColor = gcvFALSE;
        Information->alpha.writeAlpha = gcvFALSE;
        break;

    case GL_LUMINANCE_ALPHA:
    case GL_RGBA:
    case GL_BGRA_EXT:
        Information->color.writeColor = gcvTRUE;
        Information->color.writeAlpha = gcvFALSE;
        Information->alpha.writeColor = gcvFALSE;
        Information->alpha.writeAlpha = gcvTRUE;
        break;
    }

    /* Set scale. */
    Information->color.scale = 1;
    Information->alpha.scale = 1;
    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  _GetTextureCombineFunctionConfig
**
**  Determine texture combine function configuration for the specified sampler.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Sampler
**          Pointer to the sampler information.
**
**      RGBChannels
**          Specifies whether this call is for RGB channels.
**
**      Disable
**          Specifies whether the channel should be disabled.
**
**  OUTPUT:
**
**      Information
**          Pointer to the information structure.
**
**      Configuration
**          Pointer to the configuration structure.
*/

static void _GetTextureCombineFunctionConfig(
    glsCONTEXT_PTR Context,
    glsTEXTURECOMBINE_PTR Sampler,
    gctBOOL RGBChannels,
    gctBOOL Disable,
    glsFPINFO_PTR Information,
    glsFPFUNCTIONINFO_PTR Configuration
    )
{
    static gceTEXTURE_SOURCE source[] =
    {
        gcvCOLOR_FROM_TEXTURE,          /* glvTEXTURE  */
        gcvCOLOR_FROM_CONSTANT_COLOR,   /* glvCONSTANT */
        gcvCOLOR_FROM_PRIMARY_COLOR,    /* glvCOLOR    */
        gcvCOLOR_FROM_PREVIOUS_COLOR    /* glvPREVIOUS */
    };

    static gceTEXTURE_CHANNEL channel[] =
    {
        gcvFROM_ALPHA,                  /* glvSRCALPHA */
        gcvFROM_ONE_MINUS_ALPHA,        /* glvSRCALPHAINV */
        gcvFROM_COLOR,                  /* glvSRCCOLOR */
        gcvFROM_ONE_MINUS_COLOR,        /* glvSRCCOLORINV */
    };

    gctINT scale;
    gcmHEADER_ARG("Context=0x%x Sampler=0x%04x RGBChannels=%d Disable=%d "
                    "Information=0x%x Configuration=0x%x",
                    Context, Sampler, RGBChannels, Disable, Information, Configuration);

    /* Set the scale value. */
    glfGetFromMutant(&Sampler->scale, &scale, glvINT);
    Information->scale = scale;

    /* Set channel enable flags. */
    if (Disable)
    {
        Information->writeColor = gcvFALSE;
        Information->writeAlpha = gcvFALSE;

        Configuration->function = gcvTEXTURE_DUMMY;
        Configuration->source0  = (gceTEXTURE_SOURCE)  ~0;
        Configuration->channel0 = (gceTEXTURE_CHANNEL) ~0;
        Configuration->source1  = (gceTEXTURE_SOURCE)  ~0;
        Configuration->channel1 = (gceTEXTURE_CHANNEL) ~0;
        Configuration->source2  = (gceTEXTURE_SOURCE)  ~0;
        Configuration->channel2 = (gceTEXTURE_CHANNEL) ~0;
    }
    else
    {
        if (Sampler->function == glvCOMBINEDOT3RGBA)
        {
            Information->writeColor = gcvTRUE;
            Information->writeAlpha = gcvTRUE;
        }
        else if (RGBChannels)
        {
            Information->writeColor = gcvTRUE;
            Information->writeAlpha = gcvFALSE;
        }
        else
        {
            Information->writeColor = gcvFALSE;
            Information->writeAlpha = gcvTRUE;
        }

        /* Determine the function configuration. */
        switch (Sampler->function)
        {
        case glvCOMBINEREPLACE:
            Configuration->function = gcvTEXTURE_REPLACE;
            Configuration->source0  = source[Sampler->source[0]];
            Configuration->channel0 = channel[Sampler->operand[0]];
            Configuration->source1  = (gceTEXTURE_SOURCE)  ~0;
            Configuration->channel1 = (gceTEXTURE_CHANNEL) ~0;
            Configuration->source2  = (gceTEXTURE_SOURCE)  ~0;
            Configuration->channel2 = (gceTEXTURE_CHANNEL) ~0;
            break;

        case glvCOMBINEMODULATE:
            Configuration->function = gcvTEXTURE_MODULATE;
            Configuration->source0  = source[Sampler->source[0]];
            Configuration->channel0 = channel[Sampler->operand[0]];
            Configuration->source1  = source[Sampler->source[1]];
            Configuration->channel1 = channel[Sampler->operand[1]];
            Configuration->source2  = (gceTEXTURE_SOURCE)  ~0;
            Configuration->channel2 = (gceTEXTURE_CHANNEL) ~0;
            break;

        case glvCOMBINEADD:
            Configuration->function = gcvTEXTURE_ADD;
            Configuration->source0  = source[Sampler->source[0]];
            Configuration->channel0 = channel[Sampler->operand[0]];
            Configuration->source1  = source[Sampler->source[1]];
            Configuration->channel1 = channel[Sampler->operand[1]];
            Configuration->source2  = (gceTEXTURE_SOURCE)  ~0;
            Configuration->channel2 = (gceTEXTURE_CHANNEL) ~0;
            break;

        case glvCOMBINEADDSIGNED:
            Configuration->function = gcvTEXTURE_ADD_SIGNED;
            Configuration->source0  = source[Sampler->source[0]];
            Configuration->channel0 = channel[Sampler->operand[0]];
            Configuration->source1  = source[Sampler->source[1]];
            Configuration->channel1 = channel[Sampler->operand[1]];
            Configuration->source2  = (gceTEXTURE_SOURCE)  ~0;
            Configuration->channel2 = (gceTEXTURE_CHANNEL) ~0;
            break;

        case glvCOMBINEINTERPOLATE:
            Configuration->function = gcvTEXTURE_INTERPOLATE;
            Configuration->source0  = source[Sampler->source[0]];
            Configuration->channel0 = channel[Sampler->operand[0]];
            Configuration->source1  = source[Sampler->source[1]];
            Configuration->channel1 = channel[Sampler->operand[1]];
            Configuration->source2  = source[Sampler->source[2]];
            Configuration->channel2 = channel[Sampler->operand[2]];
            break;

        case glvCOMBINESUBTRACT:
            Configuration->function = gcvTEXTURE_SUBTRACT;
            Configuration->source0  = source[Sampler->source[0]];
            Configuration->channel0 = channel[Sampler->operand[0]];
            Configuration->source1  = source[Sampler->source[1]];
            Configuration->channel1 = channel[Sampler->operand[1]];
            Configuration->source2  = (gceTEXTURE_SOURCE)  ~0;
            Configuration->channel2 = (gceTEXTURE_CHANNEL) ~0;
            break;

        case glvCOMBINEDOT3RGB:
        case glvCOMBINEDOT3RGBA:
            Configuration->function = gcvTEXTURE_DOT3;
            Configuration->source0  = source[Sampler->source[0]];
            Configuration->channel0 = channel[Sampler->operand[0]];
            Configuration->source1  = source[Sampler->source[1]];
            Configuration->channel1 = channel[Sampler->operand[1]];
            Configuration->source2  = (gceTEXTURE_SOURCE)  ~0;
            Configuration->channel2 = (gceTEXTURE_CHANNEL) ~0;
            break;
        }
    }
    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  glfUpdateFragmentProcessor
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

gceSTATUS glfUpdateFragmentProcessor(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x", Context);
    do
    {
        GLint i;
        gctBOOL colorFromStream;
        gctBOOL pointSmooth;
        gctUINT32 clipPlanes;

        /* Determine whether the color comes from the stream. */
        colorFromStream
            =  (Context->lightingStates.lightingEnabled && !Context->drawTexOESEnabled)
            || Context->aColorInfo.streamEnabled;

        /* Determine whether antialised points are enabled. */
        pointSmooth
            =   Context->pointStates.pointPrimitive
            &&  Context->pointStates.smooth
            && !Context->pointStates.spriteEnable;

        /* Reset clip planes. */
        clipPlanes = 0;

        /* Determine whether user cllipping is enabled. */
        for (i = 0; i < 6; i++)
        {
            if (Context->clipPlaneEnabled[i])
            {
                clipPlanes |= (1 << i);
            }
        }

        /* Set the generic configuration of the fragment processor. */
        gcmERR_BREAK(gco3D_SetFragmentConfiguration(
            Context->hw,
            colorFromStream,
            Context->fogStates.enabled,
            pointSmooth,
            clipPlanes
            ));

        /* Set the constant color if color stream is not used. */
        if (!colorFromStream)
        {
            gltFRACTYPE constColor[4];

            glfGetFromVector4(
                &Context->aColorInfo.currValue,
                constColor, glvFRACTYPEENUM
                );

            gcmERR_BREAK(gco3D_SetFragmentColor(
                Context->hw,
                constColor[0],
                constColor[1],
                constColor[2],
                constColor[3]
                ));
        }

        /* Set the fog color. */
        if (Context->fogStates.enabled)
        {
            gltFRACTYPE fogColor[4];

            glfGetFromVector4(
                &Context->fogStates.color,
                fogColor, glvFRACTYPEENUM
                );

            gcmERR_BREAK(gco3D_SetFogColor(
                Context->hw,
                fogColor[0],
                fogColor[1],
                fogColor[2],
                fogColor[3]
                ));
        }

        for (i = 0; i < Context->texture.pixelSamplers; i++)
        {
            glsTEXTURESAMPLER_PTR sampler = &Context->texture.sampler[i];

            if (sampler->stageEnabled)
            {
                gltFRACTYPE texureColor[4];
                glsFPINFOPAIR information = {{0}};
                glsFPFUNCTIONPAIR configuration = {{gcvTEXTURE_DUMMY}};

                /* Determine the fragment processor configuration. */
                if (sampler->function == glvTEXCOMBINE)
                {
                    _GetTextureCombineFunctionConfig(
                        Context, &sampler->combColor,
                        gcvTRUE,
                        gcvFALSE,
                        &information.color,
                        &configuration.color
                        );

                    _GetTextureCombineFunctionConfig(
                        Context, &sampler->combAlpha,
                        gcvFALSE,
                        sampler->combColor.function == glvCOMBINEDOT3RGBA,
                        &information.alpha,
                        &configuration.alpha
                        );
                }
                else
                {
                    _GetTextureFunctionConfig(
                        Context, sampler,
                        &information,
                        &configuration
                        );
                }

                /* Enable the texture stage. */
                gcmERR_BREAK(gco3D_EnableTextureStage(
                    Context->hw, i, gcvTRUE
                    ));

                /* Set the color function result mask. */
                gcmERR_BREAK(gco3D_SetTextureColorMask(
                    Context->hw, i,
                    information.color.writeColor,
                    information.color.writeAlpha
                    ));

                /* Set the color function result mask. */
                gcmERR_BREAK(gco3D_SetTextureAlphaMask(
                    Context->hw, i,
                    information.alpha.writeColor,
                    information.alpha.writeAlpha
                    ));

                /* Get the texture color. */
                glfGetFromVector4(
                    &sampler->constColor,
                    texureColor, glvFRACTYPEENUM
                    );

                /* Set the constant texture color. */
                gcmERR_BREAK(gco3D_SetTetxureColor(
                    Context->hw, i,
                    texureColor[0],
                    texureColor[1],
                    texureColor[2],
                    texureColor[3]
                    ));

                /* Set color texture function. */
                gcmERR_BREAK(gco3D_SetColorTextureFunction(
                    Context->hw, i,
                    configuration.color.function,
                    configuration.color.source0,
                    configuration.color.channel0,
                    configuration.color.source1,
                    configuration.color.channel1,
                    configuration.color.source2,
                    configuration.color.channel2,
                    information.color.scale
                    ));

                /* Set alpha texture function. */
                gcmERR_BREAK(gco3D_SetAlphaTextureFunction(
                    Context->hw, i,
                    configuration.alpha.function,
                    configuration.alpha.source0,
                    configuration.alpha.channel0,
                    configuration.alpha.source1,
                    configuration.alpha.channel1,
                    configuration.alpha.source2,
                    configuration.alpha.channel2,
                    information.alpha.scale
                    ));
            }
            else
            {
                /* Disable the texture stage. */
                gcmERR_BREAK(gco3D_EnableTextureStage(
                    Context->hw, i, gcvFALSE
                    ));
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}
