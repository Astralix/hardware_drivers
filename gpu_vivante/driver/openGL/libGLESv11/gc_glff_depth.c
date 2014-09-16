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

#define _GC_OBJ_ZONE glvZONE_TRACE

/******************************************************************************\
********************************* GL Name Arrays *******************************
\******************************************************************************/

static GLenum _TestNames[] =
{
    GL_NEVER,           /* glvNEVER */
    GL_LESS,            /* glvLESS */
    GL_EQUAL,           /* glvEQUAL */
    GL_LEQUAL,          /* glvLESSOREQUAL */
    GL_GREATER,         /* glvGREATER */
    GL_NOTEQUAL,        /* glvNOTEQUAL */
    GL_GEQUAL,          /* glvGREATEROREQUAL */
    GL_ALWAYS,          /* glvALWAYS */
};

static GLenum _StencilOperationNames[] =
{
    GL_ZERO,            /* glvSTENCILZERO */
    GL_KEEP,            /* glvSTENCILKEEP */
    GL_REPLACE,         /* glvSTENCILREPLACE */
    GL_INCR,            /* glvSTENCILINC */
    GL_DECR,            /* glvSTENCILDEC */
    GL_INVERT,          /* glvSTENCILINVERT */
};


/******************************************************************************\
********************** Individual State Setting Functions **********************
\******************************************************************************/

static GLenum _UpdateDepthFunction(
    glsCONTEXT_PTR Context
    )
{
    GLenum result;
    static gceCOMPARE depthTestValues[] =
    {
        gcvCOMPARE_NEVER,               /* glvNEVER */
        gcvCOMPARE_LESS,                /* glvLESS */
        gcvCOMPARE_EQUAL,               /* glvEQUAL */
        gcvCOMPARE_LESS_OR_EQUAL,       /* glvLESSOREQUAL */
        gcvCOMPARE_GREATER,             /* glvGREATER */
        gcvCOMPARE_NOT_EQUAL,           /* glvNOTEQUAL */
        gcvCOMPARE_GREATER_OR_EQUAL,    /* glvGREATEROREQUAL */
        gcvCOMPARE_ALWAYS,              /* glvALWAYS */
    };

    /* Determine the depth function. */
    gceCOMPARE testFunction
        = Context->depthStates.testEnabled
            ? depthTestValues[Context->depthStates.testFunction]
            : gcvCOMPARE_ALWAYS;

    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set the depth function. */
    result = glmTRANSLATEHALSTATUS(gco3D_SetDepthCompare(
        Context->hw,
        testFunction
        ));

    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}

static GLenum _UpdateDepthEnable(
    glsCONTEXT_PTR Context
    )
{
    GLenum result;
    gceDEPTH_MODE depthMode;

    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        /* Determine the depth mode. */
        depthMode
            = (Context->depthStates.testEnabled
                || Context->stencilStates.testEnabled)
              && (Context->depth != gcvNULL)
                ? gcvDEPTH_Z
                : gcvDEPTH_NONE;

        if (depthMode != Context->depthStates.depthMode)
        {
            Context->depthStates.depthMode = depthMode;
            Context->depthStates.polygonOffsetDirty = gcvTRUE;
        }

        /* Set the depth function. */
        glmERR_BREAK(_UpdateDepthFunction(Context));

        /* Set the depth mode. */
        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetDepthMode(
            Context->hw,
            Context->depthStates.depthMode
            )));
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}

gceSTATUS glfUpdatePolygonOffset(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS result = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=0x%x", Context);

    if (Context->depthStates.polygonOffsetDirty)
    {
        gctBOOL depthBiasEnabled = (Context->depthStates.polygonOffsetFill
                                 && Context->depthStates.depthMode != gcvDEPTH_NONE);

        gltFRACTYPE depthScale
            = glfFracFromMutant(&Context->depthStates.depthFactor);

        gltFRACTYPE depthBias
            = glfFracFromMutant(&Context->depthStates.depthUnits);

        /* Convert depth bias to float range. */
        depthBias = glmFRACMULTIPLY(depthBias, glvFRACONEOVER65535);

        if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_DEPTH_BIAS_FIX)
            != gcvSTATUS_TRUE)
        {
            /* Use alternate path to program depth bias. */
            Context->hashKey.hashDepthBiasFixEnabled = depthBiasEnabled;
            Context->hashKey.hashDepthBias           = glmFRAC2FLOAT(depthBias)
                                                     + glmFRAC2FLOAT(depthScale);

            /* Disable the other path. */
            result = gco3D_SetDepthScaleBias(
                Context->hw,
                glvFRACZERO,
                glvFRACZERO
                );
        }
        else
        {
            if (depthBiasEnabled)
            {
                result = gco3D_SetDepthScaleBias(
                    Context->hw,
                    depthScale,
                    depthBias
                    );
            }
            else
            {
                result = gco3D_SetDepthScaleBias(
                    Context->hw,
                    glvFRACZERO,
                    glvFRACZERO
                    );
            }
        }
    }

    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}

