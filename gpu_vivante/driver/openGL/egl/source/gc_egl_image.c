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




#include "gc_egl_precomp.h"
#include <KHR/khrvivante.h>

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcdZONE_EGL_IMAGE

#if defined(ANDROID)
#include <pixelflinger/format.h>
#include <pixelflinger/pixelflinger.h>

#include <private/ui/android_natives_priv.h>

#include "gc_gralloc_priv.h"
#endif

static VEGLImage
_InitializeImage(
    VEGLThreadData Thread,
    VEGLContext ctx
    )
{
    VEGLImage image;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    /* Allocate the surface structure. */
    status = gcoOS_Allocate(gcvNULL,
                            sizeof(struct eglImage),
                            &pointer);

    if (!gcmIS_SUCCESS(status))
    {
        /* Out of memory. */
        Thread->error = EGL_BAD_ALLOC;
        return gcvNULL;
    }

    image = pointer;

    /* Initialize the image object. */
    image->signature    = EGL_IMAGE_SIGNATURE;
    image->next         = gcvNULL;

    return image;
}

#ifndef __QNXNTO__

static void
_DestroyImage(
    VEGLThreadData Thread,
    VEGLImage Image,
    VEGLDisplay Display,
    EGLBoolean FromTerminate
    )
{
    /* Dereference surface if has. */
    if (Image != gcvNULL)
    {
        if (Image->image.surface != gcvNULL)
        {
            if (Image->image.type == KHR_IMAGE_PIXMAP)
            {
                VEGLImageRef ref = gcvNULL, previous = gcvNULL;
                gctINT reference;

                gcoSURF_QueryReferenceCount(Image->image.surface, &reference);

                /* Find the surface in the reference stack. */
                for (ref = Display->imageRefStack;
                     ref != gcvNULL;
                     ref = ref->next)
                {
                    /* See if the surface matches. */
                    if (ref->surface == Image->image.surface)
                    {
                        break;
                    }

                    /* Save current pointer of the linked list. */
                    previous = ref;
                }

                /* If we have a valid reference and the reference count has
                ** reached 1, we can remove the surface from the reference
                ** stack. */
                if ((ref != gcvNULL) && (reference == 1))
                {
                    /* Unlink the reference from the linked list. */
                    if (previous == gcvNULL)
                    {
                        Display->imageRefStack = ref->next;
                    }
                    else
                    {
                        if (previous->next != ref->next)
                        {
                            previous->next = ref->next;
                        }
                    }

                    /* Free the structure. */
                    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, ref));
                }
            }

            /* Destroy the surface. */
            if (FromTerminate)
            {
                gcoHAL_DestroySurface(gcvNULL, Image->image.surface);
            }
            else
            {
                gcoSURF_Destroy(Image->image.surface);
            }
        }
    }

#if defined(ANDROID)
    /* Clean up android native eglImage. */
    if (Image->image.type == KHR_IMAGE_ANDROID_NATIVE_BUFFER)
    {
        android_native_buffer_t* native =
                (android_native_buffer_t *)Image->image.u.androidNativeBuffer.native;

        /* Decrease native buffer reference count. */
        if (native)
        {
            native->common.decRef(&native->common);
        }
    }
#endif
}

#else

static void
_DestroyImage(
    VEGLThreadData Thread,
    VEGLImage Image
    )
{
    /* Dereference surface if has. */
    if (Image != gcvNULL)
    {
        if (Image->image.surface)
        {
            gcoSURF_Destroy(Image->image.surface);
        }
    }
}

#endif


