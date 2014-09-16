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
********************** Individual State Setting Functions **********************
\******************************************************************************/

static GLenum _UpdateLogicOp(
    glsCONTEXT_PTR Context
    )
{
    GLenum result = GL_NO_ERROR;
    gcmHEADER_ARG("Context=0x%x",
                    Context);

    /* Set ROP code in the hardware if supported. */
    if (Context->hwLogicOp)
    {
        /* Determine the proper ROP2. */
        gctUINT8 rop2 = Context->logicOp.enabled
            ? (Context->logicOp.rop & 0xF)
            : 0xC;

        /* Disable software logicOp. */
        Context->logicOp.perform = GL_FALSE;

        /* Set ROP2. */
        result = glmTRANSLATEHALSTATUS(gco3D_SetLogicOp(Context->hw, rop2));
    }
    else
    {
        /* Determine whether the software logicOp should be performed. */
        Context->logicOp.perform
            =   Context->logicOp.enabled
            && (Context->logicOp.operation != GL_COPY);
    }

    gcmFOOTER_ARG("result=0x%04x", result);
    /* Return result. */
    return result;
}

static GLenum _SetLogicOp(
    glsCONTEXT_PTR Context,
    GLenum Opcode
    )
{
    static gctUINT8 ropTable[] =
    {
        /* GL_CLEAR         */ 0x00,
        /* GL_AND           */ 0x88,
        /* GL_AND_REVERSE   */ 0x44,
        /* GL_COPY          */ 0xCC,
        /* GL_AND_INVERTED  */ 0x22,
        /* GL_NOOP          */ 0xAA,
        /* GL_XOR           */ 0x66,
        /* GL_OR            */ 0xEE,
        /* GL_NOR           */ 0x11,
        /* GL_EQUIV         */ 0x99,
        /* GL_INVERT        */ 0x55,
        /* GL_OR_REVERSE    */ 0xDD,
        /* GL_COPY_INVERTED */ 0x33,
        /* GL_OR_INVERTED   */ 0xBB,
        /* GL_NAND          */ 0x77,
        /* GL_SET           */ 0xFF,
    };
    GLenum result;

    gcmHEADER_ARG("Context=0x%x Opcode=0x%04x", Context, Opcode);
    /* Set the new code. */
    Context->logicOp.operation = Opcode;

    /* Determine the ROP code. */
    Context->logicOp.rop = ropTable[Opcode - GL_CLEAR];

    /* Determine whether the operation should be performed. */
    result = _UpdateLogicOp(Context);
    gcmFOOTER_ARG("result=0x%04x", result);
    return result;
}

static GLenum _SetColorMask(
    glsCONTEXT_PTR Context,
    GLboolean Red,
    GLboolean Green,
    GLboolean Blue,
    GLboolean Alpha
    )
{
    gctUINT8 enable
        =  (gctUINT8) Red
        | ((gctUINT8) Green << 1)
        | ((gctUINT8) Blue  << 2)
        | ((gctUINT8) Alpha << 3);
    GLenum result;

    gcmHEADER_ARG("Context=0x%x Red=%d Green=%d Blue=%d Alpha=%d",
                    Context, Red, Green, Blue, Alpha);

    Context->colorMask[0] = Red;
    Context->colorMask[1] = Green;
    Context->colorMask[2] = Blue;
    Context->colorMask[3] = Alpha;

    result = glmTRANSLATEHALSTATUS(gco3D_SetColorWrite(Context->hw, enable));

    gcmFOOTER_ARG("result=0x%04x", result);
    return result;
}

static GLenum _SetClearColor(
    glsCONTEXT_PTR Context,
    GLvoid* ClearColor,
    gleTYPE Type
    )
{
    gltFRACTYPE clearColor[4];
    GLenum result;

    gcmHEADER_ARG("Context=0x%x ClearColor=0x%x Type=0x%04x", Context, ClearColor, Type);

    /* Set with [0..1] clamping. */
    glfSetClampedVector4(&Context->clearColor, ClearColor, Type);

    /* Query back the color. */
    glfGetFromVector4(
        &Context->clearColor,
        clearColor,
        glvFRACTYPEENUM
        );

    /* Set color value. */
    result = glmTRANSLATEHALSTATUS(gco3D_SetClearColorFrac(
        Context->hw,
        clearColor[0],
        clearColor[1],
        clearColor[2],
        clearColor[3]
        ));

    gcmFOOTER_ARG("result=0x%04x", result);

    return result;
}


