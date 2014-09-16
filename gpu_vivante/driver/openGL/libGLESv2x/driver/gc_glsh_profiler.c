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
**	Profiler for OpenGL ES 2.0 driver.
*/

#include "gc_glsh_precomp.h"
#include "gc_hal_driver.h"

#if VIVANTE_PROFILER

#if gcdNEW_PROFILER_FILE


#define gcmWRITE_CONST(ConstValue) \
    do \
    { \
        gceSTATUS status; \
        gctINT32 value = ConstValue; \
        gcmERR_BREAK(gcoPROFILER_Write(Context->hal, gcmSIZEOF(value), &value)); \
    } \
    while (gcvFALSE)

#define gcmWRITE_VALUE(IntData) \
    do \
    { \
        gceSTATUS status; \
        gctINT32 value = IntData; \
        gcmERR_BREAK(gcoPROFILER_Write(Context->hal, gcmSIZEOF(value), &value)); \
    } \
    while (gcvFALSE)

#define gcmWRITE_COUNTER(Counter, Value) \
    gcmWRITE_CONST(Counter); \
    gcmWRITE_VALUE(Value)

/* Write a string value (char*). */
#define gcmWRITE_STRING(String) \
    do \
    { \
        gceSTATUS status; \
        gctSIZE_T length; \
        gcmERR_BREAK(gcoOS_StrLen((gctSTRING)String, &length)); \
        gcmERR_BREAK(gcoPROFILER_Write(Context->hal, gcmSIZEOF(length), &length)); \
        gcmERR_BREAK(gcoPROFILER_Write(Context->hal, length, String)); \
    } \
    while (gcvFALSE)

#else