void
veglDestroyImage(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display,
    IN VEGLImage Image
    )
{
    if (Image != EGL_NO_IMAGE_KHR)
    {
        /* Destroy image. */
#ifndef __QNXNTO__
        _DestroyImage(Thread, Image, Display, EGL_TRUE);
#else
        _DestroyImage(Thread, Image);
#endif

        /* Free the eglImage structure. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Image));
    }
}


VEGLImage _CreateImageTex2D(
    VEGLThreadData Thread,
    VEGLDisplay Dpy,
    VEGLContext Ctx,
    EGLClientBuffer Buffer,
    const EGLint *attrib_list
    )
{
    VEGLImage   image;

    gctINT      texture;
    gctINT      level  = 0;
    gctINT      depth  = 0;

    EGLenum     status;

    /* Test if ctx is null. */
    if (Ctx == gcvNULL)
    {
        Thread->error = EGL_BAD_CONTEXT;
        return EGL_NO_IMAGE_KHR;
    }

    /* Test if ctx is valid */
    if ( (Ctx->thread   != Thread) ||
         (Ctx->api      != Thread->api) ||
         (Ctx->api      != EGL_OPENGL_ES_API) ||
         (Ctx->display  != Dpy))
    {
        Thread->error = EGL_BAD_MATCH;
        return EGL_NO_IMAGE_KHR;
    }

    /* Cast to texture ojbect. */
    texture = gcmPTR2INT(Buffer);

    /* Texture must be nonzero. */
    if (texture == 0)
    {
        Thread->error = EGL_BAD_PARAMETER;
        return EGL_NO_IMAGE_KHR;
    }

    /* Parse the attribute list. */
    if (attrib_list != gcvNULL)
    {
        EGLint i;

        for (i = 0; attrib_list[i] != EGL_NONE; i += 2)
        {
            switch(attrib_list[i])
            {
            case EGL_GL_TEXTURE_LEVEL_KHR:
                level = attrib_list[i + 1];
                break;

            case EGL_IMAGE_PRESERVED_KHR:
                break;

            default:
                /* Ignore unacceptable attributes. */
                gcmTRACE_ZONE(
                    gcvLEVEL_ERROR, gcdZONE_EGL_IMAGE,
                    "%s(%d): Unknown attribute 0x%04X = 0x%04X.",
                    __FUNCTION__, __LINE__,
                    attrib_list[i], attrib_list[i + 1]
                    );
                Thread->error = EGL_BAD_PARAMETER;
                return EGL_NO_IMAGE_KHR;
            }
        }
    }

    if (level < 0)
    {
        Thread->error = EGL_BAD_MATCH;
        return EGL_NO_IMAGE_KHR;
    }

    /* Initialize an image struct. */
    image = _InitializeImage(Thread, Ctx);

    /* Create eglImage from a texture object. */
    status  = _CreateImageTexture(Thread, EGL_GL_TEXTURE_2D_KHR, texture,
                                    level, depth, image);

    /* Clean up if error happen. */
    if (status != EGL_SUCCESS)
    {
        gcmOS_SAFE_FREE(gcvNULL, image);

        Thread->error = status;
        return EGL_NO_IMAGE_KHR;
    }
    return image;
}

VEGLImage _CreateImageTexCube(
    VEGLThreadData Thread,
    VEGLDisplay Dpy,
    VEGLContext Ctx,
    EGLenum Target,
    EGLClientBuffer Buffer,
    const EGLint *attrib_list
    )
{
    VEGLImage   image;

    gctINT      texture;
    gctINT      level  = 0;
    gctINT      depth  = 0;

    EGLenum     status;

    /* Test if ctx is null. */
    if (Ctx == gcvNULL)
    {
        Thread->error = EGL_BAD_CONTEXT;
        return EGL_NO_IMAGE_KHR;
    }

    /* Test if ctx is valid */
    if ( (Ctx->thread   != Thread) ||
         (Ctx->api      != Thread->api) ||
         (Ctx->api      != EGL_OPENGL_ES_API) ||
         (Ctx->display  != Dpy))
    {
        Thread->error = EGL_BAD_MATCH;
        return EGL_NO_IMAGE_KHR;
    }

    /* Cast to texture ojbect. */
    texture = gcmPTR2INT(Buffer);

    /* Texture must be nonzero. */
    if (texture == 0)
    {
        Thread->error = EGL_BAD_PARAMETER;
        return EGL_NO_IMAGE_KHR;
    }

    /* Parse the attribute list. */
    if (attrib_list != gcvNULL)
    {
        while (*attrib_list != EGL_NONE)
        {
            EGLint attribute = *attrib_list++;
            EGLint value     = *attrib_list++;

            switch(attribute)
            {
            case EGL_GL_TEXTURE_LEVEL_KHR:
                level = value;
                break;

            case EGL_IMAGE_PRESERVED_KHR:
                break;

            default:
                /* Ignore unacceptable attributes. */
                gcmTRACE_ZONE(
                    gcvLEVEL_ERROR, gcdZONE_EGL_IMAGE,
                    "%s(%d): Unknown attribute 0x%04X = 0x%04X.",
                    __FUNCTION__, __LINE__,
                    attribute, value
                    );
                Thread->error = EGL_BAD_PARAMETER;
                return EGL_NO_IMAGE_KHR;
            }
        }
    }

    if (level < 0)
    {
        Thread->error = EGL_BAD_MATCH;
        return EGL_NO_IMAGE_KHR;
    }

    /* Initialize an image struct. */
    image = _InitializeImage(Thread, Ctx);

    /* Create eglImage from a texture object. */
    status  = _CreateImageTexture(Thread, Target, texture,
                                    level, depth, image);

    /* Clean up if error happen. */
    if (status != EGL_SUCCESS)
    {
        gcmOS_SAFE_FREE(gcvNULL, image);

        Thread->error = status;
        return EGL_NO_IMAGE_KHR;
    }
    return image;
}

VEGLImage _CreateImagePixmap(
    VEGLThreadData Thread,
    VEGLDisplay Dpy,
    VEGLContext Ctx,
    EGLClientBuffer Buffer,
    const EGLint *attrib_list
    )
{
    VEGLImage        image;
    NativePixmapType pixmap;
    gctUINT          width, height;
    gctUINT          bitsPerPixel;
    gceSURF_FORMAT   format = gcvSURF_UNKNOWN;
    gctPOINTER       address = gcvNULL;
    gctINT           stride;
    gceSTATUS        status;
    gcoSURF          source = gcvNULL;
    VEGLImageRef     ref;

    /* Pixmap require context is EGL_NO_CONTEXT. */
    if (Ctx != EGL_NO_CONTEXT)
    {
        Thread->error = EGL_BAD_PARAMETER;
        return EGL_NO_IMAGE_KHR;
    }

    /* Parse the attribute list. */
    if (attrib_list != gcvNULL)
    {
        while (*attrib_list != EGL_NONE)
        {
            EGLint attribute = *attrib_list++;

            switch(attribute)
            {
            case EGL_IMAGE_PRESERVED_KHR:
                break;

            default:
                /* Ignore unacceptable attributes. */
                gcmTRACE_ZONE(
                    gcvLEVEL_ERROR, gcdZONE_EGL_IMAGE,
                    "%s(%d): Unknown attribute 0x%04X = 0x%04X.",
                    __FUNCTION__, __LINE__,
                    attribute, *attrib_list
                    );
                Thread->error = EGL_BAD_PARAMETER;
                return EGL_NO_IMAGE_KHR;
            }

            attrib_list++;
        }
    }

    /* Cast buffer to native pixmap type. */
    pixmap = (NativePixmapType)(Buffer);

	if (!veglGetPixmapBits(Dpy->hdc,
                           pixmap,
                           &address,
                           &stride,
                           gcvNULL))
    {
        /* Bad pixmap. */
        Thread->error = EGL_BAD_PARAMETER;
        return EGL_NO_IMAGE_KHR;
    }

    if (!veglGetPixmapInfo(Dpy->hdc,
                           pixmap,
                           &width, &height,
                           &bitsPerPixel,
                           &format))
    {
        /* Bad pixmap. */
        Thread->error = EGL_BAD_PARAMETER;
        return EGL_NO_IMAGE_KHR;
    }

    /* Check if has eglImage created from this pixmap. */
    for (ref = Dpy->imageRefStack; ref != gcvNULL; ref = ref->next)
    {
        if ((ref->pixmap  == pixmap)
        &&  (ref->surface != gcvNULL)
        )
        {
            gctINT32 refCount = 0;

            status = gcoSURF_QueryReferenceCount(ref->surface, &refCount);
            if (gcmIS_SUCCESS(status) && (refCount > 1))
            {
                Thread->error = EGL_BAD_ACCESS;
                return EGL_NO_IMAGE_KHR;
            }

            break;
        }
    }

    /* Initialize an image struct. */
    image = _InitializeImage(Thread, Ctx);

    if (!image)
    {
        Thread->error = EGL_BAD_ACCESS;
        return EGL_NO_IMAGE_KHR;
    }

    if (ref == gcvNULL)
    {
        gctPOINTER pointer = gcvNULL;

        status = gcoSURF_Construct(gcvNULL,
                                   width, height, 1,
                                   gcvSURF_TEXTURE, format,
                                   gcvPOOL_DEFAULT,
                                   &image->image.surface);

        if (!gcmIS_SUCCESS(status))
        {
            gcmOS_SAFE_FREE(gcvNULL, image);

            Thread->error = EGL_BAD_ACCESS;
            return EGL_NO_IMAGE_KHR;
        }

        if (address != gcvNULL)
        {
            /* construct a wrap surface for pixmap. */
            status = gcoSURF_Construct(gcvNULL,
                                       width, height, 1,
                                       gcvSURF_BITMAP, format,
                                       gcvPOOL_USER,
                                       &source);

            if (!gcmIS_SUCCESS(status))
            {
                Thread->error = EGL_BAD_ACCESS;
                goto OnError;
            }

            status = gcoSURF_MapUserSurface(source, 0, address, ~0U);

            if (!gcmIS_SUCCESS(status))
            {
                Thread->error = EGL_BAD_ACCESS;
                goto OnError;
            }
        }
        else
        {
            gctPOINTER bits;
            gctBOOL result;
            gctUINT aWidth, aHeight;
            gctINT stride;

            /* construct a temp surface for pixmap. */
            status = gcoSURF_Construct(gcvNULL,
                                       width, height, 1,
                                       gcvSURF_BITMAP, format,
                                       gcvPOOL_DEFAULT,
                                       &source);

            status = gcoSURF_Lock(source,
                                  gcvNULL,
                                  &bits);
            if (!gcmIS_SUCCESS(status))
            {
                Thread->error = EGL_BAD_ACCESS;
                goto OnError;
            }

            status = gcoSURF_GetAlignedSize(source,
                                            &aWidth,
                                            &aHeight,
                                            &stride);
            if (!gcmIS_SUCCESS(status))
            {
                Thread->error = EGL_BAD_ACCESS;
                goto OnError;
            }

            /* Copy context to temp surface. */
            result = veglCopyPixmapBits(
                    Dpy, pixmap,
                    aWidth, aHeight, stride, format, bits
                    );

            status = gcoSURF_Unlock(source, bits);
            if (!gcmIS_SUCCESS(status) || !result)
            {
                Thread->error = EGL_BAD_ACCESS;
                goto OnError;
            }
        }

#ifndef __QNXNTO__
        /* Copy context to image. */
        status = gcoSURF_Resolve(source, image->image.surface);

        if (!gcmIS_SUCCESS(status))
        {
            Thread->error = EGL_BAD_ACCESS;
            goto OnError;
        }

        gcoSURF_Destroy(source);
#else
        gcoSURF_Destroy(image->image.surface);
        image->image.surface = source;
#endif
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  gcmSIZEOF(struct eglImageRef),
                                  &pointer));

        ref = pointer;

        /* Initialize the reference. */
        ref->pixmap  = pixmap;
        ref->surface = image->image.surface;

        /* Push it to the stack. */
        ref->next          = Dpy->imageRefStack;
        Dpy->imageRefStack = ref;
    }
    else
    {
        image->image.surface = ref->surface;
    }

    image->image.magic   = KHR_EGL_IMAGE_MAGIC_NUM;
    image->image.type    = KHR_IMAGE_PIXMAP;

    image->image.u.pixmap.width   = width;
    image->image.u.pixmap.height  = height;
    image->image.u.pixmap.format  = format;
    image->image.u.pixmap.stride  = stride;
    image->image.u.pixmap.address = address;