/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  _GetXXXBits
**
**  Queries the number of bits per pixel for the specified component with the
**  current frame buffer format.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Number of bits of the specified component per pixel.
*/

static GLboolean _GetCurrentFormatInfo(
    glsCONTEXT_PTR Context,
    gcsSURF_FORMAT_INFO_PTR * FormatInfo
    )
{
    gceSURF_FORMAT  format;
    gceSTATUS       status;

    gcmHEADER_ARG("Context=0x%x FormatInfo=0x%x", Context, FormatInfo);

    /* Query the format. */
    gcmONERROR(gcoSURF_GetFormat(Context->draw, gcvNULL, &format));

    /* Get format details. */
    gcmONERROR(gcoSURF_QueryFormat(format, FormatInfo));

    gcmFOOTER_ARG("result=0x%04x", GL_TRUE);

    return GL_TRUE;

OnError:
    gcmFOOTER_ARG("result=0x%04x", GL_FALSE);
    return GL_FALSE;
}

static GLuint _GetRedBits(
    glsCONTEXT_PTR Context
    )
{
    /* Get format details. */
    gcsSURF_FORMAT_INFO_PTR info[2];
    gcmHEADER_ARG("Context=0x%x", Context);
    if (!_GetCurrentFormatInfo(Context, info))
    {
        gcmFOOTER_ARG("result=%u", 0);
        return 0;
    }

    /* Return component info. */
    if ((info[0]->fmtClass == gcvFORMAT_CLASS_RGBA) &&
        ((info[0]->u.rgba.red.width & gcvCOMPONENT_DONTCARE) == 0))
    {
        gcmFOOTER_ARG("result=%u", info[0]->u.rgba.red.width & gcvCOMPONENT_WIDTHMASK);
        return info[0]->u.rgba.red.width & gcvCOMPONENT_WIDTHMASK;
    }
    else if (info[0]->format == gcvSURF_YUY2)
    {
        gcmFOOTER_ARG("result=%u", 8);
        return 8;
    }
    else
    {
        gcmFOOTER_ARG("result=%d", 0);
        return 0;
    }
}

static GLuint _GetGreenBits(
    glsCONTEXT_PTR Context
    )
{
    /* Get format details. */
    gcsSURF_FORMAT_INFO_PTR info[2];
    gcmHEADER_ARG("Context=0x%x", Context);

    if (!_GetCurrentFormatInfo(Context, info))
    {
        gcmFOOTER_ARG("result=%u", 0);
        return 0;
    }

    /* Return component info. */
    if ((info[0]->fmtClass == gcvFORMAT_CLASS_RGBA) &&
        ((info[0]->u.rgba.green.width & gcvCOMPONENT_DONTCARE) == 0))
    {
        gcmFOOTER_ARG("result=%u", info[0]->u.rgba.green.width & gcvCOMPONENT_WIDTHMASK);
        return info[0]->u.rgba.green.width & gcvCOMPONENT_WIDTHMASK;
    }
    else if (info[0]->format == gcvSURF_YUY2)
    {
        gcmFOOTER_ARG("result=%u", 8);
        return 8;
    }
    else
    {
        gcmFOOTER_ARG("result=%u", 0);
        return 0;
    }
}

static GLuint _GetBlueBits(
    glsCONTEXT_PTR Context
    )
{
    /* Get format details. */
    gcsSURF_FORMAT_INFO_PTR info[2];
    gcmHEADER_ARG("Context=0x%x", Context);

    if (!_GetCurrentFormatInfo(Context, info))
    {
        gcmFOOTER_ARG("result=%u", 0);
        return 0;
    }

    /* Return component info. */
    if ((info[0]->fmtClass == gcvFORMAT_CLASS_RGBA) &&
        ((info[0]->u.rgba.blue.width & gcvCOMPONENT_DONTCARE) == 0))
    {
        gcmFOOTER_ARG("result=%u", info[0]->u.rgba.blue.width & gcvCOMPONENT_WIDTHMASK);
        return info[0]->u.rgba.blue.width & gcvCOMPONENT_WIDTHMASK;
    }
    else if (info[0]->format == gcvSURF_YUY2)
    {
        gcmFOOTER_ARG("result=%u", 8);
        return 8;
    }
    else
    {
        gcmFOOTER_ARG("result=%u", 0);
        return 0;
    }
}

