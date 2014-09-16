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
**	Profiler for OpenVG 1.1 driver.
*/

#include "gc_vgsh_precomp.h"
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
	"vgAppendPath",
	"vgAppendPathData",
	"vgChildImage",
	"vgClear",
	"vgClearGlyph",
	"vgClearImage",
	"vgClearPath",
	"vgColorMatrix",
	"vgConvolve",
	"vgCopyImage",
	"vgCopyMask",
	"vgCopyPixels",
	"vgCreateFont",
	"vgCreateImage",
	"vgCreateMaskLayer",
	"vgCreatePaint",
	"vgCreatePath",
	"vgDestroyFont",
	"vgDestroyImage",
	"vgDestroyMaskLayer",
	"vgDestroyPaint",
	"vgDestroyPath",
	"vgDrawGlyph",
	"vgDrawGlyphs",
	"vgDrawImage",
	"vgDrawPath",
	"vgFillMaskLayer",
	"vgFinish",
	"vgFlush",
	"vgGaussianBlur",
	"vgGetColor",
	"vgGetError",
	"vgGetf",
	"vgGetfv",
	"vgGeti",
	"vgGetImageSubData",
	"vgGetiv",
	"vgGetMatrix",
	"vgGetPaint",
	"vgGetParameterf",
	"vgGetParameterfv",
	"vgGetParameteri",
	"vgGetParameteriv",
	"vgGetParameterVectorSize",
	"vgGetParent",
	"vgGetPathCapabilities",
	"vgGetPixels",
	"vgGetString",
	"vgGetVectorSize",
	"vgHardwareQuery",
	"vgImageSubData",
	"vgInterpolatePath",
	"vgLoadIdentity",
	"vgLoadMatrix",
	"vgLookup",
	"vgLookupSingle",
	"vgMask",
	"vgModifyPathCoords",
	"vgMultMatrix",
	"vgPaintPattern",
	"vgPathBounds",
	"vgPathLength",
	"vgPathTransformedBounds",
	"vgPointAlongPath",
	"vgReadPixels",
	"vgRemovePathCapabilities",
	"vgRenderToMask",
	"vgRotate",
	"vgScale",
	"vgSeparableConvolve",
	"vgSetColor",
	"vgSetf",
	"vgSetfv",
	"vgSetGlyphToImage",
	"vgSetGlyphToPath",
	"vgSeti",
	"vgSetiv",
	"vgSetPaint",
	"vgSetParameterf",
	"vgSetParameterfv",
	"vgSetParameteri",
	"vgSetParameteriv",
	"vgSetPixels",
	"vgShear",
	"vgTransformPath",
	"vgTranslate",
	"vgWritePixels",
};

