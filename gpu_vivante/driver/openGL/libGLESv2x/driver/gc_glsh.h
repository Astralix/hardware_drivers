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


#ifndef __libgl_h_
#define __libgl_h_

#define gldUSE_3D_RENDERING_CLEAR   1
#define GL_USE_VERTEX_ARRAY         1
#define gldFBO_DATABASE             0
#define BLOB_CACHE_KEY_SIZE         32
#define BLOB_CACHE_MD5_SIZE         16
#define BLOB_CACHE_MAX_BINARY_SIZE  4 << 10

typedef struct _GLContext *         GLContext;
typedef struct _GLObject *          GLObject;
typedef struct _GLShader *          GLShader;
typedef struct _GLProgram *         GLProgram;
typedef struct _GLTexture *         GLTexture;
typedef struct _GLBuffer *          GLBuffer;
typedef struct _GLAttribute         GLAttribute;
typedef struct _GLFramebuffer *     GLFramebuffer;
typedef struct _GLRenderbuffer *    GLRenderbuffer;
typedef struct _GLUniform *         GLUniform;
typedef struct _GLBinding *         GLBinding;
typedef struct _GLVertexArray *     GLVertexArray;

#if defined(GC_USE_GL)
#   include "gc_gl.h"
#endif

#if gldFBO_DATABASE
#   include "gc_glsh_database.h"
#endif

#define gldNAMED_OBJECTS_HASH       32
#define gldVERTEX_ELEMENT_COUNT     16

#if gcdUSE_TRIANGLE_STRIP_PATCH
#    define  gldINDEX_MAX_SUBS      64
#endif

#define gldZONE_TRACE               (gcvZONE_API_ES20 | (1 << 0))
#define gldZONE_DRIVER              (gcvZONE_API_ES20 | (1 << 1))
#define gldZONE_BUFFER              (gcvZONE_API_ES20 | (1 << 2))
#define gldZONE_FRAMEBUFFER         (gcvZONE_API_ES20 | (1 << 3))
#define gldZONE_QUERY               (gcvZONE_API_ES20 | (1 << 4))
#define gldZONE_STATE               (gcvZONE_API_ES20 | (1 << 5))
#define gldZONE_VERTEX              (gcvZONE_API_ES20 | (1 << 6))
#define gldZONE_DRAW                (gcvZONE_API_ES20 | (1 << 7))
#define gldZONE_TEXTURE             (gcvZONE_API_ES20 | (1 << 8))
#define gldZONE_DATABASE            (gcvZONE_API_ES20 | (1 << 9))

#ifndef _GC_OBJ_ZONE
#define _GC_OBJ_ZONE                gldZONE_TRACE
#endif

#ifndef INT32_MAX
#define INT32_MAX       (0x7fffffff)
#endif
#ifndef INT32_MIN
#define INT32_MIN       (-0x7fffffff-1)
#endif

#define gldBOUNDTYPE_VERTEX         0x1
#define gldBOUNDTYPE_INDEX          0x2

#define gl2mERROR(__result__) \
{ \
    GLenum lastResult = __result__;  \
    \
    if (gl2mIS_ERROR(lastResult)) \
    { \
        GLContext _context = _glshGetCurrentContext(); \
        \
        gcmTRACE( \
            gcvLEVEL_ERROR, \
            "gl2mERROR: result=0x%04X @ %s(%d)", \
            lastResult, __FUNCTION__, __LINE__ \
            ); \
        \
        if ((_context != gcvNULL) && gl2mIS_SUCCESS(_context->error)) \
        {  \
            _context->error = lastResult; \
        } \
    } \
} \
do { } while (gcvFALSE)

#define gl2mIS_ERROR(func) \
    ((func) != GL_NO_ERROR)

#define gl2mIS_SUCCESS(func) \
    ((func) == GL_NO_ERROR)

#define glmARGINT   "%d"
#define glmARGUINT  "%u"
#define glmARGENUM  "0x%04X"
#define glmARGFIXED "0x%08X"
#define glmARGHEX   "0x%08X"
#define glmARGPTR   "0x%x"
#define glmARGFLOAT "%1.4f"
#define glmARGLINT "%ld"