static GLuint _GetAlphaBits(
    glsCONTEXT_PTR Context
    )
{
    /* Get format details. */
    gcsSURF_FORMAT_INFO_PTR info[2];
    gcmHEADER_ARG("Context=0x%x", Context);

    if (!_GetCurrentFormatInfo(Context, info))
    {
        gcmFOOTER_ARG("result=%u", 0);
        return 0;
    }

    /* Return component info. */
    if ((info[0]->fmtClass != gcvFORMAT_CLASS_RGBA) ||
        ((info[0]->u.rgba.alpha.width & gcvCOMPONENT_DONTCARE) != 0))
    {
        gcmFOOTER_ARG("result=%u", 0);
        return 0;
    }
    else
    {
        gcmFOOTER_ARG("result=%u", info[0]->u.rgba.alpha.width & gcvCOMPONENT_WIDTHMASK);
        return info[0]->u.rgba.alpha.width & gcvCOMPONENT_WIDTHMASK;
    }
}


/*******************************************************************************
**
**  glfSetDefaultMiscStates
**
**  Set default miscellaneous states.
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

gceSTATUS glfSetDefaultMiscStates(
    glsCONTEXT_PTR Context
    )
{
    GLenum result;
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        static gltFRACTYPE vec0000[] =
            { glvFRACZERO, glvFRACZERO, glvFRACZERO, glvFRACZERO };

        /* Default logic operation. */
        glmERR_BREAK(_SetLogicOp(
            Context,
            GL_COPY
            ));

        /* Default perspective hint. */
        Context->perspectiveCorrect = GL_DONT_CARE;

        /* Default color mask. */
        glmERR_BREAK(_SetColorMask(
            Context,
            GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE
            ));

        /* Default clear color. */
        glmERR_BREAK(_SetClearColor(
            Context,
            &vec0000,
            glvFRACTYPEENUM
            ));

        /* Set default dithering. */
        glmERR_BREAK(glfEnableDither(
            Context,
            GL_TRUE
            ));

        /* Disable anti-alias line. */
        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetAntiAliasLine(
            Context->hw,
            gcvFALSE
            )));

        /* Set lastPixel Disable. */
        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetLastPixelEnable(
            Context->hw, gcvFALSE
            )));

        /* Set fill mode to solid. */
        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetFill(
            Context->hw,
            gcvFILL_SOLID
            )));
    }
    while (gcvFALSE);

    status = glmTRANSLATEGLRESULT(result);

    gcmFOOTER();

    return status;
}


/*******************************************************************************
**
**  glfEnableLogicOp
**
**  Enable logic operation.
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

GLenum glfEnableLogicOp(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);
    /* Set the enable flag. */
    Context->logicOp.enabled = Enable;

    /* Determine whether the operation should be performed. */
    result = _UpdateLogicOp(Context);
    gcmFOOTER_ARG("result=0x%04x", result);
    return result;
}


/*******************************************************************************
**
**  glfEnableDither
**
**  Enable dithering.
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

GLenum glfEnableDither(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    GLenum status;
    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);
    Context->dither = Enable;
	if (Context->alphaStates.blendEnabled)
	{
		/* If blending enabled, disable dithering. */
		status = glmTRANSLATEHALSTATUS(gco3D_EnableDither(Context->hw, gcvFALSE));	
	}
	else
	{
    status = glmTRANSLATEHALSTATUS(gco3D_EnableDither(Context->hw, Enable));
	}
    gcmFOOTER_ARG("status=0x%04x", status);
    return status;
}


