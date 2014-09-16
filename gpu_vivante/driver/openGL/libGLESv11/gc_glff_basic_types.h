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




#ifndef __gc_glff_basic_types_h_
#define __gc_glff_basic_types_h_

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************\
********************************* Mutable Types ********************************
\******************************************************************************/

typedef enum _gleTYPE
{
    glvBOOL,
    glvINT,
    glvNORM,
    glvFIXED,
    glvFLOAT
}
gleTYPE, * gleTYPE_PTR;

typedef union _gluMUTABLE * gluMUTABLE_PTR;
typedef union _gluMUTABLE
{
    GLint                   i;
    GLuint                  ui;
    GLenum                  e;
    GLfixed                 x;
    GLclampx                cx;
    GLfloat                 f;
    GLclampf                cf;
}
gluMUTABLE;

typedef struct _glsMUTANT * glsMUTANT_PTR;
typedef struct _glsMUTANT
{
    /* Mutable value (must be at the beginning of the structure). */
    gluMUTABLE              value;

    /* Special value flags. */
    GLboolean               zero;
    GLboolean               one;

    /* Current type. */
    gleTYPE                 type;
}
glsMUTANT;

typedef struct _glsVECTOR * glsVECTOR_PTR;
typedef struct _glsVECTOR
{
    /* Mutable values (must be at the beginning of the structure). */
    gluMUTABLE              value[4];

    /* Special value flags. */
    GLboolean               zero3;
    GLboolean               zero4;
    GLboolean               one3;
    GLboolean               one4;

    /* Current type. */
    gleTYPE                 type;
}
glsVECTOR;

typedef struct _glsMATRIX * glsMATRIX_PTR;
typedef struct _glsMATRIX
{
    /* Matrix mutable values. */
    gluMUTABLE              value[16];

    /* Current type. */
    gleTYPE                 type;

    /* Special value flags. */
    GLboolean               identity;
}
glsMATRIX;


/******************************************************************************\
***************************** glsMUTANT Definitions ****************************
\******************************************************************************/

#define glmFIXEDMUTANTZERO \
    { \
        { 0 }, \
        GL_TRUE,            /* Zero. */ \
        GL_FALSE,           /* One. */ \
        glvFIXED \
    }

#define glmFIXEDMUTANTONE \
    { \
        { gcvONE_X }, \
        GL_FALSE,           /* Zero. */ \
        GL_TRUE,            /* One. */ \
        glvFIXED \
    }


/******************************************************************************\
******************************* Common Definitions *****************************
\******************************************************************************/

#define glvFLOATPI              3.141592653589793238462643383279502f
#define glvFIXEDPI              0x0003243F
#define glvFLOATPIOVER180       0.017453292519943295769236907684886f
#define glvFIXEDPIOVER180       0x00000478
#define glvFLOATPITIMES2        6.283185307179586476925286766559f
#define glvFIXEDPITIMES2        0x0006487F
#define glvFLOATPIOVER2         1.5707963267948966192313216916398f
#define glvFIXEDPIOVER2         0x00019220

#define glvFLOATONEOVER65535    ((GLfloat) 1.0f / 65535.0f)
#define glvFIXEDONEOVER65535    ((GLfixed) 0x00000001)

#define glvMAXLONG              0x7fffffff
#define glvMINLONG              0x80000000

#define glmFIXEDCLAMP(_x) \
    (((_x) < gcvNEGONE_X) \
        ? gcvNEGONE_X \
        : (((_x) > gcvONE_X) \
            ? gcvONE_X \
            : (_x)))

#define glmFLOATCLAMP(_f) \
    (((_f) < -1.0f) \
        ? -1.0f \
        : (((_f) > 1.0f) \
            ? 1.0f \
            : (_f)))

#define glmBOOL2INT(_b)         ((GLint) (glmINT2BOOL(_b)))
#define glmBOOL2FIXED(_b)       ((GLfixed) (glmINT2BOOL(_b) << 16))
#define glmBOOL2FLOAT(_b)       ((GLfloat) (glmINT2BOOL(_b)))

#define glmINT2BOOL(_i)         ((GLboolean) ((_i)? GL_TRUE : GL_FALSE))
#define glmINT2FIXED(_i)        ((GLfixed) ((_i) << 16))
#define glmINT2FLOAT(_i)        gcoMATH_Int2Float(_i)