#define __glmDOENTER() \
    do \
    { \
		glmGetApiStartTime(); \
		context = _glshGetCurrentContext(); \
        \
        if (context == gcvNULL) \
        { \
            break; \
        }

#define glmENTER() \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
    \
    gcmHEADER(); \
    \
    __glmDOENTER()

#define glmENTER1( \
    Type, Value \
    ) \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value "=" Type, Value); \
    \
    __glmDOENTER()

#define glmENTER2( \
    Type1, Value1, \
    Type2, Value2 \
    ) \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2, \
                  Value1, Value2); \
    \
    __glmDOENTER()

#define glmENTER3( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3 \
    ) \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3, \
                  Value1, Value2, Value3); \
    \
    __glmDOENTER()

#define glmENTER4( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4 \
    ) \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4, \
                  Value1, Value2, Value3, Value4); \
    \
    __glmDOENTER()

#define glmENTER5( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5 \
    ) \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5, \
                  Value1, Value2, Value3, Value4, Value5); \
    \
    __glmDOENTER()

#define glmENTER6( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5, \
    Type6, Value6 \
    ) \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5 \
                  " " #Value6 "=" Type6, \
                  Value1, Value2, Value3, Value4, Value5, Value6); \
    \
    __glmDOENTER()

#define glmENTER7( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5, \
    Type6, Value6, \
    Type7, Value7 \
    ) \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5 \
                  " " #Value6 "=" Type6 \
                  " " #Value7 "=" Type7, \
                  Value1, Value2, Value3, Value4, Value5, Value6, Value7); \
    \
    __glmDOENTER()

#define glmENTER8( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5, \
    Type6, Value6, \
    Type7, Value7, \
    Type8, Value8 \
    ) \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
    \
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5 \
                  " " #Value6 "=" Type6 \
                  " " #Value7 "=" Type7 \
                  " " #Value8 "=" Type8, \
                  Value1, Value2, Value3, Value4, Value5, Value6, Value7, \
                  Value8); \
    \
    __glmDOENTER()

#define glmENTER9( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5, \
    Type6, Value6, \
    Type7, Value7, \
    Type8, Value8, \
    Type9, Value9 \
    ) \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
	\
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5 \
                  " " #Value6 "=" Type6 \
                  " " #Value7 "=" Type7 \
                  " " #Value8 "=" Type8 \
                  " " #Value9 "=" Type9, \
                  Value1, Value2, Value3, Value4, Value5, Value6, Value7, \
                  Value8, Value9); \
    \
    __glmDOENTER()

#define glmENTER10( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5, \
    Type6, Value6, \
    Type7, Value7, \
    Type8, Value8, \
    Type9, Value9, \
	Type10, Value10 \
    ) \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
	\
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5 \
                  " " #Value6 "=" Type6 \
                  " " #Value7 "=" Type7 \
                  " " #Value8 "=" Type8 \
                  " " #Value9 "=" Type9, \
				  " " #Value10 "=" Type10, \
                  Value1, Value2, Value3, Value4, Value5, Value6, Value7, \
                  Value8, Value9, Value10); \
    \
    __glmDOENTER()

#define glmENTER11( \
    Type1, Value1, \
    Type2, Value2, \
    Type3, Value3, \
    Type4, Value4, \
    Type5, Value5, \
    Type6, Value6, \
    Type7, Value7, \
    Type8, Value8, \
    Type9, Value9, \
	Type10, Value10, \
	Type11, Value11 \
    ) \
    GLContext context; \
	GLboolean _HasError = GL_FALSE; \
	glmDeclareTimeVar() \
	\
    gcmHEADER_ARG(#Value1 "=" Type1 \
                  " " #Value2 "=" Type2 \
                  " " #Value3 "=" Type3 \
                  " " #Value4 "=" Type4 \
                  " " #Value5 "=" Type5 \
                  " " #Value6 "=" Type6 \
                  " " #Value7 "=" Type7 \
                  " " #Value8 "=" Type8 \
                  " " #Value9 "=" Type9, \
				  " " #Value10 "=" Type10, \
				  " " #Value11 "=" Type11, \
                  Value1, Value2, Value3, Value4, Value5, Value6, Value7, \
                  Value8, Value9, Value10, Value11); \
    \
    __glmDOENTER()