static const char * apiCallString[] =
{
    "glActiveTexture",
    "glAttachShader",
    "glBindAttribLocation",
    "glBindBuffer",
    "glBindFramebuffer",
    "glBindRenderbuffer",
    "glBindTexture",
    "glBlendColor",
    "glBlendEquation",
    "glBlendEquationSeparate",
    "glBlendFunc",
    "glBlendFuncSeparate",
    "glBufferData",
    "glBufferSubData",
    "glCheckFramebufferStatus",
    "glClear",
    "glClearColor",
    "glClearDepthf",
    "glClearStencil",
    "glColorMask",
    "glCompileShader",
    "glCompressedTexImage2D",
    "glCompressedTexSubImage2D",
    "glCopyTexImage2D",
    "glCopyTexSubImage2D",
    "glCreateProgram",
    "glCreateShader",
    "glCullFace",
    "glDeleteBuffers",
    "glDeleteFramebuffers",
    "glDeleteProgram",
    "glDeleteRenderbuffers",
    "glDeleteShader",
    "glDeleteTextures",
    "glDepthFunc",
    "glDepthMask",
    "glDepthRangef",
    "glDetachShader",
    "glDisable",
    "glDisableVertexAttribArray",
    "glDrawArrays",
    "glDrawElements",
    "glEnable",
    "glEnableVertexAttribArray",
    "glFinish",
    "glFlush",
    "glFramebufferRenderbuffer",
    "glFramebufferTexture2D",
    "glFrontFace",
    "glGenBuffers",
    "glGenerateMipmap",
    "glGenFramebuffers",
    "glGenRenderbuffers",
    "glGenTextures",
    "glGetActiveAttrib",
    "glGetActiveUniform",
    "glGetAttachedShaders",
    "glGetAttribLocation",
    "glGetBooleanv",
    "glGetBufferParameteriv",
    "glGetError",
    "glGetFloatv",
    "glGetFramebufferAttachmentParameteriv",
    "glGetIntegerv",
    "glGetProgramiv",
    "glGetProgramInfoLog",
    "glGetRenderbufferParameteriv",
    "glGetShaderiv",
    "glGetShaderInfoLog",
    "glGetShaderPrecisionFormat",
    "glGetShaderSource",
    "glGetString",
    "glGetTexParameterfv",
    "glGetTexParameteriv",
    "glGetUniformfv",
    "glGetUniformiv",
    "glGetUniformLocation",
    "glGetVertexAttribfv",
    "glGetVertexAttribiv",
    "glGetVertexAttribPointerv",
    "glHint",
    "glIsBuffer",
    "glIsEnabled",
    "glIsFramebuffer",
    "glIsProgram",
    "glIsRenderbuffer",
    "glIsShader",
    "glIsTexture",
    "glLineWidth",
    "glLinkProgram",
    "glPixelStorei",
    "glPolygonOffset",
    "glProgramBinaryOES",
    "glReadPixels",
    "glReleaseShaderCompiler",
    "glRenderbufferStorage",
    "glSampleCoverage",
    "glScissor",
    "glShaderBinary",
    "glShaderSource",
    "glStencilFunc",
    "glStencilFuncSeparate",
    "glStencilMask",
    "glStencilMaskSeparate",
    "glStencilOp",
    "glStencilOpSeparate",
    "glTexImage2D",
    "glTexParameterf",
    "glTexParameterfv",
    "glTexParameteri",
    "glTexParameteriv",
    "glTexSubImage2D",
    "glUniform1f",
    "glUniform1fv",
    "glUniform1i",
    "glUniform1iv",
    "glUniform2f",
    "glUniform2fv",
    "glUniform2i",
    "glUniform2iv",
    "glUniform3f",
    "glUniform3fv",
    "glUniform3i",
    "glUniform3iv",
    "glUniform4f",
    "glUniform4fv",
    "glUniform4i",
    "glUniform4iv",
    "glUniformMatrix2fv",
    "glUniformMatrix3fv",
    "glUniformMatrix4fv",
    "glUseProgram",
    "glValidateProgram",
    "glVertexAttrib1f",
    "glVertexAttrib1fv",
    "glVertexAttrib2f",
    "glVertexAttrib2fv",
    "glVertexAttrib3f",
    "glVertexAttrib3fv",
    "glVertexAttrib4f",
    "glVertexAttrib4fv",
    "glVertexAttribPointer",
    "glViewport",
    "glGetProgramBinaryOES",
    "glProgramBinaryOES",
    "glTexImage3DOES",
    "glTexSubImage3DOES",
    "glCopyTexSubImage3DOES",
    "glCompressedTexImage3DOES",
    "glCompressedTexSubImage3DOES",
    "glFrameBufferTexture3DOES",
    "glBindVertexArrayOES",
    "glGenVertexArrayOES",
    "glIsVertexArrayOES",
    "glDeleteVertexArrayOES",
};

static gceSTATUS
_Print(
	IN GLContext Context,
	IN gctCONST_STRING Format,
	...
	)
{
	char buffer[256];
	gctUINT offset = 0;
	gceSTATUS status;

	gcmONERROR(
		gcoOS_PrintStrVSafe(buffer,
                            gcmSIZEOF(buffer),
                            &offset,
                            Format,
                            (gctPOINTER) (&Format + 1)));

	gcmONERROR(
		gcoPROFILER_Write(Context->hal,
                          offset,
                          buffer));

	return gcvSTATUS_OK;

OnError:
	return status;
}
#endif

