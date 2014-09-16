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




#ifndef __gc_hal_user_hardware_vg_h_
#define __gc_hal_user_hardware_vg_h_

#include "gc_hal_user_vg.h"

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

/* gcoHARDWARE object. */
struct _gcoVGHARDWARE
{
    /* Object. */
    gcsOBJECT                   object;

    /* Pointer to the gcoHAL object. */
    gcoHAL                      hal;

    /* Pointer to the gcoOS object. */
    gcoOS                       os;

    /* Command buffer. */
    gcoVGBUFFER                 buffer;
    gcsCOMMAND_BUFFER_INFO      bufferInfo;

    /* Context buffer. */
#ifndef __QNXNTO__
    gcsVGCONTEXT                context;
#else
    gcsVGCONTEXT              * pContext;
#endif

    /* Chip characteristics. */
    gceCHIPMODEL                chipModel;
    gctUINT32                   chipRevision;
    gctUINT32                   chipFeatures;
    gctUINT32                   chipMinorFeatures;
    gctUINT32                   chipMinorFeatures2;

    /* API type. */
    gceAPI                      api;

    /* Temporary buffer parameters. */
    gctUINT                     tempBufferSize;
    struct _gcsSURF_INFO        tempBuffer;

    /* Filter blit. */
    gceFILTER_TYPE              loadedFilterType;
    gctUINT8                    loadedKernelSize;
    gctUINT32                   loadedScaleFactor;

    gceFILTER_TYPE              newFilterType;
    gctUINT8                    newHorKernelSize;
    gctUINT8                    newVerKernelSize;

    gcsFILTER_BLIT_ARRAY        horSyncFilterKernel;
    gcsFILTER_BLIT_ARRAY        verSyncFilterKernel;

    gcsFILTER_BLIT_ARRAY        horBlurFilterKernel;
    gcsFILTER_BLIT_ARRAY        verBlurFilterKernel;

    /* Depth mode. */
    gceDEPTH_MODE               depthMode;
    gctBOOL                     depthOnly;

    /* Maximum depth value. */
    gctUINT32                   maxDepth;
    gctBOOL                     earlyDepth;

    /* Stencil mask. */
    gctBOOL                     stencilEnabled;
    gctUINT32                   stencilMode;
    gctBOOL                     stencilKeepFront[3];
    gctBOOL                     stencilKeepBack[3];

    /* Texture sampler modes. */
    gctUINT32                   samplerMode[12];
    gctUINT32                   samplerLOD[12];

    /* Stall before rendingering triangles. */
    gctBOOL                     stallPrimitive;
    gctUINT32                   memoryConfig;

    /* Anti-alias mode. */
    gctUINT32                   sampleMask;
    gctUINT32                   sampleEnable;
    gcsSAMPLES                  samples;
    struct
    {
        gctUINT8                x;
        gctUINT8                y;
    }                           sampleCoords[4][4];
    gctUINT8                    jitterIndex[4][4];

    /* Dither. */
    gctUINT32                   dither[2];

    /***************************************************************************
    ** Feature present flags.
    */

    gctBOOL                     fe20;
    gctBOOL                     vg20;
    gctBOOL                     vg21;
    gctBOOL                     vgFilter;
    gctBOOL                     vgDoubleBuffer;
    gctBOOL                     vgRestart;
    gctBOOL                     vgPEDither;
    gctBOOL                     hw2DEngine;
    gctBOOL                     hw3DEngine;
    gctBOOL                     hwVGEngine;
    gctBOOL                     pipeEnabled[3];

    /***************************************************************************
    ** 2D states.
    */

    /* Software 2D force flag. */
    gctBOOL                     sw2DEngine;

    /* Byte write capability. */
    gctBOOL                     byteWrite;

    /* Pattern states. */
    gcoBRUSH_CACHE              brushCache;

    /* Transparency states. */
    gctUINT32                   transparency;
    gctUINT32                   transparencyColor;

    /* 2D clipping rectangle. */
    gcsRECT                     clippingRect;

    /* Source rectangle. */
    gcsRECT                     sourceRect;

    /* Surface information. */
    struct _gcsSURF_INFO        sourceSurface;
    struct _gcsSURF_INFO        targetSurface;

    /***************************************************************************
    ** VG states.
    */

    struct _gcsVG
    {
        /* Control registers. */
        gctUINT32                   targetControl;
        gctUINT32                   vgControl;

        /* Tesselator states. */
        gctUINT32                   tsQuality;
        gctUINT32                   tsDataType;
        gctUINT32                   tsFillRule;
        gctUINT32                   tsMode;

        /* Target surface. */
        gcsSURF_INFO_PTR            target;
        gctINT                      targetWidth;
        gctINT                      targetHeight;
        gctBOOL                     targetPremultiplied;

        /* Premultiplication states. */
        gctUINT32                   colorPremultiply;
        gctUINT32                   targetPremultiply;

        /* Flush states. */
        gctBOOL                     peFlushNeeded;

        /* Blending mode. */
        gceVG_BLEND                 blendMode;

        /* Color transform enable. */
        gctBOOL                     colorTransform;

        /* Primitive states. */
        gceVG_PRIMITIVE             primitiveMode;
        gceVG_PRIMITIVE             tesselationPrimitive;

        /* Image mode. */
        gceVG_IMAGE                 imageMode;

        /* Masking enable state. */
        gctBOOL                     maskEnable;

        /* Dither enable state. */
        gctBOOL                     ditherEnable;

        /* Color space modes. */
        gctBOOL                     targetLinear;
        gctBOOL                     paintLinear;
        gctBOOL                     imageLinear;

        /* Scissor surface. */
        gctBOOL                     scissorEnable;
        gcoSURF                     scissor;
        gctUINT32                   scissorAddress;
        gctPOINTER                  scissorBits;

        /* Fill color. */
        gctUINT32                   fillColor;

        /* Filter parameters. */
        gctUINT                     filterMethod;
        gceCHANNEL                  filterChannels;
        gctUINT                     filterColorSpace;
        gctUINT                     filterPremultiply;
        gctUINT                     filterDemultiply;
        gctUINT                     filterKernelSize;

        /* Software tesselation parameters. */
        gcsTESSELATION_PTR          tsBuffer;

        gctFLOAT                    bias;
        gctFLOAT                    scale;

        gctFLOAT                    userToSurface[6];

        /* TS buffer semaphore states. */
        gctUINT                     stallSkipCount;
    }
    vg;
};

#endif /* __gc_hal_user_hardware_h_ */

