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




#ifndef __gc_glff_h_
#define __gc_glff_h_

/* Define this to free texture memory after 3D composition.
 * NOTE: This feature will cause slight performance drop for 3D composition
 * but will save quite a lot of memory. And it will not affect 'hwcomposer'
 * composition performance. */
#define gldENABLE_MEMORY_REDUCTION          0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _glsCHIPINFO * glsCHIPINFO_PTR;
typedef struct _glsCHIPINFO
{
    GLubyte*                    vendor;
    GLubyte*                    renderer;
    GLubyte*                    version;
    gctSTRING                   extensions;
}
glsCHIPINFO;

#define gldATTRIBUTE_DRAWTEX_POSITION       0
#define gldATTRIBUTE_DRAWCLEAR_POSITION     1
#define gldATTRIBUTE_POSITION               2
#define gldATTRIBUTE_NORMAL                 3
#define gldATTRIBUTE_COLOR                  4
#define gldATTRIBUTE_POINT_SIZE             5
#define gldATTRIBUTE_TEXCOORD0              6
#define gldATTRIBUTE_TEXCOORD1              7
#define gldATTRIBUTE_TEXCOORD2              8
#define gldATTRIBUTE_TEXCOORD3              9
#define gldATTRIBUTE_INDEX                  10
#define gldATTRIBUTE_WEIGHT                 11
#define gldATTRIBUTE_DRAWTEX_TEXCOORD0      12
#define gldATTRIBUTE_DRAWTEX_TEXCOORD1      13
#define gldATTRIBUTE_DRAWTEX_TEXCOORD2      14
#define gldATTRIBUTE_DRAWTEX_TEXCOORD3      15