#ifdef EGL_API_XXX
	/* Reset the sequence NO. */
	image->image.u.pixmap.seqNo = 0;
#endif
    return image;

OnError:
    if (image->image.surface)
    {
        gcoSURF_Destroy(image->image.surface);
    }

    if (source)
    {
        gcoSURF_Destroy(source);
    }

    gcmOS_SAFE_FREE(gcvNULL, image);

    return EGL_NO_IMAGE_KHR;
}

VEGLImage _CreateImageRenderBuffer(
    VEGLThreadData Thread,
    VEGLDisplay Dpy,
    VEGLContext Ctx,
    EGLClientBuffer Buffer,
    const EGLint *attrib_list
    )
{
    VEGLImage   image;
    gctINT      renderbuffer;
    EGLenum     status;

    /* Test if ctx is null. */
    if (Ctx == gcvNULL)
    {
        Thread->error = EGL_BAD_CONTEXT;
        return EGL_NO_IMAGE_KHR;
    }

    /* Test if ctx is valid */
    if ( (Ctx->thread   != Thread) ||
         (Ctx->api      != Thread->api) ||
         (Ctx->api      != EGL_OPENGL_ES_API) ||
         (Ctx->display  != Dpy))
    {
        Thread->error = EGL_BAD_MATCH;
        return EGL_NO_IMAGE_KHR;
    }

    /* Cast to framebuffer name. */
    renderbuffer = gcmPTR2INT(Buffer);

    if (renderbuffer == 0)
    {
        Thread->error = EGL_BAD_MATCH;
        return EGL_NO_IMAGE_KHR;
    }

    /* Parse the attribute list. */
    if (attrib_list != gcvNULL)
    {
        while (*attrib_list != EGL_NONE)
        {
            EGLint attribute = *attrib_list++;

            switch(attribute)
            {
            case EGL_IMAGE_PRESERVED_KHR:
                break;

            default:
                /* Ignore unacceptable attributes. */
                gcmTRACE_ZONE(
                    gcvLEVEL_ERROR, gcdZONE_EGL_IMAGE,
                    "%s(%d): Unknown attribute 0x%04X = 0x%04X.",
                    __FUNCTION__, __LINE__,
                    attribute, *attrib_list
                    );
                Thread->error = EGL_BAD_PARAMETER;
                return EGL_NO_IMAGE_KHR;
            }

            attrib_list++;
        }
    }

    /* Initialize an image struct. */
    image = _InitializeImage(Thread, Ctx);

    /* Create from a framebuffer object. */
    status = _CreateImageFromRenderBuffer(Thread, renderbuffer, image);

    /* Clean up if error happen. */
    if (status != EGL_SUCCESS)
    {
        gcmOS_SAFE_FREE(gcvNULL, image);

        Thread->error = status;
        return EGL_NO_IMAGE_KHR;
    }

    return image;
}