#define glmLEAVE() \
    } \
    while (GL_FALSE); \
	glmGetApiEndTime(context); \
	if (_HasError) { \
	    gcmFOOTER_ARG("error=0x%04X", (context == gcvNULL) ? 0 : context->error); \
	} else { \
        gcmFOOTER_NO(); \
	} \


#define glmLEAVE1(Type, Value) \
    } \
    while (GL_FALSE); \
	glmGetApiEndTime(context); \
	if (_HasError) { \
	    gcmFOOTER_ARG("error=0x%04X", (context == gcvNULL) ? 0 : context->error); \
	} else { \
        gcmFOOTER_ARG(#Value "=" Type, Value); \
	} \

#define glmERROR_BREAK() \
    _HasError = GL_TRUE; break

typedef struct _GLObjectList
{
    GLuint          lastObject;
    GLObject        objects[gldNAMED_OBJECTS_HASH];
}
GLObjectList;

struct _GLBinding
{
    GLBinding       next;

    char *          name;

    GLint           index;
};

struct _GLAttribute
{
    GLboolean       enable;
    GLint           size;
    GLenum          type;
    GLboolean       normalized;
    GLsizei         stride;
    const void *    ptr;
    GLBuffer        buffer;
    GLsizeiptr      offset;
};

typedef enum _GLObjectType
{
    GLObject_Unknown,
    GLObject_VertexShader,
    GLObject_FragmentShader,
    GLObject_Program,
    GLObject_Texture,
    GLObject_Buffer,
    GLObject_Framebuffer,
    GLObject_Renderbuffer,
    GLObject_VertexArray,
}
GLObjectType;

struct _GLObject
{
    GLObject            next;
    GLObject            prev;

    GLuint              name;
    GLint               reference;

    GLObjectType        type;
};

struct _GLShader
{
    struct _GLObject    object;

    GLsizei             sourceSize;
    char *              source;

    GLboolean           compileStatus;
    char *              compileLog;
    gcSHADER            binary;
    GLboolean           dirty;

    GLint               usageCount;
    GLboolean           flagged;
};

struct _GLUniform
{
    gcUNIFORM           uniform[2];

    GLvoid *            data;

    GLboolean           dirty;

#if gldFBO_DATABASE
    GLsizei             bytes;
    GLboolean           dbDirty;
    GLvoid *            savedData;
#endif
};

typedef struct _GLLocation
{
    gcATTRIBUTE         attribute;
    GLint               index;
    GLboolean           assigned;
}
GLLocation;

#if GC_ENABLE_LOADTIME_OPT

enum gctLTCBoolValue {
    gctLTCBoolValue_false,
    gctLTCBoolValue_true,
    gctLTCBoolValue_any           /* any value, don't care, it happens when the
                                     nested loop is dead code with previous condition */
};

/* single precision floating point NaN, Infinity, etc. constant */
#define SINGLEFLOATPOSITIVENAN      0x7fc00000
#define SINGLEFLOATNEGTIVENAN       0xffc00000
#define SINGLEFLOATPOSITIVEINF      0x7f800000
#define SINGLEFLOATNEGTIVEINF       0xff800000
#define SINGLEFLOATPOSITIVEZERO     0x00000000
#define SINGLEFLOATNEGTIVEZERO      0x80000000

#define isF32PositiveNaN(f)  (*(gctUINT *)&(f) == SINGLEFLOATPOSITIVENAN)
#define isF32NegativeNaN(f)  (*(gctUINT *)&(f) == SINGLEFLOATNEGTIVENAN)
#define isF32NaN(f)  (isF32PositiveNaN(f) || isF32NegativeNaN(f))

#define FLOAT_NaN  SINGLEFLOATPOSITIVENAN

#define LTC_MAX_BRANCH_MULTIVERSION   2
#define LTC_MAX_ELEMENTS   4

typedef union {
    gctBOOL        b;
    gctUINT8       u8;
    gctUINT16      u16;
    gctUINT32      u32;
    gctUINT64      u64;
    gctINT8        i8;
    gctINT16       i16;
    gctINT32       i32;
    gctINT64       i64;
    gctFLOAT       f32;
} gctConstantValue;

typedef struct _LoadtimeConstantValue
{
    gcSL_ENABLE        enable;               /* the components enabled, for target value */
    gctUINT16          sourceInfo;           /* source type, indexed, format and swizzle info */
    gcSL_FORMAT        elementType;          /* the format of element */
    gctINT             instructionIndex;     /* the instruction index to the LTC expression in Shader */
    gctConstantValue   v[LTC_MAX_ELEMENTS];
} LTCValue;

typedef struct _gcsLTCUniformInfo
{
    gctINT              shaderKind;
    gctINT              uniformIndex;
} gcsLTCUniformInfo;

typedef struct _gcsCachedCode
{
    enum gctLTCBoolValue     value[LTC_MAX_BRANCH_MULTIVERSION];
    gctSIZE_T                statesSize;
    gctPOINTER               states;
    gcsHINT_PTR              hints;
    struct _gcsCachedCode *  next;   /* next cached code */
} gcsCachedCode;

#endif /* GC_ENABLE_LOADTIME_OPT */

struct _GLProgram
{
    struct _GLObject    object;

    GLShader            vertexShader;
    gcSHADER            vertexShaderBinary;

    GLShader            fragmentShader;
    gcSHADER            fragmentShaderBinary;

    GLboolean           linkStatus;
    GLboolean           validateStatus;
    char *              infoLog;

    gctSIZE_T           statesSize;
    gctPOINTER          states;
    gcsHINT_PTR         hints;

    GLboolean           flagged;

    GLboolean           attributeDirty;
    gctSIZE_T           attributeCount;
    gctSIZE_T           attributeMaxLength;
    gcATTRIBUTE *       attributePointers;
    GLBinding           attributeBinding;
    GLuint *            attributeLinkage;
    GLLocation *        attributeLocation;
    GLuint              attributeEnable;
    GLuint *            attributeMap;

    GLint               vertexCount;        /* vertex shader uniform count */
    GLsizei             uniformMaxLength;   /* max uniform name length */
    GLint               ltcUniformCount;    /* count of compiler created loadtime constant uniforms */
    GLint               uniformCount;       /* count of non private uniforms for all shaders */
    GLUniform           uniforms;
    GLint               privateUniformCount;/* count of private uniforms for all shaders */
    GLUniform           privateUniforms;

    GLuint              vertexSamplers;
    GLuint              fragmentSamplers;
    struct
    {
        gcSHADER_TYPE   type;
        GLuint          unit;
    }
    sampleMap[32];

#if gldFBO_DATABASE
    /* Mapped texture objects. */
    GLTexture           textures[32];
    GLTexture           savedTextures[32];

    /* Modification flag and linked list. */
    GLboolean           modified;
    GLProgram           nextModified;
#endif

#if VIVANTE_PROFILER
    gcSHADER_PROFILER   vertexShaderProfiler;
    gcSHADER_PROFILER   fragmentShaderProfiler;
#endif

    /* Special uniform handles. */
    GLUniform           depthRangeNear;
    GLUniform           depthRangeFar;
    GLUniform           depthRangeDiff;
    GLUniform           height;
#if GC_ENABLE_LOADTIME_OPT
    /* multi version code info */
    gctINT              multiversionCount;
    gctINT              LTCBranchCount;
    gcsLTCUniformInfo * LTCBranchUniforms;
    gcsCachedCode *     cachedCode;         /* a list of multiversion code */
#endif /* GC_ENABLE_LOADTIME_OPT */
};

/* For direct texture (YUV). */
struct _GLDirectTexture
{
    gctBOOL                 dirty;          /* Direct texture change flag. */
	gctBOOL					mipmapable;		/* Whether we can generate mipmap for it. */
    gcoSURF                 source;         /* Surface exposed to the user. */
    gcoSURF                 temp;           /* Temporary result surface. */
    gcoSURF                 texture[16];    /* Final texture mipmaps. */
	gctUINT					lodWidth, lodHeight;
	gctINT					maxLod;
    gceSURF_FORMAT textureFormat;
};

struct _GLTexture
{
    struct _GLObject    object;

    GLenum              target;

    gcoTEXTURE          texture;

    GLint               minFilter;
    GLint               magFilter;
    GLint               anisoFilter;
    GLint               wrapS;
    GLint               wrapT;
    GLint               wrapR;
    GLint               maxLevel;

    GLboolean           dirty;
    GLboolean           needFlush;
    GLboolean           fromImage;

    GLuint              renderingFb;

#if gldFBO_DATABASE
    GLFramebuffer       owner;

    GLboolean           modified;
    GLTexture           nextModified;

    /* Saved states. */
    GLenum              savedMinFilter;
    GLenum              savedMagFilter;
    GLenum              savedAnisoFilter;
    GLenum              savedWrapS;
    GLenum              savedWrapT;
    GLenum              savedWrapR;
    GLint               savedMaxLevel;
#endif

    gcsTEXTURE          states;

#if defined(ANDROID) && (ANDROID_SDK_VERSION >= 7)
    /* EGL_ANDROID_NATIVE_BUFFER extension. */
    gctPOINTER          privHandle;
    gcoSURF             source;
#endif

    gctINT              width;
    gctINT              height;

    struct _GLDirectTexture direct;
    GLboolean           dirtyCropRect;

#ifdef EGL_API_XXX
	GLint				seqNo;
#endif
};


#if gcdUSE_TRIANGLE_STRIP_PATCH
struct _GLSubBuffer
{
    gcoINDEX            patchIndex;
    GLint               start;
    GLint               count;
};
#endif

struct _GLBuffer
{
    struct _GLObject    object;

    GLenum              target;
    GLenum              usage;
    GLsizeiptr          size;

    gcoINDEX            index;
    gcoSTREAM           stream;

    gctBOOL             mapped;
    gctPOINTER          bufferMapPointer;

    GLuint              boundType;

#if gcdUSE_TRIANGLE_STRIP_PATCH
    struct _GLSubBuffer subs[gldINDEX_MAX_SUBS];
    GLint               subCount;
    gctBOOL             stripPatchDirty;
#endif
};

typedef struct _GLAttachment
{
    GLObject            object;
    gcoSURF             surface;
    gctUINT32           offset;
    gcoSURF             target;

    gctINT32            level;
}
GLAttachment;

struct _GLFramebuffer
{
    struct _GLObject    object;

    GLboolean           dirty;
    GLenum              completeness;
    GLboolean           needResolve;
    GLboolean           eglUsed;    /* Is it used by egl? */

    GLAttachment            color;
    GLAttachment            depth;
    GLAttachment            stencil;

#if gldFBO_DATABASE
    glsDATABASE_PTR         currentDB;
    glsDATABASE_PTR         listDB;

    glsDATABASE_PTR         freeDB[4];
    gctINT                  freeDBCount;

    glsDATABASE_RECORD_PTR  freeRecord[16];
    gctINT                  freeRecordCount;
#endif
};

struct _GLRenderbuffer
{
    struct _GLObject    object;

    GLFramebuffer       framebuffer;

    GLsizei             width, height;
    GLenum              format;

    gcoSURF             surface;

    GLRenderbuffer      combined;
    GLint               count;
    GLboolean           bound;
    GLboolean           eglUsed;    /* Is it used by egl? */
};

GLenum
_glshIsFramebufferComplete(
    GLContext Context
    );

GLContext
_glshGetCurrentContext(
    void
    );

void
_glshComputeBlobCacheKey(
        GLShader Shader,
        gctMD5_Byte* DigestMD5
    );

gceSTATUS
_glshBlobCacheGetShader(
    GLContext Context,
    GLShader Shader,
    gctMD5_Byte* BlobCacheKey
    );

void
_glshBlobCacheSetShader(
    GLShader Shader,
    gctMD5_Byte* BlobCacheKey
    );

GLContext
_glshCreateContext(
    void
    );

void
_glshDestroyContext(
    GLContext Context
    );

GLboolean
_glshInsertObject(
    GLObjectList * List,
    GLObject Object,
    GLObjectType Type,
    GLuint Name
    );

void
_glshRemoveObject(
    GLObjectList * List,
    GLObject Object
    );

GLObject
_glshFindObject(
    GLObjectList * List,
    GLuint Name
    );

void _glshReferenceObject(
    GLContext Context,
    GLObject Object
);

void _glshDereferenceObject(
    GLContext Context,
    GLObject Object
);


void
_glshDeleteShader(
    GLContext Context,
    GLShader Shader
    );

void
_glshDeleteProgram(
    GLContext Context,
    GLProgram Program
    );

void
_glshDeleteTexture(
    GLContext Context,
    GLTexture Texture
    );

void
_glshDeleteBuffer(
    GLContext Context,
    GLBuffer Buffer
    );

void
_glshDeleteFramebuffer(
    GLContext Context,
    GLFramebuffer Framebuffer
    );

void
_glshDeleteRenderbuffer(
    GLContext Context,
    GLRenderbuffer Renderbuffer
    );

void
_glshDeleteVertexArrayObject(
    GLContext Context,
    GLVertexArray array
    );

void
_glshDereferenceTexture(
    GLContext Context,
    GLTexture Texture
    );

void
_glshDereferenceRenderbuffer(
    GLContext Context,
    GLRenderbuffer Renderbuffer
    );

GLboolean
_glshCompileShader(
    IN GLContext Context,
    IN gctINT ShaderType,
    IN gctSIZE_T SourceSize,
    IN gctCONST_STRING Source,
    OUT gcSHADER * Binary,
    OUT gctSTRING * Log
    );

GLboolean
_glshLinkShaders(
    IN GLContext Context,
    IN OUT GLProgram Program
    );

GLboolean
_glshLinkProgramAttributes(
    IN GLContext Context,
    IN OUT GLProgram Program
    );

void
_glshInitializeBuffer(
    GLContext Context
    );

void
_glshInitializeClear(
    GLContext Context
    );

void
_glshDeinitializeClear(
    GLContext Context
    );

void
_glshInitializeFramebuffer(
    GLContext Context
    );

void
_glshInitializeRenderbuffer(
    GLContext Context
    );

void
_glshInitializeShader(
    GLContext Context
    );

void
_glshInitializeState(
    GLContext Context
    );

void
_glshInitializeTexture(
    GLContext Context
    );

void
_glshInitializeVertex(
    GLContext Context
    );

    void
_glshDeinitializeVertex(
    GLContext Context
    );

void
_glshInitializeDraw(
    GLContext Context
    );

void
_glshDeinitializeDraw(
    GLContext Context
    );

void
_glshInitializeObjectStates(
    GLContext Context
    );

typedef struct _GLVertex
{
    GLenum          type;
    GLsizei         stride;
    GLBuffer        buffer;
}
GLVertex;

struct _GLVertexArray
{
    struct _GLObject    object;

    GLenum              target;

    GLBuffer            elementArrayBuffer;

#if GL_USE_VERTEX_ARRAY
    gcsVERTEXARRAY      vertexArray[gldVERTEX_ELEMENT_COUNT];
    GLVertex            vertexArrayGL[gldVERTEX_ELEMENT_COUNT];
#else
    GLAttribute         vertexArray[gldVERTEX_ELEMENT_COUNT];
#endif
};

struct _GLContext
{
    /* HAL objects. */
    gcoOS               os;
    gcoHAL              hal;
    gco3D               engine;
	/* Used when migrating context to a different thread. */
	gcoHARDWARE		 	hw;

    gcoSURF             draw;
    gcoSURF             read;
    gcoSURF             depth;
    GLint               drawWidth;
    GLint               drawHeight;
    gceSURF_FORMAT      depthFormat;

    /* Resolve constaints in the hardware. */
    gctUINT             resolveMaskX;
    gctUINT             resolveMaskY;
    gctUINT             resolveMaskWidth;
    gctUINT             resolveMaskHeight;

    /* Anti-alias information. */
    gctUINT             samples;
    gctUINT             drawSamples;
    GLboolean           antiAlias;

    /* Frame buffer information. */
    GLint               width;
    GLint               height;

    /* Compiler. */
    gctHANDLE           dll;
    gceSTATUS           (*compiler)(IN gcoHAL Hal,
                                    IN gctINT ShaderType,
                                    IN gctSIZE_T SourceSize,
                                    IN gctCONST_STRING Source,
                                    OUT gcSHADER * Binary,
                                    OUT gctSTRING * Log);

    /* Last known error. */
    GLenum              error;

    /* Queries. */
    gctUINT             vertexSamplers, fragmentSamplers;
    gctUINT             maxWidth, maxHeight;
    GLuint              maxAttributes;
    gctUINT             vertexUniforms, fragmentUniforms, maxVaryings;
    gceCHIPMODEL        model;
    gctUINT32           revision;
    gctCHAR             renderer[16];
    gctSTRING           extensionString;
    gctUINT             maxAniso;
    gctUINT             maxTextureWidth, maxTextureHeight;

    /* Named objects. */
    GLObjectList        bufferObjects;
    GLObjectList        shaderObjects;
    GLObjectList        textureObjects;
    GLObjectList        framebufferObjects;
    GLObjectList        renderbufferObjects;
    GLObjectList        vertexArrayObjects;

    /* States. */
    GLboolean           viewDirty;
    GLint               viewportX, viewportY;
    GLuint              viewportWidth, viewportHeight;
    GLint               packAlignment, unpackAlignment;
    GLclampf            clearRed, clearGreen, clearBlue, clearAlpha;
    GLclampf            clearDepth;
    GLint               clearStencil;
#if gldUSE_3D_RENDERING_CLEAR
    GLboolean           clearShaderExist;
    GLuint              clearProgram;
    GLuint              clearVertShader;
    GLuint              clearFragShader;
#endif

    GLboolean           copyTexShaderExist;
    GLuint              copyTexProgram;
    GLuint              copyTexVertShader;
    GLuint              copyTexFragShader;
    GLuint              copyTexFBO;

    GLboolean           blendEnable;
    GLenum              blendFuncSourceRGB, blendFuncSourceAlpha;
    GLenum              blendFuncTargetRGB, blendFuncTargetAlpha;
    GLenum              blendModeRGB, blendModeAlpha;
    GLclampf            blendColorRed, blendColorGreen, blendColorBlue;
    GLclampf            blendColorAlpha;
    GLboolean           cullEnable;
    GLenum              cullMode;
    GLenum              cullFront;
    GLboolean           depthTest;
    GLenum              depthFunc;
    GLboolean           depthMask;
    GLclampf            depthNear, depthFar;
    GLint               textureUnit;
    GLboolean           ditherEnable;
    GLboolean           ditherDirty;
    GLfloat             lineWidth;
    GLfloat             lineWidthRange[2];
    GLboolean           depthDirty;
    GLboolean           textureFlushNeeded;

    GLboolean           scissorEnable;
    GLint               scissorX;
    GLint               scissorY;
    GLuint              scissorWidth;
    GLuint              scissorHeight;

    GLboolean           stencilEnable;
    GLuint              stencilRefFront;
    GLuint              stencilRefBack;
    GLenum              stencilFuncFront;
    GLenum              stencilFuncBack;
    GLuint              stencilMaskFront;
    GLuint              stencilMaskBack;
    GLenum              stencilOpFailFront;
    GLenum              stencilOpFailBack;
    GLenum              stencilOpDepthFailFront;
    GLenum              stencilOpDepthFailBack;
    GLenum              stencilOpDepthPassFront;
    GLenum              stencilOpDepthPassBack;
    GLuint              stencilWriteMask;
    gceSTENCIL_OPERATION frontZPass;
    gceSTENCIL_OPERATION frontZFail;
    gceSTENCIL_OPERATION frontFail;
    gceSTENCIL_OPERATION backZPass;
    gceSTENCIL_OPERATION backZFail;
    gceSTENCIL_OPERATION backFail;

    struct _GLTexture   default2D;
    GLTexture           texture2D[32];
    struct _GLTexture   defaultCube;
    GLTexture           textureCube[32];
    struct _GLTexture   default3D;
    GLTexture           texture3D[32];
    struct _GLTexture   defaultExternal;
    GLTexture           textureExternal[32];
    GLProgram           program;
    GLboolean           programDirty;
    GLenum              mipmapHint;
    GLenum              fragShaderDerivativeHint;
    gcePRIMITIVE        lastPrimitiveType;
    GLTexture           bounded[32];

    GLBuffer            arrayBuffer;
    GLBuffer            elementArrayBuffer;
    GLFramebuffer       framebuffer;
    GLboolean           framebufferChanged;
    GLboolean           hasTileStatus;
    GLRenderbuffer      renderbuffer;

#if GL_USE_VERTEX_ARRAY
    gcoVERTEXARRAY      vertex;
    gcsVERTEXARRAY      vertexArray[16];
    GLVertex            vertexArrayGL[16];
#else
    GLAttribute         vertexArray[gldVERTEX_ELEMENT_COUNT];
    GLfloat             genericVertex[gldVERTEX_ELEMENT_COUNT][4];
    GLsizei             genericVertexLength[gldVERTEX_ELEMENT_COUNT];

    /* Dynamic buffers. */
    gcoVERTEX           vertex;
    gcoSTREAM           dynamicArray;
    gcoINDEX            dynamicElement;
#endif

    GLVertexArray       vertexArrayObject;

    GLboolean           colorEnableRed;
    GLboolean           colorEnableGreen;
    GLboolean           colorEnableBlue;
    GLboolean           colorEnableAlpha;
    gctUINT8            colorWrite;

    GLboolean           offsetEnable;
    GLfloat             offsetFactor;
    GLfloat             offsetUnits;

    GLboolean           sampleAlphaToCoverage;
    GLboolean           sampleCoverage;
    GLfloat             sampleCoverageValue;
    GLboolean           sampleCoverageInvert;

#if defined(GC_USE_GL)
    /* glBegin/glEnd. */
    GLenum              immediatePrimitive;
    GLMutant4           currentVertex[2];
    GLMutant3           currentNormal;

    /* Display lists. */
    GLDispListRange     freeDispListHead;
    GLDispListRange     freeDispListTail;
#endif

    /* Stencil bug. */
    GLboolean           hasCorrectStencil;

#if VIVANTE_PROFILER
    /* Profiler. */
    glsPROFILER         profiler;
#endif

    /* Batch optimization. */
    GLboolean           batchDirty;
    GLenum              batchMode;
    GLint               batchFirst;
    GLint               batchCount;
    GLenum              batchIndexType;
    const GLvoid *      batchPointer;
    GLbitfield          batchEnable;
    GLbitfield          batchArray;
    GLint               batchSkip;

    gceCHIPMODEL        chipModel;
    gctUINT32           chipRevision;
    gctBOOL             patchStrip;
#if gldFBO_DATABASE
    gctBOOL             playing;
    gctBOOL             dbEnable;
    gctINT              dbCounter;
#endif
};

GLboolean
_glshLoadCompiler(
    IN GLContext Context
    );

void
_glshReleaseCompiler(
    IN GLContext Context
    );

gceCOMPARE
_glshTranslateFunc(
    GLenum Func
    );

gctCONST_STRING
_glshGetEnum(
    GLenum Enum
    );

gcoSURF
_glshGetFramebufferSurface(
    GLAttachment * Attachment
    );

void
_glshInitializeProfiler(
    GLContext Context
    );

void
_glshDestroyProfiler(
    GLContext Context
    );

GLboolean
_glshFrameBuffer(
    IN GLContext Context
    );

void
_glshFlush(
    IN GLContext Context,
    IN GLboolean Stall
    );


GLboolean
_glshCalculateArea(
    GLint *pdx, GLint *pdy,
    GLint *psx, GLint *psy,
    GLint *pw, GLint *ph,
    GLint dstW, GLint dstH,
    GLint srcW, GLint srcH
    );

GLenum
_SetUniformData(
    GLProgram        Program,
    GLUniform        Uniform,
    gcSHADER_TYPE    Type,
    GLsizei          Count,
    GLint            Index,
    const void *     Values
    );

#if GC_ENABLE_LOADTIME_OPT
gceSTATUS
gcComputeLoadtimeConstant(
    IN GLContext Context
    );

gceSTATUS
gcEvaluateLoadtimeConstantExpresions(
    IN GLContext                  Context,
    IN gcSHADER                   Shader
    );

GLboolean
gcIsUniformALoadtimeConstant(
    IN GLUniform   Uniform
                     );
#endif  /* GC_ENABLE_LOADTIME_OPT */


GLboolean
glshSetProgram(
    GLContext Context,
     GLProgram Program
     );

void
glshSetTexParameter(
    GLContext Context,
    GLTexture Texture,
    GLenum Target,
    GLenum Parameter,
    GLint Value
    );

void
glshProgramDither(
    IN GLContext Context
    );

#endif /* __libgl_h_ */
