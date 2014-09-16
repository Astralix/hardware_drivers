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




#ifndef __gc_hal_user_hardware_h_
#define __gc_hal_user_hardware_h_

#include "gc_hal_user_buffer.h"

#if gcdENABLE_VG
#include "gc_hal_user_vg.h"
#include "gc_hal_user_hardware_vg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define gcdSAMPLERS                 32
#define gcdSAMPLER_TS               8
#define gcdLOD_LEVELS               14
#define gcdTEMP_SURFACE_NUMBER      3

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

typedef enum
{
    gcvFILTER_BLIT_KERNEL_UNIFIED,
    gcvFILTER_BLIT_KERNEL_VERTICAL,
    gcvFILTER_BLIT_KERNEL_HORIZONTAL,
    gcvFILTER_BLIT_KERNEL_NUM,
}
gceFILTER_BLIT_KERNEL;

#ifndef VIVANTE_NO_3D
typedef struct _gcsCOLOR_INFO
{
    gctBOOL                     dirty;
    gctUINT32                   format;
    gctBOOL                     superTiled;
    gcsSURF_INFO_PTR            surface;
    gctUINT32                   cacheMode;
    gctUINT8                    rop;

    gctBOOL                     colorWrite;
    gctBOOL                     colorCompression;
    gctUINT32                   destinationRead;
}
gcsCOLOR_INFO;

typedef struct _gcsDEPTH_INFO
{
    gcsSURF_INFO_PTR            surface;
    gctUINT32                   cacheMode;

    /* Depth mode register value. */
    gctUINT32                   config;

    /* Depth config components. */
    gceDEPTH_MODE               mode;
    gctBOOL                     only;
    gctBOOL                     early;
    gctBOOL                     write;
    gceCOMPARE                  compare;

    /* Depth range. */
    gctFLOAT                    near;
    gctFLOAT                    far;
}
gcsDEPTH_INFO;

typedef struct _gcsPA_INFO
{
    gctBOOL                     aaLine;
    gctUINT                     aaLineTexSlot;
    gctFLOAT                    aaLineWidth;

    gceSHADING                  shading;
    gceCULL                     culling;
    gctBOOL                     pointSize;
    gctBOOL                     pointSprite;
    gceFILL                     fillMode;
}
gcsPA_INFO;

typedef struct _gcsSHADER_INFO
{
    gctSIZE_T                   stateBufferSize;
    gctUINT32_PTR               stateBuffer;
    gcsHINT_PTR                 hints;
}
gcsSHADER_INFO;

typedef struct _gcsCENTROIDS * gcsCENTROIDS_PTR;
typedef struct _gcsCENTROIDS
{
    gctUINT32                   value[4];
}
gcsCENTROIDS;

typedef enum
{
    gcvVAA_NONE,
    gcvVAA_COVERAGE_16,
    gcvVAA_COVERAGE_8,
}
gceVAA;

/* Composition state buffer definitions. */
#define gcdCOMP_BUFFER_COUNT        8
#define gcdCOMP_BUFFER_SIZE         (16 * 1024)

/* Composition layer descriptor. */
typedef struct _gcsCOMPOSITION_LAYER * gcsCOMPOSITION_LAYER_PTR;
typedef struct _gcsCOMPOSITION_LAYER
{
    /* Input parameters. */
    gcsCOMPOSITION                  input;

    /* Surface parameters. */
    gcsSURF_INFO_PTR                surface;
    gceSURF_TYPE                    type;
    gctUINT                         stride;
    gctUINT32                       swizzle;
    gctUINT32                       physical;

    gctUINT                         sizeX;
    gctUINT                         sizeY;

    gctUINT8                        samplesX;
    gctUINT8                        samplesY;

    gctUINT32                       format;
    gctBOOL                         hasAlpha;

    gctBOOL                         flip;

    /* Blending parameters. */
    gctBOOL                         needPrevious;
    gctBOOL                         replaceAlpha;
    gctBOOL                         modulateAlpha;

    /* Allocated resources. */
    gctUINT                         constRegister;
    gctUINT                         samplerNumber;
}
gcsCOMPOSITION_LAYER;

/* Composition layer node. */
typedef struct _gcsCOMPOSITION_NODE * gcsCOMPOSITION_NODE_PTR;
typedef struct _gcsCOMPOSITION_NODE
{
    /* Pointer to the layer structure. */
    gcsCOMPOSITION_LAYER_PTR        layer;

    /* Next layer node. */
    gcsCOMPOSITION_NODE_PTR         next;
}
gcsCOMPOSITION_NODE;