static VEGLImage _CreateImageNativeBufferEx(
    VEGLThreadData Thread,
    VEGLDisplay Dpy,
    VEGLContext Ctx,
    EGLClientBuffer Buffer,
    const EGLint *attrib_list
    )
{
#if defined(ANDROID)
    VEGLImage                   image;
    android_native_buffer_t*    native;
    struct gc_private_handle_t* handle;

    /* Context must be null. */
    if (Ctx != gcvNULL)
    {
        Thread->error = EGL_BAD_CONTEXT;
        return EGL_NO_IMAGE_KHR;
    }

    /* Create short cut of a native buffer. */
    native = (android_native_buffer_t*)Buffer;

    /* Validate native buffer object. */
    if (native->common.magic != ANDROID_NATIVE_BUFFER_MAGIC)
    {
        Thread->error = EGL_BAD_PARAMETER;
        return EGL_NO_IMAGE_KHR;
    }

    /* Validate vative buffer version. */
    if (native->common.version != sizeof(android_native_buffer_t))
    {
        Thread->error = EGL_BAD_PARAMETER;
        return EGL_NO_IMAGE_KHR;
    }

    /* Grab native handle. */
    handle = (struct gc_private_handle_t *)(native->handle);

    /* Initialize an image struct. */
    image = _InitializeImage(Thread, Ctx);

    image->image.magic   = KHR_EGL_IMAGE_MAGIC_NUM;
    image->image.type    = KHR_IMAGE_ANDROID_NATIVE_BUFFER;

    /* Stores image buffer */
    image->image.surface = (handle->surface != 0)
                         ? (gcoSURF) handle->surface
                         : (gcoSURF) handle->resolveSurface;

    image->image.u.androidNativeBuffer.privHandle = (gctPOINTER) handle;

    image->image.u.androidNativeBuffer.native = (gctPOINTER) native;

    /* Keep a reference of native buffer. */
    native->common.incRef(&native->common);

    return image;
#else
    VEGLImage image;
    gcoSURF   surf = (gcoSURF)Buffer;

    if (gcoSURF_IsValid(surf) != gcvSTATUS_TRUE)
    {
        Thread->error = EGL_BAD_PARAMETER;
        return EGL_NO_IMAGE_KHR;
    }

    /* Initialize an image struct. */
    image = _InitializeImage(Thread, Ctx);

    image->image.magic   = KHR_EGL_IMAGE_MAGIC_NUM;
    image->image.type    = KHR_IMAGE_ANDROID_NATIVE_BUFFER;
    /* Use the buffer as the image surface. */
    image->image.surface = (gcoSURF) surf;

    return image;
#endif
}

