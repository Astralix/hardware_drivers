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
*********************** Support Functions and Definitions **********************
\******************************************************************************/

#define glvREADPIXELS_TYPE                  GL_UNSIGNED_BYTE
#define glvREADPIXELS_FORMAT                GL_RGBA
#define glvREADPIXELS_HAL_FORMAT            gcvSURF_A8B8G8R8


/*******************************************************************************
**
**  glfSetDefaultPixelStates
**
**  Set default pixel states.
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

gceSTATUS glfSetDefaultPixelStates(
    glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->packAlignment   = 4;
    Context->unpackAlignment = 4;
    gcmFOOTER_ARG("return=%s", "gcvSTATUS_OK");
    return gcvSTATUS_OK;
}


/*******************************************************************************
**
**  glfQueryPixelState
**
**  Queries pixel state values.
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

GLboolean glfQueryPixelState(
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
    case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
        glfGetFromEnum(
            glvREADPIXELS_FORMAT,
            Value,
            Type
            );
        break;

    case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
        glfGetFromEnum(
            glvREADPIXELS_TYPE,
            Value,
            Type
            );
        break;

    case GL_PACK_ALIGNMENT:
        glfGetFromInt(
            Context->packAlignment,
            Value,
            Type
            );
        break;

    case GL_UNPACK_ALIGNMENT:
        glfGetFromInt(
            Context->unpackAlignment,
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
***************************** Pixel Management Code ****************************
\******************************************************************************/

/*******************************************************************************
**
**  glPixelStore
**
**  glPixelStore sets pixel storage modes that affect the operation of
**  subsequent glReadPixels as well as the unpacking of glTexImage2D, and
**  glTexSubImage2D.
**
**  'Name' is a symbolic constant indicating the parameter to be set, and
**  'Param' is the new value.
**
**  The following storage parameter affects how pixel data is returned to client
**  memory. This value is significant for glReadPixels:
**      GL_PACK_ALIGNMENT
**          Specifies the alignment requirements for the start of each pixel row
**          in memory. The allowable values are 1 (byte-alignment),
**          2 (rows aligned to even-numbered bytes), 4 (word-alignment),
**          and 8 (rows start on double-word boundaries).
**          The initial value is 4.
**
**  The following storage parameter affects how pixel data is read from client
**  memory. This value is significant for glTexImage2D and glTexSubImage2D:
**      GL_UNPACK_ALIGNMENT
**          Specifies the alignment requirements for the start of each pixel row
**          in memory. The allowable values are 1 (byte-alignment),
**          2 (rows aligned to even-numbered bytes), 4 (word-alignment),
**          and 8 (rows start on double-word boundaries).
**          The initial value is 4.
**
**  INPUT:
**
**      Name
**          Specifies the symbolic name of the parameter to be set.
**          GL_PACK_ALIGNMENT affects the packing of pixel data into memory.
**          GL_UNPACK_ALIGNMENT affects the unpacking of pixel data from memory.
**
**      Param
**          Specifies the value that Name is set to.
**
**  OUTPUT:
**
**      Nothing.
*/
#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_PIXEL