static GLenum _SetDepthCompareFunction(
    glsCONTEXT_PTR Context,
    GLenum Function
    )
{
    GLenum result;

    gcmHEADER_ARG("Context=0x%x Function=0x%04x", Context, Function);

    do
    {
        GLuint function;

        if (!glfConvertGLEnum(
                _TestNames,
                gcmCOUNTOF(_TestNames),
                &Function, glvINT,
                &function
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

        /* Set new function. */
        Context->depthStates.testFunction = (gleTESTFUNCTIONS) function;

        /* Set the depth function. */
        glmERR_BREAK(_UpdateDepthFunction(Context));
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}

static GLenum _SetDepthMask(
    glsCONTEXT_PTR Context,
    GLboolean DepthMask
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x DepthMask=%d", Context, DepthMask);

    Context->depthStates.depthMask = DepthMask;

    result = glmTRANSLATEHALSTATUS(gco3D_EnableDepthWrite(
        Context->hw,
        DepthMask
        ));

    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}

static GLenum _SetClearDepth(
    glsCONTEXT_PTR Context,
    GLvoid* ClearValue,
    gleTYPE Type
    )
{
    gltFRACTYPE clearValue;
    GLenum result;

    gcmHEADER_ARG("Context=0x%x ClearValue=0x%x Type=0x%04x", Context, ClearValue, Type);

    /* Set with [0..1] clamping. */
    glfSetClampedMutant(&Context->depthStates.clearValue, ClearValue, Type);

    /* Query the value back. */
    clearValue = glfFracFromMutant(&Context->depthStates.clearValue);

    /* Set the clear value. */
    result = glmTRANSLATEHALSTATUS(gco3D_SetClearDepth(Context->hw, clearValue));

    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}

static GLenum _SetPolygonOffset(
    glsCONTEXT_PTR Context,
    GLvoid* Factor,
    GLvoid* Units,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Context=0x%x Factor=0x%x Units=0x%x Type=0x%04x", Context, Factor, Units, Type);

    glfSetMutant(&Context->depthStates.depthFactor, Factor, Type);
    glfSetMutant(&Context->depthStates.depthUnits,  Units,  Type);

    Context->depthStates.polygonOffsetDirty = gcvTRUE;

    gcmFOOTER_ARG("return=0x%04x", GL_NO_ERROR);
    return GL_NO_ERROR;
}

static GLenum _SetDepthRange(
    glsCONTEXT_PTR Context,
    GLvoid* zNear,
    GLvoid* zFar,
    gleTYPE Type
    )
{
    gltFRACTYPE znear;
    gltFRACTYPE zfar;
    GLenum result;

    gcmHEADER_ARG("Context=0x%x zNear=0x%x zFar=0x%x Type=0x%04x", Context, zNear, zFar, Type);

    /* Set with [0..1] clamping. */
    glfSetClampedMutant(&Context->depthStates.depthRange[0], zNear, Type);
    glfSetClampedMutant(&Context->depthStates.depthRange[1], zFar,  Type);

    /* Query back the range values. */
    znear = glfFracFromMutant(&Context->depthStates.depthRange[0]);
    zfar  = glfFracFromMutant(&Context->depthStates.depthRange[1]);

    /* Set the states. */
    result = glmTRANSLATEHALSTATUS(gco3D_SetDepthRange(
        Context->hw,
        Context->depthStates.depthMode,
        znear,
        zfar
        ));

    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}

static GLuint _GetStencilBits(
    glsCONTEXT_PTR Context
    )
{
    gceSURF_FORMAT format = gcvSURF_UNKNOWN;
    GLuint result = 0;
    gcmHEADER_ARG("Context=0x%x", Context);

    if (Context->depth == gcvNULL)
    {
        gcmFOOTER_ARG("return=%d", result);
        return result;
    }

    gcmVERIFY_OK(gcoSURF_GetFormat(Context->depth, gcvNULL, &format));

    result = (format == gcvSURF_D24S8)
        ? 8
        : 0;
    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}

static GLenum _SetStencilCompareFunction(
    glsCONTEXT_PTR Context,
    GLenum Function,
    GLint Reference,
    GLuint Mask
    )
{
    GLenum result;

    static gceCOMPARE stencilTestValues[] =
    {
        gcvCOMPARE_NEVER,               /* glvNEVER */
        gcvCOMPARE_LESS,                /* glvLESS */
        gcvCOMPARE_EQUAL,               /* glvEQUAL */
        gcvCOMPARE_LESS_OR_EQUAL,       /* glvLESSOREQUAL */
        gcvCOMPARE_GREATER,             /* glvGREATER */
        gcvCOMPARE_NOT_EQUAL,           /* glvNOTEQUAL */
        gcvCOMPARE_GREATER_OR_EQUAL,    /* glvGREATEROREQUAL */
        gcvCOMPARE_ALWAYS,              /* glvALWAYS */
    };

    gcmHEADER_ARG("Context=0x%x Function=0x%04x Reference=%d Mask=%u", Context, Function, Reference, Mask);

    do
    {
        GLuint function;

        if (!glfConvertGLEnum(
                _TestNames,
                gcmCOUNTOF(_TestNames),
                &Function, glvINT,
                &function
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

        if (_GetStencilBits(Context) == 0)
        {
            function = glvALWAYS;
        }

        Context->stencilStates.testFunction = (gleTESTFUNCTIONS) function;
        Context->stencilStates.reference = Reference;
        Context->stencilStates.valueMask = Mask;

        Context->stencilStates.hal.compareFront =
        Context->stencilStates.hal.compareBack
            = stencilTestValues[function];

        Context->stencilStates.hal.referenceFront =
        Context->stencilStates.hal.referenceBack
            = (gctUINT8) Reference;

        Context->stencilStates.hal.mask = (gctUINT8)(Mask & 0x00FF);

        Context->stencilStates.dirty = GL_TRUE;

        result = GL_NO_ERROR;
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}

static void _SetWriteMask(
    glsCONTEXT_PTR Context,
    GLuint Mask
    )
{
    Context->stencilStates.writeMask = Mask;

    Context->stencilStates.hal.writeMask = (gctUINT8)(Mask & 0x00FF);
    Context->stencilStates.dirty         = GL_TRUE;
}

static GLenum _SetStencilOperations(
    glsCONTEXT_PTR Context,
    GLenum Fail,
    GLenum ZFail,
    GLenum ZPass
    )
{
    GLenum result;

    static gceSTENCIL_OPERATION stencilOperationValues[] =
    {
        gcvSTENCIL_ZERO,                /* glvSTENCILZERO */
        gcvSTENCIL_KEEP,                /* glvSTENCILKEEP */
        gcvSTENCIL_REPLACE,             /* glvSTENCILREPLACE */
        gcvSTENCIL_INCREMENT_SATURATE,  /* glvSTENCILINC */
        gcvSTENCIL_DECREMENT_SATURATE,  /* glvSTENCILDEC */
        gcvSTENCIL_INVERT,              /* glvSTENCILINVERT */
    };

    gcmHEADER_ARG("Context=0x%x Fail=0x%04x ZFail=0x%04x ZPass=0x%04x", Context, Fail, ZFail, ZPass);

    do
    {
        GLuint fail;
        GLuint zFail;
        GLuint zPass;

        if (!glfConvertGLEnum(
                _StencilOperationNames,
                gcmCOUNTOF(_StencilOperationNames),
                &Fail, glvINT,
                &fail
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

        if (!glfConvertGLEnum(
                _StencilOperationNames,
                gcmCOUNTOF(_StencilOperationNames),
                &ZFail, glvINT,
                &zFail
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

        if (!glfConvertGLEnum(
                _StencilOperationNames,
                gcmCOUNTOF(_StencilOperationNames),
                &ZPass, glvINT,
                &zPass
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

        Context->stencilStates.fail  = (gleSTENCILOPERATIONS) fail;
        Context->stencilStates.zFail = (gleSTENCILOPERATIONS) zFail;
        Context->stencilStates.zPass = (gleSTENCILOPERATIONS) zPass;

        Context->stencilStates.hal.passFront =
        Context->stencilStates.hal.passBack
            = stencilOperationValues[zPass];
        Context->stencilStates.hal.failFront =
        Context->stencilStates.hal.failBack
            = stencilOperationValues[fail];
        Context->stencilStates.hal.depthFailFront =
        Context->stencilStates.hal.depthFailBack
            = stencilOperationValues[zFail];

        Context->stencilStates.dirty = GL_TRUE;

        result = GL_NO_ERROR;
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}

static GLenum _SetClearStencil(
    glsCONTEXT_PTR Context,
    GLint ClearValue
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x ClearValue=%d", Context, ClearValue);
    Context->stencilStates.clearValue = ClearValue;
    result = glmTRANSLATEHALSTATUS(gco3D_SetClearStencil(
        Context->hw,
        ClearValue
        ));
    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}


/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  glfSetDefaultDepthStates
**
**  Set default depth states.
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

gceSTATUS glfSetDefaultDepthStates(
    glsCONTEXT_PTR Context
    )
{
    GLenum result;
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        static gltFRACTYPE value0 = glvFRACZERO;
        static gltFRACTYPE value1 = glvFRACONE;

        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetDepthOnly(
            Context->hw,
            gcvFALSE
            )));

        /* Turn on early-depth. */
        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetEarlyDepth(
            Context->hw,
            gcvTRUE
            )));

        /* Disable depth test. */
        glmERR_BREAK(glfEnableDepthTest(
            Context,
            GL_FALSE
            ));

        /* Set default compare function. */
        glmERR_BREAK(_SetDepthCompareFunction(
            Context,
            GL_LESS
            ));

        /* Enable depth write. */
        glmERR_BREAK(_SetDepthMask(
            Context,
            GL_TRUE
            ));

        /* Set default clear value. */
        glmERR_BREAK(_SetClearDepth(
            Context,
            &value1,
            glvFRACTYPEENUM
            ));

        /* Disable poligon offset fill. */
        glmERR_BREAK(glfEnablePolygonOffsetFill(
            Context,
            GL_FALSE
            ));

        /* Set default poligon offset. */
        glmERR_BREAK(_SetPolygonOffset(
            Context,
            &value0,
            &value0,
            glvFRACTYPEENUM
            ));

        /* Set default range. */
        glmERR_BREAK(_SetDepthRange(
            Context,
            &value0,
            &value1,
            glvFRACTYPEENUM
            ));

        /* Disable stencil test. */
        glmERR_BREAK(glfEnableStencilTest(
            Context,
            GL_FALSE
            ));

        glmERR_BREAK(_SetClearStencil(
            Context,
            0
            ));

        glmERR_BREAK(_SetStencilCompareFunction(
            Context,
            GL_ALWAYS,      /* Function. */
             0,             /* Reference. */
            ~0U             /* Mask. */
            ));

        /* Write mask. */
        _SetWriteMask(Context, ~0U);

        glmERR_BREAK(_SetStencilOperations(
            Context,
            GL_KEEP,        /* Fail operation. */
            GL_KEEP,        /* Z fail operation. */
            GL_KEEP         /* Z pass operation. */
            ));

        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetStencilCompare(
            Context->hw,
            gcvSTENCIL_BACK,
            gcvCOMPARE_NEVER
            )));

        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetStencilFail(
            Context->hw,
            gcvSTENCIL_BACK,
            gcvSTENCIL_KEEP
            )));

        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetStencilDepthFail(
            Context->hw,
            gcvSTENCIL_BACK,
            gcvSTENCIL_KEEP
            )));

        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetStencilPass(
            Context->hw,
            gcvSTENCIL_BACK,
            gcvSTENCIL_KEEP
            )));
    }
    while (gcvFALSE);

    status = glmTRANSLATEGLRESULT(result);
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  glfEnableDepthTest
**
**  Enable depth test.
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
#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_STATES

GLenum glfEnableDepthTest(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);
    Context->depthStates.testEnabled = Enable;
    result = _UpdateDepthEnable(Context);
    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}


