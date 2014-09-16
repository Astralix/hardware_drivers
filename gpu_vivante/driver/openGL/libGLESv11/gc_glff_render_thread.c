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


/*******************************************************************************
 *
 *  This file contains the thread dispatcher when using pre-frame render
 *  threads. It is makeing extensive use of macros to easy the maintainability.
 *  There is now only one file that needs to be maintained - gc_glff_fuctions.h.
 *  This file contais all function names and argument types, and the macros used
 *  here are "parsing" this file multiple times.
 *
 ******************************************************************************/

#include "gc_glff_precomp.h"

#if gcdRENDER_THREADS


/* Dispatch synchronization types. */
typedef enum gleDISPATCH_SYNC
{
    gl_sync_ASYNC,
    gl_sync_SYNC,
    gl_sync_SHARED,
}
gleDISPATCH_SYNC;

/* This is a list of function prototypes that the dispatcher can call, depending
 * on the number of arguments. */
typedef GL_API GLvoid * (GL_APIENTRY *gltFUNC_0) (void);
typedef GL_API GLvoid * (GL_APIENTRY *gltFUNC_1) (GLuint);
typedef GL_API GLvoid * (GL_APIENTRY *gltFUNC_2) (GLuint, GLuint);
typedef GL_API GLvoid * (GL_APIENTRY *gltFUNC_3) (GLuint, GLuint, GLuint);
typedef GL_API GLvoid * (GL_APIENTRY *gltFUNC_4) (GLuint, GLuint, GLuint,
                                                  GLuint);
typedef GL_API GLvoid * (GL_APIENTRY *gltFUNC_5) (GLuint, GLuint, GLuint,
                                                  GLuint, GLuint);
typedef GL_API GLvoid * (GL_APIENTRY *gltFUNC_6) (GLuint, GLuint, GLuint,
                                                  GLuint, GLuint, GLuint);
typedef GL_API GLvoid * (GL_APIENTRY *gltFUNC_7) (GLuint, GLuint, GLuint,
                                                  GLuint, GLuint, GLuint,
                                                  GLuint);
typedef GL_API GLvoid * (GL_APIENTRY *gltFUNC_8) (GLuint, GLuint, GLuint,
                                                  GLuint, GLuint, GLuint,
                                                  GLuint, GLuint);
typedef GL_API GLvoid * (GL_APIENTRY *gltFUNC_9) (GLuint, GLuint, GLuint,
                                                  GLuint, GLuint, GLuint,
                                                  GLuint, GLuint, GLuint);