/* Set of overlapping layers. */
typedef struct _gcsCOMPOSITION_SET * gcsCOMPOSITION_SET_PTR;
typedef struct _gcsCOMPOSITION_SET
{
    /* Blurring layer. */
    gctBOOL                         blur;

    /* Bounding box. */
    gcsRECT                         trgRect;

    /* List of layer nodes. */
    gcsCOMPOSITION_NODE_PTR         nodeHead;
    gcsCOMPOSITION_NODE_PTR         nodeTail;

    /* Pointer to the previous/next sets. */
    gcsCOMPOSITION_SET_PTR          prev;
    gcsCOMPOSITION_SET_PTR          next;
}
gcsCOMPOSITION_SET;

/* Composition state buffer. */
typedef struct _gcsCOMPOSITION_STATE_BUFFER * gcsCOMPOSITION_STATE_BUFFER_PTR;
typedef struct _gcsCOMPOSITION_STATE_BUFFER
{
    gctSIGNAL                       signal;

    gctSIZE_T                       size;
    gctPHYS_ADDR                    physical;
    gctUINT32                       address;
    gctUINT32_PTR                   logical;
    gctUINT                         reserve;

    gctUINT32_PTR                   head;
    gctUINT32_PTR                   tail;
    gctINT                          available;
    gctUINT                         count;
    gctUINT32_PTR                   rectangle;

    gcsCOMPOSITION_STATE_BUFFER_PTR next;
}
gcsCOMPOSITION_STATE_BUFFER;

/* Composition states. */
typedef struct _gcsCOMPOSITION_STATE
{
    /* State that indicates whether we are in the composition mode. */
    gctBOOL                         enabled;

    /* Shader parameters. */
    gctUINT                         maxConstCount;
    gctUINT                         maxShaderLength;
    gctUINT                         constBase;
    gctUINT                         instBase;

    /* Composition state buffer. */
    gcsCOMPOSITION_STATE_BUFFER     compStateBuffer[gcdCOMP_BUFFER_COUNT];
    gcsCOMPOSITION_STATE_BUFFER_PTR compStateBufferCurrent;

    /* Allocated structures. */
    gcsCONTAINER                    freeSets;
    gcsCONTAINER                    freeNodes;
    gcsCONTAINER                    freeLayers;

    /* User signals. */
    gctHANDLE                       process;
    gctSIGNAL                       signal1;
    gctSIGNAL                       signal2;

    /* Current states. */
    gctBOOL                         synchronous;
    gctBOOL                         initDone;
    gcsSURF_INFO_PTR                target;

    /* Size of hardware event command. */
    gctUINT                         eventSize;

    /* Temporary surface for blurring. */
    gcsSURF_INFO                    tempBuffer;
    gcsCOMPOSITION_LAYER            tempLayer;

    /* The list of layer sets to be composed. */
    gcsCOMPOSITION_SET              head;
}
gcsCOMPOSITION_STATE;
#endif /* VIVANTE_NO_3D */

/* gcoHARDWARE object. */
struct _gcoHARDWARE
{
    /* Object. */
    gcsOBJECT                   object;

    /* Pointer to gckCONTEXT object. */
    gckCONTEXT                  context;

    /* Number of states managed by the context manager. */
    gctSIZE_T                   stateCount;

    /* Command buffer. */
    gcoBUFFER                   buffer;

    /* Context buffer. */
    gcePIPE_SELECT              currentPipe;

    /* Event queue. */
    gcoQUEUE                    queue;

    /* List of state delta buffers. */
    gcsSTATE_DELTA_PTR          delta;

    /* Chip characteristics. */
    gceCHIPMODEL                chipModel;
    gctUINT32                   chipRevision;
    gctUINT32                   chipFeatures;
    gctUINT32                   chipMinorFeatures;
    gctUINT32                   chipMinorFeatures1;
    gctUINT32                   chipMinorFeatures2;
    gctUINT32                   chipMinorFeatures3;

#ifndef VIVANTE_NO_3D
    gctUINT32                   streamCount;
    gctUINT32                   registerMax;
    gctUINT32                   threadCount;
    gctUINT32                   shaderCoreCount;
    gctUINT32                   vertexCacheSize;
    gctUINT32                   vertexOutputBufferSize;
    gctUINT32                   instructionCount;
    gctUINT32                   numConstants;
    gctUINT32                   bufferSize;
    gctINT                      needStriping;
    gctBOOL                     mixedStreams;