#define glmFIXED2BOOL(_x)       ((GLboolean) ((_x)? GL_TRUE : GL_FALSE))
#define glmFIXED2INT(_x)        ((GLint) (((_x) + 0x8000) >> 16))
#define glmFIXED2NORM(_x) \
    ((GLint) \
    ((((GLfixed) (_x)) < 0) \
        ? ((((gctINT64) (glmFIXEDCLAMP(_x))) * (GLint) glvMINLONG) >> 16) \
        : ((((gctINT64) (glmFIXEDCLAMP(_x))) * (GLint) glvMAXLONG) >> 16)))
#define glmFIXED2FLOAT(_x)      gcoMATH_Fixed2Float(_x)

#define glmFLOAT2BOOL(_f)       ((GLboolean) ((_f)? GL_TRUE : GL_FALSE))
#define glmFLOAT2NORM(_f)       ((GLint) (glmFLOATCLAMP(_f) * (GLint) glvMAXLONG))
#define glmFLOAT2INT(_f)        ((GLint) ((_f) + 0.5f))
#define glmFLOAT2FIXED(_f)      ((GLfixed) ((_f) * 65536.0f))


#define glmVEC(Vector, Component) \
    (Vector)->value[Component]

#define glmVECFIXED(Vector, Component) \
    glmVEC(Vector, Component).x

#define glmVECFLOAT(Vector, Component) \
    glmVEC(Vector, Component).f


#define glmMAT(Matrix, X, Y) \
    (Matrix)->value[(Y) + ((X) << 2)]

#define glmMATFIXED(Matrix, X, Y) \
    glmMAT(Matrix, X, Y).x

#define glmMATFLOAT(Matrix, X, Y) \
    glmMAT(Matrix, X, Y).f


#define glmFIXEDCLAMP_NEG1_TO_1(_x) \
    (((_x) < gcvNEGONE_X) \
        ? gcvNEGONE_X \
        : (((_x) > gcvONE_X) \
            ? gcvONE_X \
            : (_x)))

#define glmFLOATCLAMP_NEG1_TO_1(_f) \
    (((_f) < -1.0f) \
        ? -1.0f \
        : (((_f) > 1.0f) \
            ? 1.0f \
            : (_f)))


#define glmFIXEDCLAMP_0_TO_1(_x) \
    (((_x) < 0) \
        ? 0 \
        : (((_x) > gcvONE_X) \
            ? gcvONE_X \
            : (_x)))

#define glmFLOATCLAMP_0_TO_1(_f) \
    (((_f) < 0.0f) \
        ? 0.0f \
        : (((_f) > 1.0f) \
            ? 1.0f \
            : (_f)))


#define glmFIXEDMULTIPLY(x1, x2) \
    gcmXMultiply(x1, x2)

#define glmFLOATMULTIPLY(x1, x2) \
    ((x1) * (x2))


#define glmFIXEDDIVIDE(x1, x2) \
    gcmXDivide(x1, x2)

#define glmFLOATDIVIDE(x1, x2) \
    ((x1) / (x2))


#define glmFIXEDMULTIPLY3(x1, x2, x3) \
    gcmXMultiply(gcmXMultiply(x1, x2), x3)

#define glmFLOATMULTIPLY3(x1, x2, x3) \
    ((x1) * (x2) * (x3))

#define glmFIXEDMULTIPLYDIVIDE(x1, x2, x3) \
    gcmXMultiplyDivide(x1, x2, x3)


#define glmSWAP(Type, Value1, Value2) \
    { \
        Type temp; \
        temp = (Value1); \
        (Value1) = (Value2); \
        (Value2) = temp; \
    }

#define glmABS(Value) \
    (((Value) > 0)? (Value) : -(Value))


#if defined(COMMON_LITE)

#   define glvGLESDRIVERNAME            "OpenGL ES-CL 1.1"

#   define glvFRACGLTYPEENUM            GL_FIXED
#   define glvFRACTYPEENUM              glvFIXED
#   define gltFRACTYPE                  GLfixed