static gceSTATUS
_Print(
	_VGContext * Context,
	gctCONST_STRING Format,
	...
	)
{
	char buffer[256];
	gctUINT offset = 0;
	gceSTATUS status;

	gcmONERROR(
		gcoOS_PrintStrVSafe(buffer, gcmSIZEOF(buffer),
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
**	InitializeVGProfiler
**
**	Initialize the profiler for the context provided.
**
**	Arguments:
**
**		VGContext Context
**			Pointer to a new VGContext object.
*/
void
InitializeVGProfiler(
    _VGContext * Context
	)
{
	gceSTATUS status;
	gctUINT rev;
        char *env;

	status = gcoPROFILER_Initialize(Context->hal);

	if (gcmIS_ERROR(status))
	{
		Context->profiler.enable = gcvFALSE;
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
        if (bitsLen > 4)
        {
            Context->profiler.drvEnable = (env[4] == '1');
        }
        else
        {
            Context->profiler.drvEnable = gcvTRUE;
        }
    }

    Context->profiler.enable = gcvTRUE;

#if gcdNEW_PROFILER_FILE
    {
        /* Write Generic Info. */
        char* infoCompany = "Vivante Corporation";
        char* infoVersion = "1.0";
        char  infoRevision[255] = {'\0'};   /* read from hw */
        char* infoRenderer = Context->chipName;
        char* infoDriver = "OpenVG 1.1";
        gctUINT offset = 0;
        rev = Context->revision;
#define BCD(digit)      ((rev >> (digit * 4)) & 0xF)
        gcoOS_MemFill(infoRevision, 0, gcmSIZEOF(infoRevision));
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

        gcmWRITE_CONST(VPG_END);
    }
#else
    /* Print generic info */
    _Print(Context, "<GenericInfo company=\"Vivante Corporation\" "
		"version=\"%d.%d\" renderer=\"%s\" ",
		1, 0, Context->chipName);

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
    _Print(Context, "driver=\"%s\" />\n", "OpenVG 1.1");
#endif

	gcoOS_GetTime(&Context->profiler.frameStart);
    Context->profiler.frameStartTimeusec     = Context->profiler.frameStart;
    Context->profiler.primitiveStartTimeusec = Context->profiler.frameStart;
	gcoOS_GetCPUTime(&Context->profiler.frameStartCPUTimeusec);
}

/*******************************************************************************
**	_DestroyVGProfiler
**
**	Initialize the profiler for the context provided.
**
**	Arguments:
**
**		VGContext Context
**			Pointer to a new VGContext object.
*/
void
DestroyVGProfiler(
	_VGContext * Context
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
#define PRINT_XMLF(counter)	_Print(Context, "<%s value=\"%f\"/>\n", \
								   #counter, Context->profiler.counter)


/* Function for printing frame number only once */
static void
beginFrame(
	_VGContext * Context
	)
{
	if (Context->profiler.enable)
	{
		if (!Context->profiler.frameBegun)
		{
#if gcdNEW_PROFILER_FILE
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
	_VGContext* Context
	)
{
    int i;
    gctUINT32 totalVGCalls = 0;
    gctUINT32 totalVGDrawCalls = 0;
    gctUINT32 totalVGStateChangeCalls = 0;
	gctUINT32 maxrss, ixrss, idrss, isrss;

	if (Context->profiler.enable)
	{
    	beginFrame(Context);

        if (Context->profiler.timeEnable)
        {
#if gcdNEW_PROFILER_FILE
            gcmWRITE_CONST(VPG_TIME);

            gcmWRITE_COUNTER(VPC_ELAPSETIME, Context->profiler.frameEndTimeusec
                - Context->profiler.frameStartTimeusec);
            gcmWRITE_COUNTER(VPC_CPUTIME, Context->profiler.frameEndCPUTimeusec
                - Context->profiler.frameStartCPUTimeusec);

            gcmWRITE_CONST(VPG_END);
#else
			_Print(Context, "<Time>\n");

			_Print(Context, "<ElapseTime value=\"%llu\"/>\n",
        		   ( Context->profiler.frameEndTimeusec
    	    	   - Context->profiler.frameStartTimeusec
	        	   ));

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
            gcmWRITE_CONST(VPG_VG11);
#else
			_Print(Context, "<VGCounters>\n");
#endif

    	for (i = 0; i < NUM_API_CALLS; ++i)
    	{
        	if (Context->profiler.apiCalls[i] > 0)
        	{
#if gcdNEW_PROFILER_FILE
                    gcmWRITE_COUNTER(VPG_VG11 + 1 + i, Context->profiler.apiCalls[i]);
#else
				_Print(Context, "<%s value=\"%d\"/>\n",
					   apiCallString[i], Context->profiler.apiCalls[i]);
#endif

            	totalVGCalls += Context->profiler.apiCalls[i];

            	/* TODO: Correctly place function calls into bins. */
            	switch(i + APICALLBASE)
            	{
                case VGDRAWPATH:
                case VGDRAWIMAGE:
				case VGDRAWGLYPH:
                    totalVGDrawCalls += Context->profiler.apiCalls[i];
                    break;

                case VGFILLMASKLAYER:
                case VGLOADIDENTITY:
                case VGLOADMATRIX:
                case VGMASK:
                case VGMULTMATRIX:
                case VGRENDERTOMASK:
                case VGROTATE:
                case VGSCALE:
                case VGSETCOLOR:
                case VGSETF:
                case VGSETFV:
                case VGSETI:
                case VGSETIV:
                case VGSETPAINT:
                case VGSETPARAMETERF:
                case VGSETPARAMETERFV:
                case VGSETPARAMETERI:
                case VGSETPARAMETERIV:
                case VGSHEAR:
                case VGTRANSLATE:
                    totalVGStateChangeCalls += Context->profiler.apiCalls[i];
                    break;

				default:
                    break;
            	}
			}

				/* Clear variables for next frame. */
				Context->profiler.apiCalls[i] = 0;
	    	}

#if gcdNEW_PROFILER_FILE
            gcmWRITE_COUNTER(VPC_VG11CALLS, totalVGCalls);
            gcmWRITE_COUNTER(VPC_VG11DRAWCALLS, totalVGDrawCalls);
            gcmWRITE_COUNTER(VPC_VG11STATECHANGECALLS, totalVGStateChangeCalls);

            gcmWRITE_COUNTER(VPC_VG11FILLCOUNT, Context->profiler.drawFillCount);
            gcmWRITE_COUNTER(VPC_VG11STROKECOUNT, Context->profiler.drawStrokeCount);

            gcmWRITE_CONST(VPG_END);
#else
	    	PRINT_XML2(totalVGCalls);
    		PRINT_XML2(totalVGDrawCalls);
    		PRINT_XML2(totalVGStateChangeCalls);

	    	PRINT_XML(drawFillCount);
    		PRINT_XML(drawStrokeCount);

    		_Print(Context, "</VGCounters>\n");
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
    	Context->profiler.drawFillCount   = 0;
    	Context->profiler.drawStrokeCount = 0;
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
vgProfiler(
	gctPOINTER Profiler,
	gctUINT32 Enum,
	gctUINT32 Value
	)
{
    _VGContext* context;

	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	if (! context->profiler.enable)
	{
		return;
	}

    switch (Enum)
    {
    case VG_PROFILER_FRAME_END:
        if (context->profiler.timeEnable)
        {
		    gcoOS_GetTime(&context->profiler.frameEndTimeusec);
			gcoOS_GetCPUTime(&context->profiler.frameEndCPUTimeusec);
		}
        endFrame(context);
        break;

    case VG_PROFILER_PRIMITIVE_END:
        /*endPrimitive(context);*/
        break;

    case VG_PROFILER_PRIMITIVE_TYPE:
        if (context->profiler.drvEnable)
        {
	        context->profiler.primitiveType = Value;
		}
		break;

    case VG_PROFILER_PRIMITIVE_COUNT:
        if (!context->profiler.drvEnable)
            break;

        context->profiler.primitiveCount = Value;

        switch (context->profiler.primitiveType)
        {
        case VG_PATH:
            context->profiler.drawPathCount += Value;
            break;
        case VG_GLYPH:
            context->profiler.drawGlyphCount += Value;
            break;
        case VG_IMAGE:
            context->profiler.drawImageCount += Value;
            break;
        }
        break;

	case VG_PROFILER_STROKE:
        if (context->profiler.drvEnable)
        {
			context->profiler.drawStrokeCount += Value;
		}
		break;

	case VG_PROFILER_FILL:
        if (context->profiler.drvEnable)
        {
			context->profiler.drawFillCount += Value;
		}
		break;

    default:
        if (!context->profiler.drvEnable)
            break;

        if (Enum - APICALLBASE < NUM_API_CALLS)
        {
            context->profiler.apiCalls[Enum - APICALLBASE]++;
        }
        break;
    }
}
#endif
