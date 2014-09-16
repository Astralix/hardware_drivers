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


#include "gc_glsh_precomp.h"

GLboolean
_glshCalculateArea(
    GLint *pdx, GLint *pdy,
    GLint *psx, GLint *psy,
    GLint *pw, GLint *ph,
    GLint dstW, GLint dstH,
    GLint srcW, GLint srcH
    )
{
    gctINT32 srcsx, srcex, dstsx, dstex;
    gctINT32 srcsy, srcey, dstsy, dstey;

    gctINT32 dx = *pdx, dy = *pdy, sx = *psx, sy = *psy, w = *pw, h = *ph;

    gcmHEADER_ARG("pdx=0x%x pdy=0x%x psx=0x%x psy=0x%x pw=0x%x ph=0x%x dstW=%d dstH=%d srcW=%d srcH=%d",
        pdx, pdy, psx, psy, pw, ph, dstW, dstH, srcW, srcH);

    sx = gcmMIN(gcmMAX(sx, (gctINT32)(INT32_MIN>>2)), (gctINT32)(INT32_MAX>>2));
    sy = gcmMIN(gcmMAX(sy, (gctINT32)(INT32_MIN>>2)), (gctINT32)(INT32_MAX>>2));
    dx = gcmMIN(gcmMAX(dx, (gctINT32)(INT32_MIN>>2)), (gctINT32)(INT32_MAX>>2));
    dy = gcmMIN(gcmMAX(dy, (gctINT32)(INT32_MIN>>2)), (gctINT32)(INT32_MAX>>2));
    w = gcmMIN(w, (gctINT32)(INT32_MAX>>2));
    h = gcmMIN(h, (gctINT32)(INT32_MAX>>2));

    srcsx = sx;
    srcex = sx + w;
    dstsx = dx;
    dstex = dx + w;

    if(srcsx < 0)
    {
        dstsx -= srcsx;
        srcsx = 0;
    }
    if(srcex > srcW)
    {
        dstex -= srcex - srcW;
        srcex = srcW;
    }
    if(dstsx < 0)
    {
        srcsx -= dstsx;
        dstsx = 0;
    }
    if(dstex > dstW)
    {
        srcex -= dstex - dstW;
        dstex = dstW;
    }

    gcmASSERT(srcsx >= 0 && dstsx >= 0 && srcex <= srcW && dstex <= dstW);
    w = srcex - srcsx;
    gcmASSERT(w == dstex - dstsx);

    if(w <= 0)
    {
        gcmFOOTER_ARG("return=%s", "FALSE");

        return gcvFALSE;
    }

    srcsy = sy;
    srcey = sy + h;
    dstsy = dy;
    dstey = dy + h;

    if(srcsy < 0)
    {
        dstsy -= srcsy;
        srcsy = 0;
    }
    if(srcey > srcH)
    {
        dstey -= srcey - srcH;
        srcey = srcH;
    }
    if(dstsy < 0)
    {
        srcsy -= dstsy;
        dstsy = 0;
    }
    if(dstey > dstH)
    {
        srcey -= dstey - dstH;
        dstey = dstH;
    }
    gcmASSERT(srcsy >= 0 && dstsy >= 0 && srcey <= srcH && dstey <= dstH);
    h = srcey - srcsy;
    gcmASSERT(h == dstey - dstsy);

    if(h <= 0)
    {
        gcmFOOTER_ARG("return=%s", "FALSE");

        return gcvFALSE;
    }

    *pdx = dstsx;
    *pdy = dstsy;
    *psx = srcsx;
    *psy = srcsy;
    *pw  = w;
    *ph  = h;

    gcmFOOTER_ARG("return=%s", "TRUE");

    return gcvTRUE;
}