/*******************************************************************************
**
**  glfEnablePolygonOffsetFill
**
**  Enable poligon offset fill.
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

GLenum glfEnablePolygonOffsetFill(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);

    Context->depthStates.polygonOffsetFill = Enable;
    Context->depthStates.polygonOffsetDirty = gcvTRUE;

    gcmFOOTER_ARG("return=0x%04x", GL_NO_ERROR);

    return GL_NO_ERROR;
}


/*******************************************************************************
**
**  glfEnableStencilTest
**
**  Enable stencil test.
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

GLenum glfEnableStencilTest(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    gceSTENCIL_MODE mode;

    gcmHEADER();

    /* Determine the new stencil mode. */
    mode = Enable
        ? gcvSTENCIL_SINGLE_SIDED
        : gcvSTENCIL_NONE;

    /* Update the local state. */
    Context->stencilStates.testEnabled = Enable;

    /* Set the hardware state. */
    Context->stencilStates.hal.mode = mode;
    Context->stencilStates.dirty    = GL_TRUE;

    gcmFOOTER_NO();
    return GL_NO_ERROR;
}


/*******************************************************************************
**
**  glfQueryDepthState
**
**  Queries depth state values.
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

GLboolean glfQueryDepthState(
    glsCONTEXT_PTR Context,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result = GL_TRUE;

    gcmHEADER_ARG("Context=0x%x Name=0x%04x Value=0x%x Type=0x04x", Context, Name, Value, Type);

    switch (Name)
    {
    case GL_DEPTH_CLEAR_VALUE:
        if (Type == glvINT)
        {
            /* Normalize when converting to INT. */
            Type = glvNORM;
        }

        glfGetFromMutant(
            &Context->depthStates.clearValue,
            Value,
            Type
            );
        break;

    case GL_DEPTH_FUNC:
        glfGetFromEnum(
            _TestNames[Context->depthStates.testFunction],
            Value,
            Type
            );
        break;

    case GL_DEPTH_RANGE:
        glfGetFromMutantArray(
            Context->depthStates.depthRange,
            gcmCOUNTOF(Context->depthStates.depthRange),
            Value,
            Type
            );
        break;

    case GL_DEPTH_TEST:
        glfGetFromInt(
            Context->depthStates.testEnabled,
            Value,
            Type
            );
        break;

    case GL_DEPTH_WRITEMASK:
        glfGetFromInt(
            Context->depthStates.depthMask,
            Value,
            Type
            );
        break;

    case GL_DEPTH_BITS:
        {
            gceSURF_FORMAT format;
            GLuint depthBits;

            if (Context->depth == gcvNULL)
            {
                format = gcvSURF_UNKNOWN;
            }
            else
            {
                gcmVERIFY_OK(gcoSURF_GetFormat(
                    Context->depth,
                    gcvNULL,
                    &format
                    ));
            }

            if ((format == gcvSURF_D24S8)
            ||  (format == gcvSURF_D24X8)
            )
            {
                depthBits = 24;
            }
            else if (format == gcvSURF_D16)
            {
                depthBits = 16;
            }
            else
            {
                depthBits = 0;
            }

            glfGetFromInt(
                depthBits,
                Value,
                Type
                );
        }
        break;

    case GL_POLYGON_OFFSET_FACTOR:
        glfGetFromMutant(
            &Context->depthStates.depthFactor,
            Value,
            Type
            );
        break;

    case GL_POLYGON_OFFSET_UNITS:
        glfGetFromMutant(
            &Context->depthStates.depthUnits,
            Value,
            Type
            );
        break;

    case GL_POLYGON_OFFSET_FILL:
        glfGetFromInt(
            Context->depthStates.polygonOffsetFill,
            Value,
            Type
            );
        break;

    case GL_STENCIL_BITS:
        glfGetFromInt(
            _GetStencilBits(Context),
            Value,
            Type
            );
        break;

    case GL_STENCIL_CLEAR_VALUE:
        glfGetFromInt(
            Context->stencilStates.clearValue,
            Value,
            Type
            );
        break;

    case GL_STENCIL_FAIL:
        glfGetFromEnum(
            _StencilOperationNames[Context->stencilStates.fail],
            Value,
            Type
            );
        break;

    case GL_STENCIL_FUNC:
        glfGetFromEnum(
            _TestNames[Context->stencilStates.testFunction],
            Value,
            Type
            );
        break;

    case GL_STENCIL_PASS_DEPTH_FAIL:
        glfGetFromEnum(
            _StencilOperationNames[Context->stencilStates.zFail],
            Value,
            Type
            );
        break;

    case GL_STENCIL_PASS_DEPTH_PASS:
        glfGetFromEnum(
            _StencilOperationNames[Context->stencilStates.zPass],
            Value,
            Type
            );
        break;

    case GL_STENCIL_REF:
        glfGetFromInt(
            Context->stencilStates.reference,
            Value,
            Type
            );
        break;

    case GL_STENCIL_TEST:
        glfGetFromInt(
            Context->stencilStates.testEnabled,
            Value,
            Type
            );
        break;

    case GL_STENCIL_VALUE_MASK:
        glfGetFromInt(
            Context->stencilStates.valueMask,
            Value,
            Type
            );
        break;

    case GL_STENCIL_WRITEMASK:
        glfGetFromInt(
            Context->stencilStates.writeMask,
            Value,
            Type
            );
        break;

    default:
        result = GL_FALSE;
    }

    /* Return result. */
    gcmFOOTER_ARG("return=%d", result);
    return result;
}