/*******************************************************************************
**	_glshInitializeProfiler
**
**	Initialize the profiler for the context provided.
**
**	Arguments:
**
**		GLContext Context
**			Pointer to a new GLContext object.
*/
void
_glshInitializeProfiler(
    IN GLContext Context
	)
{
	gceSTATUS status;
   	gctUINT rev;
    char *env = gcvNULL;

	status = gcoPROFILER_Initialize(Context->hal);

	if (gcmIS_ERROR(status))
	{
		Context->profiler.enable = gcvFALSE;
		return;
	}

    /* Clear the profiler. */
    gcmVERIFY_OK(
    	gcoOS_ZeroMemory(&Context->profiler, gcmSIZEOF(Context->profiler)));

    Context->profiler.enable = gcvTRUE;

    gcoOS_GetEnv(Context->os, "VP_COUNTER_FILTER", &env);
    if ((env == gcvNULL) || (env[0] ==0))
    {
        Context->profiler.timeEnable =
            Context->profiler.drvEnable =
            Context->profiler.memEnable =
            Context->profiler.progEnable = gcvTRUE;
    }
    else
    {
        gctSIZE_T bitsLen;
        gcoOS_StrLen(env, &bitsLen);
        if (bitsLen > 0)
        {
            Context->profiler.timeEnable = (env[0] == '1');
        }
        else
        {
            Context->profiler.timeEnable = gcvTRUE;
        }
        if (bitsLen > 6)
        {
            Context->profiler.drvEnable = (env[6] == '1');
        }
        else
        {
            Context->profiler.drvEnable = gcvTRUE;
        }
        if (bitsLen > 1)
        {
            Context->profiler.memEnable = (env[1] == '1');
        }
        else
        {
            Context->profiler.memEnable = gcvTRUE;
        }
        if (bitsLen > 7)
        {
            Context->profiler.progEnable = (env[7] == '1');
        }
        else
        {
            Context->profiler.progEnable = gcvTRUE;
        }
    }
	if (env)
	{
		gcmOS_SAFE_FREE(Context->os, env);
	}

#if gcdNEW_PROFILER_FILE
    {
        /* Write Generic Info. */
        char* infoCompany = "Vivante Corporation";
        char* infoVersion = "1.0";
        char  infoRevision[255] = {'\0'};   /* read from hw */
        char* infoRenderer = Context->renderer;
        char* infoDriver = "OpenGL ES 2.0";
        gctUINT offset = 0;
        rev = Context->revision;
#define BCD(digit)      ((rev >> (digit * 4)) & 0xF)
        gcoOS_MemFill(infoRevision, 0, gcmSIZEOF(infoRevision));
        if (BCD(3) == 0)
        {
            /* Old format. */
            gcoOS_PrintStrSafe(infoRevision, gcmSIZEOF(infoRevision),
                &offset, "revision=\"%d.%d\" ", BCD(1), BCD(0));
        }
        else
        {
            /* New format. */
            gcoOS_PrintStrSafe(infoRevision, gcmSIZEOF(infoRevision),
                &offset, "revision=\"%d.%d.%d_rc%d\" ",
                BCD(3), BCD(2), BCD(1), BCD(0));
        }

        gcmWRITE_CONST(VPG_INFO);

        gcmWRITE_CONST(VPC_INFOCOMPANY);
        gcmWRITE_STRING(infoCompany);
        gcmWRITE_CONST(VPC_INFOVERSION);
        gcmWRITE_STRING(infoVersion);
        gcmWRITE_CONST(VPC_INFORENDERER);
        gcmWRITE_STRING(infoRenderer);
        gcmWRITE_CONST(VPC_INFOREVISION);
        gcmWRITE_STRING(infoRevision);
        gcmWRITE_CONST(VPC_INFODRIVER);
        gcmWRITE_STRING(infoDriver);
#if gcdNULL_DRIVER >= 2
 	    {
 		char* infoDiverMode = "NULL Driver";
 		gcmWRITE_CONST(VPC_INFODRIVERMODE);
        gcmWRITE_STRING(infoDiverMode);
 	    }
#endif
        /* gcmWRITE_CONST(VPG_END); */
    }
#else
    /* Print generic info */
    _Print(Context, "<GenericInfo company=\"Vivante Corporation\" "
		"version=\"%d.%d\" renderer=\"%s\" ",
		1, 0, Context->renderer);

   	rev = Context->revision;
#define BCD(digit)		((rev >> (digit * 4)) & 0xF)
   	if (BCD(3) == 0)
   	{
   		/* Old format. */
   		_Print(Context, "revision=\"%d.%d\" ", BCD(1), BCD(0));
   	}
   	else
   	{
   		/* New format. */
   		_Print(Context, "revision=\"%d.%d.%d_rc%d\" ",
			BCD(3), BCD(2), BCD(1), BCD(0));
   	}
    _Print(Context, "driver=\"%s\" />\n", "OpenGL ES 2.0");
#endif

	gcoOS_GetTime(&Context->profiler.frameStart);
    Context->profiler.frameStartTimeusec     = Context->profiler.frameStart;
    Context->profiler.primitiveStartTimeusec = Context->profiler.frameStart;
	gcoOS_GetCPUTime(&Context->profiler.frameStartCPUTimeusec);
}