#   define glvFRACZERO                  ((GLfixed) 0x00000000)
#   define glvFRACPOINT2                ((GLfixed) 0x00003333)
#   define glvFRACHALF                  ((GLfixed) 0x00008000)
#   define glvFRACPOINT8                ((GLfixed) 0x0000CCCC)
#   define glvFRACONE                   ((GLfixed) 0x00010000)
#   define glvFRACNEGONE                ((GLfixed) 0xFFFF0000)
#   define glvFRACTWO                   ((GLfixed) 0x00020000)
#   define glvFRAC90                    ((GLfloat) 0x005A0000)
#   define glvFRAC128                   ((GLfixed) 0x00800000)
#   define glvFRAC180                   ((GLfixed) 0x00B40000)
#   define glvFRAC256                   ((GLfixed) 0x01000000)
#   define glvFRAC4096                  ((GLfixed) 0x10000000)
#   define glvFRACONEOVER255            ((GLfixed) 0x00000101)
#   define glvFRACONEOVER65535          glvFIXEDONEOVER65535

#   define glvFRACPI                    ((GLfixed) glvFIXEDPI)
#   define glvFRACPIOVER180             ((GLfixed) glvFIXEDPIOVER180)
#   define glvFRACPITIMES2              ((GLfixed) glvFIXEDPITIMES2)
#   define glvFRACPIOVER2               ((GLfixed) glvFIXEDPIOVER2)

#   define gcSHADER_FRAC_X1             gcSHADER_FIXED_X1
#   define gcSHADER_FRAC_X2             gcSHADER_FIXED_X2
#   define gcSHADER_FRAC_X3             gcSHADER_FIXED_X3
#   define gcSHADER_FRAC_X4             gcSHADER_FIXED_X4

#   define glmBOOL2FRAC                 glmBOOL2FIXED
#   define glmINT2FRAC                  glmINT2FIXED
#   define glmFIXED2FRAC
#   define glmFLOAT2FRAC                glmFLOAT2FIXED
#   define glmFRAC2BOOL                 glmFIXED2BOOL
#   define glmFRAC2INT                  glmFIXED2INT
#   define glmFRAC2NORM                 glmFIXED2NORM
#   define glmFRACVEC                   glmVECFIXED
#   define glmFRACMAT                   glmMATFIXED
#   define glmFRAC2FLOAT                glmFIXED2FLOAT

#   define glmFRACMULTIPLY              glmFIXEDMULTIPLY
#   define glmFRACDIVIDE                glmFIXEDDIVIDE
#   define glmFRACMULTIPLY3             glmFIXEDMULTIPLY3

#   define gco3D_SetClearDepth          gco3D_SetClearDepthX
#   define gco3D_SetDepthScaleBias      gco3D_SetDepthScaleBiasX
#   define gco3D_SetDepthRange          gco3D_SetDepthRangeX
#   define gco3D_SetClearColorFrac      gco3D_SetClearColorX
#   define gco3D_SetFragmentColor       gco3D_SetFragmentColorX
#   define gco3D_SetFogColor            gco3D_SetFogColorX
#   define gco3D_SetTetxureColor        gco3D_SetTetxureColorX

#   define gcUNIFORM_SetFracValue       gcUNIFORM_SetValueX

#   define glfFracFromRaw               glfFixedFromRaw
#   define glfFracFromMutable           glfFixedFromMutable
#   define glfFracFromMutant            glfFixedFromMutant
#   define glfSetFracMutant             glfSetFixedMutant
#   define glfSetFracVector4            glfSetFixedVector4

#else

#   define glvGLESDRIVERNAME            "OpenGL ES-CM 1.1"

#   define glvFRACGLTYPEENUM            GL_FLOAT
#   define glvFRACTYPEENUM              glvFLOAT
#   define gltFRACTYPE                  GLfloat

#   define glvFRACZERO                  ((GLfloat)    0.0f)
#   define glvFRACPOINT2                ((GLfloat)    0.2f)
#   define glvFRACHALF                  ((GLfloat)    0.5f)
#   define glvFRACPOINT8                ((GLfloat)    0.8f)
#   define glvFRACONE                   ((GLfloat)    1.0f)
#   define glvFRACNEGONE                ((GLfloat)   -1.0f)
#   define glvFRACTWO                   ((GLfloat)    2.0f)
#   define glvFRAC90                    ((GLfloat)   90.0f)
#   define glvFRAC128                   ((GLfloat)  128.0f)
#   define glvFRAC180                   ((GLfloat)  180.0f)
#   define glvFRAC256                   ((GLfloat)  256.0f)
#   define glvFRAC4096                  ((GLfloat) 4096.0f)
#   define glvFRACONEOVER255            ((GLfloat) 1.0f / 255.0f)
#   define glvFRACONEOVER65535          glvFLOATONEOVER65535