gceSTATUS
glfUpdateStencil(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

    /* Only process if stencil is dirty. */
    if (Context->stencilStates.dirty)
    {
        do
        {
            if (!Context->stencilStates.testEnabled)
            {
                gcsSTENCIL_INFO faked = Context->stencilStates.hal;

                faked.mode           = gcvSTENCIL_SINGLE_SIDED;
                faked.compareFront   = gcvCOMPARE_ALWAYS;
                faked.compareBack    = gcvCOMPARE_ALWAYS;
                faked.passFront      = gcvSTENCIL_KEEP;
                faked.passBack       = gcvSTENCIL_KEEP;
                faked.depthFailFront = gcvSTENCIL_KEEP;
                faked.depthFailBack  = gcvSTENCIL_KEEP;

                /* Fake for z compression with fast clear. */
                gcmERR_BREAK(gco3D_SetStencilAll(Context->hw,
                                                 &faked));
            }
            else
            {
                /* Set all stencil states to HAL. */
                gcmERR_BREAK(gco3D_SetStencilAll(Context->hw,
                                                 &Context->stencilStates.hal));
            }

            status = _UpdateDepthEnable(Context) == GL_NO_ERROR ?
                gcvSTATUS_OK :
                gcvSTATUS_INVALID_ARGUMENT;

            /* Mark stencil as no clean. */
            Context->stencilStates.dirty = gcvFALSE;
        }
        while (gcvFALSE);
    }

    gcmFOOTER();
    return status;
}