/*******************************************************************************
**	_glshDestroyProfiler
**
**	Initialize the profiler for the context provided.
**
**	Arguments:
**
**		GLContext Context
**			Pointer to a new GLContext object.
*/
void
_glshDestroyProfiler(
	IN GLContext Context
	)
{
    gcoPROFILER_Destroy(Context->hal);

    if (Context->profiler.enable)
    {
	    Context->profiler.enable = gcvFALSE;
    	gcmVERIFY_OK(gcoPROFILER_Destroy(Context->hal));
	}
}

#define PRINT_XML(counter)	_Print(Context, "<%s value=\"%d\"/>\n", \
								   #counter, Context->profiler.counter)
#define PRINT_XML2(counter)	_Print(Context, "<%s value=\"%d\"/>\n", \
								   #counter, counter)
#define PRINT_XMLL(counter)	_Print(Context, "<%s value=\"%llu\"/>\n", \
								   #counter, Context->profiler.counter)
#define PRINT_XMLF(counter)	_Print(Context, "<%s value=\"%f\"/>\n", \
								   #counter, Context->profiler.counter)


/* Function for printing frame number only once */
static void
beginFrame(
	IN GLContext Context
	)
{
	if (Context->profiler.enable)
	{
		if (!Context->profiler.frameBegun)
		{
#if gcdNEW_PROFILER_FILE
			/* write screen size */
 			if (Context->profiler.frameNumber == 0 && Context->draw)
 			{
 				gctUINT width, height;
 				gctUINT offset = 0;
 				char  infoScreen[255] = {'\0'};
 				gcoSURF_GetSize(Context->draw, &width, &height, gcvNULL);
 				gcoOS_MemFill(infoScreen, 0, gcmSIZEOF(infoScreen));
 				gcoOS_PrintStrSafe(infoScreen, gcmSIZEOF(infoScreen),
 						&offset, "%d x %d", width, height);
 				gcmWRITE_CONST(VPC_INFOSCREENSIZE);
 				gcmWRITE_STRING(infoScreen);

 				gcmWRITE_CONST(VPG_END);
 			}

            gcmWRITE_COUNTER(VPG_FRAME, Context->profiler.frameNumber);
#else
			_Print(Context, "<Frame value=\"%d\">\n",
				   Context->profiler.frameNumber);
#endif
			Context->profiler.frameBegun = 1;
		}
    }
}

