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

static GLint _lineWidthRange[] =
{
    1,  /* Minimum width. */
    1   /* Maximum width. */
};

static GLenum _SetLineWidth(
    glsCONTEXT_PTR Context,
    const GLvoid* LineWidth,
    gleTYPE Type
    )
{
    GLenum result;

    gcmHEADER_ARG("Context=0x%x LineWidth=0x%x Type=0x%04x",
                    Context, LineWidth, Type);

    do
    {
        /* Convert the width value. */
        gltFRACTYPE lineWidth = glfFracFromRaw(LineWidth, Type);

        /* Validate the width. */
        if (lineWidth <= glvFRACZERO)
        {
            result = GL_INVALID_VALUE;
            break;
        }

        /* Make sure the width is within the supported range. */
        if (lineWidth < glmINT2FRAC(_lineWidthRange[0]))
        {
            lineWidth = glmINT2FRAC(_lineWidthRange[0]);
        }

        if (lineWidth > glmINT2FRAC(_lineWidthRange[1]))
        {
            lineWidth = glmINT2FRAC(_lineWidthRange[1]);
        }

        /* Set the new value. */
        glfSetMutant(&Context->lineStates.width, &lineWidth, Type);

        /* Set the query value. */
        glfSetMutant(&Context->lineStates.queryWidth, LineWidth, Type);

        if (_lineWidthRange[1] > 1)
        {
            /* Set the line width for the hardware. */
            glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetAALineWidth(
                Context->hw,
                gcoMATH_Floor(gcoMATH_Add(glmFRAC2FLOAT(lineWidth), 0.5f)))));
        }

        result = GL_NO_ERROR;
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
**  glfSetDefaultLineStates
**
**  Set default line states.
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

gceSTATUS glfSetDefaultLineStates(
    glsCONTEXT_PTR Context
    )
{
    GLenum result;

    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        static gltFRACTYPE value1 = glvFRACONE;

        /* Set default hint. */
        Context->lineStates.hint = GL_DONT_CARE;

        if (gcoHAL_IsFeatureAvailable(Context->hal,
                                      gcvFEATURE_WIDE_LINE) == gcvSTATUS_TRUE)
        {
            /* Wide lines are supported. */
            _lineWidthRange[1] = 8192;

            /* Set the hardware states. */
            glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetAntiAliasLine(
                Context->hw,
                gcvTRUE)));
        }

        /* Set default line width. */
        glmERR_BREAK(_SetLineWidth(Context, &value1, glvFRACTYPEENUM));
    }
    while (gcvFALSE);

    status = glmTRANSLATEGLRESULT(result);

    gcmFOOTER();

    return status;
}


/*******************************************************************************
**
**  glfQueryLineState
**
**  Queries line state values.
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

GLboolean glfQueryLineState(
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
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_SMOOTH_LINE_WIDTH_RANGE:
        glfGetFromIntArray(
            _lineWidthRange,
            gcmCOUNTOF(_lineWidthRange),
            Value,
            Type
            );
        break;

    case GL_LINE_SMOOTH:
        glfGetFromInt(
            Context->lineStates.smooth,
            Value,
            Type
            );
        break;

    case GL_LINE_WIDTH:
        glfGetFromMutant(
            &Context->lineStates.queryWidth,
            Value,
            Type
            );
        break;

    case GL_LINE_SMOOTH_HINT:
        glfGetFromEnum(
            Context->lineStates.hint,
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
****************************** Line Management Code ****************************
\******************************************************************************/

/*******************************************************************************
**
**  glLineWidth
**
**  glLineWidth specifies the rasterized width of both aliased and antialiased
**  lines. Using a line width other than 1 has different effects, depending on
**  whether line antialiasing is enabled. To enable and disable line
**  antialiasing, call glEnable and glDisable with argument GL_LINE_SMOOTH.
**  Line antialiasing is initially disabled.
**
**  If line antialiasing is disabled, the actual width is determined by rounding
**  the supplied width to the nearest integer. (If the rounding results in the
**  value 0, it is as if the line width were 1.) If | ?x | ? | ?y | , i pixels
**  are filled in each column that is rasterized, where i is the rounded value
**  of width. Otherwise, i pixels are filled in each row that is rasterized.
**
**  If antialiasing is enabled, line rasterization produces a fragment for each
**  pixel square that intersects the region lying within the rectangle having
**  width equal to the current line width, length equal to the actual length
**  of the line, and centered on the mathematical line segment. The coverage
**  value for each fragment is the window coordinate area of the intersection
**  of the rectangular region with the corresponding pixel square. This value
**  is saved and used in the final rasterization step.
**
**  Not all widths can be supported when line antialiasing is enabled. If an
**  unsupported width is requested, the nearest supported width is used. Only
**  width 1 is guaranteed to be supported; others depend on the implementation.
**  Likewise, there is a range for aliased line widths as well. To query the
**  range of supported widths and the size difference between supported widths
**  within the range, call glGetInteger with arguments
**  GL_ALIASED_LINE_WIDTH_RANGE, GL_SMOOTH_LINE_WIDTH_RANGE,
**  GL_SMOOTH_LINE_WIDTH_GRANULARITY.
**
**  INPUT:
**
**      Width
**          Specifies the width of rasterized lines. The initial value is 1.
**
**  OUTPUT:
**
**      Nothing.
*/
#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_POLIGON

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glLineWidth(
    GLfloat Width
    )
{
    glmENTER1(glmARGFLOAT, Width)
    {
		gcmDUMP_API("${ES11 glLineWidth 0x%08X}", *(GLuint*)&Width);

        glmPROFILE(context, GLES1_LINEWIDTH, 0);
        glmERROR(_SetLineWidth(context, &Width, glvFLOAT));
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glLineWidthx(
    GLfixed Width
    )
{
    glmENTER1(glmARGFIXED, Width)
    {
		gcmDUMP_API("${ES11 glLineWidthx 0x%08X}", Width);

		glmPROFILE(context, GLES1_LINEWIDTHX, 0);
        glmERROR(_SetLineWidth(context, &Width, glvFIXED));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glLineWidthxOES(
    GLfixed Width
    )
{
    glmENTER1(glmARGFIXED, Width)
    {
		gcmDUMP_API("${ES11 glLineWidthxOES 0x%08X}", Width);

		glmERROR(_SetLineWidth(context, &Width, glvFIXED));
    }
    glmLEAVE();
}