typedef struct _glsCONTEXT
{
    GLenum                      error;

    gcoHAL                      hal;
    gco3D                       hw;
	gcoOS						os;

    struct _glsCONTEXT          *shared;

    gceCHIPMODEL                chipModel;
    gctUINT32                   chipRevision;
    glsCHIPINFO                 chipInfo;
    char                        chipName[23];
    gctUINT                     maxWidth;
    gctUINT                     maxHeight;
    gctUINT                     maxAniso;
    gctUINT                     maxTextureWidth;
    gctUINT                     maxTextureHeight;

    /* Read surface. */
    gcoSURF                     read;

    /* Draw surface. */
    gcoSURF                     draw;

#if defined(ANDROID) && gcdDEFER_RESOLVES
    /* Imported render target for 3D apps. */
    gcoSURF                     importedRT;
#endif

    gcsSURF_FORMAT_INFO_PTR     drawFormatInfo[2];
    GLuint                      drawWidth, effectiveWidth;
    GLuint                      drawHeight, effectiveHeight;
    gctUINT                     drawSamples;

    /* Depth surface. */
    gcoSURF                     depth;

    /* Temporary bitmap. */
    gcoSURF                     tempBitmap;
    gctUINT8_PTR                tempBits;
    gctUINT8_PTR                tempLastLine;
    gceSURF_FORMAT              tempFormat;
    gctUINT                     tempBitsPerPixel;
    gctUINT                     tempX;
    gctUINT                     tempY;
    gctUINT                     tempWidth;
    gctUINT                     tempHeight;
    gctINT                      tempStride;

    gctBOOL                     initialized;

    gcoSURF                     tempDraw;
    gcoSURF                     fbLinear;

    glsTEXTURE                  texture;

    /* Currently bound buffers. */
    glsNAMEDOBJECTLIST          bufferList;
    glsNAMEDOBJECT_PTR          arrayBuffer;
    glsNAMEDOBJECT_PTR          elementArrayBuffer;

    /* Render buffer object. */
    glsNAMEDOBJECTLIST          renderBufferList;
    glsRENDER_BUFFER_PTR        renderBuffer;

    /* Frame buffer object. */
    glsNAMEDOBJECTLIST          frameBufferList;
    glsFRAME_BUFFER_PTR         frameBuffer;
    gctBOOL                     frameBufferChanged;

    /* Matrix stack. */
    gleMATRIXMODE               matrixMode;
    glsMATRIXSTACK              matrixStackArray[glvSTACKCOUNT];
    gctUINT                     currentPalette;

    /* Matrix shortcuts. */
    glsMATRIXSTACK_PTR          currentStack;
    glsMATRIX_PTR               currentMatrix;
    glsMATRIX_PTR               modelViewMatrix;
    glsMATRIX_PTR               projectionMatrix;
    glsMATRIX_PTR               textureMatrix;

    /* Dependent (computed) matrices */
    glsDEPENDENTMATRIX          modelViewInverse3x3TransposedMatrix;
    glsDEPENDENTMATRIX          modelViewInverse4x4TransposedMatrix;
    glsDEPENDENTMATRIX          modelViewProjectionMatrix;
    glsDEPENDENTMATRIX          convertedProjectionMatrix;
    glsDEPENDENTMATRIX          paletteMatrixInverse3x3[glvMAX_PALETTE_MATRICES];
    GLboolean                   paletteMatrixInverse3x3Recompute;

    glsATTRIBUTEINFO            aPositionInfo;
    glsATTRIBUTEINFO            aPositionDrawTexInfo;
    glsATTRIBUTEINFO            aPositionDrawClearRectInfo;
    glsATTRIBUTEINFO            aNormalInfo;
    glsATTRIBUTEINFO            aColorInfo;
    glsATTRIBUTEINFO            aPointSizeInfo;
    glsATTRIBUTEINFO            aMatrixIndexInfo;
    glsATTRIBUTEINFO            aWeightInfo;

    GLboolean                   rescaleNormal;
    GLboolean                   normalizeNormal;
    GLboolean                   matrixPaletteEnabled;

    glsVIEWPORT                 viewportStates;
    glsCULL                     cullStates;
    glsALPHA                    alphaStates;
    glsSTENCIL                  stencilStates;
    glsDEPTH                    depthStates;
    glsLIGHTING                 lightingStates;
    glsFOG                      fogStates;
    glsPOINT                    pointStates;
    gctBOOL                     hwPointSprite;
    glsLINE                     lineStates;
    glsMULTISAMPLE              multisampleStates;
    glsLOGICOP                  logicOp;

    GLboolean                   dither;
    GLboolean                   colorMask[4];
    glsVECTOR                   clearColor;
    GLenum                      perspectiveCorrect;

    GLboolean                   clipPlaneEnabled[glvMAX_CLIP_PLANES];
    glsVECTOR                   clipPlane[glvMAX_CLIP_PLANES];

    /* Pixel alignment for glReadPixels, glTexImage2D and glTexSubImage2D. */
    GLint                       unpackAlignment;
    GLint                       packAlignment;

    /* Miscellaneous flags. */
    GLboolean                   fsRoundingEnabled;
    GLboolean                   drawTexOESEnabled;
    GLboolean                   drawClearRectEnabled;
    gctBOOL                     useFragmentProcessor;
    gctBOOL                     hasCorrectStencil;
    gctBOOL                     hasTileStatus;
    gctBOOL                     hwLogicOp;

    /* Shader program hash. */
    glsHASHKEY                  hashKey;
    glsHASHTABLE_PTR            hashTable;
    glsPROGRAMINFO_PTR          currProgram;

    /* Shader uniforms dirty bits. */
    glsVSUNIFORMDIRTYINFO       vsUniformDirty;
    glsFSUNIFORMDIRTYINFO       fsUniformDirty;

    /* 0:  dtawtex position     */
    /* 1:  drawclear position   */
    /* 2:  position             */
    /* 3:  normal               */
    /* 4:  color                */
    /* 5:  point size           */
    /* 6:  texture coord 0      */
    /* 7:  texture coord 1      */
    /* 8: texture coord 2       */
    /* 9: texture coord 3       */
    /* 10:  matrix index        */
    /* 11:  matrix weight       */
    /* 12:  dtawtex texture coord 0      */
    /* 13:  dtawtex texture coord 1      */
    /* 14:  dtawtex texture coord 2       */
    /* 15:  dtawtex texture coord 3       */
    gcsVERTEXARRAY              attributeArray[16];

    gcoVERTEXARRAY              vertexArray;

    /* streams store vertex data that replace the stream cache.     *
     * If vertex attributes size is larger than 1M, stream cache    *
     * doesn't cache so much data.                                  */
    gcoSTREAM                   streams[gldSTREAM_POOL_SIZE];
    gctUINT32                   streamIndex;
    gctSIGNAL                   streamSignals[gldSTREAM_SIGNAL_NUM];
    gctBOOL                     streamPending;

    gctBOOL                     bSyncSubVBO;

    /* Profiler */
#if VIVANTE_PROFILER
    glsPROFILER                 profiler;
#endif
    gctBOOL                     patchStrip;

#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
    /* pid of this context */
    gctINT                      pid;
#endif
}
glsCONTEXT;

    gctBOOL glfSetContext(void * Context,
                          gcoSURF Draw, gcoSURF Read, gcoSURF Depth);

glsCONTEXT_PTR
GetCurrentContext(
    void
    );

#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_h_ */