    gctUINT                     unifiedConst;
    gctUINT                     vsConstBase;
    gctUINT                     psConstBase;
    gctUINT                     vsConstMax;
    gctUINT                     psConstMax;
    gctUINT                     constMax;

    gctUINT                     unifiedShader;
    gctUINT                     vsInstBase;
    gctUINT                     psInstBase;
    gctUINT                     vsInstMax;
    gctUINT                     psInstMax;
    gctUINT                     instMax;

    gctUINT32                   renderTargets;
#endif

    gctUINT32                   pixelPipes;

    /* Big endian */
    gctBOOL                     bigEndian;

#ifndef VIVANTE_NO_3D
    /* API type. */
    gceAPI                      api;

    /* Current API type. */
    gceAPI                      currentApi;
#endif

    /* Temporary buffer parameters. */
    struct _gcsSURF_INFO        tempBuffer;

    /* Temporary surface for 2D. */
    gcsSURF_INFO_PTR            temp2DSurf[gcdTEMP_SURFACE_NUMBER];

    /* Filter blit. */
    struct __gcsLOADED_KERNEL_INFO
    {
        gceFILTER_TYPE              type;
        gctUINT8                    kernelSize;
        gctUINT32                   scaleFactor;
        gctUINT32                   kernelAddress;
    } loadedKernel[gcvFILTER_BLIT_KERNEL_NUM];

#ifndef VIVANTE_NO_3D
    /* Flush state. */
    gctBOOL                     flushedColor;
    gctBOOL                     flushedDepth;

    /* Render target states. */
    gctBOOL                     colorConfigDirty;
    gctBOOL                     colorTargetDirty;
    gcsCOLOR_INFO               colorStates;

    /* Depth states. */
    gctBOOL                     depthConfigDirty;
    gctBOOL                     depthRangeDirty;
    gctBOOL                     depthTargetDirty;
    gcsDEPTH_INFO               depthStates;
    gctBOOL                     disableAllEarlyDepth;
    gctBOOL                     previousPEDepth;

    /* Primitive assembly states. */
    gctBOOL                     paConfigDirty;
    gctBOOL                     paLineDirty;
    gcsPA_INFO                  paStates;

    /* Maximum depth value. */
    gctUINT32                   maxDepth;
    gctBOOL                     earlyDepth;

    /* Stencil states. */
    gctBOOL                     stencilEnabled;
    gctBOOL                     stencilKeepFront[3];
    gctBOOL                     stencilKeepBack[3];

    gctBOOL                     stencilDirty;
    gctBOOL                     stencilNeedFlush;
    gcsSTENCIL_INFO             stencilStates;

    /* Alpha states. */
    gctBOOL                     alphaDirty;
    gcsALPHA_INFO               alphaStates;

    /* Viewport/scissor states. */
    gctBOOL                     viewportDirty;
    gcsRECT                     viewportStates;

    gctBOOL                     scissorDirty;
    gcsRECT                     scissorStates;

    /* Shader states. */
    gctBOOL                     shaderDirty;
    gcsSHADER_INFO              shaderStates;

#endif /* VIVANTE_NO_3D */

    /* Stall before rendingering triangles. */
    gctBOOL                     stallPrimitive;

#ifndef VIVANTE_NO_3D
    /* Tile status information. */
    gctUINT32                   memoryConfig;
    gctBOOL                     paused;
    gctBOOL                     cacheDirty;
    gctBOOL                     inFlush;
    gctUINT32                   physicalTileColor;
    gctUINT32                   physicalTileDepth;

    /* Anti-alias mode. */
    gctBOOL                     msaaConfigDirty;
    gctBOOL                     msaaModeDirty;
    gctUINT32                   sampleMask;
    gctUINT32                   sampleEnable;
    gcsSAMPLES                  samples;
    gceVAA                      vaa;
    gctUINT32                   vaa8SampleCoords;
    gctUINT32                   vaa16SampleCoords;
    gctUINT32                   jitterIndex;
    gctUINT32                   sampleCoords2;
    gctUINT32                   sampleCoords4[3];
    gcsCENTROIDS                centroids2;
    gcsCENTROIDS                centroids4[3];
    gctBOOL                     centroidsDirty;