/* Process one message from the render queue. */
void glfRenderThread(IN gcsRENDER_THREAD* ThreadInfo)
{
    gcsRENDER_MESSAGE *msg;
    gctUINT i;

    /* Get the current message. */
    msg = &ThreadInfo->queue[ThreadInfo->consumer];

    /* Dispatch on the numkber of arguments. */
    switch (msg->argCount)
    {
        case 0:
            /* No arguments. */
            msg->output.p = (*(gltFUNC_0) msg->function) ();
            break;

        case 1:
            /* One argument. */
            msg->output.p = (*(gltFUNC_1) msg->function)(msg->arg[0].u);
            break;

        case 2:
            /* Two arguments. */
            msg->output.p = (*(gltFUNC_2) msg->function)(msg->arg[0].u,
                                                         msg->arg[1].u);
            break;

        case 3:
            /* Three arguments. */
            msg->output.p = (*(gltFUNC_3) msg->function)(msg->arg[0].u,
                                                         msg->arg[1].u,
                                                         msg->arg[2].u);
            break;

        case 4:
            /* Four arguments. */
            if (msg->function == gcvNULL)
            {
                /* Special case for calling glfSetContext. */
                msg->output.u = glfSetContext(msg->arg[0].p,
                                              msg->arg[1].p,
                                              msg->arg[2].p,
                                              msg->arg[3].p);
            }
            else
            {
                msg->output.p = (*(gltFUNC_4) msg->function)(msg->arg[0].u,
                                                             msg->arg[1].u,
                                                             msg->arg[2].u,
                                                             msg->arg[3].u);
            }
            break;

        case 5:
            /* Five arguments. */
            msg->output.p = (*(gltFUNC_5) msg->function)(msg->arg[0].u,
                                                         msg->arg[1].u,
                                                         msg->arg[2].u,
                                                         msg->arg[3].u,
                                                         msg->arg[4].u);
            break;

        case 6:
            /* Six arguments. */
            msg->output.p = (*(gltFUNC_6) msg->function)(msg->arg[0].u,
                                                         msg->arg[1].u,
                                                         msg->arg[2].u,
                                                         msg->arg[3].u,
                                                         msg->arg[4].u,
                                                         msg->arg[5].u);
            break;

        case 7:
            /* Seven arguments. */
            msg->output.p = (*(gltFUNC_7) msg->function)(msg->arg[0].u,
                                                         msg->arg[1].u,
                                                         msg->arg[2].u,
                                                         msg->arg[3].u,
                                                         msg->arg[4].u,
                                                         msg->arg[5].u,
                                                         msg->arg[6].u);
            break;

        case 8:
            /* Eight arguments. */
            msg->output.p = (*(gltFUNC_8) msg->function)(msg->arg[0].u,
                                                         msg->arg[1].u,
                                                         msg->arg[2].u,
                                                         msg->arg[3].u,
                                                         msg->arg[4].u,
                                                         msg->arg[5].u,
                                                         msg->arg[6].u,
                                                         msg->arg[7].u);
            break;

        case 9:
            /* Nine arguments. */
            msg->output.p = (*(gltFUNC_9) msg->function)(msg->arg[0].u,
                                                         msg->arg[1].u,
                                                         msg->arg[2].u,
                                                         msg->arg[3].u,
                                                         msg->arg[4].u,
                                                         msg->arg[5].u,
                                                         msg->arg[6].u,
                                                         msg->arg[7].u,
                                                         msg->arg[8].u);
            break;
    }

    /* Increment the consumer index. */
    ThreadInfo->consumer = (ThreadInfo->consumer + 1) % gcdRENDER_MESSAGES;

    /* Loop through any allocated memory. */
    for (i = 0; i < 2; i++)
    {
        /* Test for any allocation. */
        if (msg->bytes[i] != 0)
        {
            /* Test for overflow of the memory buffer. */
            if (ThreadInfo->consumerOffset + msg->bytes[i] > gcdRENDER_MEMORY)
            {
                /* Reset the memory pointer. */
                ThreadInfo->consumerOffset = msg->bytes[i];
            }
            else
            {
                /* Adjust the memory pointer. */
                ThreadInfo->consumerOffset += msg->bytes[i];
            }
        }
    }
}

/* Send the function and its args to the render ThreadInfo. */
static void glffDispatch(IN gleDISPATCH_SYNC Sync)
{
    /* Get the current render thread. */
    gcsRENDER_THREAD *thread = eglfGetCurrentRenderThread();

    if (thread->signalGo == NULL)
    {
        /* Increment the producer index. */
        thread->producer = (thread->producer + 1) % gcdRENDER_MESSAGES;

        /* Call the render thread directly. */
        glfRenderThread(thread);
    }
    else
    {
        if (Sync == gl_sync_SHARED)
        {
            /* Wait for all render threads to be finished. */
            eglfWaitRenderThread(gcvNULL);
        }

        /* Increment the producer index. */
        thread->producer = (thread->producer + 1) % gcdRENDER_MESSAGES;

        /* Wake up the render thread. */
        gcoOS_Signal(gcvNULL, thread->signalGo, gcvTRUE);

        if (Sync != gl_sync_ASYNC)
        {
            /* Wait until the render thread is finished. */
            eglfWaitRenderThread(thread);
        }
    }
}


#include "GLES/glunname.h"

/* Missing typedefs for our dispatch types macros. */
typedef GLboolean *                     GLboolean_P;
typedef const GLfixed *                 GLfixed_CP;
typedef GLfixed *                       GLfixed_P;
typedef const GLfloat *                 GLfloat_CP;
typedef GLfloat *                       GLfloat_P;
typedef const GLint *                   GLint_CP;
typedef GLint *                         GLint_P;
typedef const GLshort *                 GLshort_CP;
typedef const GLsizei *                 GLsizei_CP;
typedef GLsizei *                       GLsizei_P;
typedef const GLubyte *                 GLubyte_CP;
typedef const GLuint *                  GLuint_CP;
typedef GLuint *                        GLuint_P;
typedef const GLvoid *                  GLvoid_CP;
typedef const GLvoid **                 GLvoid_CPP;
typedef GLvoid *                        GLvoid_P;
typedef GLvoid **                       GLvoid_PP;

