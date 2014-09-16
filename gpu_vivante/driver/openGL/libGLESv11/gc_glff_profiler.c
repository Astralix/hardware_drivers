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
**  Profiler for OpenGL ES 1.1 driver.
*/

#include "gc_glff_precomp.h"

#define _GC_OBJ_ZONE    glvZONE_TRACE

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

static const char *
apiCallString[] =
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
    "glBlendEquationOES",
    "glBlendFuncSeparateOES",
    "glBlendEquationSeparateOES",
};

static gceSTATUS
_Print(
	glsCONTEXT_PTR Context,
	gctCONST_STRING Format,
	...
	)
{
	char buffer[256];
	gctUINT offset = 0;
	gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Format=0x%x", Context, Format);

	gcmONERROR(
		gcoOS_PrintStrVSafe(buffer, gcmSIZEOF(buffer),
							&offset,
							Format,
							&Format + 1));

	gcmONERROR(
		gcoPROFILER_Write(Context->hal,
                          offset,
                          buffer));

    gcmFOOTER_ARG("return=%s", "gcvSTATUS_OK");
	return gcvSTATUS_OK;

OnError:
    gcmFOOTER_ARG("return=%s", status);
	return status;
}
#endif
/*******************************************************************************
**	_glffInitializeProfiler
**
**	Initialize the profiler for the context provided.
**
**	Arguments:
**
**		GLContext Context
**			Pointer to a new GLContext object.
*/
void
_glffInitializeProfiler(
    glsCONTEXT_PTR Context
	)
{
	gceSTATUS status;
	gctUINT rev;
	char *env;

    gcmHEADER_ARG("Context=0x%x", Context);

	status = gcoPROFILER_Initialize(Context->hal);

    if (gcmIS_ERROR(status))
    {
        Context->profiler.enable = gcvFALSE;
        gcmFOOTER_NO();
        return;
    }

    /* Clear the profiler. */
	gcmVERIFY_OK(
		gcoOS_ZeroMemory(&Context->profiler, gcmSIZEOF(Context->profiler)));

    gcoOS_GetEnv(Context->os, "VP_COUNTER_FILTER", &env);
    if ((env == gcvNULL) || (env[0] ==0))
    {
        Context->profiler.drvEnable =
            Context->profiler.timeEnable =
            Context->profiler.memEnable = gcvTRUE;
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
        if (bitsLen > 1)
        {
            Context->profiler.memEnable = (env[1] == '1');
        }
        else
        {
            Context->profiler.memEnable = gcvTRUE;
        }
        if (bitsLen > 5)
        {
            Context->profiler.drvEnable = (env[5] == '1');
        }
        else
        {
            Context->profiler.drvEnable = gcvTRUE;
        }
    }
    if (env != gcvNULL)
    {
        gcmOS_SAFE_FREE(Context->os, env);
    }

    Context->profiler.enable = gcvTRUE;
#if gcdNEW_PROFILER_FILE
    {
        /* Write Generic Info. */
        char* infoCompany = "Vivante Corporation";
        char* infoVersion = "1.0";
        char  infoRevision[255] = {'\0'};   /* read from hw */
        char* infoRenderer = Context->chipName;
        char* infoDriver = "OpenGL ES 1.1";
        gctUINT offset = 0;
        rev = Context->chipRevision;
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

    }
#else

    /* Print generic info */
    _Print(Context, "<GenericInfo company=\"Vivante Corporation\" "
		"version=\"%d.%d\" renderer=\"%s\" ",
		1, 0, Context->chipName);

   	rev = Context->chipRevision;
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
    _Print(Context, "driver=\"%s\" />\n", "OpenGL ES 1.1");
#endif

    if (Context->profiler.timeEnable)
    {
		gcoOS_GetTime(&Context->profiler.frameStart);
    	Context->profiler.frameStartTimeusec     = Context->profiler.frameStart;
	    Context->profiler.primitiveStartTimeusec = Context->profiler.frameStart;
		gcoOS_GetCPUTime(&Context->profiler.frameStartCPUTimeusec);
	}
    gcmFOOTER_NO();
}

