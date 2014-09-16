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




#ifndef __gc_glff_fixed_func_h_
#define __gc_glff_fixed_func_h_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
**************************** Miscellaneous constants. **************************
\******************************************************************************/

/* Maximum number of matrix indices per vertex for matrix palette. */
#define glvMAX_VERTEX_UNITS                 3

/* Maximum number of palette matrices supported. */
#define glvMAX_PALETTE_MATRICES             9

/* Maximum number of textures. */
#define glvMAX_TEXTURES                     4

/* Maximum number of clip planes. */
#define glvMAX_CLIP_PLANES                  6

/* Maximum number of lights. */
#define glvMAX_LIGHTS                       8

/* Shader uniform index definition. */
#define glmUNIFORM_INDEX(Shader, Name) \
    glvUNIFORM_ ## Shader ## _ ## Name

/* Shader attribute index definition. */
#define glmATTRIBUTE_INDEX(Shader, Name) \
    glvATTRIBUTE_ ## Shader ## _ ## Name

/* Vertex shader uniforms. */
typedef enum _gleVS_UNIFORMS
{
    glmUNIFORM_INDEX(VS, uColor),
    glmUNIFORM_INDEX(VS, uNormal),

    glmUNIFORM_INDEX(VS, uModelView),
    glmUNIFORM_INDEX(VS, uModelViewInverse3x3Transposed),
    glmUNIFORM_INDEX(VS, uModelViewProjection),
    glmUNIFORM_INDEX(VS, uProjection),

    glmUNIFORM_INDEX(VS, uEcm),
    glmUNIFORM_INDEX(VS, uAcm),
    glmUNIFORM_INDEX(VS, uDcm),
    glmUNIFORM_INDEX(VS, uAcs),
    glmUNIFORM_INDEX(VS, uSrm),
    glmUNIFORM_INDEX(VS, uScm),

    glmUNIFORM_INDEX(VS, uPpli),
    glmUNIFORM_INDEX(VS, uKi),
    glmUNIFORM_INDEX(VS, uSrli),
    glmUNIFORM_INDEX(VS, uCrli),
    glmUNIFORM_INDEX(VS, uAcli),
    glmUNIFORM_INDEX(VS, uDcli),
    glmUNIFORM_INDEX(VS, uScli),
    glmUNIFORM_INDEX(VS, uAcmAcli),
    glmUNIFORM_INDEX(VS, uVPpli),
    glmUNIFORM_INDEX(VS, uDcmDcli),
    glmUNIFORM_INDEX(VS, uCosCrli),
    glmUNIFORM_INDEX(VS, uNormedSdli),

    glmUNIFORM_INDEX(VS, uTexCoord),
    glmUNIFORM_INDEX(VS, uTexMatrix),
    glmUNIFORM_INDEX(VS, uClipPlane),
    glmUNIFORM_INDEX(VS, uPointAttenuation),
    glmUNIFORM_INDEX(VS, uPointSize),
    glmUNIFORM_INDEX(VS, uViewport),
    glmUNIFORM_INDEX(VS, uMatrixPalette),
    glmUNIFORM_INDEX(VS, uMatrixPaletteInverse),

    glvUNIFORM_VS_COUNT
}
gleVS_UNIFORMS;

/* Fragment shader uniforms. */
typedef enum _gleFS_UNIFORMS
{
    glmUNIFORM_INDEX(FS, uColor),
    glmUNIFORM_INDEX(FS, uFogFactors),
    glmUNIFORM_INDEX(FS, uFogColor),
    glmUNIFORM_INDEX(FS, uTexColor),
    glmUNIFORM_INDEX(FS, uTexCombScale),
    glmUNIFORM_INDEX(FS, uTexSampler0),
    glmUNIFORM_INDEX(FS, uTexSampler1),
    glmUNIFORM_INDEX(FS, uTexSampler2),
    glmUNIFORM_INDEX(FS, uTexSampler3),
    glmUNIFORM_INDEX(FS, uTexCoord),

    glvUNIFORM_FS_COUNT
}
gleFS_UNIFORMS;