    /* Dither. */
    gctUINT32                   dither[2];
    gctBOOL                     peDitherDirty;
#endif /* VIVANTE_NO_3D */

    /* Stall signal. */
    gctSIGNAL                   stallSignal;

#ifndef VIVANTE_NO_3D
    /* Multi-pixel pipe information. */
    gctUINT32                   resolveAlignmentX;
    gctUINT32                   resolveAlignmentY;

    /* Texture states. */
    gctBOOL                     hwTxDirty;
    gctBOOL                     hwTxFlush;
    gctUINT32                   hwTxSamplerModeDirty;
    gctUINT32                   hwTxSamplerSizeDirty;
    gctUINT32                   hwTxSamplerSizeLogDirty;
    gctUINT32                   hwTxSamplerLODDirty;
    gctUINT32                   hwTxSamplerTSDirty;
    gctUINT32                   hwTxSamplerAddressDirty[gcdLOD_LEVELS];
    gctUINT32                   hwTxSamplerMode[gcdSAMPLERS];
    gctUINT32                   hwTxSamplerModeEx[gcdSAMPLERS];
    gctUINT32                   hwTxSamplerSize[gcdSAMPLERS];
    gctUINT32                   hwTxSamplerSizeLog[gcdSAMPLERS];
    gctUINT32                   hwTxSamplerLOD[gcdSAMPLERS];
    gctUINT32                   hwTxSamplerAddress[gcdLOD_LEVELS][gcdSAMPLERS];
    gctUINT32                   hwTXSampleTSConfig[gcdSAMPLER_TS];
    gctUINT32                   hwTXSampleTSBuffer[gcdSAMPLER_TS];
    gctUINT32                   hwTXSampleTSClearValue[gcdSAMPLER_TS];

    /* State to collect flushL2 requests, and process it at draw time. */
    gctBOOL                     flushL2;
#endif /* VIVANTE_NO_3D */

    /***************************************************************************
    ** 2D states.
    */

    /* 2D hardware availability flag. */
    gctBOOL                     hw2DEngine;

    /* 3D hardware availability flag. */
    gctBOOL                     hw3DEngine;

    /* Software 2D force flag. */
    gctBOOL                     sw2DEngine;

    /* 2D hardware Pixel Engine 2.0 availability flag. */
    gctBOOL                     hw2DPE20;

    /* Enhancement for DFB features. */
    gctBOOL                     hw2DFullDFB;

    /* Byte write capability. */
    gctBOOL                     byteWrite;

    /* BitBlit rotation capability. */
    gctBOOL                     fullBitBlitRotation;

    /* FilterBlit rotation capability. */
    gctBOOL                     fullFilterBlitRotation;

    /* Need to shadow RotAngleReg? */
    gctBOOL                     shadowRotAngleReg;

    /* The shadow value. */
    gctUINT32                   rotAngleRegShadow;

    /* Dither and filter+alpha capability. */
    gctBOOL                     dither2DandAlphablendFilter;

    /* Mirror extension. */
    gctBOOL                     mirrorExtension;

    /* Support one pass filter blit and tiled/YUV input&output for Blit/StretchBlit. */
    gctBOOL                     hw2DOPF;

    /* Support Multi-source blit. */
    gctBOOL                     hw2DMultiSrcBlit;

    /* This feature including 8 multi source, 2x2 minor tile, U/V separate stride,
        AXI reorder, RGB non-8-pixel alignement. */
    gctBOOL                     hw2DNewFeature0;

    /* Support L2 cache for 2D 420 input. */
    gctBOOL                     hw2D420L2Cache;

    /* Not support index8 & Mono/Color brush. */
    gctBOOL                     hw2DNoIndex8_Brush;

#ifndef VIVANTE_NO_3D
    /* OpenCL threadwalker in PS. */
    gctBOOL                     threadWalkerInPS;

    /* Composition states. */
    gcsCOMPOSITION_STATE        composition;

    /* Composition engine support. */
    gctBOOL                     hwComposition;
#endif

    /* Temp surface for fast clear */
    gcoSURF                     tempSurface;

    gctUINT32_PTR               hw2DCmdBuffer;
    gctUINT32                   hw2DCmdIndex;
    gctUINT32                   hw2DCmdSize;