/*******************************************************************************
**
**  glfEnableMultisampling
**
**  Enable multisampling.
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

gceSTATUS glfEnableMultisampling(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);
    Context->hashKey.hashMultisampleEnabled =
    Context->multisampleStates.enabled = Enable;
    status = gco3D_SetAntiAlias(Context->hw, Enable);
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  glfQueryMiscState
**
**  Queries miscellaneous state values
**  (the ones that I wasn't sure where else to put).
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

GLboolean glfQueryMiscState(
    glsCONTEXT_PTR Context,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result = GL_TRUE;
    gcmHEADER_ARG("Context=0x%x Name=0x%04x Value=0x%x Type=0x%x",
                    Context, Name, Value, Type);

    switch (Name)
    {
    case GL_COLOR_WRITEMASK:
        glfGetFromBoolArray(
            Context->colorMask,
            gcmCOUNTOF(Context->colorMask),
            Value,
            Type
            );
        break;

    case GL_COLOR_CLEAR_VALUE:
        if (Type == glvINT)
        {
            /* Normalize when converting to INT. */
            Type = glvNORM;
        }

        glfGetFromVector4(
            &Context->clearColor,
            Value,
            Type
            );
        break;

    case GL_RED_BITS:
        glfGetFromInt(
            _GetRedBits(Context),
            Value,
            Type
            );
        break;

    case GL_GREEN_BITS:
        glfGetFromInt(
            _GetGreenBits(Context),
            Value,
            Type
            );
        break;

    case GL_BLUE_BITS:
        glfGetFromInt(
            _GetBlueBits(Context),
            Value,
            Type
            );
        break;

    case GL_ALPHA_BITS:
        glfGetFromInt(
            _GetAlphaBits(Context),
            Value,
            Type
            );
        break;

    case GL_PERSPECTIVE_CORRECTION_HINT:
        glfGetFromEnum(
            Context->perspectiveCorrect,
            Value,
            Type
            );
        break;

    case GL_DITHER:
        glfGetFromInt(
            Context->dither,
            Value,
            Type
            );
        break;

    case GL_LOGIC_OP_MODE:
        glfGetFromEnum(
            Context->logicOp.operation,
            Value,
            Type
            );
        break;

    case GL_SUBPIXEL_BITS:
        glfGetFromInt(
            32,
            Value,
            Type
            );
        break;

    case GL_MAX_PALETTE_MATRICES_OES:
        glfGetFromInt(
            glvMAX_PALETTE_MATRICES,
            Value,
            Type
            );
        break;

    case GL_MAX_VERTEX_UNITS_OES:
        glfGetFromInt(
            glvMAX_VERTEX_UNITS,
            Value,
            Type
            );
        break;

    case GL_MAX_CLIP_PLANES:
        glfGetFromInt(
            glvMAX_CLIP_PLANES,
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
***************************** State Management Code ****************************
\******************************************************************************/

/*******************************************************************************
**
**  glColorMask
**
**  glColorMask specifies whether the individual components in the color buffer
**  can or cannot be written. If red is GL_FALSE, for example, no change is made
**  to the red component of any pixel in the color buffer, regardless of the
**  drawing operation attempted, including glClear.
**
**  Changes to individual bits of components cannot be controlled. Rather,
**  changes are either enabled or disabled for entire color components.
**
**  INPUT:
**
**      Red, Green, Blue, Alpha
**          Specify whether red, green, blue, and alpha can or cannot be written
**          into the color buffer. The initial values are all GL_TRUE,
**          indicating that all color components can be written.
**
**  OUTPUT:
**
**      Nothing.
*/
#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_FRAGMENT

GL_API void GL_APIENTRY glColorMask(
    GLboolean Red,
    GLboolean Green,
    GLboolean Blue,
    GLboolean Alpha
    )
{
    glmENTER4(glmARGUINT, Red, glmARGUINT, Green,
             glmARGUINT, Blue, glmARGUINT, Alpha)
    {
		gcmDUMP_API("${ES11 glColorMask 0x%08X 0x%08X 0x%08X 0x%08X}",
			Red, Green, Blue, Alpha);

        glmPROFILE(context, GLES1_COLORMASK, 0);
        glmERROR(_SetColorMask(context, Red, Green, Blue, Alpha));
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glLogicOp
**
**  glLogicOp specifies a logical operation that, when enabled, is applied
**  between the incoming color and the color at the corresponding location
**  in the frame buffer. To enable or disable the logical operation, call
**  glEnable and glDisable with argument GL_COLOR_LOGIC_OP.
**  Logical operation is initially disabled.
**
**  INPUT:
**
**      Opcode
**          Specifies a symbolic constant that selects a logical operation.
**          The following symbols are accepted: GL_CLEAR, GL_SET, GL_COPY,
**          GL_COPY_INVERTED, GL_NOOP, GL_INVERT, GL_AND, GL_NAND, GL_OR,
**          GL_NOR, GL_XOR, GL_EQUIV, GL_AND_REVERSE, GL_AND_INVERTED,
**          GL_OR_REVERSE, and GL_OR_INVERTED. The initial value is GL_COPY.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glLogicOp(
    GLenum Opcode
    )
{
    glmENTER1(glmARGENUM, Opcode)
    {
		gcmDUMP_API("${ES11 glLogicOp 0x%08X}", Opcode);

		glmPROFILE(context, GLES1_LOGICOP, 0);
        /* Validate the code. */
        if ((Opcode < GL_CLEAR) || (Opcode > GL_SET))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Set the new operation. */
        glmERROR(_SetLogicOp(
            context,
            Opcode
            ));
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glHint
**
**  Certain aspects of GL behavior, when there is room for interpretation,
**  can be controlled with hints. A hint is specified with two arguments.
**  'Target' is a symbolic constant indicating the behavior to be controlled,
**  and 'Mode' is another symbolic constant indicating the desired behavior.
**  The initial value for each target is GL_DONT_CARE.
**  'Mode' can be one of the following:
**
**      GL_FASTEST
**          The most efficient option should be chosen.
**
**      GL_NICEST
**          The most correct, or highest quality, option should be chosen.
**
**      GL_DONT_CARE
**          No preference.
**
**  Though the implementation aspects that can be hinted are well defined,
**  the interpretation of the hints depends on the implementation. The hint
**  aspects that can be specified with target, along with suggested semantics,
**  are as follows:
**
**      GL_FOG_HINT
**          Indicates the accuracy of fog calculation. If per-pixel fog
**          calculation is not efficiently supported by the GL implementation,
**          hinting GL_DONT_CARE or GL_FASTEST can result in per-vertex
**          calculation of fog effects.
**
**      GL_LINE_SMOOTH_HINT
**          Indicates the sampling quality of antialiased lines. If a larger
**          filter function is applied, hinting GL_NICEST can result in more
**          pixel fragments being generated during rasterization.
**
**      GL_PERSPECTIVE_CORRECTION_HINT
**          Indicates the quality of color and texture coordinate interpolation.
**          If perspective-corrected parameter interpolation is not efficiently
**          supported by the GL implementation, hinting GL_DONT_CARE or
**          GL_FASTEST can result in simple linear interpolation of colors
**          and/or texture coordinates.
**
**      GL_POINT_SMOOTH_HINT
**          Indicates the sampling quality of antialiased points. If a larger
**          filter function is applied, hinting GL_NICEST can result in more
**          pixel fragments being generated during rasterization.
**
**  INPUT:
**
**      Target
**          Specifies a symbolic constant indicating the behavior to be
**          controlled.
**
**      Mode
**          Specifies a symbolic constant indicating the desired behavior.
**          GL_FASTEST, GL_NICEST, and GL_DONT_CARE are accepted.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_CONTEXT

GL_API void GL_APIENTRY glHint(
    GLenum Target,
    GLenum Mode
    )
{
    glmENTER2(glmARGENUM, Target, glmARGENUM, Mode)
    {
		gcmDUMP_API("${ES11 glHint 0x%08X 0x%08X}", Target, Mode);

		if ( (Mode != GL_FASTEST) &&
             (Mode != GL_NICEST) &&
             (Mode != GL_DONT_CARE) )
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        glmPROFILE(context, GLES1_HINT, 0);

        switch (Target)
        {
        case GL_PERSPECTIVE_CORRECTION_HINT:
            context->perspectiveCorrect = Mode;
            break;

        case GL_POINT_SMOOTH_HINT:
            context->pointStates.hint = Mode;
            break;

        case GL_LINE_SMOOTH_HINT:
            context->lineStates.hint = Mode;
            break;

        case GL_FOG_HINT:
            context->fogStates.hint = Mode;
            break;

        case GL_GENERATE_MIPMAP_HINT:
            context->texture.generateMipmapHint = Mode;
            break;

        default:
            glmERROR(GL_INVALID_ENUM);
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glClearColor
**
**  glClearColor specifies the red, green, blue, and alpha values used by
**  glClear to clear the color buffer. Values specified by glClearColor are
**  clamped to the range [0, 1].
**
**  INPUT:
**
**      Mask
**          Bitwise OR of masks that indicate the buffers to be cleared.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_CLEAR

GL_API void GL_APIENTRY glClearColorx(
    GLclampx Red,
    GLclampx Green,
    GLclampx Blue,
    GLclampx Alpha
    )
{
    glmENTER4(glmARGFIXED, Red, glmARGFIXED, Green,
              glmARGFIXED, Blue, glmARGFIXED, Alpha)
    {
        GLclampx clearColor[4];

		gcmDUMP_API("${ES11 glClearColorx 0x%08X 0x%08X 0x%08X 0x%08X}", Red, Green, Blue, Alpha);

        glmPROFILE(context, GLES1_CLEARCOLORX, 0);

        clearColor[0] = Red;
        clearColor[1] = Green;
        clearColor[2] = Blue;
        clearColor[3] = Alpha;

        glmERROR(_SetClearColor(context, clearColor, glvFIXED));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glClearColorxOES(
    GLclampx Red,
    GLclampx Green,
    GLclampx Blue,
    GLclampx Alpha
    )
{
    glmENTER4(glmARGFIXED, Red, glmARGFIXED, Green,
              glmARGFIXED, Blue, glmARGFIXED, Alpha)
    {
        GLclampx clearColor[4];

		gcmDUMP_API("${ES11 glClearColorxOES 0x%08X 0x%08X 0x%08X 0x%08X}", Red, Green, Blue, Alpha);

        clearColor[0] = Red;
        clearColor[1] = Green;
        clearColor[2] = Blue;
        clearColor[3] = Alpha;

        glmERROR(_SetClearColor(context, clearColor, glvFIXED));
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glClearColor(
    GLclampf Red,
    GLclampf Green,
    GLclampf Blue,
    GLclampf Alpha
    )
{
    glmENTER4(glmARGFLOAT, Red, glmARGFLOAT, Green,
             glmARGFLOAT, Blue, glmARGFLOAT, Alpha)
    {
        GLclampf clearColor[4];

		gcmDUMP_API("${ES11 glClearColor 0x%08X 0x%08X 0x%08X 0x%08X}",
			*(GLuint*)&Red, *(GLuint*)&Green, *(GLuint*)&Blue, *(GLuint*)&Alpha);

        glmPROFILE(context, GLES1_CLEARCOLOR, 0);

        clearColor[0] = Red;
        clearColor[1] = Green;
        clearColor[2] = Blue;
        clearColor[3] = Alpha;

        glmERROR(_SetClearColor(context, clearColor, glvFLOAT));
    }
    glmLEAVE();
}
#endif


/*******************************************************************************
**
**  glfUpdatePrimitveType
**
**  Update primitve type info.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Type
**          Primitive type.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS glfUpdatePrimitveType(
    glsCONTEXT_PTR Context,
    GLenum Type
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    GLboolean ptSizeEnable;

    gcmHEADER();

    /* Invalidate point sprite state. */
    Context->pointStates.spriteDirty = GL_TRUE;

    /* Disable two-sided lighting. */
    Context->hashKey.hashTwoSidedLighting =
    Context->lightingStates.doTwoSidedlighting =
        ((Type == GL_TRIANGLES)      ||
         (Type == GL_TRIANGLE_STRIP) ||
         (Type == GL_TRIANGLE_FAN))
            ? Context->lightingStates.twoSidedLighting
            : GL_FALSE;

    ptSizeEnable = (Type == GL_POINTS) ? GL_TRUE
                                       : GL_FALSE;

    /* Update point states. */
    Context->hashKey.hashPointPrimitive = ptSizeEnable;

    if (Context->pointStates.pointPrimitive != ptSizeEnable)
    {
        Context->pointStates.pointPrimitive = ptSizeEnable;

        /* Proram point size. */
        status = gco3D_SetPointSizeEnable(Context->hw,
                                          Context->pointStates.pointPrimitive);
    }

    gcmFOOTER();
    return status;
}