static void
endFrame(
	IN GLContext Context
	)
{
    int i;
    gctUINT32 totalCalls = 0;
    gctUINT32 totalDrawCalls = 0;
    gctUINT32 totalStateChangeCalls = 0;
	gctUINT32 maxrss, ixrss, idrss, isrss;

	if (Context->profiler.enable)
	{
		beginFrame(Context);

        if (Context->profiler.timeEnable)
        {
#if gcdNEW_PROFILER_FILE
            gcmWRITE_CONST(VPG_TIME);

            gcmWRITE_COUNTER(VPC_ELAPSETIME, (gctINT32) (Context->profiler.frameEndTimeusec
                - Context->profiler.frameStartTimeusec));
            gcmWRITE_COUNTER(VPC_CPUTIME, (gctINT32) Context->profiler.totalDriverTime);

            gcmWRITE_CONST(VPG_END);
#else
        _Print(Context, "<Time>\n");

        _Print(Context, "<ElapseTime value=\"%llu\"/>\n",
            Context->profiler.frameEndTimeusec
			- Context->profiler.frameStartTimeusec);

        _Print(Context, "<CPUTime value=\"%llu\"/>\n",
            Context->profiler.frameEndCPUTimeusec
			- Context->profiler.frameStartCPUTimeusec);

        _Print(Context, "</Time>\n");
#endif
		}

        if (Context->profiler.memEnable)
        {
#if gcdNEW_PROFILER_FILE
            gcoOS_GetMemoryUsage(&maxrss, &ixrss, &idrss, &isrss);

            gcmWRITE_CONST(VPG_MEM);

            gcmWRITE_COUNTER(VPC_MEMMAXRES, maxrss);
            gcmWRITE_COUNTER(VPC_MEMSHARED, ixrss);
            gcmWRITE_COUNTER(VPC_MEMUNSHAREDDATA, idrss);
            gcmWRITE_COUNTER(VPC_MEMUNSHAREDSTACK, isrss);

            gcmWRITE_CONST(VPG_END);
#else
	   	    _Print(Context, "<Memory>\n");

			gcoOS_GetMemoryUsage(&maxrss, &ixrss, &idrss, &isrss);
   		    _Print(Context, "<MemMaxResidentSize value=\"%lu\"/>\n", maxrss);
	   	    _Print(Context, "<MemSharedSize value=\"%lu\"/>\n", ixrss);
   	    	_Print(Context, "<MemUnsharedDataSize value=\"%lu\"/>\n", idrss);
   		    _Print(Context, "<MemUnsharedStackSize value=\"%lu\"/>\n", isrss);

	   	    _Print(Context, "</Memory>\n");
#endif
        }

        if (Context->profiler.drvEnable)
        {
#if gcdNEW_PROFILER_FILE
            gcmWRITE_CONST(VPG_ES20);
#else
            _Print(Context, "<OGL20Counters>\n");
#endif

			for (i = 0; i < GLES2_NUM_API_CALLS; ++i)
			{
        		if (Context->profiler.apiCalls[i] > 0)
        		{
#if gcdNEW_PROFILER_FILE
                    gcmWRITE_COUNTER(VPG_ES20 + 1 + i, Context->profiler.apiCalls[i]);
#else
					_Print(Context, "<%s value=\"%d\"/>\n",
    	        		   apiCallString[i], Context->profiler.apiCalls[i]);
#endif

	            	totalCalls += Context->profiler.apiCalls[i];

					/* TODO: Correctly place function calls into bins. */
					switch (GLES2_APICALLBASE + i)
					{
					case GLES2_DRAWARRAYS:
					case GLES2_DRAWELEMENTS:
						totalDrawCalls += Context->profiler.apiCalls[i];
						break;

					case GLES2_ATTACHSHADER:
					case GLES2_BLENDCOLOR:
					case GLES2_BLENDEQUATION:
					case GLES2_BLENDEQUATIONSEPARATE:
					case GLES2_BLENDFUNC:
					case GLES2_BLENDFUNCSEPARATE:
					case GLES2_CLEARCOLOR:
					case GLES2_COLORMASK:
					case GLES2_DEPTHFUNC:
					case GLES2_DEPTHMASK:
					case GLES2_DEPTHRANGEF:
					case GLES2_STENCILFUNC:
					case GLES2_STENCILFUNCSEPARATE:
					case GLES2_STENCILMASK:
					case GLES2_STENCILMASKSEPARATE:
					case GLES2_STENCILOP:
					case GLES2_STENCILOPSEPARATE:
					case GLES2_UNIFORM1F:
					case GLES2_UNIFORM1FV:
					case GLES2_UNIFORM1I:
					case GLES2_UNIFORM1IV:
					case GLES2_UNIFORM2F:
					case GLES2_UNIFORM2FV:
					case GLES2_UNIFORM2I:
					case GLES2_UNIFORM2IV:
					case GLES2_UNIFORM3F:
					case GLES2_UNIFORM3FV:
					case GLES2_UNIFORM3I:
					case GLES2_UNIFORM3IV:
					case GLES2_UNIFORM4F:
					case GLES2_UNIFORM4FV:
					case GLES2_UNIFORM4I:
					case GLES2_UNIFORM4IV:
					case GLES2_UNIFORMMATRIX2FV:
					case GLES2_UNIFORMMATRIX3FV:
					case GLES2_UNIFORMMATRIX4FV:
					case GLES2_USEPROGRAM:
					case GLES2_CULLFACE:
					case GLES2_DISABLE:
					case GLES2_ENABLE:
					case GLES2_DISABLEVERTEXATTRIBARRAY:
					case GLES2_ENABLEVERTEXATTRIBARRAY:
					case GLES2_FRONTFACE:
					case GLES2_HINT:
					case GLES2_LINEWIDTH:
					case GLES2_PIXELSTOREI:
					case GLES2_POLYGONOFFSET:
					case GLES2_SAMPLECOVERAGE:
					case GLES2_SCISSOR:
					case GLES2_TEXIMAGE2D:
					case GLES2_TEXPARAMETERF:
					case GLES2_TEXPARAMETERFV:
					case GLES2_TEXPARAMETERI:
					case GLES2_TEXPARAMETERIV:
					case GLES2_TEXSUBIMAGE2D:
					case GLES2_VERTEXATTRIB1F:
					case GLES2_VERTEXATTRIB1FV:
					case GLES2_VERTEXATTRIB2F:
					case GLES2_VERTEXATTRIB2FV:
					case GLES2_VERTEXATTRIB3F:
					case GLES2_VERTEXATTRIB3FV:
					case GLES2_VERTEXATTRIB4F:
					case GLES2_VERTEXATTRIB4FV:
					case GLES2_VERTEXATTRIBPOINTER:
					case GLES2_VIEWPORT:
					case GLES2_BINDATTRIBLOCATION:
					case GLES2_BINDBUFFER:
					case GLES2_BINDFRAMEBUFFER:
					case GLES2_BINDRENDERBUFFER:
					case GLES2_BINDTEXTURE:
					case GLES2_COMPRESSEDTEXIMAGE2D:
					case GLES2_COMPRESSEDTEXSUBIMAGE2D:
					case GLES2_DETACHSHADER:
					case GLES2_SHADERBINARY:
					case GLES2_SHADERSOURCE:
					case GLES2_BUFFERDATA:
					case GLES2_BUFFERSUBDATA:
					case GLES2_CREATEPROGRAM:
					case GLES2_CREATESHADER:
					case GLES2_DELETEBUFFERS:
					case GLES2_DELETEFRAMEBUFFERS:
					case GLES2_DELETEPROGRAM:
					case GLES2_DELETERENDERBUFFERS:
					case GLES2_DELETESHADER:
					case GLES2_DELETETEXTURES:
					case GLES2_FRAMEBUFFERRENDERBUFFER:
					case GLES2_FRAMEBUFFERTEXTURE2D:
					case GLES2_GENBUFFERS:
					case GLES2_GENERATEMIPMAP:
					case GLES2_GENFRAMEBUFFERS:
					case GLES2_GENRENDERBUFFERS:
					case GLES2_GENTEXTURES:
					case GLES2_RELEASESHADERCOMPILER:
					case GLES2_RENDERBUFFERSTORAGE:
					case GLES2_PROGRAMBINARYOES:
						totalStateChangeCalls += Context->profiler.apiCalls[i];
						break;

					case GLES2_CHECKFRAMEBUFFERSTATUS:
					case GLES2_CLEAR:
					case GLES2_CLEARDEPTHF:
					case GLES2_CLEARSTENCIL:
					case GLES2_COMPILESHADER:
					case GLES2_COPYTEXIMAGE2D:
					case GLES2_COPYTEXSUBIMAGE2D:
					case GLES2_FINISH:
					case GLES2_FLUSH:
					case GLES2_GETACTIVEATTRIB:
					case GLES2_GETACTIVEUNIFORM:
					case GLES2_GETATTACHEDSHADERS:
					case GLES2_GETATTRIBLOCATION:
					case GLES2_GETBOOLEANV:
					case GLES2_GETBUFFERPARAMETERIV:
					case GLES2_GETERROR:
					case GLES2_GETFLOATV:
					case GLES2_GETFRAMEBUFFERATTACHMENTPARAMETERIV:
					case GLES2_GETINTEGERV:
					case GLES2_GETPROGRAMIV:
					case GLES2_GETPROGRAMINFOLOG:
					case GLES2_GETPROGRAMBINARYOES:
					case GLES2_GETRENDERBUFFERPARAMETERIV:
					case GLES2_GETSHADERIV:
					case GLES2_GETSHADERINFOLOG:
					case GLES2_GETSHADERPRECISIONFORMAT:
					case GLES2_GETSHADERSOURCE:
					case GLES2_GETSTRING:
					case GLES2_GETTEXPARAMETERFV:
					case GLES2_GETTEXPARAMETERIV:
					case GLES2_GETUNIFORMFV:
					case GLES2_GETUNIFORMIV:
					case GLES2_GETUNIFORMLOCATION:
					case GLES2_GETVERTEXATTRIBFV:
					case GLES2_GETVERTEXATTRIBIV:
					case GLES2_GETVERTEXATTRIBPOINTERV:
					case GLES2_ISBUFFER:
					case GLES2_ISENABLED:
					case GLES2_ISFRAMEBUFFER:
					case GLES2_ISPROGRAM:
					case GLES2_ISRENDERBUFFER:
					case GLES2_ISSHADER:
					case GLES2_ISTEXTURE:
					case GLES2_LINKPROGRAM:
					case GLES2_READPIXELS:
					case GLES2_VALIDATEPROGRAM:
						break;
					}
				}

				/* Clear variables for next frame. */
		        Context->profiler.apiCalls[i] = 0;
				Context->profiler.apiTimes[i] = 0;
				Context->profiler.totalDriverTime = 0;
			}

#if gcdNEW_PROFILER_FILE
            gcmWRITE_COUNTER(VPC_ES20CALLS, totalCalls);
            gcmWRITE_COUNTER(VPC_ES20DRAWCALLS, totalDrawCalls);
            gcmWRITE_COUNTER(VPC_ES20STATECHANGECALLS, totalStateChangeCalls);

            gcmWRITE_COUNTER(VPC_ES20POINTCOUNT, Context->profiler.drawPointCount);
            gcmWRITE_COUNTER(VPC_ES20LINECOUNT, Context->profiler.drawLineCount);
            gcmWRITE_COUNTER(VPC_ES20TRIANGLECOUNT, Context->profiler.drawTriangleCount);

/*          gcmWRITE_COUNTER(VPC_ES20SHADERCOMPILETIME, Context->profiler.shaderCompileTime); */
            gcmWRITE_CONST(VPG_END);
#else
			PRINT_XML2(totalCalls);
			PRINT_XML2(totalDrawCalls);
			PRINT_XML2(totalStateChangeCalls);

			PRINT_XML(drawPointCount);
			PRINT_XML(drawLineCount);
			PRINT_XML(drawTriangleCount);

			/* Shader related info. */
			PRINT_XMLL(shaderCompileTime);

			_Print(Context, "</OGL20Counters>\n");
#endif
		}

		gcoPROFILER_EndFrame(Context->hal);

#if gcdNEW_PROFILER_FILE
        gcmWRITE_CONST(VPG_END);
#else
		_Print(Context, "</Frame>\n\n");
#endif

		gcoPROFILER_Flush(Context->hal);

		/* Clear variables for next frame. */
		Context->profiler.drawPointCount      = 0;
		Context->profiler.drawLineCount       = 0;
		Context->profiler.drawTriangleCount   = 0;
		Context->profiler.textureUploadSize   = 0;

		Context->profiler.shaderCompileTime   = 0;
		Context->profiler.shaderStartTimeusec = 0;
		Context->profiler.shaderEndTimeusec   = 0;

        if (Context->profiler.timeEnable)
		{
			Context->profiler.frameStartCPUTimeusec =
				Context->profiler.frameEndCPUTimeusec;
			gcoOS_GetTime(&Context->profiler.frameStartTimeusec);
		}

		/* Next frame. */
		Context->profiler.frameNumber++;
		Context->profiler.frameBegun = 0;
	}
}