/* Vertex shader attributes. */
typedef enum _gleVS_ATTRIBUTES
{
    glmATTRIBUTE_INDEX(VS, aPosition),
    glmATTRIBUTE_INDEX(VS, aNormal),
    glmATTRIBUTE_INDEX(VS, aColor),
    glmATTRIBUTE_INDEX(VS, aTexCoord0),
    glmATTRIBUTE_INDEX(VS, aTexCoord1),
    glmATTRIBUTE_INDEX(VS, aTexCoord2),
    glmATTRIBUTE_INDEX(VS, aTexCoord3),
    glmATTRIBUTE_INDEX(VS, aPointSize),
    glmATTRIBUTE_INDEX(VS, aMatrixIndex),
    glmATTRIBUTE_INDEX(VS, aMatrixWeight),

    glvATTRIBUTE_VS_COUNT
}
gleVS_ATTRIBUTES;

/* Fragment shader varyings. */
typedef enum _gleFS_ATTRIBUTES
{
    glmATTRIBUTE_INDEX(FS, vPosition),
    glmATTRIBUTE_INDEX(FS, vEyePosition),
    glmATTRIBUTE_INDEX(FS, vColor0),
    glmATTRIBUTE_INDEX(FS, vColor1),
    glmATTRIBUTE_INDEX(FS, vTexCoord0),
    glmATTRIBUTE_INDEX(FS, vTexCoord1),
    glmATTRIBUTE_INDEX(FS, vTexCoord2),
    glmATTRIBUTE_INDEX(FS, vTexCoord3),
    glmATTRIBUTE_INDEX(FS, vClipPlane0),
    glmATTRIBUTE_INDEX(FS, vClipPlane1),
    glmATTRIBUTE_INDEX(FS, vClipPlane2),
    glmATTRIBUTE_INDEX(FS, vClipPlane3),
    glmATTRIBUTE_INDEX(FS, vClipPlane4),
    glmATTRIBUTE_INDEX(FS, vClipPlane5),
    glmATTRIBUTE_INDEX(FS, vPointFade),
    glmATTRIBUTE_INDEX(FS, vPointSmooth),
    glmATTRIBUTE_INDEX(FS, vFace),

    glvATTRIBUTE_FS_COUNT
}
gleFS_ATTRIBUTES;

/* Functions for alpha, depth and stencil tests. */
typedef enum _gleTESTFUNCTIONS
{
    glvNEVER,
    glvLESS,
    glvEQUAL,
    glvLESSOREQUAL,
    glvGREATER,
    glvNOTEQUAL,
    glvGREATEROREQUAL,
    glvALWAYS
}
gleTESTFUNCTIONS, * gleTESTFUNCTIONS_PTR;

/* Supported fog modes. */
typedef enum _gleFOGMODES
{
    glvLINEARFOG,
    glvEXPFOG,
    glvEXP2FOG
}
gleFOGMODES, * gleFOGMODES_PTR;

/* Alpha blending functions. */
typedef enum _gleBLENDFUNCTIONS
{
    glvBLENDZERO,
    glvBLENDONE,
    glvBLENDSRCCOLOR,
    glvBLENDSRCCOLORINV,
    glvBLENDSRCALPHA,
    glvBLENDSRCALPHAINV,
    glvBLENDDSTCOLOR,
    glvBLENDDSTCOLORINV,
    glvBLENDDSTALPHA,
    glvBLENDDSTALPHAINV,
    glvBLENDSRCALPHASATURATE
}
gleBLENDFUNCTIONS, * gleBLENDFUNCTIONS_PTR;

typedef enum _gleBLENDMODES
{
	gcvBLENDADD,
    gcvBLENDSUBTRACT,
    gcvBLENDREVERSESUBTRACT,
    gcvBLENDMIN,
    gcvBLENDMAX,
}
gleBLENDMODES, * gleBLENDMODES_PTR;

/* Supported stencil operations. */
typedef enum _gleSTENCILOPERATIONS
{
    glvSTENCILZERO,
    glvSTENCILKEEP,
    glvSTENCILREPLACE,
    glvSTENCILINC,
    glvSTENCILDEC,
    glvSTENCILINVERT
}
gleSTENCILOPERATIONS, * gleSTENCILOPERATIONS_PTR;


/******************************************************************************\
**************************** Shader generation macros. *************************
\******************************************************************************/