/******************************************************************************\
******************** Depth and Stencil State Management Code *******************
\******************************************************************************/

/*******************************************************************************
**
**  glDepthRange
**
**  After clipping and division by w, depth coordinates range from -1 to 1,
**  corresponding to the near and far clipping planes. glDepthRange specifies
**  a linear mapping of the normalized depth coordinates in this range to window
**  depth coordinates. Regardless of the actual depth buffer implementation,
**  window coordinate depth values are treated as though they range from
**  0 through 1 (like color components). Thus, the values accepted by
**  glDepthRange are both clamped to this range before they are accepted.
**
**  The setting of (0, 1) maps the near plane to 0 and the far plane to 1.
**  With this mapping, the depth buffer range is fully utilized.
**
**  It is not necessary that near be less than far. Reverse mappings such as
**  near = 1, and far = 0 are acceptable.
**
**  INPUT:
**
**      zNear
**          Specifies the mapping of the near clipping plane to window
**          coordinates. The initial value is 0.
**
**      zFar
**          Specifies the mapping of the far clipping plane to window
**          coordinates. The initial value is 1.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_STATES

#if !defined(COMMON_LITE)
GL_API void GL_APIENTRY glDepthRangef(
    GLclampf zNear,
    GLclampf zFar
    )
{
    glmENTER2(glmARGFLOAT, zNear, glmARGFLOAT, zFar)
    {
		gcmDUMP_API("${ES11 glDepthRangef 0x%08X 0x%08x}", *(GLuint*) &zNear, *(GLuint*) &zFar);

		glmPROFILE(context, GLES1_DEPTHRANGEF, 0);
        glmERROR(_SetDepthRange(context, &zNear, &zFar, glvFLOAT));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glDepthRangefOES(
    GLclampf zNear,
    GLclampf zFar
    )
{
    glmENTER2(glmARGFLOAT, zNear, glmARGFLOAT, zFar)
    {
		gcmDUMP_API("${ES11 glDepthRangefOES 0x%08X 0x%08x}", *(GLuint*) &zNear, *(GLuint*) &zFar);

		glmPROFILE(context, GLES1_DEPTHRANGEF, 0);
        glmERROR(_SetDepthRange(context, &zNear, &zFar, glvFLOAT));
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glDepthRangex(
    GLclampx zNear,
    GLclampx zFar
    )
{
    glmENTER2(glmARGFIXED, zNear, glmARGFIXED, zFar)
    {
		gcmDUMP_API("${ES11 glDepthRangex 0x%08X 0x%08x}", zNear, zFar);

		glmPROFILE(context, GLES1_DEPTHRANGEX, 0);
        glmERROR(_SetDepthRange(context, &zNear, &zFar, glvFIXED));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glDepthRangexOES(
    GLclampx zNear,
    GLclampx zFar
    )
{
    glmENTER2(glmARGFIXED, zNear, glmARGFIXED, zFar)
    {
		gcmDUMP_API("${ES11 glDepthRangexOES 0x%08X 0x%08x}", zNear, zFar);

		glmPROFILE(context, GLES1_DEPTHRANGEX, 0);
        glmERROR(_SetDepthRange(context, &zNear, &zFar, glvFIXED));
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glDepthFunc
**
**  glDepthFunc specifies the function used to compare each incoming pixel depth
**  value with the depth value present in the depth buffer. The comparison is
**  performed only if depth testing is enabled. To enable and disable depth
**  testing, call glEnable and glDisable with argument GL_DEPTH_TEST.
**  Depth testing is initially disabled.
**
**  INPUT:
**
**      Function
**          Specifies the depth comparison function. Symbolic constants
**          GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL,
**          GL_GEQUAL, and GL_ALWAYS are accepted.
**          The initial value is GL_LESS.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_FRAGMENT

GL_API void GL_APIENTRY glDepthFunc(
    GLenum Function
    )
{
    glmENTER1(glmARGENUM, Function)
    {
		gcmDUMP_API("${ES11 glDepthFunc 0x%08X}", Function);

		glmPROFILE(context, GLES1_DEPTHFUNC, 0);
        glmERROR(_SetDepthCompareFunction(context, Function));
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glDepthMask
**
**  glDepthMask specifies whether the depth buffer is enabled for writing.
**  If flag is GL_FALSE, depth buffer writing is disabled. Otherwise, it is
**  enabled. Initially, depth buffer writing is enabled.
**
**  INPUT:
**
**      DepthMask
**          Specifies whether the depth buffer is enabled for writing.
**          If flag is GL_FALSE, depth buffer writing is disabled, otherwise
**          it is enabled. The initial value is GL_TRUE.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glDepthMask(
    GLboolean DepthMask
    )
{
    glmENTER1(glmARGUINT, DepthMask)
    {
		gcmDUMP_API("${ES11 glDepthMask 0x%08X}", DepthMask);

		glmPROFILE(context, GLES1_DEPTHMASK, 0);
        glmERROR(_SetDepthMask(context, DepthMask));
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glClearDepth
**
**  glClearDepth specifies the depth value used by glClear to clear the depth
**  buffer. Values specified by glClearDepth are clamped to the range [0, 1].
**
**  INPUT:
**
**      ClearValue
**          Specifies the depth value used when the depth buffer is cleared.
**          The initial value is 1.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_CLEAR

GL_API void GL_APIENTRY glClearDepthx(
    GLclampx ClearValue
    )
{
    glmENTER1(glmARGFIXED, ClearValue)
    {
		gcmDUMP_API("${ES11 glClearDepthx 0x%08X}", ClearValue);

		glmPROFILE(context, GLES1_CLEARDEPTHX, 0);
        glmERROR(_SetClearDepth(context, &ClearValue, glvFIXED));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glClearDepthxOES(
    GLclampx ClearValue
    )
{
    glmENTER1(glmARGFIXED, ClearValue)
    {
		gcmDUMP_API("${ES11 glClearDepthxOES 0x%08X}", ClearValue);

		glmPROFILE(context, GLES1_CLEARDEPTHX, 0);
        glmERROR(_SetClearDepth(context, &ClearValue, glvFIXED));
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glClearDepthf(
    GLclampf ClearValue
    )
{
    glmENTER1(glmARGFLOAT, ClearValue)
    {
		gcmDUMP_API("${ES11 glClearDepthf 0x%08X}", *(GLuint*) &ClearValue);

		glmPROFILE(context, GLES1_CLEARDEPTHF, 0);
        glmERROR(_SetClearDepth(context, &ClearValue, glvFLOAT));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glClearDepthfOES(
    GLclampf ClearValue
    )
{
    glmENTER1(glmARGFLOAT, ClearValue)
    {
		gcmDUMP_API("${ES11 glClearDepthfOES 0x%08X}", ClearValue);

		glmPROFILE(context, GLES1_CLEARDEPTHF, 0);
        glmERROR(_SetClearDepth(context, &ClearValue, glvFLOAT));
    }
    glmLEAVE();
}
#endif


/*******************************************************************************
**
**  glPolygonOffset
**
**  When GL_POLYGON_OFFSET_FILL is enabled, each fragment's depth value will
**  be offset after it is interpolated from the depth values of the appropriate
**  vertices. The value of the offset is m * factor + r * units, where m is
**  a measurement of the change in depth relative to the screen area of the
**  polygon, and r is the smallest value that is guaranteed to produce
**  a resolvable offset for a given implementation. The offset is added before
**  the depth test is performed and before the value is written into the depth
**  buffer.
**
**  glPolygonOffset is useful for for applying decals to surfaces.
**
**  INPUT:
**
**      Factor
**          Specifies a scale factor that is used to create a variable depth
**          offset for each polygon. The initial value is 0.
**
**      Units
**          Is multiplied by an implementation-specific value to create
**          a constant depth offset. The initial value is 0.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_POLIGON

GL_API void GL_APIENTRY glPolygonOffsetx(
    GLfixed Factor,
    GLfixed Units
    )
{
    glmENTER2(glmARGFIXED, Factor, glmARGFIXED, Units)
    {
		gcmDUMP_API("${ES11 glPolygonOffsetx 0x%08X 0x%08X}", Factor, Units);

		glmPROFILE(context, GLES1_POLYGONOFFSETX, 0);
        glmERROR(_SetPolygonOffset(
            context,
            &Factor,
            &Units,
            glvFIXED
            ));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glPolygonOffsetxOES(
    GLfixed Factor,
    GLfixed Units
    )
{
    glmENTER2(glmARGFIXED, Factor, glmARGFIXED, Units)
    {
		gcmDUMP_API("${ES11 glPolygonOffsetxOES 0x%08X 0x%08X}", Factor, Units);

		glmPROFILE(context, GLES1_POLYGONOFFSETX, 0);
        glmERROR(_SetPolygonOffset(
            context,
            &Factor,
            &Units,
            glvFIXED
            ));
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glPolygonOffset(
    GLfloat Factor,
    GLfloat Units
    )
{
    glmENTER2(glmARGFLOAT, Factor, glmARGFLOAT, Units)
    {
		gcmDUMP_API("${ES11 glPolygonOffset 0x%08X 0x%08X}", *(GLuint*)&Factor, *(GLuint*)&Units);

		glmPROFILE(context, GLES1_POLYGONOFFSET, 0);
        glmERROR(_SetPolygonOffset(
            context,
            &Factor,
            &Units,
            glvFLOAT
            ));
    }
    glmLEAVE();
}
#endif


/*******************************************************************************
**
**  glStencilFunc
**
**  Stenciling, like depth-buffering, enables and disables drawing on
**  a per-pixel basis. You draw into the stencil planes using GL drawing
**  primitives, then render geometry and images, using the stencil planes
**  to mask out portions of the screen. Stenciling is typically used in
**  multipass rendering algorithms to achieve special effects, such as decals,
**  outlining, and constructive solid geometry rendering.
**
**  The stencil test conditionally eliminates a pixel based on the outcome of
**  a comparison between the reference value and the value in the stencil
**  buffer. To enable and disable stencil test, call glEnable and glDisable
**  with argument GL_STENCIL_TEST. Stencil test is initially disabled. To
**  specify actions based on the outcome of the stencil test, call glStencilOp.
**
**  INPUT:
**
**      Function
**          Specifies the test function. Eight tokens are valid: GL_NEVER,
**          GL_LESS, GL_LEQUAL, GL_GREATER, GL_GEQUAL, GL_EQUAL, GL_NOTEQUAL,
**          and GL_ALWAYS. The initial value is GL_ALWAYS.
**
**      Reference
**          Specifies the reference value for the stencil test. Reference is
**          clamped to the range [0, 2^(n - 1)] , where n is the number of
**          bitplanes in the stencil buffer. The initial value is 0.
**
**      Mask
**          Specifies a mask that is ANDed with both the reference value
**          and the stored stencil value when the test is done.
**          The initial value is all 1's.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_FRAGMENT

GL_API void GL_APIENTRY glStencilFunc(
    GLenum Function,
    GLint Reference,
    GLuint Mask
    )
{
    gceCOMPARE function;
    gleTESTFUNCTIONS testFunction;

    glmENTER3(glmARGENUM, Function, glmARGINT, Reference, glmARGUINT, Mask)
    {
		gcmDUMP_API("${ES11 glStencilFunc 0x%08X 0x%08X 0x%08X}", Function, Reference, Mask);

		glmPROFILE(context, GLES1_STENCILFUNC, 0);

        switch(Function)
        {
        case GL_NEVER:
            function = gcvCOMPARE_NEVER;
            testFunction = glvNEVER;
            break;
        case GL_LESS:
            function = gcvCOMPARE_LESS;
            testFunction = glvLESS;
            break;
        case GL_EQUAL:
            function = gcvCOMPARE_EQUAL;
            testFunction = glvEQUAL;
            break;
        case GL_ALWAYS:
            function = gcvCOMPARE_ALWAYS;
            testFunction = glvALWAYS;
            break;
        case GL_LEQUAL:
            function = gcvCOMPARE_LESS_OR_EQUAL;
            testFunction = glvLESSOREQUAL;
            break;
        case GL_GREATER:
            function = gcvCOMPARE_GREATER;
            testFunction = glvGREATER;
            break;
        case GL_NOTEQUAL:
            function = gcvCOMPARE_NOT_EQUAL;
            testFunction = glvNOTEQUAL;
            break;
        case GL_GEQUAL:
            function = gcvCOMPARE_GREATER_OR_EQUAL;
            testFunction = glvGREATEROREQUAL;
            break;
        default:
            function = gcvCOMPARE_INVALID;
        }

        if (function == gcvCOMPARE_INVALID)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (_GetStencilBits(context) == 0)
        {
            function = gcvCOMPARE_ALWAYS;
        }

        context->stencilStates.testFunction = testFunction;
        context->stencilStates.reference = Reference;
        context->stencilStates.valueMask = Mask;

        context->stencilStates.hal.compareFront =
        context->stencilStates.hal.compareBack
            = function;

        context->stencilStates.hal.referenceFront =
        context->stencilStates.hal.referenceBack
            = (gctUINT8) Reference;

        context->stencilStates.hal.mask = (gctUINT8)(Mask & 0x0F);

        context->stencilStates.dirty = GL_TRUE;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glStencilMask
**
**  glStencilMask controls the writing of individual bits in the stencil planes.
**  The least significant n bits of mask, where n is the number of bits in the
**  stencil buffer, specify a mask. Where a 1 appears in the mask, it's possible
**  to write to the corresponding bit in the stencil buffer. Where a 0 appears,
**  the corresponding bit is write-protected. Initially, all bits are enabled
**  for writing.
**
**  INPUT:
**
**      Mask
**          Specifies a bit mask to enable and disable writing of individual
**          bits in the stencil planes. The initial value is all 1's.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glStencilMask(
    GLuint Mask
    )
{
    glmENTER1(glmARGUINT, Mask)
    {
		gcmDUMP_API("${ES11 glStencilMask 0x%08X}", Mask);

		glmPROFILE(context, GLES1_STENCILMASK, 0);
        _SetWriteMask(context, Mask);
    }
    glmLEAVE();
}

/*******************************************************************************
**
**  _glfConvertGLStencilOpEnum
**
**  Converts OpenGL GL_xxx constants into intrernal enumerations.
**
**  INPUT:
**
**      Names
**      NameCount
**          Pointer and a count of the GL enumaration array.
**
**      Value
**      Type
**          GL enumeration value to be converted.
**
**  OUTPUT:
**
**      Result
**          Internal value corresponding to the GL enumeration.
*/