/*******************************************************************************
**	_glffDestroyProfiler
**
**	Destroy the profiler for the context provided.
**
**	Arguments:
**
**		GLContext Context
**			Pointer to a new GLContext object.
*/
void
_glffDestroyProfiler(
	glsCONTEXT_PTR Context
	)
{
    gcmHEADER_ARG("Context=0x%x", Context);

    gcoPROFILER_Destroy(Context->hal);

    if (Context->profiler.enable)
    {
        Context->profiler.enable = gcvFALSE;
        gcmVERIFY_OK(gcoPROFILER_Destroy(Context->hal));
    }
    gcmFOOTER_NO();
}

#define PRINT_XML(counter)	_Print(Context, "<%s value=\"%d\"/>\n", \
								   #counter, Context->profiler.counter)
#define PRINT_XML2(counter)	_Print(Context, "<%s value=\"%d\"/>\n", \
								   #counter, counter)
#define PRINT_XMLL(counter)	_Print(Context, "<%s value=\"%lld\"/>\n", \
								   #counter, Context->profiler.counter)
#define PRINT_XMLF(counter)	_Print(Context, "<%s value=\"%f\"/>\n", \
								   #counter, Context->profiler.counter)


/* Function for printing frame number only once */
static void
beginFrame(
	glsCONTEXT_PTR Context
	)
{
    gcmHEADER_ARG("Context=0x%x", Context);
	if (Context->profiler.enable)
	{
		if (!Context->profiler.frameBegun)
		{
#if gcdNEW_PROFILER_FILE
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
    gcmFOOTER_NO();
}

static void
endFrame(
    glsCONTEXT_PTR Context
    )
{
    int i;
    gctUINT32 totalCalls_11 = 0;
    gctUINT32 totalDrawCalls_11 = 0;
    gctUINT32 totalStateChangeCalls_11 = 0;
	gctUINT32 maxrss, ixrss, idrss, isrss;
    gcmHEADER_ARG("Context=0x%x", Context);

    if (Context->profiler.enable)
    {
    	beginFrame(Context);

        if (Context->profiler.timeEnable)
        {
#if gcdNEW_PROFILER_FILE
            gcmWRITE_CONST(VPG_TIME);

            gcmWRITE_COUNTER(VPC_ELAPSETIME, (gctUINT32)(Context->profiler.frameEndTimeusec
                - Context->profiler.frameStartTimeusec));
            gcmWRITE_COUNTER(VPC_CPUTIME, (gctUINT32) Context->profiler.totalDriverTime);
			/*gcmPRINT("----------------- frame %d: total driver time: %lu\n", Context->profiler.frameNumber, Context->profiler.totalDriverTime);*/
            gcmWRITE_CONST(VPG_END);
#else
			_Print(Context, "<Time>\n");

    		_Print(Context, "<ElapseTime value=\"%lld\"/>\n",
    	    	   Context->profiler.frameEndTimeusec
	        	   - Context->profiler.frameStartTimeusec);

    		_Print(Context, "<CPUTime value=\"%lld\"/>\n",
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
            gcmWRITE_CONST(VPG_ES11);
#else
			_Print(Context, "<OGL11Counters>\n");
#endif

		for (i = 0; i < GLES1_NUM_API_CALLS; ++i)
    	{
        	if (Context->profiler.apiCalls[i] > 0)
        	{
#if gcdNEW_PROFILER_FILE
                    gcmWRITE_COUNTER(VPG_ES11 + 1 + i, Context->profiler.apiCalls[i]);
					/*gcmPRINT("api call %d, num: %d, time: %lu\n", i, Context->profiler.apiCalls[i], Context->profiler.apiTimes[i]);*/
#else
            	_Print(Context, "<%s value=\"%d\"/>\n",
            		   apiCallString[i], Context->profiler.apiCalls[i]);
#endif

            	totalCalls_11 += Context->profiler.apiCalls[i];

	            /* TODO: Correctly place function calls into bins. */
    	        switch (GLES1_APICALLBASE + i)
            	{
                case GLES1_DRAWARRAYS:
                case GLES1_DRAWELEMENTS:
                    totalDrawCalls_11 += Context->profiler.apiCalls[i];
                    break;

                    case    GLES1_ACTIVETEXTURE :
                    case    GLES1_ALPHAFUNC :
                    case    GLES1_ALPHAFUNCX    :
                    case    GLES1_BLENDFUNC :
                    case    GLES1_CLEARCOLOR    :
                    case    GLES1_CLEARCOLORX   :
                    case    GLES1_CLEARDEPTHF   :
                    case    GLES1_CLEARDEPTHX   :
                    case    GLES1_CLEARSTENCIL  :
                    case    GLES1_CLIENTACTIVETEXTURE   :
                    case    GLES1_CLIPPLANEF    :
                    case    GLES1_CLIPPLANEX    :
                    case    GLES1_COLOR4F   :
                    case    GLES1_COLOR4UB  :
                    case    GLES1_COLOR4X   :
                    case    GLES1_COLORMASK :
                    case    GLES1_CULLFACE  :
                    case    GLES1_DEPTHFUNC :
                    case    GLES1_DEPTHMASK :
                    case    GLES1_DEPTHRANGEF   :
                    case    GLES1_DEPTHRANGEX   :
                    case    GLES1_DISABLE   :
                    case    GLES1_DISABLECLIENTSTATE    :
                    case    GLES1_ENABLE    :
                    case    GLES1_ENABLECLIENTSTATE :
                    case    GLES1_FOGF  :
                    case    GLES1_FOGFV :
                    case    GLES1_FOGX  :
                    case    GLES1_FOGXV :
                    case    GLES1_FRONTFACE :
                    case    GLES1_FRUSTUMF  :
                    case    GLES1_FRUSTUMX  :
                    case    GLES1_LIGHTF    :
                    case    GLES1_LIGHTFV   :
                    case    GLES1_LIGHTMODELF   :
                    case    GLES1_LIGHTMODELFV  :
                    case    GLES1_LIGHTMODELX   :
                    case    GLES1_LIGHTMODELXV  :
                    case    GLES1_LIGHTX    :
                    case    GLES1_LIGHTXV   :
                    case    GLES1_LINEWIDTH :
                    case    GLES1_LINEWIDTHX    :
                    case    GLES1_LOGICOP   :
                    case    GLES1_MATERIALF :
                    case    GLES1_MATERIALFV    :
                    case    GLES1_MATERIALX :
                    case    GLES1_MATERIALXV    :
                    case    GLES1_MATRIXMODE    :
                    case    GLES1_NORMAL3F  :
                    case    GLES1_NORMAL3X  :
                    case    GLES1_ORTHOF    :
                    case    GLES1_ORTHOX    :
                    case    GLES1_PIXELSTOREI   :
                    case    GLES1_POINTPARAMETERF   :
                    case    GLES1_POINTPARAMETERFV  :
                    case    GLES1_POINTPARAMETERX   :
                    case    GLES1_POINTPARAMETERXV  :
                    case    GLES1_POINTSIZE :
                    case    GLES1_POINTSIZEX    :
                    case    GLES1_POLYGONOFFSET :
                    case    GLES1_POLYGONOFFSETX    :
                    case    GLES1_SAMPLECOVERAGE    :
                    case    GLES1_SAMPLECOVERAGEX   :
                    case    GLES1_SCISSOR   :
                    case    GLES1_SHADEMODEL    :
                    case    GLES1_STENCILFUNC   :
                    case    GLES1_STENCILMASK   :
                    case    GLES1_STENCILOP :
                    case    GLES1_TEXENVF   :
                    case    GLES1_TEXENVFV  :
                    case    GLES1_TEXENVI   :
                    case    GLES1_TEXENVIV  :
                    case    GLES1_TEXENVX   :
                    case    GLES1_TEXENVXV  :
                    case    GLES1_TEXPARAMETERF :
                    case    GLES1_TEXPARAMETERFV    :
                    case    GLES1_TEXPARAMETERI :
                    case    GLES1_TEXPARAMETERIV    :
                    case    GLES1_TEXPARAMETERX :
                    case    GLES1_TEXPARAMETERXV    :
                    case    GLES1_VIEWPORT  :
                    case    GLES1_BLENDEQUATIONOES  :
                    case    GLES1_BLENDFUNCSEPERATEOES  :
                    case    GLES1_BLENDEQUATIONSEPARATEOES  :
    	                totalStateChangeCalls_11 += Context->profiler.apiCalls[i];
                    break;

					default:
	                    break;
	            	}
		        }

	    	    /* Clear variables for next frame. */
	        	Context->profiler.apiCalls[i] = 0;
				Context->profiler.apiTimes[i] = 0;
 				Context->profiler.totalDriverTime = 0;
	    	}

#if gcdNEW_PROFILER_FILE
            gcmWRITE_COUNTER(VPC_ES11CALLS, totalCalls_11);
            gcmWRITE_COUNTER(VPC_ES11DRAWCALLS, totalDrawCalls_11);
            gcmWRITE_COUNTER(VPC_ES11STATECHANGECALLS, totalStateChangeCalls_11);

            gcmWRITE_COUNTER(VPC_ES11POINTCOUNT, Context->profiler.drawPointCount_11);
            gcmWRITE_COUNTER(VPC_ES11LINECOUNT, Context->profiler.drawLineCount_11);
            gcmWRITE_COUNTER(VPC_ES11TRIANGLECOUNT, Context->profiler.drawTriangleCount_11);

            gcmWRITE_CONST(VPG_END);
#else
	    	PRINT_XML2(totalCalls_11);
    		PRINT_XML2(totalDrawCalls_11);
		    PRINT_XML2(totalStateChangeCalls_11);

	    	PRINT_XML(drawPointCount_11);
    		PRINT_XML(drawLineCount_11);
		    PRINT_XML(drawTriangleCount_11);

    		/* Shader related info. */
		    PRINT_XMLL(shaderCompileTime);

	    	_Print(Context, "</OGL11Counters>\n");
#endif
		}

    	gcoPROFILER_EndFrame(Context->hal);

#if gcdNEW_PROFILER_FILE
        gcmWRITE_CONST(VPG_END);
#else
    	_Print(Context, "</Frame>\n\n");
#endif

		gcoPROFILER_Flush(Context->hal);

    	/* Reset counters. */
    	Context->profiler.drawPointCount_11        = 0;
	    Context->profiler.drawLineCount_11         = 0;
    	Context->profiler.drawTriangleCount_11     = 0;
	    Context->profiler.textureUploadSize     = 0;
	    Context->profiler.shaderCompileTime     = 0;
    	Context->profiler.shaderStartTimeusec   = 0;
	    Context->profiler.shaderEndTimeusec     = 0;
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
    gcmFOOTER_NO();
}

void
_glffProfiler(
	gctPOINTER Profiler,
	gctUINT32 Enum,
	gctUINT32 Value
	)
{
	glsCONTEXT_PTR context = GetCurrentContext();
    gcmHEADER_ARG("Profiler=0x%x Enum=%u Value=%u", Profiler, Enum, Value);
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    if (! context->profiler.enable)
    {
        gcmFOOTER_NO();
        return;
    }

	switch (Enum)
	{
	case GL_PROFILER_FRAME_END:
        if (context->profiler.timeEnable)
        {
			gcoOS_GetTime(&context->profiler.frameEndTimeusec);
			gcoOS_GetCPUTime(&context->profiler.frameEndCPUTimeusec);
		}
		endFrame(context);
		break;

    case GL1_PROFILER_PRIMITIVE_END:
        /*endPrimitive(context);*/
        break;

    case GL1_PROFILER_PRIMITIVE_TYPE:
        if (context->profiler.drvEnable)
        {
	        context->profiler.primitiveType = Value;
		}
        break;

    case GL1_PROFILER_PRIMITIVE_COUNT:
        if (!context->profiler.drvEnable)
            break;
        context->profiler.primitiveCount = Value;

        switch (context->profiler.primitiveType)
        {
        case GL_POINTS:
            context->profiler.drawPointCount_11 += Value;
            break;

        case GL_LINES:
        case GL_LINE_LOOP:
        case GL_LINE_STRIP:
            context->profiler.drawLineCount_11 += Value;
            break;

        case GL_TRIANGLES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
            context->profiler.drawTriangleCount_11 += Value;
            break;
        }
        break;

    case GL_TEXUPLOAD_SIZE:
        context->profiler.textureUploadSize += Value;
        break;

    default:
        if (!context->profiler.drvEnable)
            break;
        if (Enum - GLES1_APICALLBASE < GLES1_NUM_API_CALLS)
        {
            context->profiler.apiCalls[Enum - GLES1_APICALLBASE]++;
        }
        break;
    }
    gcmFOOTER_NO();
}
#endif