VEGLImage _CreateImageVGParent(
    VEGLThreadData Thread,
    VEGLDisplay Dpy,
    VEGLContext Ctx,
    EGLClientBuffer Buffer,
    const EGLint *attrib_list
    )
{
    VEGLImage       image;
    unsigned int    vgimage_obj;
    EGLenum         status;

    /* Test if ctx is null. */
    if (Ctx == gcvNULL)
    {
        Thread->error = EGL_BAD_CONTEXT;
        return EGL_NO_IMAGE_KHR;
    }

    /* Test if ctx is a valid EGL context. */
    if ( (Ctx->thread   != Thread) ||
         (Ctx->api      != Thread->api) ||
         (Ctx->display  != Dpy))
    {
        Thread->error = EGL_BAD_CONTEXT;
        return EGL_NO_IMAGE_KHR;
    }

    /* Test if ctx is a valid OpenVG context. */
    if ((Ctx->api != EGL_OPENVG_API))
    {
        Thread->error = EGL_BAD_MATCH;
        return EGL_NO_IMAGE_KHR;
    }


    /* Cast to a vgImage object handle. */
    vgimage_obj = gcmPTR2INT(Buffer);

    if (vgimage_obj == 0)
    {
        Thread->error = EGL_BAD_PARAMETER;
        return EGL_NO_IMAGE_KHR;
    }

    /* Parse the attribute list. */
    if (attrib_list != gcvNULL)
    {
        while (*attrib_list != EGL_NONE)
        {
            EGLint attribute = *attrib_list++;

            switch(attribute)
            {
            case EGL_IMAGE_PRESERVED_KHR:
                break;

            default:
                /* Ignore unacceptable attributes. */
                gcmTRACE_ZONE(
                    gcvLEVEL_ERROR, gcdZONE_EGL_IMAGE,
                    "%s(%d): Unknown attribute 0x%04X = 0x%04X.",
                    __FUNCTION__, __LINE__,
                    attribute, *attrib_list
                    );

                Thread->error = EGL_BAD_PARAMETER;
                return EGL_NO_IMAGE_KHR;
            }

            attrib_list++;
        }
    }

    /* Initialize an image struct. */
    image = _InitializeImage(Thread, Ctx);

    /* Create from a openvg image. */
    status = _CreateImageFromVGParentImage(Thread, vgimage_obj, image);

    /* Clean up if error happen. */
    if (status != EGL_SUCCESS)
    {
        gcmOS_SAFE_FREE(gcvNULL, image);

        Thread->error = status;
        return EGL_NO_IMAGE_KHR;
    }

    return image;
}