#define glmUSING_UNIFORM(Name) \
    gcmERR_BREAK(_Using_##Name(Context, ShaderControl))

#define glmUSING_ATTRIBUTE(Name) \
    gcmERR_BREAK(_Using_##Name(Context, ShaderControl))

#define glmUSING_INDEXED_ATTRIBUTE(Name, Index) \
    gcmERR_BREAK(_Using_##Name(Context, ShaderControl, Index))

#define glmUSING_VARYING(Name) \
    gcmERR_BREAK(_Using_##Name(Context, ShaderControl))

#define glmUSING_INDEXED_VARYING(Name, Index) \
    gcmERR_BREAK(_Using_##Name(Context, ShaderControl, Index))

#define glmDEFINE_TEMP(Name) \
    gctUINT16 Name

#define glmALLOCATE_LOCAL_TEMP(Name) \
    glmDEFINE_TEMP(Name) = _AllocateTemp(ShaderControl)

#define glmALLOCATE_TEMP(Name) \
    Name = _AllocateTemp(ShaderControl)

#define glmDEFINE_LABEL(Name) \
    gctUINT Name

#define glmALLOCATE_LOCAL_LABEL(Name) \
    glmDEFINE_LABEL(Name) = _AllocateLabel(ShaderControl)

#define glmALLOCATE_LABEL(Name) \
    Name = _AllocateLabel(ShaderControl)

#define glmASSIGN_OUTPUT(Name) \
    if (ShaderControl->Name != 0) \
    { \
        gcmERR_BREAK(_Assign_##Name( \
            Context, \
            ShaderControl, \
            ShaderControl->Name \
            )); \
    }

#define glmASSIGN_INDEXED_OUTPUT(Name, Index) \
    if (ShaderControl->Name[Index] != 0) \
    { \
        gcmERR_BREAK(_Assign_##Name( \
            Context, \
            ShaderControl, \
            Index \
            )); \
    }

#define glmOPCODE(Opcode, TempRegister, ComponentEnable) \
    gcmASSERT(TempRegister != 0); \
    gcmERR_BREAK(gcSHADER_AddOpcode( \
        ShaderControl->i->shader, \
        gcSL_##Opcode, \
        TempRegister, \
        gcSL_ENABLE_##ComponentEnable, \
        gcSL_FLOAT \
        ))

#define glmOPCODE_COND(Opcode, Condition, TempRegister, ComponentEnable) \
    gcmASSERT(TempRegister != 0); \
    gcmERR_BREAK(gcSHADER_AddOpcode2( \
        ShaderControl->i->shader, \
        gcSL_##Opcode, \
        gcSL_##Condition, \
        TempRegister, \
        gcSL_ENABLE_##ComponentEnable, \
        gcSL_FLOAT \
        ))

#define glmOPCODEV(Opcode, TempRegister, ComponentEnable) \
    gcmASSERT(TempRegister != 0); \
    gcmASSERT((ComponentEnable & ~gcSL_ENABLE_XYZW) == 0); \
    gcmERR_BREAK(gcSHADER_AddOpcode( \
        ShaderControl->i->shader, \
        gcSL_##Opcode, \
        TempRegister, \
        ComponentEnable, \
        gcSL_FLOAT \
        ))

#define glmOPCODE_BRANCH(Opcode, Condition, Target) \
    gcmERR_BREAK(gcSHADER_AddOpcodeConditional( \
        ShaderControl->i->shader, \
        gcSL_##Opcode, \
        gcSL_##Condition, \
        Target \
        ))

#define glmCONST(Value) \
    gcmERR_BREAK(gcSHADER_AddSourceConstant( \
        ShaderControl->i->shader, \
        (gctFLOAT) (Value) \
        ))

#define glmUNIFORM_WRAP(Shader, Name) \
        ShaderControl->uniforms[glmUNIFORM_INDEX(Shader, Name)]

#define glmUNIFORM_WRAP_INDEXED(Shader, Name, Index) \
        ShaderControl->uniforms[glmUNIFORM_INDEX(Shader, Name) + Index]

#define glmUNIFORM(Shader, Name, Swizzle) \
    gcmERR_BREAK(gcSHADER_AddSourceUniform( \
        ShaderControl->i->shader, \
        glmUNIFORM_WRAP(Shader, Name)->uniform, \
        gcSL_SWIZZLE_##Swizzle, \
        0 \
        ))

#define glmUNIFORM_INDEXED(Shader, Name, Index, Swizzle) \
    gcmERR_BREAK(gcSHADER_AddSourceUniform( \
        ShaderControl->i->shader, \
        glmUNIFORM_WRAP_INDEXED(Shader, Name, Index)->uniform, \
        gcSL_SWIZZLE_##Swizzle, \
        0 \
        ))

#define glmUNIFORM_STATIC(Shader, Name, Swizzle, Index) \
    gcmERR_BREAK(gcSHADER_AddSourceUniform( \
        ShaderControl->i->shader, \
        glmUNIFORM_WRAP(Shader, Name)->uniform, \
        gcSL_SWIZZLE_##Swizzle, \
        Index \
        ))

#define glmUNIFORM_DYNAMIC(Shader, Name, Swizzle, IndexRegister) \
    gcmERR_BREAK(gcSHADER_AddSourceUniformIndexed( \
        ShaderControl->i->shader, \
        glmUNIFORM_WRAP(Shader, Name)->uniform, \
        gcSL_SWIZZLE_##Swizzle, \
        0, \
        gcSL_INDEXED_X, \
        IndexRegister \
        ))

#define glmUNIFORM_DYNAMIC_MATRIX(Shader, Name, Swizzle, IndexRegister, \
                                  Offset, IndexMode) \
    gcmERR_BREAK(gcSHADER_AddSourceUniformIndexed( \
        ShaderControl->i->shader, \
        glmUNIFORM_WRAP(Shader, Name)->uniform, \
        gcSL_SWIZZLE_##Swizzle, \
        Offset, \
        IndexMode, \
        IndexRegister \
        ))

#define glmATTRIBUTE_WRAP(Shader, Name) \
        ShaderControl->attributes[glmATTRIBUTE_INDEX(Shader, Name)]

#define glmATTRIBUTE_WRAP_INDEXED(Shader, Name, Index) \
        ShaderControl->attributes[glmATTRIBUTE_INDEX(Shader, Name) + Index]

#define glmATTRIBUTE(Shader, Name, Swizzle) \
    gcmERR_BREAK(gcSHADER_AddSourceAttribute( \
        ShaderControl->i->shader, \
        glmATTRIBUTE_WRAP(Shader, Name)->attribute, \
        gcSL_SWIZZLE_##Swizzle, \
        0 \
        ))

#define glmATTRIBUTE_INDEXED(Shader, Name, Index, Swizzle) \
    gcmERR_BREAK(gcSHADER_AddSourceAttribute( \
        ShaderControl->i->shader, \
        glmATTRIBUTE_WRAP_INDEXED(Shader, Name, Index)->attribute, \
        gcSL_SWIZZLE_##Swizzle, \
        0 \
        ))

#define glmATTRIBUTEV(Shader, Name, Swizzle) \
    gcmERR_BREAK(gcSHADER_AddSourceAttribute( \
        ShaderControl->i->shader, \
        glmATTRIBUTE_WRAP(Shader, Name)->attribute, \
        Swizzle, \
        0 \
        ))

#define glmVARYING(Shader, Name, Swizzle) \
    gcmERR_BREAK(gcSHADER_AddSourceAttribute( \
        ShaderControl->i->shader, \
        glmATTRIBUTE_WRAP(Shader, Name)->attribute, \
        gcSL_SWIZZLE_##Swizzle, \
        0 \
        ))

#define glmVARYING_INDEXED(Shader, Name, Index, Swizzle) \
    gcmERR_BREAK(gcSHADER_AddSourceAttribute( \
        ShaderControl->i->shader, \
        glmATTRIBUTE_WRAP_INDEXED(Shader, Name, Index)->attribute, \
        gcSL_SWIZZLE_##Swizzle, \
        0 \
        ))

#define glmVARYINGV_INDEXED(Shader, Name, Index, Swizzle) \
    gcmERR_BREAK(gcSHADER_AddSourceAttribute( \
        ShaderControl->i->shader, \
        glmATTRIBUTE_WRAP_INDEXED(Shader, Name, Index)->attribute, \
        Swizzle, \
        0 \
        ))

#define glmTEMP(TempRegister, Swizzle) \
    gcmASSERT(TempRegister != 0); \
    gcmERR_BREAK(gcSHADER_AddSource( \
        ShaderControl->i->shader, \
        gcSL_TEMP, \
        TempRegister, \
        gcSL_SWIZZLE_##Swizzle, \
        gcSL_FLOAT \
        ))

#define glmTEMPV(TempRegister, Swizzle) \
    gcmASSERT(TempRegister != 0); \
    gcmERR_BREAK(gcSHADER_AddSource( \
        ShaderControl->i->shader, \
        gcSL_TEMP, \
        TempRegister, \
        Swizzle, \
        gcSL_FLOAT \
        ))

#define glmADD_FUNCTION(FunctionName) \
    gcmERR_BREAK(gcSHADER_AddFunction( \
        ShaderControl->i->shader, \
        #FunctionName, \
        &ShaderControl->FunctionName \
        ))

#define glmBEGIN_FUNCTION(FunctionName) \
    gcmERR_BREAK(gcSHADER_BeginFunction( \
        ShaderControl->i->shader, \
        ShaderControl->FunctionName \
        ))

#define glmEND_FUNCTION(FunctionName) \
    gcmERR_BREAK(gcSHADER_EndFunction( \
        ShaderControl->i->shader, \
        ShaderControl->FunctionName \
        ))

#define glmRET() \
    gcmERR_BREAK(gcSHADER_AddOpcodeConditional( \
        ShaderControl->i->shader, \
        gcSL_RET, \
        gcSL_ALWAYS, \
        0 \
        ))

#define glmLABEL(Label) \
    gcmERR_BREAK(gcSHADER_AddLabel( \
        ShaderControl->i->shader, \
        Label \
        ))


/******************************************************************************\
******************************** Viewport states. ******************************
\******************************************************************************/

typedef struct _glsVIEWPORT * glsVIEWPORT_PTR;
typedef struct _glsVIEWPORT
{
    GLboolean               clippedOut;
    GLboolean               recomputeClipping;
    GLboolean               reprogramClipping;
    GLboolean               scissorTest;

    /* All box states: X, Y, Width, Height. */
    GLint                   viewportBox[4];
    GLint                   viewportClippedBox[4];
    GLint                   scissorBox[4];
    GLint                   scissorClippedBox[4];
}
glsVIEWPORT;


/******************************************************************************\
********************************** Alpha states. *******************************
\******************************************************************************/

typedef struct _glsALPHA * glsALPHA_PTR;
typedef struct _glsALPHA
{
    /* Alpha blend states. */
    GLboolean               blendEnabled;
    gleBLENDFUNCTIONS       blendSrcFunction;
    gleBLENDFUNCTIONS       blendDestFunction;
	gleBLENDFUNCTIONS		blendSrcFunctionRGB;
	gleBLENDFUNCTIONS		blendDstFunctionRGB;
	gleBLENDFUNCTIONS		blendSrcFunctionAlpha;
	gleBLENDFUNCTIONS		blendDstFunctionAlpha;
	gleBLENDMODES			blendModeRGB;
	gleBLENDMODES			blendModeAlpha;

    /* Alpha test states. */
    GLboolean               testEnabled;
    gleTESTFUNCTIONS        testFunction;
    glsMUTANT               testReference;
}
glsALPHA;


/******************************************************************************\
********************************* Stencil states. ******************************
\******************************************************************************/

typedef struct _glsSTENCIL * glsSTENCIL_PTR;
typedef struct _glsSTENCIL
{
    GLboolean               testEnabled;
    gleTESTFUNCTIONS        testFunction;
    GLint                   reference;
    GLuint                  valueMask;
    GLuint                  writeMask;
    gleSTENCILOPERATIONS    fail;
    gleSTENCILOPERATIONS    zFail;
    gleSTENCILOPERATIONS    zPass;
    GLint                   clearValue;

    GLboolean               dirty;
    gcsSTENCIL_INFO         hal;
}
glsSTENCIL;


/******************************************************************************\
********************************** Depth states. *******************************
\******************************************************************************/

typedef struct _glsDEPTH * glsDEPTH_PTR;
typedef struct _glsDEPTH
{
    GLboolean               polygonOffsetFill;
    glsMUTANT               clearValue;
    glsMUTANT               depthFactor;
    glsMUTANT               depthUnits;
    glsMUTANT               depthRange[2];
    GLboolean               testEnabled;
    gleTESTFUNCTIONS        testFunction;
    GLboolean               depthMask;
    gceDEPTH_MODE           depthMode;
    GLboolean               depthSuspended;

    /* Gets affected by depthUnits, depthFactor,
       depthMode and polygonOffsetFill. */
    GLboolean               polygonOffsetDirty;
}
glsDEPTH;


/******************************************************************************\
********************************* Culling states. ******************************
\******************************************************************************/

typedef struct _glsCULL * glsCULL_PTR;
typedef struct _glsCULL
{
    GLboolean               enabled;
    GLboolean               clockwiseFront;
    GLenum                  frontFace;
    GLenum                  cullFace;
}
glsCULL;


/******************************************************************************\
********************************* Lighting states. *****************************
\******************************************************************************/

typedef struct _glsLIGHTING * glsLIGHTING_PTR;
typedef struct _glsLIGHTING
{
    /***************************************************************************
    ** Global lighting states.
    */

    GLboolean               lightingEnabled;
    GLboolean               materialEnabled;
    GLboolean               lightEnabled[glvMAX_LIGHTS];
    GLenum                  shadeModel;

    /***************************************************************************
    ** Lighting model states.
    */

    /* Use two-sided lighting. */
    GLboolean               twoSidedLighting;

    /* Do two-sided lighting when rendering. */
    GLboolean               doTwoSidedlighting;

    /* Ambient color of scene. */
    glsVECTOR               Acs;

    /***************************************************************************
    ** Color material states.
    */

    /* Ambient color of material. */
    glsVECTOR               Acm;

    /* Diffuse color of material. */
    glsVECTOR               Dcm;

    /* Specular color of material. */
    glsVECTOR               Scm;

    /* Emissive color of material. */
    glsVECTOR               Ecm;

    /* Specular exponent, range: [0.0, 128.0]. */
    glsMUTANT               Srm;

    /***************************************************************************
    ** Lighting parameters for each individual light source.
    */

    /* Number of enabled lights. */
    GLuint                  lightCount;

    /* Generuc function genertion enable flag. */
    GLuint                  useFunction;

    /* Ambient intensity. */
    glsVECTOR               Acli[glvMAX_LIGHTS];

    /* Diffuse intensity. */
    glsVECTOR               Dcli[glvMAX_LIGHTS];

    /* Specular intensity. */
    glsVECTOR               Scli[glvMAX_LIGHTS];

    /* Light position. */
    glsVECTOR               Ppli[glvMAX_LIGHTS];

    /* Position. */
    GLboolean               Directional[glvMAX_LIGHTS];

    /* Direction of spotlight. */
    glsVECTOR               Sdli[glvMAX_LIGHTS];

    /* Spotlight exponent, range: [0.0, 128.0]. */
    glsMUTANT               Srli[glvMAX_LIGHTS];

    /* Spotlight cutoff angle. */
    glsMUTANT               Crli[glvMAX_LIGHTS];

    /* Spotlight cutoff angle == 180 flag. */
    GLboolean               Crli180[glvMAX_LIGHTS];

    /* Spotlight cutoff is 180 flag for shader uniform. */
    glsMUTANT               uCrli180[glvMAX_LIGHTS];

    /* Spotlight cutoff angle in radians. */
    glsMUTANT               RadCrli[glvMAX_LIGHTS];

    /* Constant attenuation factor, range: [0.0, infinity). */
    glsMUTANT               K0i[glvMAX_LIGHTS];

    /* Linear attenuation factor, range: [0.0, infinity). */
    glsMUTANT               K1i[glvMAX_LIGHTS];

    /* Quadratic attenuation factor, range: [0.0, infinity). */
    glsMUTANT               K2i[glvMAX_LIGHTS];
}
glsLIGHTING;


/******************************************************************************\
************************************ Fog states. *******************************
\******************************************************************************/

typedef struct _glsFOG * glsFOG_PTR;
typedef struct _glsFOG
{
    GLboolean               enabled;

    gleFOGMODES             mode;
    glsVECTOR               color;
    glsMUTANT               density;
    glsMUTANT               start;
    glsMUTANT               end;

    GLboolean               linearDirty;
    gltFRACTYPE             linearFactor0;
    gltFRACTYPE             linearFactor1;

    GLboolean               expDirty;
    gltFRACTYPE             expFactor;

    GLboolean               exp2Dirty;
    gltFRACTYPE             exp2Factor;

    GLenum                  hint;
}
glsFOG;


/******************************************************************************\
********************************** Point states. *******************************
\******************************************************************************/

typedef struct _glsPOINT * glsPOINT_PTR;
typedef struct _glsPOINT
{
    /* Not zero if current primitive is a point. */
    GLboolean               pointPrimitive;

    /* Point sprite states. */
    GLboolean               spriteDirty;
    GLboolean               spriteEnable;
    GLboolean               spriteActive;

    /* Point smooth enable flag. */
    GLboolean               smooth;

    /* Lower bound to which the derived point size is clamped. */
    glsMUTANT               clampFrom;

    /* Upper bound to which the derived point size is clamped. */
    glsMUTANT               clampTo;

    /* Distance attenuation function coefficients. */
    glsVECTOR               attenuation;

    /* Point fade threshold size. */
    glsMUTANT               fadeThrdshold;

    GLenum                  hint;
}
glsPOINT;


/******************************************************************************\
******************************* Multisample states. ****************************
\******************************************************************************/

typedef struct _glsMULTISAMPLE * glsMULTISAMPLE_PTR;
typedef struct _glsMULTISAMPLE
{
    GLboolean               enabled;
    glsMUTANT               coverageValue;
    GLboolean               coverageInvert;
    GLboolean               coverage;
    GLboolean               alphaToCoverage;
    GLboolean               alphaToOne;
}
glsMULTISAMPLE;


/******************************************************************************\
******************************* Multisample states. ****************************
\******************************************************************************/

typedef struct _glsLOGICOP * glsLOGICOP_PTR;
typedef struct _glsLOGICOP
{
    GLboolean               enabled;
    GLenum                  operation;
    GLboolean               perform;
    gctUINT8                rop;
}
glsLOGICOP;


/******************************************************************************\
********************************** Line states. ********************************
\******************************************************************************/

typedef struct _glsLINE * glsLINE_PTR;
typedef struct _glsLINE
{
    GLboolean               smooth;
    glsMUTANT               width;
    glsMUTANT               queryWidth;
    GLenum                  hint;
}
glsLINE;


/******************************************************************************\
******************** Input attribute (stream) type definition. *****************
\******************************************************************************/

typedef struct _glsATTRIBUTEINFO * glsATTRIBUTEINFO_PTR;
typedef struct _glsATTRIBUTEINFO
{
    /* Current value. */
    glsVECTOR               currValue;
    GLboolean               dirty;

    /* Stream. */
    GLboolean               streamEnabled;
    gceVERTEX_FORMAT        format;
    GLboolean               normalize;
    GLuint                  components;
    gcSHADER_TYPE           attributeType;
    gcSHADER_TYPE           varyingType;
    gcSL_SWIZZLE            varyingSwizzle;
    GLsizei                 stride;
    GLsizei                 attributeSize;
    gctCONST_POINTER        pointer;
    glsNAMEDOBJECT_PTR      buffer;

    /* GL information used for query. */
    GLenum                  queryFormat;
    GLsizei                 queryStride;
}
glsATTRIBUTEINFO;


/******************************************************************************\
************************** Shader container definitions. ***********************
\******************************************************************************/

typedef gceSTATUS (*glfUNIFORMSET) (
    glsCONTEXT_PTR,
    gcUNIFORM
    );

typedef struct _glsUNIFORMWRAP * glsUNIFORMWRAP_PTR;
typedef struct _glsUNIFORMWRAP
{
    gcUNIFORM               uniform;
    glfUNIFORMSET           set;
    gctBOOL_PTR             dirty;
}
glsUNIFORMWRAP;

typedef struct _glsATTRIBUTEWRAP * glsATTRIBUTEWRAP_PTR;
typedef struct _glsATTRIBUTEWRAP
{
    gcATTRIBUTE             attribute;
    glsATTRIBUTEINFO_PTR    info;
    gctINT                  binding;
}
glsATTRIBUTEWRAP;

typedef struct _glsSHADERCONTROL * glsSHADERCONTROL_PTR;
typedef struct _glsSHADERCONTROL
{
    gcSHADER                shader;
    glsUNIFORMWRAP_PTR      uniforms;
    glsATTRIBUTEWRAP_PTR    attributes;
    glsUNIFORMWRAP_PTR      texture[glvMAX_TEXTURES];
}
glsSHADERCONTROL;

/******************************************************************************\
************************** Shader uniform dirty bits definitions. *************
\******************************************************************************/

typedef struct _glsVSUNIFORMDIRTYINFO * glsVSUNIFORMDIRTYINFO_PTR;
typedef struct _glsVSUNIFORMDIRTYINFO
{
    /* Input uniforms. */
    gctBOOL                 uColorDirty;
    gctBOOL                 uNormalDirty;
    gctBOOL                 uTexCoordDirty;

    /* Matrix uniforms. */
    gctBOOL                 uModelViewDirty;
    gctBOOL                 uProjectionDirty;
    gctBOOL                 uModeViewProjectionDirty;
    gctBOOL                 uModelViewInverse3x3TransposedDirty;
    gctBOOL                 uTexMatrixDirty;
    gctBOOL                 uMatrixPaletteDirty;
    gctBOOL                 uMatrixPaletteInverseDirty;

    /* Meterial uniforms. */
    gctBOOL                 uAcsDirty;
    gctBOOL                 uAcmDirty;
    gctBOOL                 uDcmDirty;
    gctBOOL                 uScmDirty;
    gctBOOL                 uEcmDirty;
    gctBOOL                 uSrmDirty;

    /* Light uniforms. */
    gctBOOL                 uAcliDirty;
    gctBOOL                 uDcliDirty;
    gctBOOL                 uScliDirty;
    gctBOOL                 uPpliDirty;
    gctBOOL                 uSrliDirty;
    gctBOOL                 uCrli180Dirty;
    gctBOOL                 uKiDirty;

    /* Dependent light uniforms. */
    gctBOOL                 uAcmAcliDirty;
    gctBOOL                 uVPpliDirty;
    gctBOOL                 uDcmDcliDirty;
    gctBOOL                 uCosCrliDirty;
    gctBOOL                 uNormedSdliDirty;

    /* Clip plane uniforms. */
    gctBOOL                 uClipPlaneDirty;

    /* Point uniforms. */
    gctBOOL                 uPointAttenuationDirty;
    gctBOOL                 uPointSizeDirty;
    gctBOOL                 uViewportDirty;
}
glsVSUNIFORMDIRTYINFO;

typedef struct _glsFSUNIFORMDIRTYINFO * glsFSUNIFORMDIRTYINFO_PTR;
typedef struct _glsFSUNIFORMDIRTYINFO
{
    /* Input uniforms. */
    gctBOOL                 uColorDirty;
    gctBOOL                 uTexCoordDirty;
    gctBOOL                 uTexColorDirty;

    /* Tex Env uniforms. */
    gctBOOL                 uTexCombScaleDirty;

    /* Fog uniforms. */
    gctBOOL                 uFogFactorsDirty;
    gctBOOL                 uFogColorDirty;
}
glsFSUNIFORMDIRTYINFO;

/******************************************************************************\
*************************** Shader generation helpers. *************************
\******************************************************************************/

gceSTATUS glfSetUniformFromFractions(
    IN gcUNIFORM Uniform,
    IN gltFRACTYPE X,
    IN gltFRACTYPE Y,
    IN gltFRACTYPE Z,
    IN gltFRACTYPE W
    );

gceSTATUS glfSetUniformFromMutants(
    IN gcUNIFORM Uniform,
    IN glsMUTANT_PTR MutantX,
    IN glsMUTANT_PTR MutantY,
    IN glsMUTANT_PTR MutantZ,
    IN glsMUTANT_PTR MutantW,
    IN gltFRACTYPE* ValueArray,
    IN gctUINT Count
    );

gceSTATUS glfSetUniformFromVectors(
    IN gcUNIFORM Uniform,
    IN glsVECTOR_PTR Vector,
    IN gltFRACTYPE* ValueArray,
    IN gctUINT Count
    );

gceSTATUS glfSetUniformFromMatrix(
    IN gcUNIFORM Uniform,
    IN glsMATRIX_PTR Matrix,
    IN gltFRACTYPE* ValueArray,
    IN gctUINT MatrixCount,
    IN gctUINT ColumnCount,
    IN gctUINT RowCount
    );

gceSTATUS glfUsingUniform(
    IN glsSHADERCONTROL_PTR ShaderControl,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    IN glfUNIFORMSET UniformSet,
    IN gctBOOL_PTR DirtyBit,
    IN glsUNIFORMWRAP_PTR* UniformWrap
    );

gceSTATUS glfUsingAttribute(
    IN glsSHADERCONTROL_PTR ShaderControl,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    IN gctBOOL IsTexture,
    IN glsATTRIBUTEINFO_PTR AttributeInfo,
    IN glsATTRIBUTEWRAP_PTR* AttributeWrap,
    IN gctINT Binding
    );

gceSTATUS glfUsingVarying(
    IN glsSHADERCONTROL_PTR ShaderControl,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    IN gctBOOL IsTexture,
    IN glsATTRIBUTEWRAP_PTR* AttributeWrap
    );

gceSTATUS glfUsing_uTexCoord(
    IN glsSHADERCONTROL_PTR ShaderControl,
    IN gctBOOL_PTR DirtyBit,
    OUT glsUNIFORMWRAP_PTR* UniformWrap
    );


/******************************************************************************\
***************************** Shader generation API. ***************************
\******************************************************************************/

gceSTATUS glfLoadShader(
    IN glsCONTEXT_PTR Context
    );

gceSTATUS glfGenerateVSFixedFunction(
    IN glsCONTEXT_PTR Context
    );

gceSTATUS glfGenerateFSFixedFunction(
    IN glsCONTEXT_PTR Context
    );

#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_fixed_func_h_ */