/* Macros that put a dispatch variable on the stack. */
#define glmPUT_bitfield(_i, _var)       msg->arg[_i].u = _var
#define glmPUT_boolean(_i, _var)        msg->arg[_i].u = _var
#define glmPUT_boolean_P(_i, _var)      msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_clampf(_i, _var)         msg->arg[_i].f = _var
#define glmPUT_clampx(_i, _var)         msg->arg[_i].i = _var
#define glmPUT_eglImageOES(_i, _var)    msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_enum(_i, _var)           msg->arg[_i].u = _var
#define glmPUT_fixed(_i, _var)          msg->arg[_i].i = _var
#define glmPUT_fixed_CP(_i, _var)       msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_fixed_P(_i, _var)        msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_float(_i, _var)          msg->arg[_i].f = _var
#define glmPUT_float_CP(_i, _var)       msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_float_P(_i, _var)        msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_int(_i, _var)            msg->arg[_i].i = _var
#define glmPUT_int_CP(_i, _var)         msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_int_P(_i, _var)          msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_intptr(_i, _var)         msg->arg[_i].i = _var
#define glmPUT_short(_i, _var)          msg->arg[_i].i = _var
#define glmPUT_short_CP(_i, _var)       msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_sizei(_i, _var)          msg->arg[_i].i = _var
#define glmPUT_sizei_CP(_i, _var)       msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_sizei_P(_i, _var)        msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_sizeiptr(_i, _var)       msg->arg[_i].i = _var
#define glmPUT_ubyte(_i, _var)          msg->arg[_i].u = _var
#define glmPUT_ubyte_CP(_i, _var)       msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_uint(_i, _var)           msg->arg[_i].u = _var
#define glmPUT_uint_CP(_i, _var)        msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_uint_P(_i, _var)         msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_void_CP(_i, _var)        msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_void_CPP(_i, _var)       msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_void_P(_i, _var)         msg->arg[_i].p = (GLvoid *) _var
#define glmPUT_void_PP(_i, _var)        msg->arg[_i].p = (GLvoid *) _var

/* These macros define the API entry functions that call the dispatcher with all
 * the proper information already setup. */
#define glmDECLARE(_name, _sync) \
    GL_API void GL_APIENTRY gl##_name(void) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 0; \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(); \
        } \
    }

#define glmDECLARE_1(_name, _sync, _type1) \
    GL_API void GL_APIENTRY gl##_name(GL##_type1 _1) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 1; \
            glmPUT_##_type1(0, _1); \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(_1); \
        } \
    }

#define glmDECLARE_2(_name, _sync, _type1, _type2) \
    GL_API void GL_APIENTRY gl##_name(GL##_type1 _1, \
                                      GL##_type2 _2) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 2; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _2); \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(_1, _2); \
        } \
    }

#define glmDECLARE_3(_name, _sync, _type1, _type2, _type3) \
    GL_API void GL_APIENTRY gl##_name(GL##_type1 _1, \
                                      GL##_type2 _2, \
                                      GL##_type3 _3) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 3; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _2); \
            glmPUT_##_type3(2, _3); \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(_1, _2, _3); \
        } \
    }

#define glmDECLARE_4(_name, _sync, _type1, _type2, _type3, _type4) \
    GL_API void GL_APIENTRY gl##_name(GL##_type1 _1, \
                                      GL##_type2 _2, \
                                      GL##_type3 _3, \
                                      GL##_type4 _4) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 4; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _2); \
            glmPUT_##_type3(2, _3); \
            glmPUT_##_type4(3, _4); \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(_1, _2, _3, _4); \
        } \
    }

#define glmDECLARE_5(_name, _sync, _type1, _type2, _type3, _type4, _type5) \
    GL_API void GL_APIENTRY gl##_name(GL##_type1 _1, \
                                      GL##_type2 _2, \
                                      GL##_type3 _3, \
                                      GL##_type4 _4, \
                                      GL##_type5 _5) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 5; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _2); \
            glmPUT_##_type3(2, _3); \
            glmPUT_##_type4(3, _4); \
            glmPUT_##_type5(4, _5); \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(_1, _2, _3, _4, _5); \
        } \
    }