    gctBOOL                     hw2DAppendCacheFlush;
	gcsSURF_INFO_PTR			hw2DCacheFlushSurf;

    gctUINT32                   hw2DWalkerVersion;
};

#ifndef VIVANTE_NO_3D
gceSTATUS
gcoHARDWARE_ComputeCentroids(
    IN gcoHARDWARE Hardware,
    IN gctUINT Count,
    IN gctUINT32_PTR SampleCoords,
    OUT gcsCENTROIDS_PTR Centroids
    );

/* Flush the vertex caches. */
gceSTATUS
gcoHARDWARE_FlushL2Cache(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_FlushViewport(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_FlushScissor(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_FlushAlpha(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_FlushStencil(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_FlushTarget(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_FlushDepth(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_FlushSampling(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_FlushPA(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_FlushShaders(
    IN gcoHARDWARE Hardware,
    IN gcePRIMITIVE PrimitiveType
    );

gceSTATUS
gcoHARDWARE_ProgramTexture(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_InitializeComposition(
    IN gcoHARDWARE Hardware
    );

gceSTATUS
gcoHARDWARE_DestroyComposition(
    IN gcoHARDWARE Hardware
    );
#endif /* VIVANTE_NO_3D */

gceSTATUS
gcoHARDWARE_Load2DState32(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    );

gceSTATUS
gcoHARDWARE_Load2DState(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctSIZE_T Count,
    IN gctPOINTER Data
    );

/* Set clipping rectangle. */
gceSTATUS
gcoHARDWARE_SetClipping(
    IN gcoHARDWARE Hardware,
    IN gcsRECT_PTR Rect
    );

/* Translate API source color format to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateSourceFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT APIValue,
    OUT gctUINT32* HwValue,
    OUT gctUINT32* HwSwizzleValue,
    OUT gctUINT32* HwIsYUVValue
    );

/* Translate API pattern color format to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePatternFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT APIValue,
    OUT gctUINT32* HwValue,
    OUT gctUINT32* HwSwizzleValue,
    OUT gctUINT32* HwIsYUVValue
    );

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateTransparency(
    IN gceSURF_TRANSPARENCY APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API transparency mode to its PE 1.0 hardware value. */
gceSTATUS
gcoHARDWARE_TranslateTransparencies(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 srcTransparency,
    IN gctUINT32 dstTransparency,
    IN gctUINT32 patTransparency,
    OUT gctUINT32* HwValue
    );

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateSourceTransparency(
    IN gce2D_TRANSPARENCY APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API rotation mode to its hardware value. */
gceSTATUS gcoHARDWARE_TranslateSourceRotation(
    IN gceSURF_ROTATION APIValue,
    OUT gctUINT32 * HwValue
    );

gceSTATUS gcoHARDWARE_TranslateDestinationRotation(
    IN gceSURF_ROTATION APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateDestinationTransparency(
    IN gce2D_TRANSPARENCY APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API transparency mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePatternTransparency(
    IN gce2D_TRANSPARENCY APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API DFB color key mode to its hardware value. */
gceSTATUS gcoHARDWARE_TranslateDFBColorKeyMode(
    IN  gctBOOL APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API pixel color multiply mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePixelColorMultiplyMode(
    IN  gce2D_PIXEL_COLOR_MULTIPLY_MODE APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API global color multiply mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateGlobalColorMultiplyMode(
    IN  gce2D_GLOBAL_COLOR_MULTIPLY_MODE APIValue,
    OUT gctUINT32 * HwValue
    );

/* Translate API mono packing mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateMonoPack(
    IN gceSURF_MONOPACK APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API 2D command to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateCommand(
    IN gce2D_COMMAND APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API per-pixel alpha mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePixelAlphaMode(
    IN gceSURF_PIXEL_ALPHA_MODE APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API global alpha mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateGlobalAlphaMode(
    IN gceSURF_GLOBAL_ALPHA_MODE APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API per-pixel color mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslatePixelColorMode(
    IN gceSURF_PIXEL_COLOR_MODE APIValue,
    OUT gctUINT32* HwValue
    );

/* Translate API alpha factor mode to its hardware value. */
gceSTATUS
gcoHARDWARE_TranslateAlphaFactorMode(
    IN gcoHARDWARE Hardware,
    IN  gctBOOL IsSrcFactor,
    IN  gceSURF_BLEND_FACTOR_MODE APIValue,
    OUT gctUINT32_PTR HwValue,
    OUT gctUINT32_PTR FactorExpansion
    );

/* Configure monochrome source. */
gceSTATUS gcoHARDWARE_SetMonochromeSource(
    IN gcoHARDWARE Hardware,
    IN gctUINT8 MonoTransparency,
    IN gceSURF_MONOPACK DataPack,
    IN gctBOOL CoordRelative,
    IN gctUINT32 FgColor32,
    IN gctUINT32 BgColor32,
    IN gctBOOL ColorConvert,
    IN gceSURF_FORMAT DstFormat,
    IN gctBOOL Stream,
    IN gctUINT32 Transparency
    );

/* Configure color source. */
gceSTATUS
gcoHARDWARE_SetColorSource(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Surface,
    IN gctBOOL CoordRelative,
    IN gctUINT32 Transparency,
    IN gce2D_YUV_COLOR_MODE Mode
    );

/* Configure masked color source. */
gceSTATUS
gcoHARDWARE_SetMaskedSource(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Surface,
    IN gctBOOL CoordRelative,
    IN gceSURF_MONOPACK MaskPack,
    IN gctUINT32 Transparency
    );

/* Setup the source rectangle. */
gceSTATUS
gcoHARDWARE_SetSource(
    IN gcoHARDWARE Hardware,
    IN gcsRECT_PTR SrcRect
    );

/* Setup the fraction of the source origin for filter blit. */
gceSTATUS
gcoHARDWARE_SetOriginFraction(
    IN gcoHARDWARE Hardware,
    IN gctUINT16 HorFraction,
    IN gctUINT16 VerFraction
    );

/* Load 256-entry color table for INDEX8 source surfaces. */
gceSTATUS
gcoHARDWARE_LoadPalette(
    IN gcoHARDWARE Hardware,
    IN gctUINT FirstIndex,
    IN gctUINT IndexCount,
    IN gctPOINTER ColorTable,
    IN gctBOOL ColorConvert,
    IN gceSURF_FORMAT DstFormat,
    IN OUT gctBOOL *Program,
    IN OUT gceSURF_FORMAT *ConvertFormat
    );

/* Setup the source global color value in ARGB8 format. */
gceSTATUS
gcoHARDWARE_SetSourceGlobalColor(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Color
    );

/* Setup the target global color value in ARGB8 format. */
gceSTATUS
gcoHARDWARE_SetTargetGlobalColor(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Color
    );

/* Setup the source and target pixel multiply modes. */
gceSTATUS
gcoHARDWARE_SetMultiplyModes(
    IN gcoHARDWARE Hardware,
    IN gce2D_PIXEL_COLOR_MULTIPLY_MODE SrcPremultiplySrcAlpha,
    IN gce2D_PIXEL_COLOR_MULTIPLY_MODE DstPremultiplyDstAlpha,
    IN gce2D_GLOBAL_COLOR_MULTIPLY_MODE SrcPremultiplyGlobalMode,
    IN gce2D_PIXEL_COLOR_MULTIPLY_MODE DstDemultiplyDstAlpha,
    IN gctUINT32 SrcGlobalColor
    );

/*
 * Set the transparency for source, destination and pattern.
 * It also enable or disable the DFB color key mode.
 */
gceSTATUS
gcoHARDWARE_SetTransparencyModesEx(
    IN gcoHARDWARE Hardware,
    IN gce2D_TRANSPARENCY SrcTransparency,
    IN gce2D_TRANSPARENCY DstTransparency,
    IN gce2D_TRANSPARENCY PatTransparency,
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop,
    IN gctBOOL EnableDFBColorKeyMode
    );

/* Setup the source, target and pattern transparency modes.
   Used only for have backward compatibility.
*/
gceSTATUS
gcoHARDWARE_SetAutoTransparency(
    IN gctUINT8 FgRop,
    IN gctUINT8 BgRop
    );

/* Setup the source color key value in ARGB8 format. */
gceSTATUS
gcoHARDWARE_SetSourceColorKeyRange(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 ColorLow,
    IN gctUINT32 ColorHigh,
    IN gctBOOL ColorPack,
    IN gceSURF_FORMAT SrcFormat
    );

gceSTATUS
gcoHARDWARE_SetMultiSource(
    IN gcoHARDWARE Hardware,
    IN gctUINT RegGroupIndex,
    IN gctUINT SrcIndex,
    IN gcs2D_State_PTR State
    );

gceSTATUS
gcoHARDWARE_SetMultiSourceEx(
    IN gcoHARDWARE Hardware,
    IN gctUINT RegGroupIndex,
    IN gctUINT SrcIndex,
    IN gcs2D_State_PTR State
    );

/* Configure destination. */
gceSTATUS
gcoHARDWARE_SetTarget(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Surface
    );

gceSTATUS gcoHARDWARE_SetMultiTarget(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Surface,
    IN gceSURF_FORMAT SrcFormat
    );

/* Setup the destination color key value in ARGB8 format. */
gceSTATUS
gcoHARDWARE_SetTargetColorKeyRange(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 ColorLow,
    IN gctUINT32 ColorHigh
    );

/* Load solid (single) color pattern. */
gceSTATUS
gcoHARDWARE_LoadSolidColorPattern(
    IN gcoHARDWARE Hardware,
    IN gctBOOL ColorConvert,
    IN gctUINT32 Color,
    IN gctUINT64 Mask,
    IN gceSURF_FORMAT DstFormat
    );

/* Load monochrome pattern. */
gceSTATUS
gcoHARDWARE_LoadMonochromePattern(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 OriginX,
    IN gctUINT32 OriginY,
    IN gctBOOL ColorConvert,
    IN gctUINT32 FgColor,
    IN gctUINT32 BgColor,
    IN gctUINT64 Bits,
    IN gctUINT64 Mask,
    IN gceSURF_FORMAT DstFormat
    );

/* Load color pattern. */
gceSTATUS
gcoHARDWARE_LoadColorPattern(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 OriginX,
    IN gctUINT32 OriginY,
    IN gctUINT32 Address,
    IN gceSURF_FORMAT Format,
    IN gctUINT64 Mask
    );

/* Calculate stretch factor. */
gctUINT32
gcoHARDWARE_GetStretchFactor(
    IN gctINT32 SrcSize,
    IN gctINT32 DestSize
    );

/* Calculate and program the stretch factors. */
gceSTATUS
gcoHARDWARE_SetStretchFactors(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 HorFactor,
    IN gctUINT32 VerFactor
    );

/* Set 2D clear color in A8R8G8B8 format. */
gceSTATUS
gcoHARDWARE_Set2DClearColor(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Color,
    IN gceSURF_FORMAT DstFormat
    );

/* Enable/disable 2D BitBlt mirrorring. */
gceSTATUS
gcoHARDWARE_SetBitBlitMirror(
    IN gcoHARDWARE Hardware,
    IN gctBOOL HorizontalMirror,
    IN gctBOOL VerticalMirror
    );

/* Enable alpha blending engine in the hardware and disengage the ROP engine. */
gceSTATUS
gcoHARDWARE_EnableAlphaBlend(
    IN gcoHARDWARE Hardware,
    IN gceSURF_PIXEL_ALPHA_MODE SrcAlphaMode,
    IN gceSURF_PIXEL_ALPHA_MODE DstAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE SrcGlobalAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE DstGlobalAlphaMode,
    IN gceSURF_BLEND_FACTOR_MODE SrcFactorMode,
    IN gceSURF_BLEND_FACTOR_MODE DstFactorMode,
    IN gceSURF_PIXEL_COLOR_MODE SrcColorMode,
    IN gceSURF_PIXEL_COLOR_MODE DstColorMode,
    IN gctUINT8 SrcGlobalAlphaValue,
    IN gctUINT8 DstGlobalAlphaValue
    );

/* Disable alpha blending engine in the hardware and engage the ROP engine. */
gceSTATUS
gcoHARDWARE_DisableAlphaBlend(
    );

gceSTATUS
gcoHARDWARE_ColorConvertToARGB8(
    IN gceSURF_FORMAT Format,
    IN gctUINT32 NumColors,
    IN gctUINT32_PTR Color,
    OUT gctUINT32_PTR Color32
    );

gceSTATUS
gcoHARDWARE_ColorConvertFromARGB8(
    IN gceSURF_FORMAT Format,
    IN gctUINT32 NumColors,
    IN gctUINT32_PTR Color32,
    OUT gctUINT32_PTR Color
    );

gceSTATUS
gcoHARDWARE_ColorPackFromARGB8(
    IN gceSURF_FORMAT Format,
    IN gctUINT32 Color32,
    OUT gctUINT32_PTR Color
    );

/* Software clear. */
gceSTATUS
gcoHARDWARE_ClearSoftware(
    IN gcoHARDWARE Hardware,
    IN gctPOINTER LogicalAddress,
    IN gctUINT32 Stride,
    IN gctINT32 Left,
    IN gctINT32 Top,
    IN gctINT32 Right,
    IN gctINT32 Bottom,
    IN gceSURF_FORMAT Format,
    IN gctUINT32 ClearValue,
    IN gctUINT8 ClearMask,
    IN gctBOOL Stall,
	IN gctUINT32 AlignedHeight
    );

/* Convert pixel format. */
gceSTATUS
gcoHARDWARE_ConvertPixel(
    IN gctPOINTER SrcPixel,
    OUT gctPOINTER TrgPixel,
    IN gctUINT SrcBitOffset,
    IN gctUINT TrgBitOffset,
    IN gcsSURF_FORMAT_INFO_PTR SrcFormat,
    IN gcsSURF_FORMAT_INFO_PTR TrgFormat,
    IN gcsBOUNDARY_PTR SrcBoundary,
    IN gcsBOUNDARY_PTR TrgBoundary
    );

/* Copy a rectangular area with format conversion. */
gceSTATUS
gcoHARDWARE_CopyPixels(
    IN gcsSURF_INFO_PTR Source,
    IN gcsSURF_INFO_PTR Target,
    IN gctINT SourceX,
    IN gctINT SourceY,
    IN gctINT TargetX,
    IN gctINT TargetY,
    IN gctINT Width,
    IN gctINT Height
    );

/* Enable or disable anti-aliasing. */
gceSTATUS
gcoHARDWARE_SetAntiAlias(
    IN gctBOOL Enable
    );

/* Write data into the command buffer. */
gceSTATUS
gcoHARDWARE_WriteBuffer(
    IN gcoHARDWARE Hardware,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    );

#ifndef VIVANTE_NO_3D
typedef enum _gceTILE_STATUS_CONTROL
{
    gcvTILE_STATUS_PAUSE,
    gcvTILE_STATUS_RESUME,
}
gceTILE_STATUS_CONTROL;

/* Pause or resume tile status. */
gceSTATUS
gcoHARDWARE_PauseTileStatus(
    IN gcoHARDWARE Hardware,
    IN gceTILE_STATUS_CONTROL Control
    );

/* Compute the offset of the specified pixel location. */
gceSTATUS
gcoHARDWARE_ComputeOffset(
    IN gctINT32 X,
    IN gctINT32 Y,
    IN gctUINT Stride,
    IN gctINT BytesPerPixel,
    IN gceTILING Tiling,
    OUT gctUINT32_PTR Offset
    );

/* Query the tile size of the given surface. */
gceSTATUS
gcoHARDWARE_GetSurfaceTileSize(
    IN gcsSURF_INFO_PTR Surface,
    OUT gctINT32 * TileWidth,
    OUT gctINT32 * TileHeight
    );

/* Program the Resolve offset, Window and then trigger the resolve. */
gceSTATUS
gcoHARDWARE_ProgramResolve(
    IN gcoHARDWARE Hardware,
    IN gcsPOINT RectSize
    );
#endif /* VIVANTE_NO_3D */

/* Load one 32-bit state. */
gceSTATUS
gcoHARDWARE_LoadState32(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    );

/* Load one 32-bit load state. */
gceSTATUS
gcoHARDWARE_LoadState32x(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctFIXED_POINT Data
    );

/* Load one 64-bit load state. */
gceSTATUS
gcoHARDWARE_LoadState64(
    IN gctUINT32 Address,
    IN gctUINT64 Data
    );

gceSTATUS
gcoHARDWARE_SetDither2D(
    IN gcoHARDWARE Hardware,
    IN gctBOOL Enable
    );

/* Update the state delta. */
gceSTATUS gcoHARDWARE_UpdateDelta(
    IN gcsSTATE_DELTA_PTR StateDelta,
    IN gctBOOL FixedPoint,
    IN gctUINT32 Address,
    IN gctUINT32 Mask,
    IN gctUINT32 Data
    );

gceSTATUS
gcoHARDWARE_2DAppendNop(
    );

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_user_hardware_h_ */

