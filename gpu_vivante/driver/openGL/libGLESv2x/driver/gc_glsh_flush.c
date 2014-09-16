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
#include <EGL/egl.h>

GL_APICALL void GL_APIENTRY
glFlush(
    void
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER()
    {
        gcmDUMP_API("${ES20 glFlush}");

        glmPROFILE(context, GLES2_FLUSH, 0);

#if gldFBO_DATABASE
        /* Check if we have a current framebuffer and we are not playing
         * back. */
        if (context->dbEnable
            &&
            (context->framebuffer != gcvNULL)
            &&
            !context->playing
            )
        {
            /* Add flush to database. */
            if (gcmIS_SUCCESS(glshAddFlush(context)))
            {
                /* Don't process flush now. */
                break;
            }

            /* Playback database as-is. */
            if (gcmIS_ERROR(glshPlayDatabase(context,
                                             context->framebuffer->currentDB)))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                glmERROR_BREAK();
            }
        }
#endif

        _glshFlush(context, GL_FALSE);

        /* Disable batch optmization. */
        context->batchDirty = GL_TRUE;
	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glFinish(
    void
    )
{
#if gcdNULL_DRIVER < 3
    glmENTER()
    {
        gcmDUMP_API("${ES20 glFinish}");

        glmPROFILE(context, GLES2_FINISH, 0);

#if gldFBO_DATABASE
        /* Check if we have a current framebuffer and we are not playing
         * back. */
        if (context->dbEnable
            &&
            (context->framebuffer != gcvNULL)
            &&
            !context->playing
            )
        {
            /* Add finish to database. */
            if (gcmIS_SUCCESS(glshAddFinish(context)))
            {
                /* Don't process finish now. */
                break;
            }

            /* Playback database as-is. */
            if (gcmIS_ERROR(glshPlayDatabase(context,
                                             context->framebuffer->currentDB)))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                glmERROR_BREAK();
            }
        }
#endif

        _glshFlush(context, GL_TRUE);

	    /* TODO: need to do pixmap resolve after hide the pixmap wordaround in HAL
	     * Currently design cannot access EGL surface in GLES.
	     */
	    eglWaitClient();

#if gcdFRAME_DB
    gcoHAL_DumpFrameDB(gcdFRAME_DB_NAME);
#endif

    /* Disable batch optimization. */
    context->batchDirty = GL_TRUE;
	}
	glmLEAVE();
#endif
}