GL_APICALL void GL_APIENTRY
glReadPixels(
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height,
    GLenum format,
    GLenum type,
    void * pixels
    )
{
#if gcdNULL_DRIVER < 3
    /*GLContext context;*/
    gcoSURF target = gcvNULL, surface;
    gceSTATUS status;
    gceSURF_FORMAT wrapformat = gcvSURF_A8B8G8R8;
    GLint right, bottom;
    gctUINT w, h;
    GLint dx, dy, sx, sy, Width, Height;
    gctUINT dstWidth, dstHeight;

	glmENTER7(glmARGINT, x, glmARGINT, y, glmARGLINT, width, glmARGLINT, height, glmARGENUM, format, glmARGENUM, type, glmARGPTR, pixels)
	{

    glmPROFILE(context, GLES2_READPIXELS, 0);

    do
    {
#if gldFBO_DATABASE
        if ((context->framebuffer != gcvNULL)
            &&
            (context->framebuffer->currentDB != gcvNULL)
            )
        {
            /* Play current database. */
            if (gcmIS_ERROR(glshPlayDatabase(context,
                                             context->framebuffer->currentDB)))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                glmERROR_BREAK();
            }
        }
#endif

        /* Check the size of the bitmap to read back. */
        if ((width < 0) || (height < 0))
        {
            /* Invalid size. */
            gl2mERROR(GL_INVALID_VALUE);
            break;
        }

        /* Check format and type. */
        if (format != GL_RGBA &&
            format != GL_BGRA_EXT &&
            format != GL_BGRA_IMG)
        {
            /* Invalid format. */
            gl2mERROR(GL_INVALID_OPERATION);
            break;
        }
        else if(format == GL_RGBA && type != GL_UNSIGNED_BYTE)
        {
            /* Invalid type. */
            gl2mERROR(GL_INVALID_OPERATION);
            break;
        }
        else if(format == GL_BGRA_EXT || format == GL_BGRA_IMG)
        {
            if(type != GL_UNSIGNED_BYTE &&
               type != GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT &&
               type != GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT &&
               type != GL_UNSIGNED_SHORT_4_4_4_4_REV_IMG)
            {
                /* Invalid type. */
                gl2mERROR(GL_INVALID_OPERATION);
                break;
            }

            switch(type)
            {
            case GL_UNSIGNED_BYTE:
                wrapformat = gcvSURF_A8R8G8B8;
                break;
            case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
            /*case GL_UNSIGNED_SHORT_4_4_4_4_REV_IMG:*/
                wrapformat = gcvSURF_A4R4G4B4;
                break;
            case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
                wrapformat = gcvSURF_A1R5G5B5;
                break;
            }
        }

        /* Create the wrapper surface. */
        gcmERR_BREAK(
            gcoSURF_Construct(context->hal,
                              width, height, 1,
                              gcvSURF_BITMAP,
                              wrapformat,
                              gcvPOOL_USER,
                              &target));

        /* Set the user buffer to the surface. */
        gcmERR_BREAK(
            gcoSURF_MapUserSurface(target, context->packAlignment, pixels, 0));

        /* Read pixels. */
        if (context->framebuffer != gcvNULL)
        {
            surface = (context->framebuffer->color.object == gcvNULL)
                    ? gcvNULL
                    : _glshGetFramebufferSurface(&context->framebuffer->color);
        }
        else
        {
            surface = context->read;
        }

        gcmERR_BREAK(gcoSURF_GetSize(surface, &w, &h, gcvNULL));
        right  = gcmMIN(x + width,  (gctINT) w);
        bottom = gcmMIN(y + height, (gctINT) h);
        gcmERR_BREAK(gcoSURF_GetSize(target, &dstWidth, &dstHeight, gcvNULL));

        sx = x;
        sy = y;
        dx = 0;
        dy = 0;
        Width = right - x;
        Height = bottom - y;

        if (!_glshCalculateArea(&dx, &dy, &sx, &sy, &Width, &Height, dstWidth, dstHeight, w, h))
        {
            gl2mERROR(GL_INVALID_VALUE);
            break;
        }

        /* Update frame buffer. */
        _glshFrameBuffer(context);

        gcmERR_BREAK(gcoSURF_CopyPixels(surface,
                                        target,
                                        sx, sy,
                                        dx, dy,
                                        Width, Height));

        gcmDUMP_API("${ES20 glReadPixels 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X "
                    "(0x%08X)",
                    x, y, width, height, format, type, pixels);
        gcmDUMP_API_ARRAY(pixels, width * height);
        gcmDUMP_API("$}");
    }
    while (gcvFALSE);

    /* Destroy the wrapper surface. */
    if (target != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_Destroy(target));
    }
	}
	glmLEAVE();

#endif
}