#define glmDECLARE_6(_name, _sync, _type1, _type2, _type3, _type4, _type5, \
                     _type6) \
    GL_API void GL_APIENTRY gl##_name(GL##_type1 _1, \
                                      GL##_type2 _2, \
                                      GL##_type3 _3, \
                                      GL##_type4 _4, \
                                      GL##_type5 _5, \
                                      GL##_type6 _6) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 6; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _2); \
            glmPUT_##_type3(2, _3); \
            glmPUT_##_type4(3, _4); \
            glmPUT_##_type5(4, _5); \
            glmPUT_##_type6(5, _6); \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(_1, _2, _3, _4, _5, _6); \
        } \
    }

#define glmDECLARE_7(_name, _sync, _type1, _type2, _type3, _type4, _type5, \
                     _type6, _type7) \
    GL_API void GL_APIENTRY gl##_name(GL##_type1 _1, \
                                      GL##_type2 _2, \
                                      GL##_type3 _3, \
                                      GL##_type4 _4, \
                                      GL##_type5 _5, \
                                      GL##_type6 _6, \
                                      GL##_type7 _7) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 7; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _2); \
            glmPUT_##_type3(2, _3); \
            glmPUT_##_type4(3, _4); \
            glmPUT_##_type5(4, _5); \
            glmPUT_##_type6(5, _6); \
            glmPUT_##_type7(6, _7); \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(_1, _2, _3, _4, _5, _6, _7); \
        } \
    }

#define glmDECLARE_8(_name, _sync, _type1, _type2, _type3, _type4, _type5, \
                     _type6, _type7, _type8) \
    GL_API void GL_APIENTRY gl##_name(GL##_type1 _1, \
                                      GL##_type2 _2, \
                                      GL##_type3 _3, \
                                      GL##_type4 _4, \
                                      GL##_type5 _5, \
                                      GL##_type6 _6, \
                                      GL##_type7 _7, \
                                      GL##_type8 _8) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 8; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _2); \
            glmPUT_##_type3(2, _3); \
            glmPUT_##_type4(3, _4); \
            glmPUT_##_type5(4, _5); \
            glmPUT_##_type6(5, _6); \
            glmPUT_##_type7(6, _7); \
            glmPUT_##_type8(7, _8); \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(_1, _2, _3, _4, _5, _6, _7, _8); \
        } \
    }

#define glmDECLARE_9(_name, _sync, _type1, _type2, _type3, _type4, _type5, \
                     _type6, _type7, _type8, _type9) \
    GL_API void GL_APIENTRY gl##_name(GL##_type1 _1, \
                                      GL##_type2 _2, \
                                      GL##_type3 _3, \
                                      GL##_type4 _4, \
                                      GL##_type5 _5, \
                                      GL##_type6 _6, \
                                      GL##_type7 _7, \
                                      GL##_type8 _8, \
                                      GL##_type9 _9) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 9; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _2); \
            glmPUT_##_type3(2, _3); \
            glmPUT_##_type4(3, _4); \
            glmPUT_##_type5(4, _5); \
            glmPUT_##_type6(5, _6); \
            glmPUT_##_type7(6, _7); \
            glmPUT_##_type8(7, _8); \
            glmPUT_##_type9(8, _9); \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(_1, _2, _3, _4, _5, _6, _7, _8, _9); \
        } \
    }

#define glmDECLARE_ENUM(_name, _sync) \
    GL_API GLenum GL_APIENTRY gl##_name(void) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 0; \
            glffDispatch(gl_sync_##_sync); \
            return (GLenum) msg->output.u; \
        } \
        else \
        { \
            return gl##_name##_THREAD(); \
        } \
    }

#define glmDECLARE_BOOLEAN_1(_name, _sync, _type1) \
    GL_API GLboolean GL_APIENTRY gl##_name(GL##_type1 _1) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 1; \
            glmPUT_##_type1(0, _1); \
            glffDispatch(gl_sync_##_sync); \
            return (GLboolean) msg->output.u; \
        } \
        else \
        { \
            return gl##_name##_THREAD(_1); \
        } \
    }