#   define glvFRACPI                    ((GLfloat) glvFLOATPI)
#   define glvFRACPIOVER180             ((GLfloat) glvFLOATPIOVER180)
#   define glvFRACPITIMES2              ((GLfloat) glvFLOATPITIMES2)
#   define glvFRACPIOVER2               ((GLfloat) glvFLOATPIOVER2)

#   define gcSHADER_FRAC_X1             gcSHADER_FLOAT_X1
#   define gcSHADER_FRAC_X2             gcSHADER_FLOAT_X2
#   define gcSHADER_FRAC_X3             gcSHADER_FLOAT_X3
#   define gcSHADER_FRAC_X4             gcSHADER_FLOAT_X4

#   define glmBOOL2FRAC                 glmBOOL2FLOAT
#   define glmINT2FRAC                  glmINT2FLOAT
#   define glmFIXED2FRAC                glmFIXED2FLOAT
#   define glmFLOAT2FRAC
#   define glmFRAC2BOOL                 glmFLOAT2BOOL
#   define glmFRAC2INT                  glmFLOAT2INT
#   define glmFRAC2NORM                 glmFLOAT2NORM
#   define glmFRACVEC                   glmVECFLOAT
#   define glmFRACMAT                   glmMATFLOAT
#   define glmFRAC2FLOAT

#   define glmFRACMULTIPLY              glmFLOATMULTIPLY
#   define glmFRACDIVIDE                glmFLOATDIVIDE
#   define glmFRACMULTIPLY3             glmFLOATMULTIPLY3

#   define gco3D_SetClearDepth          gco3D_SetClearDepthF
#   define gco3D_SetDepthScaleBias      gco3D_SetDepthScaleBiasF
#   define gco3D_SetDepthRange          gco3D_SetDepthRangeF
#   define gco3D_SetClearColorFrac      gco3D_SetClearColorF
#   define gco3D_SetFragmentColor       gco3D_SetFragmentColorF
#   define gco3D_SetFogColor            gco3D_SetFogColorF
#   define gco3D_SetTetxureColor        gco3D_SetTetxureColorF

#   define gcUNIFORM_SetFracValue       gcUNIFORM_SetValueF

#   define glfFracFromRaw               glfFloatFromRaw
#   define glfFracFromMutable           glfFloatFromMutable
#   define glfFracFromMutant            glfFloatFromMutant
#   define glfSetFracMutant             glfSetFloatMutant
#   define glfSetFracVector4            glfSetFloatVector4

#endif


/******************************************************************************\
***************** Value Conversion From/To OpenGL Enumerations *****************
\******************************************************************************/

GLboolean glfConvertGLEnum(
    const GLenum* Names,
    GLint NameCount,
    const GLvoid* Value,
    gleTYPE Type,
    GLuint* Result
    );

GLboolean glfConvertGLboolean(
    const GLvoid* Value,
    gleTYPE Type,
    GLuint* Result
    );


/******************************************************************************\
***************************** Value Type Converters ****************************
\******************************************************************************/