void
_glshProfiler(
	IN gctPOINTER Profiler,
	IN gctUINT32 Enum,
	IN gctUINT32 Value
	)
{
    GLContext context = _glshGetCurrentContext();
#if gcdNEW_PROFILER_FILE
    GLContext Context = context;
#endif
    if (context == gcvNULL)
	{
		return;
	}

	if (! context->profiler.enable)
	{
		return;
	}

    switch (Enum)
    {
    case GL2_PROFILER_FRAME_END:
        if (context->profiler.timeEnable)
        {
			gcoOS_GetTime(&context->profiler.frameEndTimeusec);
			gcoOS_GetCPUTime(&context->profiler.frameEndCPUTimeusec);
		}
        endFrame(context);
        break;

    case GL2_PROFILER_PRIMITIVE_END:
        /*endPrimitive(context);*/
        break;

    case GL2_PROFILER_PRIMITIVE_TYPE:
        if (!context->profiler.drvEnable)
            break;
        context->profiler.primitiveType = Value;

    case GL2_PROFILER_PRIMITIVE_COUNT:
        if (!context->profiler.drvEnable)
            break;
        context->profiler.primitiveCount = Value;
        switch (context->profiler.primitiveType)
        {
		case GL_POINTS:
			context->profiler.drawPointCount += Value;
			break;

		case GL_LINES:
		case GL_LINE_LOOP:
		case GL_LINE_STRIP:
			context->profiler.drawLineCount += Value;
			break;

		case GL_TRIANGLES:
		case GL_TRIANGLE_STRIP:
		case GL_TRIANGLE_FAN:
			context->profiler.drawTriangleCount += Value;
			break;
        }
        break;

    case GL_COMPILER_START_TIME:
        if (!context->profiler.timeEnable)
            break;
		gcoOS_GetTime(&context->profiler.shaderStartTimeusec);
        break;

    case GL_COMPILER_END_TIME:
        if (!context->profiler.timeEnable)
            break;
		gcoOS_GetTime(&context->profiler.shaderEndTimeusec);
        context->profiler.shaderCompileTime +=
        	( context->profiler.shaderEndTimeusec
        	- context->profiler.shaderStartTimeusec);
        break;

    case GL2_TEXUPLOAD_SIZE:
        context->profiler.textureUploadSize += Value;
        break;

    /* Print program info immediately as we do not save it. */
    case GL_PROGRAM_IN_USE_BEGIN:
        if (!context->profiler.progEnable)
            break;
        beginFrame(context);
#if gcdNEW_PROFILER_FILE
        gcmWRITE_CONST(VPG_PROG);
#else
        _Print(context, "<Program>\n");
#endif
        break;

    case GL_PROGRAM_IN_USE_END:
        if (!context->profiler.progEnable)
            break;
#if gcdNEW_PROFILER_FILE
        gcmWRITE_CONST(VPG_END);
#else
        _Print(context, "</Program>\n");
#endif
        break;

    case GL_PROGRAM_ATTRIB_CNT:
        if (!context->profiler.progEnable)
            break;
#if gcdNEW_PROFILER_FILE
#else
        _Print(context, "<Attributes value=\"%d\"/>\n", Value);
#endif
        break;

    case GL_PROGRAM_UNIFORM_CNT:
        if (!context->profiler.progEnable)
            break;
#if gcdNEW_PROFILER_FILE
#else
        _Print(context, "<Uniforms value=\"%d\"/>\n", Value);
#endif
        break;

    case GL_PROGRAM_VERTEX_SHADER:
        if (!context->profiler.progEnable)
            break;
        if (Value != 0) gcoPROFILER_ShaderVS(context->hal, (gcSHADER) Value);
        break;

    case GL_PROGRAM_FRAGMENT_SHADER:
        if (!context->profiler.progEnable)
            break;
        if (Value != 0) gcoPROFILER_ShaderFS(context->hal, (gcSHADER) Value);
        break;

    default:
        if (!context->profiler.drvEnable)
            break;
		if (Enum - GLES2_APICALLBASE < GLES2_NUM_API_CALLS)
        {
            context->profiler.apiCalls[Enum - GLES2_APICALLBASE]++;
        }
        break;
    }
}

#endif /* VIVANTE_PROFILER */
