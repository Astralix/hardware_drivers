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

#define _GC_OBJ_ZONE glvZONE_FRAGMENT

/******************************************************************************\
********************************* GL Name Arrays *******************************
\******************************************************************************/

static GLenum _AlphaTestNames[] =
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

static GLenum _BlendModeNames[] =
{
	GL_FUNC_ADD_OES,				/* glvBLENDADD */
	GL_FUNC_SUBTRACT_OES,			/* glvBLENDSUBTRACT */
	GL_FUNC_REVERSE_SUBTRACT_OES,	/* glvBLENDREVERSESUBTRACT */
	GL_MIN_EXT,                     /* glvBLNEDMIN */
	GL_MAX_EXT                      /* glvBLENDMAX */
};

static GLenum _SrcBlendFunctionNames[] =
{
    GL_ZERO,                    /* glvBLENDZERO */
    GL_ONE,                     /* glvBLENDONE */
    GL_SRC_COLOR,               /* glvBLENDSRCCOLOR */
    GL_ONE_MINUS_SRC_COLOR,     /* glvBLENDSRCCOLORINV */
    GL_SRC_ALPHA,               /* glvBLENDSRCALPHA */
    GL_ONE_MINUS_SRC_ALPHA,     /* glvBLENDSRCALPHAINV */
    GL_DST_COLOR,               /* glvBLENDDSTCOLOR */
    GL_ONE_MINUS_DST_COLOR,     /* glvBLENDDSTCOLORINV */
    GL_DST_ALPHA,               /* glvBLENDDSTALPHA */
    GL_ONE_MINUS_DST_ALPHA,     /* glvBLENDDSTALPHAINV */
    GL_SRC_ALPHA_SATURATE,      /* glvBLENDSRCALPHASATURATE */
};

static GLenum _DestBlendFunctionNames[] =
{
    GL_ZERO,                    /* glvBLENDZERO */
    GL_ONE,                     /* glvBLENDONE */
    GL_SRC_COLOR,               /* glvBLENDSRCCOLOR */
    GL_ONE_MINUS_SRC_COLOR,     /* glvBLENDSRCCOLORINV */
    GL_SRC_ALPHA,               /* glvBLENDSRCALPHA */
    GL_ONE_MINUS_SRC_ALPHA,     /* glvBLENDSRCALPHAINV */
    GL_DST_COLOR,               /* glvBLENDDSTCOLOR */
    GL_ONE_MINUS_DST_COLOR,     /* glvBLENDDSTCOLORINV */
    GL_DST_ALPHA,               /* glvBLENDDSTALPHA */
    GL_ONE_MINUS_DST_ALPHA,     /* glvBLENDDSTALPHAINV */
};


/******************************************************************************\
***************************** HAL Translation Arrays ***************************
\******************************************************************************/

static gceCOMPARE _AlphaTestValues[] =
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

static gceBLEND_MODE _BlendModeValues[] =
{
    gcvBLEND_ADD,					/* glvBLENDADD */
    gcvBLEND_SUBTRACT,				/* glvBLENDSUBTRACT */
    gcvBLEND_REVERSE_SUBTRACT,		/* glvBLENDREVERSESUBTRACT */
	gcvBLEND_MIN,                   /* glvBLENDMIN */
	gcvBLEND_MAX                    /* glvBLENDMAX */
};

static gceBLEND_FUNCTION _BlendFunctionValues[] =
{
    gcvBLEND_ZERO,                  /* glvBLENDZERO */
    gcvBLEND_ONE,                   /* glvBLENDONE */
    gcvBLEND_SOURCE_COLOR,          /* glvBLENDSRCCOLOR */
    gcvBLEND_INV_SOURCE_COLOR,      /* glvBLENDSRCCOLORINV */
    gcvBLEND_SOURCE_ALPHA,          /* glvBLENDSRCALPHA */
    gcvBLEND_INV_SOURCE_ALPHA,      /* glvBLENDSRCALPHAINV */
    gcvBLEND_TARGET_COLOR,          /* glvBLENDDSTCOLOR */
    gcvBLEND_INV_TARGET_COLOR,      /* glvBLENDDSTCOLORINV */
    gcvBLEND_TARGET_ALPHA,          /* glvBLENDDSTALPHA */
    gcvBLEND_INV_TARGET_ALPHA,      /* glvBLENDDSTALPHAINV */
    gcvBLEND_SOURCE_ALPHA_SATURATE, /* glvBLENDSRCALPHASATURATE */
};