void glfGetFromBool(
    GLboolean Variable,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromBoolArray(
    const GLboolean* Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromInt(
    GLint Variable,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromIntArray(
    const GLint* Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromFixed(
    GLfixed Variable,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromFixedArray(
    const GLfixed* Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromFloat(
    GLfloat Variable,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromFloatArray(
    const GLfloat* Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromEnum(
    GLenum Variable,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromEnumArray(
    const GLenum* Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromMutable(
    gluMUTABLE Variable,
    gleTYPE VariableType,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromMutableArray(
    const gluMUTABLE_PTR Variables,
    gleTYPE VariableType,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromMutant(
    const glsMUTANT_PTR Variable,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromMutantArray(
    const glsMUTANT_PTR Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromVector3(
    const glsVECTOR_PTR Variable,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromVector4(
    const glsVECTOR_PTR Variable,
    GLvoid* Value,
    gleTYPE Type
    );

void glfGetFromMatrix(
    const glsMATRIX_PTR Variable,
    GLvoid* Value,
    gleTYPE Type
    );


/******************************************************************************\
**************************** Get Values From Raw Input *************************
\******************************************************************************/

GLfixed glfFixedFromRaw(
    const GLvoid* Variable,
    gleTYPE Type
    );

GLfloat glfFloatFromRaw(
    const GLvoid* Variable,
    gleTYPE Type
    );


/******************************************************************************\
**************************** Get Values From Mutables **************************
\******************************************************************************/

GLboolean glfBoolFromMutable(
    gluMUTABLE Variable,
    gleTYPE Type
    );

GLint glfIntFromMutable(
    gluMUTABLE Variable,
    gleTYPE Type
    );

GLfixed glfFixedFromMutable(
    gluMUTABLE Variable,
    gleTYPE Type
    );

GLfloat glfFloatFromMutable(
    gluMUTABLE Variable,
    gleTYPE Type
    );


/******************************************************************************\
**************************** Get Values From Mutants ***************************
\******************************************************************************/

GLboolean glfBoolFromMutant(
    const glsMUTANT_PTR Variable
    );

GLint glfIntFromMutant(
    const glsMUTANT_PTR Variable
    );

GLfixed glfFixedFromMutant(
    const glsMUTANT_PTR Variable
    );

GLfloat glfFloatFromMutant(
    const glsMUTANT_PTR Variable
    );


/******************************************************************************\
***************************** Set Values To Mutants ****************************
\******************************************************************************/

void glfSetIntMutant(
    glsMUTANT_PTR Variable,
    GLint Value
    );

void glfSetFixedMutant(
    glsMUTANT_PTR Variable,
    GLfixed Value
    );

void glfSetFloatMutant(
    glsMUTANT_PTR Variable,
    GLfloat Value
    );

void glfSetMutant(
    glsMUTANT_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    );

void glfSetClampedMutant(
    glsMUTANT_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    );


/******************************************************************************\
***************************** Set Values To Vectors ****************************
\******************************************************************************/

void glfSetIntVector4(
    glsVECTOR_PTR Variable,
    GLint X,
    GLint Y,
    GLint Z,
    GLint W
    );

void glfSetFixedVector4(
    glsVECTOR_PTR Variable,
    GLfixed X,
    GLfixed Y,
    GLfixed Z,
    GLfixed W
    );

void glfSetFloatVector4(
    glsVECTOR_PTR Variable,
    GLfloat X,
    GLfloat Y,
    GLfloat Z,
    GLfloat W
    );

void glfSetVector3(
    glsVECTOR_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    );

void glfSetVector4(
    glsVECTOR_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    );

void glfSetClampedVector4(
    glsVECTOR_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    );

void glfSetHomogeneousVector4(
    glsVECTOR_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    );

void glfSetVector4FromMutants(
    glsVECTOR_PTR Variable,
    const glsMUTANT_PTR X,
    const glsMUTANT_PTR Y,
    const glsMUTANT_PTR Z,
    const glsMUTANT_PTR W
    );


/******************************************************************************\
*********************** Operations on Mutants and Vectors **********************
\******************************************************************************/

void glfMulMutant(
    const glsMUTANT_PTR Variable1,
    const gltFRACTYPE Variable2,
    glsMUTANT_PTR Result
    );

void glfCosMutant(
    const glsMUTANT_PTR Variable,
    glsMUTANT_PTR Result
    );

void glfAddVector4(
    const glsVECTOR_PTR Variable1,
    const glsVECTOR_PTR Variable2,
    glsVECTOR_PTR Result
    );

void glfMulVector4(
    const glsVECTOR_PTR Variable1,
    const glsVECTOR_PTR Variable2,
    glsVECTOR_PTR Result
    );

void glfNorm3Vector4(
    const glsVECTOR_PTR Variable,
    glsVECTOR_PTR Result
    );

void glfNorm3Vector4f(
    const glsVECTOR_PTR Variable,
    glsVECTOR_PTR Result
    );

void glfHomogeneousVector4(
    const glsVECTOR_PTR Variable,
    glsVECTOR_PTR Result
    );


/******************************************************************************\
**************** Debug Printing for Mutants, Vectors and Matrices **************
\******************************************************************************/

#if gcmIS_DEBUG(gcdDEBUG_TRACE)

void glfPrintMatrix3x3(
    gctSTRING Name,
    glsMATRIX_PTR Matrix
    );

void glfPrintMatrix4x4(
    gctSTRING Name,
    glsMATRIX_PTR Matrix
    );

void glfPrintVector3(
    gctSTRING Name,
    glsVECTOR_PTR Vector
    );

void glfPrintVector4(
    gctSTRING Name,
    glsVECTOR_PTR Vector
    );

void glfPrintMutant(
    gctSTRING Name,
    glsMUTANT_PTR Mutant
    );

#endif

#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_basic_types_h_ */