#define glmDECLARE_ENUM_1(_name, _sync, _type1) \
    GL_API GLenum GL_APIENTRY gl##_name(GL##_type1 _1) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 1; \
            glmPUT_##_type1(0, _1); \
            glffDispatch(gl_sync_##_sync); \
            return (GLenum) msg->output.u; \
        } \
        else \
        { \
            return gl##_name##_THREAD(_1); \
        } \
    }

#define glmDECLARE_UBYTE_CP_1(_name, _sync, _type1) \
    GL_API const GLubyte * GL_APIENTRY gl##_name(GL##_type1 _1) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 1; \
            glmPUT_##_type1(0, _1); \
            glffDispatch(gl_sync_##_sync); \
            return (const GLubyte *) msg->output.p; \
        } \
        else \
        { \
            return gl##_name##_THREAD(_1); \
        } \
    }

#define glmDECLARE_BITFIELD_2(_name, _sync, _type1, _type2) \
    GL_API GLbitfield GL_APIENTRY gl##_name(GL##_type1 _1, \
                                      GL##_type2 _2) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 2; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _2); \
            glffDispatch(gl_sync_##_sync); \
            return (GLbitfield) msg->output.u; \
        } \
        else \
        { \
            return gl##_name##_THREAD(_1, _2); \
        } \
    }

#define glmDECLARE_VOID_P_2(_name, _sync, _type1, _type2) \
    GL_API GLvoid * GL_APIENTRY gl##_name(GL##_type1 _1, \
                                      GL##_type2 _2) \
    { \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 2; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _2); \
            glffDispatch(gl_sync_##_sync); \
            return (GLvoid *) msg->output.p; \
        } \
        else \
        { \
            return gl##_name##_THREAD(_1, _2); \
        } \
    }

#define glmDECLARE_MULTIDRAWA(_name, _sync, _type1, _type2, _type3, _type4) \
    GL_API void GL_APIENTRY gl##_name(GL##_type1 _1, GL##_type2 _2, \
                                      GL##_type3 _3, GL##_type4 _4) \
    { \
        gctPOINTER _ptr2, _ptr3; \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            _ptr2 = eglfCopyRenderMemory(_2, _4 * sizeof(GL##_type2)); \
            msg->bytes[0] = _4 * sizeof(GL##_type2); \
            _ptr3 = eglfCopyRenderMemory(_3, _4 * sizeof(GL##_type3)); \
            msg->bytes[1] = _4 * sizeof(GL##_type3); \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 4; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _ptr2); \
            glmPUT_##_type3(2, _ptr3); \
            glmPUT_##_type4(3, _4); \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(_1, _2, _3, _4); \
        } \
    }

#define glmDECLARE_MULTIDRAWE(_name, _sync, _type1, _type2, _type3, _type4, \
                              _type5) \
    GL_API void GL_APIENTRY gl##_name(GL##_type1 _1, GL##_type2 _2, \
                                      GL##_type3 _3, GL##_type4 _4, \
                                      GL##_type5 _5) \
    { \
        gctPOINTER _ptr2, _ptr4; \
        gcsRENDER_MESSAGE *msg = eglfGetRenderMessage(gcvNULL); \
        if (msg != gcvNULL) \
        { \
            _ptr2 = eglfCopyRenderMemory(_2, _5 * sizeof(GL##_type2)); \
            msg->bytes[0] = _5 * sizeof(GL##_type2); \
            _ptr4 = eglfCopyRenderMemory(_4, _5 * sizeof(GL##_type4)); \
            msg->bytes[1] = _5 * sizeof(GL##_type4); \
            msg->function = (gltRENDER_FUNCTION) gl##_name##_THREAD; \
            msg->argCount = 5; \
            glmPUT_##_type1(0, _1); \
            glmPUT_##_type2(1, _ptr2); \
            glmPUT_##_type3(2, _3); \
            glmPUT_##_type4(3, _ptr4); \
            glmPUT_##_type5(4, _5); \
            glffDispatch(gl_sync_##_sync); \
        } \
        else \
        { \
            gl##_name##_THREAD(_1, _2, _3, _4, _5); \
        } \
    }

/* Build the functions. */
#include "gc_glff_functions.h"

#endif /* gcdRENDER_THREADS */