/******************************************************************************\
********************** Individual State Setting Functions **********************
\******************************************************************************/

static GLenum _SetAlphaTestReference(
    glsCONTEXT_PTR Context,
    GLenum Function,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x Function=0x%04x Value=0x%x Type=0x%04x",
                    Context, Function, Value, Type);
    do
    {
        GLuint function;
        gceSTATUS status;

        if (!glfConvertGLEnum(
                _AlphaTestNames,
                gcmCOUNTOF(_AlphaTestNames),
                &Function, glvINT,
                &function
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

        Context->alphaStates.testFunction = (gleTESTFUNCTIONS) function;

        /* Set with [0..1] clamping. */
        glfSetClampedMutant(&Context->alphaStates.testReference, Value, Type);

        do
        {
            /* Get fixed point reference value. */
            GLfixed referencex
                = glfFixedFromMutant(&Context->alphaStates.testReference);

            /* Alpha reference is in 0..1 range; use high 8 bits
               of the fraction; cap reference if needed. */
            gctUINT8 reference = (referencex == gcvONE_X)
                ? 0xFF
                : (gctUINT8) (referencex >> 8);

            gcmERR_BREAK(gco3D_SetAlphaCompare(
                Context->hw,
                _AlphaTestValues[function]
                ));

            gcmERR_BREAK(gco3D_SetAlphaReference(
                Context->hw,
                reference
                ));
        }
        while (gcvFALSE);

        result = glmTRANSLATEHALSTATUS(status);
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}

static GLenum _SetBlendFunction(
    glsCONTEXT_PTR Context,
    GLenum SrcFunction,
    GLenum DestFunction
    )
{
    GLenum result;

    gcmHEADER_ARG("Context=0x%x SrcFunction=0x%04x DestFunction=0x%04x",
                    Context, SrcFunction, DestFunction);

    do
    {
        GLuint srcFunction;
        GLuint destFunction;
        gceSTATUS status;

        if (!glfConvertGLEnum(
                _SrcBlendFunctionNames,
                gcmCOUNTOF(_SrcBlendFunctionNames),
                &SrcFunction, glvINT,
                &srcFunction
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

        if (!glfConvertGLEnum(
                _DestBlendFunctionNames,
                gcmCOUNTOF(_DestBlendFunctionNames),
                &DestFunction, glvINT,
                &destFunction
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

        Context->alphaStates.blendSrcFunction  = (gleBLENDFUNCTIONS) srcFunction;
        Context->alphaStates.blendDestFunction = (gleBLENDFUNCTIONS) destFunction;

        do
        {
            gceBLEND_FUNCTION srcColorFunction =
                _BlendFunctionValues[srcFunction];

            gceBLEND_FUNCTION srcAlphaFunction =
                (srcFunction == glvBLENDSRCALPHASATURATE)
                    ? gcvBLEND_ONE
                    : srcColorFunction;

            gceBLEND_FUNCTION dstFunction =
                _BlendFunctionValues[destFunction];

            gcmERR_BREAK(gco3D_SetBlendFunction(
                Context->hw,
                gcvBLEND_SOURCE,
                srcColorFunction,
                srcAlphaFunction
                ));

            gcmERR_BREAK(gco3D_SetBlendFunction(
                Context->hw,
                gcvBLEND_TARGET,
                dstFunction,
                dstFunction
                ));
        }
        while (gcvFALSE);

        result = glmTRANSLATEHALSTATUS(status);
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}

static GLenum _SetBlendFuncSeparate(
    glsCONTEXT_PTR Context,
    GLenum SrcRGB,
    GLenum DstRGB,
    GLenum SrcAlpha,
    GLenum DstAlpha
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x SrcRGB=0x%04x DstRGB=0x%04x SrcAlpha=0x%04x DstAlpha=0x%04x",
                    Context, SrcRGB, DstRGB, SrcAlpha, DstAlpha);
    do
    {
		GLuint blendSrcRGB, blendSrcAlpha;
		GLuint blendDstRGB, blendDstAlpha;
        gceSTATUS status;

		if (!glfConvertGLEnum(
                _SrcBlendFunctionNames,
                gcmCOUNTOF(_SrcBlendFunctionNames),
                &SrcRGB, glvINT,
                &blendSrcRGB
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

		if (!glfConvertGLEnum(
                _DestBlendFunctionNames,
                gcmCOUNTOF(_DestBlendFunctionNames),
                &DstRGB, glvINT,
                &blendDstRGB
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

		if (!glfConvertGLEnum(
                _SrcBlendFunctionNames,
                gcmCOUNTOF(_SrcBlendFunctionNames),
                &SrcAlpha, glvINT,
                &blendSrcAlpha
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

		if (!glfConvertGLEnum(
                _DestBlendFunctionNames,
                gcmCOUNTOF(_DestBlendFunctionNames),
                &DstAlpha, glvINT,
                &blendDstAlpha
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

		Context->alphaStates.blendSrcFunctionRGB   = (gleBLENDFUNCTIONS) blendSrcRGB;
		Context->alphaStates.blendDstFunctionRGB   = (gleBLENDFUNCTIONS) blendDstRGB;
		Context->alphaStates.blendSrcFunctionAlpha = (gleBLENDFUNCTIONS) blendSrcAlpha;
		Context->alphaStates.blendDstFunctionAlpha = (gleBLENDFUNCTIONS) blendDstAlpha;

        do
        {
            gceBLEND_FUNCTION srcFunctionRGB =
                _BlendFunctionValues[blendSrcRGB];

            gceBLEND_FUNCTION dstFunctionRGB =
                _BlendFunctionValues[blendDstRGB];

            gceBLEND_FUNCTION srcFunctionAlpha =
                _BlendFunctionValues[blendSrcAlpha];

            gceBLEND_FUNCTION dstFunctionAlpha =
                _BlendFunctionValues[blendDstAlpha];

			gcmERR_BREAK(
				gco3D_SetBlendFunction(Context->hw,
                               gcvBLEND_SOURCE,
                               srcFunctionRGB, srcFunctionAlpha));

			gcmERR_BREAK(
				gco3D_SetBlendFunction(Context->hw,
                               gcvBLEND_TARGET,
                               dstFunctionRGB, dstFunctionAlpha));
        }
        while (gcvFALSE);

        result = glmTRANSLATEHALSTATUS(status);
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}


static GLenum _SetBlendEquation(
    glsCONTEXT_PTR Context,
    GLenum mode
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x SrcRGB=0x%04x ",
                    Context, mode);
    do
    {
        gceSTATUS status;
		GLuint blendMode;

		if (!glfConvertGLEnum(
                _BlendModeNames,
                gcmCOUNTOF(_BlendModeNames),
                &mode, glvINT,
                &blendMode
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

		Context->alphaStates.blendModeRGB = (gleBLENDMODES)blendMode;
		Context->alphaStates.blendModeAlpha = (gleBLENDMODES)blendMode;

        do
        {
			gceBLEND_MODE gceBlendMode =
                _BlendModeValues[blendMode];

            gcmERR_BREAK(gco3D_SetBlendMode(
                Context->hw,
                gceBlendMode,
				gceBlendMode
                ));
        }
        while (gcvFALSE);

        result = glmTRANSLATEHALSTATUS(status);
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}

static GLenum _SetBlendEquationSeparate(
    glsCONTEXT_PTR Context,
    GLenum modeRGB,
	GLenum modeAlpha
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x modeRGB=0x%04x modeAlpha=0x%04x ",
                    Context, modeRGB, modeAlpha);

    do
    {
        gceSTATUS status;
		GLuint blendModeRGB;
		GLuint blendModeAlpha;

		if (!glfConvertGLEnum(
                _BlendModeNames,
                gcmCOUNTOF(_BlendModeNames),
                &modeRGB, glvINT,
                &blendModeRGB
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

		if (!glfConvertGLEnum(
                _BlendModeNames,
                gcmCOUNTOF(_BlendModeNames),
                &modeAlpha, glvINT,
                &blendModeAlpha
                ))
        {
            result = GL_INVALID_ENUM;
            break;
        }

		Context->alphaStates.blendModeRGB   = (gleBLENDMODES) blendModeRGB;
		Context->alphaStates.blendModeAlpha = (gleBLENDMODES) blendModeAlpha;

        do
        {
			gceBLEND_MODE gceBlendModeRGB =
                _BlendModeValues[blendModeRGB];

			gceBLEND_MODE gceBlendModeAlpha =
                _BlendModeValues[blendModeAlpha];

            gcmERR_BREAK(gco3D_SetBlendMode(
                Context->hw,
                gceBlendModeRGB,
				gceBlendModeAlpha
                ));
        }
        while (gcvFALSE);

        result = glmTRANSLATEHALSTATUS(status);
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  glfSetDefaultAlphaStates
**
**  Set default alpha states.
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

gceSTATUS glfSetDefaultAlphaStates(
    glsCONTEXT_PTR Context
    )
{
    GLenum result;
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        /* Default alpha test reference. */
        static gltFRACTYPE defaultReference = glvFRACZERO;

        /* Set blend mode. */
        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetBlendMode(
            Context->hw,
            gcvBLEND_ADD,
            gcvBLEND_ADD
            )));

        /* Disable alpha blending by default. */
        glmERR_BREAK(glfEnableAlphaBlend(
            Context,
            GL_FALSE
            ));

		/* Set default blend function separate. */
        glmERR_BREAK(_SetBlendFuncSeparate(
            Context,
            GL_ONE,
			GL_ZERO,
			GL_ONE,
			GL_ZERO
            ));

		/* Set default blend equation. */
        glmERR_BREAK(_SetBlendEquation(
            Context,
            GL_FUNC_ADD_OES
            ));

		/* Set default blend equation separate. */
        glmERR_BREAK(_SetBlendEquationSeparate(
            Context,
            GL_FUNC_ADD_OES,
            GL_FUNC_ADD_OES
            ));

        /* Disable alpha test by default. */
        glmERR_BREAK(glfEnableAlphaTest(
            Context,
            GL_FALSE
            ));

        /* Set test function and reference. */
        glmERR_BREAK(_SetAlphaTestReference(
            Context,
            GL_ALWAYS,
            &defaultReference,
            glvFRACTYPEENUM
            ));
    }
    while (gcvFALSE);

    status = glmTRANSLATEGLRESULT(result);

    gcmFOOTER();

    return status;
}


/*******************************************************************************
**
**  glfEnableAlphaBlend
**
**  Enable alpha blending.
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

GLenum glfEnableAlphaBlend(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);
    Context->alphaStates.blendEnabled = Enable;
	if (Enable)
	{
		/* Turn off dither since alpha is enabled. */
		gco3D_EnableDither(Context->hw, gcvFALSE);
	}
	else
	{
		if (Context->dither)
		{
			/* Restore dither since alpha is disabled. */
			gco3D_EnableDither(Context->hw, gcvTRUE);
		}
	}
    result = glmTRANSLATEHALSTATUS(gco3D_EnableBlending(Context->hw, Enable));
    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}


/*******************************************************************************
**
**  glfEnableAlphaTest
**
**  Enable alpha test.
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

GLenum glfEnableAlphaTest(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);
    Context->alphaStates.testEnabled = Enable;
    result = glmTRANSLATEHALSTATUS(gco3D_SetAlphaTest(Context->hw, Enable));
    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}


/*******************************************************************************
**
**  glfQueryAlphaState
**
**  Queries alpha state values.
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

GLboolean glfQueryAlphaState(
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
    case GL_ALPHA_TEST:
        glfGetFromInt(
            Context->alphaStates.testEnabled,
            Value,
            Type
            );
        break;

    case GL_ALPHA_TEST_FUNC:
        glfGetFromEnum(
            _AlphaTestNames[Context->alphaStates.testFunction],
            Value,
            Type
            );
        break;

    case GL_ALPHA_TEST_REF:
        if (Type == glvINT)
        {
            /* Normalize when converting to INT. */
            Type = glvNORM;
        }

        glfGetFromMutant(
            &Context->alphaStates.testReference,
            Value,
            Type
            );
        break;

    case GL_BLEND:
        glfGetFromInt(
            Context->alphaStates.blendEnabled,
            Value,
            Type
            );
        break;

    case GL_BLEND_DST:
        glfGetFromEnum(
            _DestBlendFunctionNames[Context->alphaStates.blendDestFunction],
            Value,
            Type
            );
        break;

    case GL_BLEND_SRC:
        glfGetFromEnum(
            _SrcBlendFunctionNames[Context->alphaStates.blendSrcFunction],
            Value,
            Type
            );
        break;

	case GL_BLEND_EQUATION_RGB_OES:
		glfGetFromEnum(
			_BlendModeNames[Context->alphaStates.blendModeRGB],
			Value,
			Type
			);
		break;

	case GL_BLEND_EQUATION_ALPHA_OES:
		glfGetFromEnum(
			_BlendModeNames[Context->alphaStates.blendModeAlpha],
			Value,
			Type
			);
		break;

	case GL_BLEND_DST_RGB_OES:
		glfGetFromEnum(
			_DestBlendFunctionNames[Context->alphaStates.blendDstFunctionRGB],
			Value,
			Type
			);
		break;

	case GL_BLEND_SRC_RGB_OES:
		glfGetFromEnum(
			_SrcBlendFunctionNames[Context->alphaStates.blendSrcFunctionRGB],
			Value,
			Type
			);
		break;

	case GL_BLEND_DST_ALPHA_OES:
		glfGetFromEnum(
			_DestBlendFunctionNames[Context->alphaStates.blendDstFunctionAlpha],
			Value,
			Type
			);
		break;

	case GL_BLEND_SRC_ALPHA_OES:
		glfGetFromEnum(
			_SrcBlendFunctionNames[Context->alphaStates.blendSrcFunctionAlpha],
			Value,
			Type
			);
		break;

    default:
        result = GL_FALSE;
    }

    gcmFOOTER_ARG("return=%d", result);

    /* Return result. */
    return result;
}


/******************************************************************************\
******************** OpenGL Alpha Test State Management Code *******************
\******************************************************************************/

/*******************************************************************************
**
**  glAlphaFunc
**
**  The alpha test discards fragments depending on the outcome of a comparison
**  between an incoming fragment's alpha value and a constant reference value.
**  glAlphaFunc specifies the reference value and the comparison function.
**  The comparison is performed only if alpha testing is enabled. To enable
**  and disable alpha testing, call glEnable and glDisable with argument
**  GL_ALPHA_TEST. Alpha testing is initially disabled.
**
**  INPUT:
**
**      Function
**          Specifies the alpha comparison function. Symbolic constants
**          GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL,
**          GL_GEQUAL, and GL_ALWAYS are accepted.
**          The initial value is GL_ALWAYS.
**
**      Reference
**          Specifies the reference value that incoming alpha values are
**          compared to. This value is clamped to the range [0, 1], where 0
**          represents the lowest possible alpha value and 1 the highest
**          possible value. The initial reference value is 0.
**
**  OUTPUT:
**
**      Nothing.
*/

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glAlphaFunc(
    GLenum Function,
    GLclampf Reference
    )
{
    glmENTER2(glmARGENUM, Function, glmARGFLOAT, Reference)
    {
		gcmDUMP_API("${ES11 glAlphaFunc 0x%08X 0x%08X}", Function, Reference);

		glmPROFILE(context, GLES1_ALPHAFUNC, 0);
        glmERROR(_SetAlphaTestReference(
            context,
            Function,
            &Reference,
            glvFLOAT
            ));
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glAlphaFuncx(
    GLenum Function,
    GLclampx Reference
    )
{
    glmENTER2(glmARGENUM, Function, glmARGFIXED, Reference)
    {
		gcmDUMP_API("${ES11 glAlphaFuncx 0x%08X 0x%08X}", Function, Reference);

		glmPROFILE(context, GLES1_ALPHAFUNC, 0);
        glmERROR(_SetAlphaTestReference(
            context,
            Function,
            &Reference,
            glvFIXED
            ));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glAlphaFuncxOES(
    GLenum Function,
    GLclampx Reference
    )
{
    glmENTER2(glmARGENUM, Function, glmARGFIXED, Reference)
    {
		gcmDUMP_API("${ES11 glAlphaFuncxOES 0x%08X 0x%08X}", Function, Reference);

		glmPROFILE(context, GLES1_ALPHAFUNC, 0);
        glmERROR(_SetAlphaTestReference(
            context,
            Function,
            &Reference,
            glvFIXED
            ));
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glBlendFunc
**
**  Pixels can be drawn using a function that blends the incoming (source)
**  values with the values that are already in the color buffer
**  (the destination values). Use glEnable and glDisable with argument GL_BLEND
**  to enable and disable blending. Blending is initially disabled.
**
**  INPUT:
**
**      SrcFunction
**          Specifies how the red, green, blue, and alpha source blending
**          factors are computed. The following symbolic constants are accepted:
**          GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR,
**          GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
**          GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA, and GL_SRC_ALPHA_SATURATE.
**          The initial value is GL_ONE.
**
**      DestFunction
**          Specifies how the red, green, blue, and alpha destination blending
**          factors are computed. Eight symbolic constants are accepted:
**          GL_ZERO, GL_ONE, GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR, GL_DST_COLOR,
**          GL_ONE_MINUS_DST_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
**          GL_DST_ALPHA, and GL_ONE_MINUS_DST_ALPHA.
**          The initial value is GL_ZERO.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glBlendFunc(
    GLenum SrcFunction,
    GLenum DestFunction
    )
{
    glmENTER2(glmARGENUM, SrcFunction, glmARGENUM, DestFunction)
    {
		gcmDUMP_API("${ES11 glBlendFunc 0x%08X 0x%08X}", SrcFunction, DestFunction);

		glmPROFILE(context, GLES1_BLENDFUNC, 0);
        glmERROR(_SetBlendFunction(
            context,
            SrcFunction,
            DestFunction
            ));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glBlendEquationOES(
    GLenum mode
    )
{
	glmENTER1(glmARGENUM, mode)
	{
		gcmDUMP_API("${ES11 glBlendEquationOES 0x%08X}", mode);

		glmPROFILE(context, GLES1_BLENDEQUATIONOES, 0);
		glmERROR(_SetBlendEquation(
			context,
			mode
			));
	}
	glmLEAVE();
}

GL_API void GL_APIENTRY glBlendFuncSeparateOES(
    GLenum srcRGB,
    GLenum dstRGB,
    GLenum srcAlpha,
    GLenum dstAlpha
    )
{
	glmENTER4(glmARGENUM, srcRGB, glmARGENUM, dstRGB, glmARGENUM, srcAlpha, glmARGENUM, dstAlpha);
	{
		gcmDUMP_API("${ES11 glBlendFuncSeparateOES 0x%08X 0x%08X 0x%08X 0x%08X}",
			srcRGB, dstRGB, srcAlpha, dstAlpha);

		glmPROFILE(context, GLES1_BLENDFUNCSEPERATEOES, 0);
		glmERROR(_SetBlendFuncSeparate(
			context,
			srcRGB,
			dstRGB,
			srcAlpha,
			dstAlpha
			));;
	}
	glmLEAVE();
}

GL_API void GL_APIENTRY glBlendEquationSeparateOES(
    GLenum modeRGB,
	GLenum modeAlpha
    )
{
	glmENTER2(glmARGENUM, modeRGB, glmARGENUM, modeAlpha)
	{
		gcmDUMP_API("${ES11 glBlendEquationSeparateOES 0x%08X 0x%08X}",
			modeRGB, modeAlpha);

		glmPROFILE(context, GLES1_BLENDEQUATIONSEPARATEOES, 0);
		glmERROR(_SetBlendEquationSeparate(
			context,
			modeRGB,
			modeAlpha
			));
	}
	glmLEAVE();
}