GLboolean _glfConvertGLStencilOpEnum(
    GLenum Name,
    gceSTENCIL_OPERATION* Result,
    gleSTENCILOPERATIONS* GLResult
    )
{
    /* Convert the enum. */
    switch(Name)
    {
    case GL_ZERO:
        *Result = gcvSTENCIL_ZERO;
        *GLResult = glvSTENCILZERO;
        break;
    case GL_KEEP:
        *Result = gcvSTENCIL_KEEP;
        *GLResult = glvSTENCILKEEP;
        break;
    case GL_REPLACE:
        *Result = gcvSTENCIL_REPLACE;
        *GLResult = glvSTENCILREPLACE;
        break;
    case GL_INCR:
        *Result = gcvSTENCIL_INCREMENT_SATURATE;
        *GLResult = glvSTENCILINC;
        break;
    case GL_DECR:
        *Result = gcvSTENCIL_DECREMENT_SATURATE;
        *GLResult = glvSTENCILDEC;
        break;
    case GL_INVERT:
        *Result = gcvSTENCIL_INVERT;
        *GLResult = glvSTENCILINVERT;
        break;
    default:
        *Result = gcvSTENCIL_ZERO;
        *GLResult = glvSTENCILZERO;
        /* Bad enum. */
        return GL_TRUE;
    }

    /* Good enum. */
    return GL_TRUE;
}