GL_API void GL_APIENTRY glPixelStorei(
    GLenum Name,
    GLint Param
    )
{
    glmENTER2(glmARGENUM, Name, glmARGINT, Param)
    {
        gcmDUMP_API("${ES11 glPixelStorei 0x%08X 0x%08X}", Name, Param);

        glmPROFILE(context, GLES1_PIXELSTOREI, 0);
        /* Check the value. */
        if ((Param != 1) && (Param != 2) && (Param != 4) && (Param != 8))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Dispatch on name. */
        switch (Name)
        {
        case GL_PACK_ALIGNMENT:
            context->packAlignment = Param;
            break;

        case GL_UNPACK_ALIGNMENT:
            context->unpackAlignment = Param;
            break;

        default:
            glmERROR(GL_INVALID_ENUM);
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glReadPixels
**
**  glReadPixels returns pixel data from the color buffer, starting with the
**  pixel whose lower left corner is at location (X, Y), into client memory
**  starting at location 'Pixels'. The processing of the pixel data before
**  it is placed into client memory can be controlled with glPixelStore.
**
**  glReadPixels returns values from each pixel with lower left corner at
**  (x + i, y + j) for 0 <= i < Width and 0 <= j < Height. This pixel is said
**  to be the ith pixel in the jth row. Pixels are returned in row order from
**  the lowest to the highest row, left to right in each row.
**
**  'Format' specifies the format of the returned pixel values. GL_RGBA is
**  always accepted, the value of GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES may
**  allow another format:
**
**      GL_RGBA
**          Each color component is converted to floating point such that zero
**          intensity maps to 0 and full intensity maps to 1.
**
**      GL_RGB
**          Each element is an RGB triple. The GL converts it to floating point
**          and assembles it into an RGBA element by attaching 1 for alpha.
**
**      GL_LUMINANCE
**          Each element is a single luminance value. The GL converts it to
**          floating point and assembles it into an RGBA element by replicating
**          the luminance value three times for red, green and blue and
**          attaching 1 for alpha.
**
**      GL_LUMINANCE_ALPHA
**          Each element is a luminance/alpha pair. The GL converts it to
**          floating point and assembles it into an RGBA element by replicating
**          the luminance value three times for red, green and blue.
**
**      GL_ALPHA
**          Each element is a single alpha component. The GL converts it to
**          floating point and assembles it into an RGBA element by attaching 0
**          for red, green and blue.
**
**  Unneeded data is then discarded. For example, GL_ALPHA discards the red,
**  green, and blue components, while GL_RGB discards only the alpha component.
**  GL_LUMINANCE computes a single-component value as the sum of the red, green,
**  and blue components, and GL_LUMINANCE_ALPHA does the same, while keeping
**  alpha as a second value. The final values are clamped to the range [0, 1].
**
**  Finally, the components are converted to the proper, as specified by 'Type'
**  where each component is multiplied by 2**n - 1, where n is the number of bits
**  per component.
**
**  Return values are placed in memory as follows. If format is GL_ALPHA, or
**  GL_LUMINANCE, a single value is returned and the data for the ith pixel in
**  the jth row is placed in location j * width + i. GL_RGB returns three
**  values, GL_RGBA returns four values, and GL_LUMINANCE_ALPHA returns two
**  values for each pixel, with all values corresponding to a single pixel
**  occupying contiguous space in pixels. Storage parameter GL_PACK_ALIGNMENT
**  set by glPixelStore, affects the way that data is written into memory.
**  See glPixelStore for a description.
**
**  INPUT:
**
**      X, Y
**          Specify the window coordinates of the first pixel that is read from
**          the color buffer. This location is the lower left corner of
**          a rectangular block of pixels.
**
**      Width, Height
**          Specify the dimensions of the pixel rectangle. width and height of
**          one correspond to a single pixel.
**
**      Format
**          Specifies the format of the pixel data. Must be either GL_RGBA
**          or the value of GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES.
**
**      Type
**          Specifies the data type of the pixel data.
**          Must be either GL_UNSIGNED_BYTE or the value of
**          GL_IMPLEMENTATION_COLOR_READ_TYPE_OES.
**
**      Pixels
**          Returns the pixel data.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glReadPixels(
    GLint X,
    GLint Y,
    GLsizei Width,
    GLsizei Height,
    GLenum Format,
    GLenum Type,
    GLvoid* Pixels
    )
{
    gceSTATUS status;
    gcoSURF surface;
    gcoSURF target = gcvNULL;

    glmENTER7(glmARGINT, X, glmARGINT, Y, glmARGINT, Width, glmARGINT, Height,
              glmARGENUM, Format, glmARGENUM, Type, glmARGPTR, Pixels)
    {
        gceSURF_FORMAT halFormat = gcvSURF_A8B8G8R8;

        glmPROFILE(context, GLES1_READPIXELS, 0);
        /* Check format and type. */
        if (((Format != GL_RGBA) && (Format != GL_BGRA_EXT) && (Format != GL_BGRA_IMG)) ||
            ((Type != GL_UNSIGNED_BYTE) &&
             (Type != GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT) &&
             (Type != GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT) &&
             (Type != GL_UNSIGNED_SHORT_4_4_4_4_REV_IMG)))
        {
            /* Invalid format. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Check the size of the bitmap to read back. */
        if ((Width < 0) || (Height < 0))
        {
            /* Invalid size. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Check Format/Type combination. */
        if (((Format == GL_RGBA) && (Type != GL_UNSIGNED_BYTE)) ||
            ((Format == GL_BGRA_EXT) &&
             (Type != GL_UNSIGNED_BYTE) &&
             (Type != GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT) &&
             (Type != GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT)) ||
            ((Format == GL_BGRA_IMG) &&
             (Type != GL_UNSIGNED_BYTE) &&
             (Type != GL_UNSIGNED_SHORT_4_4_4_4_REV_IMG)))
        {
            /* Invalid input combination. */
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        if(Format == GL_BGRA_EXT || Format == GL_BGRA_IMG)
        {
            switch(Type)
            {
            case GL_UNSIGNED_BYTE:
                halFormat = gcvSURF_A8R8G8B8;
                break;
            case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
            /*case GL_UNSIGNED_SHORT_4_4_4_4_REV_IMG:*/
                halFormat = gcvSURF_A4R4G4B4;
                break;
            case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
                halFormat = gcvSURF_A1R5G5B5;
                break;
            default:
                break;
            }
        }

        /* Create the wrapper surface. */
        gcmERR_BREAK(gcoSURF_Construct(
            context->hal,
            Width, Height, 1,
            gcvSURF_BITMAP,
            halFormat,
            gcvPOOL_USER,
            &target
            ));

        /* Set the user buffer to the surface. */
        gcmERR_BREAK(gcoSURF_MapUserSurface(
            target, context->packAlignment, Pixels, 0
            ));

        if (context->frameBuffer != gcvNULL)
        {
            surface = glfGetFramebufferSurface(&context->frameBuffer->color);
        }
        else
        {
            surface = context->read;
        }

        /* Update frame buffer. */
        gcmERR_BREAK(glfUpdateFrameBuffer(context));

        /* Read pixels. */
        gcmERR_BREAK(gcoSURF_CopyPixels(
            surface,
            target,
            X, Y,
            0, 0,
            Width, Height
            ));

        gcmDUMP_API("${ES11 glReadPixels 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X (0x%08X)",
            X, Y, Width, Height, Format, Type, Pixels);
        gcmDUMP_API_ARRAY(Pixels, Width * Height);
        gcmDUMP_API("$}");
    }
    glmLEAVE();

    /* Destroy the wrapper surface. */
    if (target != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Destroy(target));
    }
}