EGLAPI EGLImageKHR EGLAPIENTRY
eglCreateImageKHR (
    EGLDisplay Dpy,
    EGLContext Ctx,
    EGLenum Target,
    EGLClientBuffer Buffer,
    const EGLint *attrib_list
    )
{
    VEGLThreadData  thread;
    VEGLDisplay     dpy;
    VEGLContext     ctx = gcvNULL;
    VEGLImage       image;

    gcmHEADER_ARG("Dpy=0x%08x Ctx=0x%08x Target=0x%04x Buffer=0x%08x attrib_list=0x%08x",
                    Dpy, Ctx, Target, Buffer, attrib_list);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_NO_IMAGE_KHR);
        return EGL_NO_IMAGE_KHR;
    }

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    if ( (dpy == gcvNULL) ||
         (dpy->signature != EGL_DISPLAY_SIGNATURE) )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_NO_IMAGE_KHR);
        return EGL_NO_IMAGE_KHR;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_NO_IMAGE_KHR);
        return EGL_NO_IMAGE_KHR;
    }

    /* Get context, context may be NULL. */
    if (Ctx == EGL_NO_CONTEXT)
    {
        ctx = EGL_NO_CONTEXT;
    }
    else
    {
        for (ctx = dpy->contextStack; ctx != EGL_NO_CONTEXT; ctx = ctx->next)
        {
            if (ctx == VEGL_CONTEXT(Ctx))
            {
                break;
            }
        }

        if (ctx == gcvNULL)
        {
            thread->error = EGL_BAD_CONTEXT;
            gcmFOOTER_ARG("return=%d", EGL_NO_IMAGE_KHR);
            return EGL_NO_IMAGE_KHR;
        }
    }

    /* Create eglImge by target. */
    switch (Target)
    {
    case EGL_GL_TEXTURE_2D_KHR:
        image = _CreateImageTex2D(thread, dpy, ctx, Buffer, attrib_list);
        break;

    case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:
    case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:
    case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:
    case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:
    case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:
    case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:
        image = _CreateImageTexCube(thread, dpy, ctx, Target, Buffer, attrib_list);
        break;

    case EGL_NATIVE_PIXMAP_KHR:
        image = _CreateImagePixmap(thread, dpy, ctx, Buffer, attrib_list);
        break;

    case EGL_GL_RENDERBUFFER_KHR:
        image = _CreateImageRenderBuffer(thread, dpy, ctx, Buffer, attrib_list);
        break;

    case EGL_NATIVE_BUFFER_ANDROID:
        image = _CreateImageNativeBufferEx(thread, dpy, ctx, Buffer, attrib_list);
        break;

    case EGL_VG_PARENT_IMAGE_KHR:
        image = _CreateImageVGParent(thread, dpy, ctx, Buffer, attrib_list);
        break;
    default:
        /* Not a valid target type. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=%d", EGL_NO_IMAGE_KHR);
        return EGL_NO_IMAGE_KHR;
    }

    /* Test if create successful. */
    if (image == EGL_NO_IMAGE_KHR)
    {
        gcmFOOTER_ARG("return=%d", EGL_NO_IMAGE_KHR);
        return EGL_NO_IMAGE_KHR;
    }

    /* Reference the image surface if it has. */
    if (image->image.surface)
    {
        gcoSURF_ReferenceSurface(image->image.surface);
    }

    /* Push image into stack. */
    if (image->next != gcvNULL)
    {
        VEGLImage img = image;
        while (img->next != gcvNULL)
        {
            img = img->next;
        }
        img->next = dpy->imageStack;
    }
    else
    {
        image->next = dpy->imageStack;
    }
    dpy->imageStack = image;

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglCreateImageKHR 0x%08X 0x%08X 0x%08X 0x%08X (0x%08X) "
                ":= 0x%08X",
                Dpy, Ctx, Target, Buffer, attrib_list, image);
    gcmDUMP_API_ARRAY_TOKEN(attrib_list, EGL_NONE);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("return=0x%x", image);
    return (EGLImageKHR) image;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyImageKHR(
    EGLDisplay Dpy,
    EGLImageKHR Image
    )
{
    VEGLThreadData  thread;
    VEGLDisplay     dpy;
    VEGLImage       image;
    VEGLImage       stack;

    gcmHEADER_ARG("Dpy=0x%x Image=0x%x", Dpy, Image);
    gcmDUMP_API("${EGL eglDestroyImageKHR 0x%08X 0x%08X}", Dpy, Image);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    if ( (dpy == gcvNULL) ||
         (dpy->signature != EGL_DISPLAY_SIGNATURE) )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Get shortcut of the eglImage. */
    image = VEGL_IMAGE(Image);

    /* Test if eglImage is valid. */
    if ((image == gcvNULL) ||
        (image->signature != EGL_IMAGE_SIGNATURE))
    {
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Pop EGLImage from the stack. */
    if (image == dpy->imageStack)
    {
        /* Simple - it is the top of the stack. */
        dpy->imageStack = image->next;
    }
    else
    {
        /* Walk the stack. */
        for (stack = dpy->imageStack;
             stack != gcvNULL;
             stack = stack->next)
        {
            /* Check if the next image on the stack is ours. */
            if (stack->next == image)
            {
                /* Pop the image from the stack. */
                stack->next = image->next;
                break;
            }
        }
    }

    /* Destroy image. */
#ifndef __QNXNTO__
    _DestroyImage(thread, image, dpy, EGL_FALSE);
#else
    _DestroyImage(thread, image);
#endif

    /* Free the eglImage structure. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, image));

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;
}