/*******************************************************************************
**
**  glStencilOp
**
**  Stenciling, like depth-buffering, enables and disables drawing on a
**  per-pixel basis. You draw into the stencil planes using GL drawing
**  primitives, then render geometry and images, using the stencil planes
**  to mask out portions of the screen. Stenciling is typically used in
**  multipass rendering algorithms to achieve special effects, such as decals,
**  outlining, and constructive solid geometry rendering.
**
**  The stencil test conditionally eliminates a pixel based on the outcome of
**  a comparison between the value in the stencil buffer and a reference value.
**  To enable and disable stencil test, call glEnable and glDisable with
**  argument GL_STENCIL_TEST. To control it, call glStencilFunc.
**  Stenciling is initially disabled.
**
**  INPUT:
**
**      Fail
**          Specifies the action to take when the stencil test fails.
**          Six symbolic constants are accepted: GL_KEEP, GL_ZERO, GL_REPLACE,
**          GL_INCR, GL_DECR, and GL_INVERT. The initial value is GL_KEEP.
**
**      ZFail
**          Specifies the stencil action when the stencil test passes,
**          but the depth test fails. zfail accepts the same symbolic
**          constants as fail. The initial value is GL_KEEP.
**
**      ZPass
**          Specifies the stencil action when both the stencil test and
**          the depth test pass, or when the stencil test passes and either
**          there is no depth buffer or depth testing is not enabled. zpass
**          accepts the same symbolic constants as fail.
**          The initial value is GL_KEEP.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glStencilOp(
    GLenum Fail,
    GLenum ZFail,
    GLenum ZPass
    )
{
    gceSTENCIL_OPERATION fail;
    gceSTENCIL_OPERATION zFail;
    gceSTENCIL_OPERATION zPass;
    gleSTENCILOPERATIONS gleFail;
    gleSTENCILOPERATIONS gleZFail;
    gleSTENCILOPERATIONS gleZPass;

    glmENTER3(glmARGENUM, Fail, glmARGENUM, ZFail, glmARGENUM, ZPass)
    {
		gcmDUMP_API("${ES11 glStencilOp 0x%08X 0x%08X 0x%08X}", Fail, ZFail, ZPass);

		glmPROFILE(context, GLES1_STENCILOP, 0);

        if(!_glfConvertGLStencilOpEnum(Fail, &fail, &gleFail)
        || !_glfConvertGLStencilOpEnum(ZFail, &zFail, &gleZFail)
        || !_glfConvertGLStencilOpEnum(ZPass, &zPass, &gleZPass))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        context->stencilStates.fail  = gleFail;
        context->stencilStates.zFail = gleZFail;
        context->stencilStates.zPass = gleZPass;

        context->stencilStates.hal.passFront =
        context->stencilStates.hal.passBack = zPass;

        context->stencilStates.hal.failFront =
        context->stencilStates.hal.failBack = fail;

        context->stencilStates.hal.depthFailFront =
        context->stencilStates.hal.depthFailBack = zFail;

        context->stencilStates.dirty = GL_TRUE;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glClearStencil
**
**  glClearStencil specifies the index used by glClear to clear the stencil
**  buffer. ClearValue is masked with 2^m - 1, where m is the number of bits
**  in the stencil buffer.
**
**  INPUT:
**
**      ClearValue
**          Specifies the index used when the stencil buffer is cleared.
**          The initial value is 0.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_CLEAR

GL_API void GL_APIENTRY glClearStencil(
    GLint ClearValue
    )
{
    glmENTER1(glmARGINT, ClearValue)
    {
		gcmDUMP_API("${ES11 glClearStencil 0x%08X}", ClearValue);

		glmPROFILE(context, GLES1_CLEARSTENCIL, 0);
        glmERROR(_SetClearStencil(context, ClearValue));
    }
    glmLEAVE();
}
