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
**	Profiler for Vivante HAL.
*/

#include "gc_hal_user_precomp.h"

#define _GC_OBJ_ZONE            gcvZONE_HAL

#if VIVANTE_PROFILER

#if gcdNEW_PROFILER_FILE

/* Write a data value. */
#define gcmWRITE_VALUE(IntData) \
    do \
    { \
        gceSTATUS status; \
        gctINT32 value = IntData; \
        gcmERR_BREAK(gcoPROFILER_Write(gcvNULL, gcmSIZEOF(value), &value)); \
    } \
    while (gcvFALSE)

#define gcmWRITE_CONST(Const) \
    do \
    { \
        gceSTATUS status; \
        gctINT32 data = Const; \
        gcmERR_BREAK(gcoPROFILER_Write(gcvNULL, gcmSIZEOF(data), &data)); \
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
        gcmERR_BREAK(gcoPROFILER_Write(gcvNULL, gcmSIZEOF(length), &length)); \
        gcmERR_BREAK(gcoPROFILER_Write(gcvNULL, length, String)); \
    } \
    while (gcvFALSE)

#define gcmWRITE_BUFFER(Size, Buffer) \
    do \
    { \
        gceSTATUS status; \
        gcmERR_BREAK(gcoPROFILER_Write(gcvNULL, Size, Buffer)); \
    } \
    while (gcvFALSE)

#else

#define gcmWRITE_STRING(String) \
	do \
	{ \
		gceSTATUS status; \
		gctSIZE_T length; \
		gcmERR_BREAK(gcoOS_StrLen((gctSTRING) String, &length)); \
		gcmERR_BREAK(gcoPROFILER_Write(gcvNULL, length, String)); \
	} \
	while (gcvFALSE)

#define gcmWRITE_BUFFER(Buffer, ByteCount) \
	do \
	{ \
		gceSTATUS status; \
		gcmERR_BREAK(gcoPROFILER_Write(gcvNULL, ByteCount, String)); \
	} \
	while (gcvFALSE)

#define gcmPRINT_XML_COUNTER(Counter) \
	do \
	{ \
		char buffer[256]; \
		gctUINT offset = 0; \
		gceSTATUS status; \
		gcmERR_BREAK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), \
										&offset, \
										"<%s value=\"%d\"/>\n", \
										# Counter, \
										gcPLS.hal->profiler.Counter)); \
		gcmWRITE_STRING(buffer); \
	} \
	while (gcvFALSE)

#define gcmPRINT_XML(Format, Value) \
	do \
	{ \
		char buffer[256]; \
		gctUINT offset = 0; \
		gceSTATUS status; \
		gcmERR_BREAK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), \
										&offset, \
										Format, \
										Value)); \
		gcmWRITE_STRING(buffer); \
	} \
	while (gcvFALSE)
#endif

gceSTATUS
gcoPROFILER_Initialize(
	IN gcoHAL Hal
	)
{
    gceSTATUS status = gcvSTATUS_OK;
    char* fileName;
    char* filter = gcvNULL;
    gctSTRING portName;
    gctINT port;
	gcsHAL_INTERFACE iface;

    gcmHEADER();

    /* Check if already initialized. */
   	if (gcPLS.hal->profiler.enable)
    {
        gcPLS.hal->profiler.enable++;
        gcmFOOTER();
        return status;
    }

	/* Get profile setting. */
	iface.command = gcvHAL_GET_PROFILE_SETTING;

	/* Call the kernel. */
	status = gcoOS_DeviceControl(gcvNULL,
								 IOCTL_GCHAL_INTERFACE,
								 &iface, gcmSIZEOF(iface),
								 &iface, gcmSIZEOF(iface));

	if (gcmIS_ERROR(status) || !iface.u.GetProfileSetting.enable)
	{
    	gcPLS.hal->profiler.enable = 0;
        status = gcvSTATUS_GENERIC_IO;

        gcmFOOTER();
        return status;
	}

	gcmVERIFY_OK(gcoOS_ZeroMemory(&gcPLS.hal->profiler, gcmSIZEOF(gcPLS.hal->profiler)));

    gcoOS_GetEnv(gcvNULL,
                 "VP_COUNTER_FILTER",
                 &filter);

    /* Enable/Disable specific counters. */
    if ((filter != gcvNULL))
    {
        gctSIZE_T bitsLen;
        gcoOS_StrLen(filter, &bitsLen);
        if (bitsLen > 2)
        {
            gcPLS.hal->profiler.enableHal = (filter[2] == '1');
        }
        else
        {
            gcPLS.hal->profiler.enableHal = gcvTRUE;
        }

        if (bitsLen > 3)
        {
            gcPLS.hal->profiler.enableHW = (filter[3] == '1');
        }
        else
        {
            gcPLS.hal->profiler.enableHW = gcvTRUE;
        }

        if (bitsLen > 8)
        {
            gcPLS.hal->profiler.enableSH = (filter[8] == '1');
        }
        else
        {
            gcPLS.hal->profiler.enableSH = gcvTRUE;
        }
    }
    else
    {
        gcPLS.hal->profiler.enableHal = gcvTRUE;
        gcPLS.hal->profiler.enableHW = gcvTRUE;
        gcPLS.hal->profiler.enableSH = gcvTRUE;
    }

	gcoOS_GetEnv(gcvNULL,
				 "VPROFILER_OUTPUT",
				 &fileName);

    gcPLS.hal->profiler.useSocket = gcvFALSE;
	if (fileName && *fileName != '\0' && *fileName != ' ')
	{
        /* Extract port info. */
        gcoOS_StrFindReverse(fileName, ':', &portName);

        if (portName)
        {
            gcoOS_StrToInt(portName + 1, &port);

            if (port > 0)
            {
                /*status = gcoOS_Socket(gcvNULL, AF_INET, SOCK_STREAM, 0, &gcPLS.hal->profiler.sockFd);*/
                status = gcoOS_Socket(gcvNULL, 2, 1, 0, &gcPLS.hal->profiler.sockFd);

                if (gcmIS_SUCCESS(status))
                {
                    *portName = '\0';
                    status = gcoOS_Connect(gcvNULL,
                            gcPLS.hal->profiler.sockFd, fileName, port);
                    *portName = ':';

                    if (gcmIS_SUCCESS(status))
	                {
                        gcPLS.hal->profiler.useSocket = gcvTRUE;
                    }
                }
            }
        }
	}
    else
    {
		fileName = iface.u.GetProfileSetting.fileName;
    }

    if (! gcPLS.hal->profiler.useSocket)
	{
		status = gcoOS_Open(gcvNULL,
							fileName,
#ifdef gcdNEW_PROFILER_FILE
							gcvFILE_CREATE,
#else
							gcvFILE_CREATETEXT,
#endif
							&gcPLS.hal->profiler.file);
	}

    if (gcmIS_ERROR(status))
	{
    	gcPLS.hal->profiler.enable = 0;
        status = gcvSTATUS_GENERIC_IO;

        gcmFOOTER();
        return status;
	}

    gcPLS.hal->profiler.enable = 1;
	gcoOS_GetTime(&gcPLS.hal->profiler.frameStart);
	gcPLS.hal->profiler.frameStartTimeusec = gcPLS.hal->profiler.frameStart;
	gcPLS.hal->profiler.prevVSInstCount = 0;
	gcPLS.hal->profiler.prevVSBranchInstCount = 0;
	gcPLS.hal->profiler.prevVSTexInstCount = 0;
	gcPLS.hal->profiler.prevVSVertexCount = 0;
	gcPLS.hal->profiler.prevPSInstCount = 0;
	gcPLS.hal->profiler.prevPSBranchInstCount = 0;
	gcPLS.hal->profiler.prevPSTexInstCount = 0;
	gcPLS.hal->profiler.prevPSPixelCount = 0;

#if gcdNEW_PROFILER_FILE
    gcmWRITE_CONST(VPHEADER);
    gcmWRITE_BUFFER(4, "VP12");
#else
	gcmWRITE_STRING("<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n<VProfile>\n");
#endif

    /* Success. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoPROFILER_Destroy(
	IN gcoHAL Hal
	)
{
    gcmHEADER();

   	if (gcPLS.hal->profiler.enable == 1)
    {
#if gcdNEW_PROFILER_FILE
        gcmWRITE_CONST(VPG_END);
#else
    	gcmWRITE_STRING("</VProfile>\n");
#endif

        gcoPROFILER_Flush(gcvNULL);
        if (gcPLS.hal->profiler.useSocket)
        {
            /* Close the socket. */
            gcmVERIFY_OK(gcoOS_CloseSocket(gcvNULL, gcPLS.hal->profiler.sockFd));
        }
        else
        {
            /* Close the profiler file. */
            gcmVERIFY_OK(gcoOS_Close(gcvNULL, gcPLS.hal->profiler.file));
        }
        gcPLS.hal->profiler.enable = 0;
    }
    else
    {
        gcPLS.hal->profiler.enable--;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Write data to profile. */
gceSTATUS
gcoPROFILER_Write(
    IN gcoHAL Hal,
    IN gctSIZE_T ByteCount,
    IN gctCONST_POINTER Data
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("ByteCount=%lu Data=0x%x", ByteCount, Data);

    /* Check if already destroyed. */
   	if (gcPLS.hal->profiler.enable)
    {
        if (gcPLS.hal->profiler.useSocket)
        {
            status = gcoOS_Send(gcvNULL,
                                gcPLS.hal->profiler.sockFd,
                                ByteCount, Data, 0);
        }
        else
        {
            status = gcoOS_Write(gcvNULL,
                                 gcPLS.hal->profiler.file,
                                 ByteCount, Data);
        }
    }

    gcmFOOTER();
    return status;
}

/* Flush data out. */
gceSTATUS
gcoPROFILER_Flush(
    IN gcoHAL Hal
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

   	if (gcPLS.hal->profiler.enable)
    {
        if (gcPLS.hal->profiler.useSocket)
        {
            status = gcoOS_WaitForSend(gcvNULL,
                                       gcPLS.hal->profiler.sockFd,
                                       0, 50000);
        }
        else
        {
            status = gcoOS_Flush(gcvNULL,
                                 gcPLS.hal->profiler.file);
        }
	}

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoPROFILER_Count(
	IN gcoHAL Hal,
	IN gctUINT32 Enum,
	IN gctINT Value
	)
{
#if PROFILE_HAL_COUNTERS
    gcmHEADER_ARG("Enum=%lu Value=%d", Enum, Value);

	if (gcPLS.hal->profiler.enable)
	{
		switch (Enum)
		{
		case GLINDEX_OBJECT:
			gcPLS.hal->profiler.indexBufferNewObjectsAlloc   += Value;
			gcPLS.hal->profiler.indexBufferTotalObjectsAlloc += Value;
			break;

		case GLINDEX_OBJECT_BYTES:
			gcPLS.hal->profiler.indexBufferNewBytesAlloc   += Value;
			gcPLS.hal->profiler.indexBufferTotalBytesAlloc += Value;
			break;

		case GLVERTEX_OBJECT:
			gcPLS.hal->profiler.vertexBufferNewObjectsAlloc   += Value;
			gcPLS.hal->profiler.vertexBufferTotalObjectsAlloc += Value;
			break;

		case GLVERTEX_OBJECT_BYTES:
			gcPLS.hal->profiler.vertexBufferNewBytesAlloc   += Value;
			gcPLS.hal->profiler.vertexBufferTotalBytesAlloc += Value;
			break;

		case GLTEXTURE_OBJECT:
			gcPLS.hal->profiler.textureBufferNewObjectsAlloc   += Value;
			gcPLS.hal->profiler.textureBufferTotalObjectsAlloc += Value;
			break;

		case GLTEXTURE_OBJECT_BYTES:
			gcPLS.hal->profiler.textureBufferNewBytesAlloc   += Value;
			gcPLS.hal->profiler.textureBufferTotalBytesAlloc += Value;
			break;

		default:
			break;
		}
	}
#endif

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#ifndef VIVANTE_NO_3D
#if !gcdNEW_PROFILER_FILE
gceSTATUS
gcoPROFILER_Shader(
    IN gcoHAL Hal,
    IN gcSHADER Shader
    )
{
/*#if PROFILE_SHADER_COUNTERS*/

    gcmHEADER_ARG("Shader=0x%x", Shader);

	if (gcPLS.hal->profiler.enableSH)
	{
	    gctUINT16 alu = 0, tex = 0, i;

		if (gcPLS.hal->profiler.enable)
		{
		    /* Profile shader */
    		for (i = 0; i < Shader->codeCount; i++ )
    		{
	        	switch (Shader->code[i].opcode)
    	    	{
				case gcSL_NOP:
					break;

				case gcSL_TEXLD:
					tex++;
					break;

				default:
					alu++;
					break;
				}
			}

    		gcmPRINT_XML("<InstructionCount value=\"%d\"/>\n", tex + alu);
			gcmPRINT_XML("<ALUInstructionCount value=\"%d\"/>\n", alu);
			gcmPRINT_XML("<TextureInstructionCount value=\"%d\"/>\n", tex);
			gcmPRINT_XML("<Attributes value=\"%lu\"/>\n", Shader->attributeCount);
			gcmPRINT_XML("<Uniforms value=\"%lu\"/>\n", Shader->uniformCount);
			gcmPRINT_XML("<Functions value=\"%lu\"/>\n", Shader->functionCount);
		}
/*#endif*/
	}

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}
#endif

gceSTATUS
gcoPROFILER_ShaderVS(
    IN gcoHAL Hal,
    IN gctPOINTER Vs
    )
{
/*#if PROFILE_SHADER_COUNTERS*/
    gcmHEADER_ARG("Vs=0x%x", Vs);

	if (gcPLS.hal->profiler.enableSH)
	{
		if (gcPLS.hal->profiler.enable)
		{
#if gcdNEW_PROFILER_FILE
            gctUINT16 alu = 0, tex = 0, i;
            gcSHADER Shader = (gcSHADER)Vs;

            /* Profile shader */
            for (i = 0; i < Shader->codeCount; i++ )
            {
                switch (Shader->code[i].opcode)
                {
                case gcSL_NOP:
                    break;

                case gcSL_TEXLD:
                    tex++;
                    break;

                default:
                    alu++;
                    break;
                }
            }

            gcmWRITE_CONST(VPG_PVS);

            gcmWRITE_COUNTER(VPC_PVSINSTRCOUNT, (tex + alu));
            gcmWRITE_COUNTER(VPC_PVSALUINSTRCOUNT, alu);
            gcmWRITE_COUNTER(VPC_PVSTEXINSTRCOUNT, tex);
            gcmWRITE_COUNTER(VPC_PVSATTRIBCOUNT, (Shader->attributeCount));
            gcmWRITE_COUNTER(VPC_PVSUNIFORMCOUNT, (Shader->uniformCount));
            gcmWRITE_COUNTER(VPC_PVSFUNCTIONCOUNT, (Shader->functionCount));
#else
			gcmWRITE_STRING("<VS>\n");
			gcoPROFILER_Shader(gcvNULL, (gcSHADER) Vs);
#endif
#if gcdNEW_PROFILER_FILE
            gcmWRITE_CONST(VPG_END);
#else
			gcmWRITE_STRING("</VS>\n");
#endif
		}
	}
/*#endif*/

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoPROFILER_ShaderFS(
    IN gcoHAL Hal,
    IN void* Fs
    )
{
/*#if PROFILE_SHADER_COUNTERS*/
    gcmHEADER_ARG("Fs=0x%x", Fs);

	if (gcPLS.hal->profiler.enableSH)
	{
		if (gcPLS.hal->profiler.enable)
		{
#if gcdNEW_PROFILER_FILE
            gctUINT16 alu = 0, tex = 0, i;
            gcSHADER Shader = (gcSHADER)Fs;

            /* Profile shader */
            for (i = 0; i < Shader->codeCount; i++ )
            {
                switch (Shader->code[i].opcode)
                {
                case gcSL_NOP:
                    break;

                case gcSL_TEXLD:
                    tex++;
                    break;

                default:
                    alu++;
                    break;
                }
            }

            gcmWRITE_CONST(VPG_PPS);
            gcmWRITE_COUNTER(VPC_PPSINSTRCOUNT, (tex + alu));
            gcmWRITE_COUNTER(VPC_PPSALUINSTRCOUNT, alu);
            gcmWRITE_COUNTER(VPC_PPSTEXINSTRCOUNT, tex);
            gcmWRITE_COUNTER(VPC_PPSATTRIBCOUNT, (Shader->attributeCount));
            gcmWRITE_COUNTER(VPC_PPSUNIFORMCOUNT, (Shader->uniformCount));
            gcmWRITE_COUNTER(VPC_PPSFUNCTIONCOUNT, (Shader->functionCount));
#else
			gcmWRITE_STRING("<FS>\n");
			gcoPROFILER_Shader(gcvNULL, (gcSHADER) Fs);
#endif
#if gcdNEW_PROFILER_FILE
            gcmWRITE_CONST(VPG_END);
#else
		gcmWRITE_STRING("</FS>\n");
#endif
		}
	}
/*#endif*/

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#endif /* vivante_no_3d */
gceSTATUS
gcoPROFILER_EndFrame(
    IN gcoHAL Hal
    )
{
/*#if	(PROFILE_HAL_COUNTERS || PROFILE_HW_COUNTERS)*/
	gcsHAL_INTERFACE iface;
    gceSTATUS status;
/*#endif*/

    gcmHEADER();

	if (!gcPLS.hal->profiler.enable)
	{
        gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}

/*#if PROFILE_HAL_COUNTERS*/
    if (gcPLS.hal->profiler.enableHal)
    {
#if gcdNEW_PROFILER_FILE
        gcmWRITE_CONST(VPG_HAL);

        gcmWRITE_COUNTER(VPC_HALVERTBUFNEWBYTEALLOC, gcPLS.hal->profiler.vertexBufferNewBytesAlloc);
        gcmWRITE_COUNTER(VPC_HALVERTBUFTOTALBYTEALLOC, gcPLS.hal->profiler.vertexBufferTotalBytesAlloc);
        gcmWRITE_COUNTER(VPC_HALVERTBUFNEWOBJALLOC, gcPLS.hal->profiler.vertexBufferNewObjectsAlloc);
        gcmWRITE_COUNTER(VPC_HALVERTBUFTOTALOBJALLOC, gcPLS.hal->profiler.vertexBufferTotalObjectsAlloc);

        gcmWRITE_COUNTER(VPC_HALINDBUFNEWBYTEALLOC, gcPLS.hal->profiler.indexBufferNewBytesAlloc);
        gcmWRITE_COUNTER(VPC_HALINDBUFTOTALBYTEALLOC, gcPLS.hal->profiler.indexBufferTotalBytesAlloc);
        gcmWRITE_COUNTER(VPC_HALINDBUFNEWOBJALLOC, gcPLS.hal->profiler.indexBufferNewObjectsAlloc);
        gcmWRITE_COUNTER(VPC_HALINDBUFTOTALOBJALLOC, gcPLS.hal->profiler.indexBufferTotalObjectsAlloc);

        gcmWRITE_COUNTER(VPC_HALTEXBUFNEWBYTEALLOC, gcPLS.hal->profiler.textureBufferNewBytesAlloc);
        gcmWRITE_COUNTER(VPC_HALTEXBUFTOTALBYTEALLOC, gcPLS.hal->profiler.textureBufferTotalBytesAlloc);
        gcmWRITE_COUNTER(VPC_HALTEXBUFNEWOBJALLOC, gcPLS.hal->profiler.textureBufferNewObjectsAlloc);
        gcmWRITE_COUNTER(VPC_HALTEXBUFTOTALOBJALLOC, gcPLS.hal->profiler.textureBufferTotalObjectsAlloc);

        gcmWRITE_CONST(VPG_END);
#else
    	gcmWRITE_STRING("<HALCounters>\n");

	    gcmPRINT_XML_COUNTER(vertexBufferNewBytesAlloc);
    	gcmPRINT_XML_COUNTER(vertexBufferTotalBytesAlloc);
	    gcmPRINT_XML_COUNTER(vertexBufferNewObjectsAlloc);
    	gcmPRINT_XML_COUNTER(vertexBufferTotalObjectsAlloc);

	    gcmPRINT_XML_COUNTER(indexBufferNewBytesAlloc);
    	gcmPRINT_XML_COUNTER(indexBufferTotalBytesAlloc);
	    gcmPRINT_XML_COUNTER(indexBufferNewObjectsAlloc);
    	gcmPRINT_XML_COUNTER(indexBufferTotalObjectsAlloc);

	    gcmPRINT_XML_COUNTER(textureBufferNewBytesAlloc);
    	gcmPRINT_XML_COUNTER(textureBufferTotalBytesAlloc);
	    gcmPRINT_XML_COUNTER(textureBufferNewObjectsAlloc);
    	gcmPRINT_XML_COUNTER(textureBufferTotalObjectsAlloc);

	    gcmWRITE_STRING("</HALCounters>\n");
#endif

	    /* Reset per-frame counters. */
    	gcPLS.hal->profiler.vertexBufferNewBytesAlloc   = 0;
	    gcPLS.hal->profiler.vertexBufferNewObjectsAlloc = 0;

	    gcPLS.hal->profiler.indexBufferNewBytesAlloc   = 0;
    	gcPLS.hal->profiler.indexBufferNewObjectsAlloc = 0;

	    gcPLS.hal->profiler.textureBufferNewBytesAlloc   = 0;
  		gcPLS.hal->profiler.textureBufferNewObjectsAlloc = 0;
	}
/*#endif*/

/*#if PROFILE_HW_COUNTERS*/
    /* gcvHAL_READ_ALL_PROFILE_REGISTERS. */
    if (gcPLS.hal->profiler.enableHW)
    {
		iface.command = gcvHAL_READ_ALL_PROFILE_REGISTERS;

		/* Call the kernel. */
		status = gcoOS_DeviceControl(gcvNULL,
									 IOCTL_GCHAL_INTERFACE,
									 &iface, gcmSIZEOF(iface),
									 &iface, gcmSIZEOF(iface));

		/* Verify result. */
		if (gcmNO_ERROR(status))
		{
        #define gcmCOUNTER(name)    iface.u.RegisterProfileData.counters.name

#if gcdNEW_PROFILER_FILE
            gcmWRITE_CONST(VPG_HW);
            gcmWRITE_CONST(VPG_GPU);
            gcmWRITE_COUNTER(VPC_GPUREAD64BYTE, gcmCOUNTER(gpuTotalRead64BytesPerFrame));
            gcmWRITE_COUNTER(VPC_GPUWRITE64BYTE, gcmCOUNTER(gpuTotalWrite64BytesPerFrame));
            gcmWRITE_COUNTER(VPC_GPUCYCLES, gcmCOUNTER(gpuCyclesCounter));
            gcmWRITE_CONST(VPG_END);

            gcmWRITE_CONST(VPG_VS);
            gcmWRITE_COUNTER(VPC_VSINSTCOUNT, gcmCOUNTER(vs_inst_counter) - gcPLS.hal->profiler.prevVSInstCount);
            gcmWRITE_COUNTER(VPC_VSBRANCHINSTCOUNT, gcmCOUNTER(vtx_branch_inst_counter) - gcPLS.hal->profiler.prevVSBranchInstCount);
            gcmWRITE_COUNTER(VPC_VSTEXLDINSTCOUNT, gcmCOUNTER(vtx_texld_inst_counter) - gcPLS.hal->profiler.prevVSTexInstCount);
			gcmWRITE_COUNTER(VPC_VSRENDEREDVERTCOUNT, gcmCOUNTER(rendered_vertice_counter) - gcPLS.hal->profiler.prevVSVertexCount);
            gcmWRITE_CONST(VPG_END);

			gcPLS.hal->profiler.prevVSInstCount = gcmCOUNTER(vs_inst_counter);
			gcPLS.hal->profiler.prevVSBranchInstCount = gcmCOUNTER(vtx_branch_inst_counter);
			gcPLS.hal->profiler.prevVSTexInstCount = gcmCOUNTER(vtx_texld_inst_counter);
			gcPLS.hal->profiler.prevVSVertexCount = gcmCOUNTER(rendered_vertice_counter);

            gcmWRITE_CONST(VPG_PA);
            gcmWRITE_COUNTER(VPC_PAINVERTCOUNT, gcmCOUNTER(pa_input_vtx_counter));
            gcmWRITE_COUNTER(VPC_PAINPRIMCOUNT, gcmCOUNTER(pa_input_prim_counter));
            gcmWRITE_COUNTER(VPC_PAOUTPRIMCOUNT, gcmCOUNTER(pa_output_prim_counter));
            gcmWRITE_COUNTER(VPC_PADEPTHCLIPCOUNT, gcmCOUNTER(pa_depth_clipped_counter));
            gcmWRITE_COUNTER(VPC_PATRIVIALREJCOUNT, gcmCOUNTER(pa_trivial_rejected_counter));
            gcmWRITE_COUNTER(VPC_PACULLCOUNT, gcmCOUNTER(pa_culled_counter));
            gcmWRITE_CONST(VPG_END);

            gcmWRITE_CONST(VPG_SETUP);
            gcmWRITE_COUNTER(VPC_SETRIANGLECOUNT, gcmCOUNTER(se_culled_triangle_count));
            gcmWRITE_COUNTER(VPC_SELINECOUNT, gcmCOUNTER(se_culled_lines_count));
            gcmWRITE_CONST(VPG_END);

            gcmWRITE_CONST(VPG_RA);
            gcmWRITE_COUNTER(VPC_RAVALIDPIXCOUNT, gcmCOUNTER(ra_valid_pixel_count));
            gcmWRITE_COUNTER(VPC_RATOTALQUADCOUNT, gcmCOUNTER(ra_total_quad_count));
            gcmWRITE_COUNTER(VPC_RAVALIDQUADCOUNTEZ, gcmCOUNTER(ra_valid_quad_count_after_early_z));
            gcmWRITE_COUNTER(VPC_RATOTALPRIMCOUNT, gcmCOUNTER(ra_total_primitive_count));
            gcmWRITE_COUNTER(VPC_RAPIPECACHEMISSCOUNT, gcmCOUNTER(ra_pipe_cache_miss_counter));
            gcmWRITE_COUNTER(VPC_RAPREFCACHEMISSCOUNT, gcmCOUNTER(ra_prefetch_cache_miss_counter));
            gcmWRITE_COUNTER(VPC_RAEEZCULLCOUNT, gcmCOUNTER(ra_eez_culled_counter));
            gcmWRITE_CONST(VPG_END);

            gcmWRITE_CONST(VPG_TX);
            gcmWRITE_COUNTER(VPC_TXTOTBILINEARREQ, gcmCOUNTER(tx_total_bilinear_requests));
            gcmWRITE_COUNTER(VPC_TXTOTTRILINEARREQ, gcmCOUNTER(tx_total_trilinear_requests));
            gcmWRITE_COUNTER(VPC_TXTOTTEXREQ, gcmCOUNTER(tx_total_texture_requests));
            gcmWRITE_COUNTER(VPC_TXMEMREADCOUNT, gcmCOUNTER(tx_mem_read_count));
            gcmWRITE_COUNTER(VPC_TXMEMREADIN8BCOUNT, gcmCOUNTER(tx_mem_read_in_8B_count));
            gcmWRITE_COUNTER(VPC_TXCACHEMISSCOUNT, gcmCOUNTER(tx_cache_miss_count));
            gcmWRITE_COUNTER(VPC_TXCACHEHITTEXELCOUNT, gcmCOUNTER(tx_cache_hit_texel_count));
            gcmWRITE_COUNTER(VPC_TXCACHEMISSTEXELCOUNT, gcmCOUNTER(tx_cache_miss_texel_count));
            gcmWRITE_CONST(VPG_END);

            gcmWRITE_CONST(VPG_PS);
            gcmWRITE_COUNTER(VPC_PSINSTCOUNT, gcmCOUNTER(ps_inst_counter) - gcPLS.hal->profiler.prevPSInstCount);
            gcmWRITE_COUNTER(VPC_PSBRANCHINSTCOUNT, gcmCOUNTER(pxl_branch_inst_counter) - gcPLS.hal->profiler.prevPSBranchInstCount);
            gcmWRITE_COUNTER(VPC_PSTEXLDINSTCOUNT, gcmCOUNTER(pxl_texld_inst_counter) - gcPLS.hal->profiler.prevPSTexInstCount);
			gcmWRITE_COUNTER(VPC_PSRENDEREDPIXCOUNT, gcmCOUNTER(rendered_pixel_counter) - gcPLS.hal->profiler.prevPSPixelCount);
            gcmWRITE_CONST(VPG_END);

			gcPLS.hal->profiler.prevPSInstCount = gcmCOUNTER(ps_inst_counter);
			gcPLS.hal->profiler.prevPSBranchInstCount = gcmCOUNTER(pxl_branch_inst_counter);
			gcPLS.hal->profiler.prevPSTexInstCount = gcmCOUNTER(pxl_texld_inst_counter);
			gcPLS.hal->profiler.prevPSPixelCount = gcmCOUNTER(rendered_pixel_counter);

            gcmWRITE_CONST(VPG_PE);
            gcmWRITE_COUNTER(VPC_PEKILLEDBYCOLOR, gcmCOUNTER(pe_pixel_count_killed_by_color_pipe));
            gcmWRITE_COUNTER(VPC_PEKILLEDBYDEPTH, gcmCOUNTER(pe_pixel_count_killed_by_depth_pipe));
            gcmWRITE_COUNTER(VPC_PEDRAWNBYCOLOR, gcmCOUNTER(pe_pixel_count_drawn_by_color_pipe));
            gcmWRITE_COUNTER(VPC_PEDRAWNBYDEPTH, gcmCOUNTER(pe_pixel_count_drawn_by_depth_pipe));
            gcmWRITE_CONST(VPG_END);

            gcmWRITE_CONST(VPG_MC);
            gcmWRITE_COUNTER(VPC_MCREADREQ8BPIPE, gcmCOUNTER(mc_total_read_req_8B_from_pipeline));
            gcmWRITE_COUNTER(VPC_MCREADREQ8BIP, gcmCOUNTER(mc_total_read_req_8B_from_IP));
            gcmWRITE_COUNTER(VPC_MCWRITEREQ8BPIPE, gcmCOUNTER(mc_total_write_req_8B_from_pipeline));
            gcmWRITE_CONST(VPG_END);

            gcmWRITE_CONST(VPG_AXI);
            gcmWRITE_COUNTER(VPC_AXIREADREQSTALLED, gcmCOUNTER(hi_axi_cycles_read_request_stalled));
            gcmWRITE_COUNTER(VPC_AXIWRITEREQSTALLED, gcmCOUNTER(hi_axi_cycles_write_request_stalled));
            gcmWRITE_COUNTER(VPC_AXIWRITEDATASTALLED, gcmCOUNTER(hi_axi_cycles_write_data_stalled));
            gcmWRITE_CONST(VPG_END);

            gcmWRITE_CONST(VPG_END);
#else
		    gcmWRITE_STRING("<HWCounters>\n");

	        gcmPRINT_XML("<read_64Byte value=\"%u\"/>\n",
    	    			 gcmCOUNTER(gpuTotalRead64BytesPerFrame));
        	gcmPRINT_XML("<write_64Byte value=\"%u\"/>\n",
	        			 gcmCOUNTER(gpuTotalWrite64BytesPerFrame));
	        gcmPRINT_XML("<gpu_cycles value=\"%u\"/>\n",
    	    			 gcmCOUNTER(gpuCyclesCounter));
        	gcmPRINT_XML("<pe_pixel_count_killed_by_color_pipe value=\"%u\"/>\n",
        				 gcmCOUNTER(pe_pixel_count_killed_by_color_pipe));
	        gcmPRINT_XML("<pe_pixel_count_killed_by_depth_pipe value=\"%u\"/>\n",
    	    			 gcmCOUNTER(pe_pixel_count_killed_by_depth_pipe));
        	gcmPRINT_XML("<pe_pixel_count_drawn_by_color_pipe value=\"%u\"/>\n",
						 gcmCOUNTER(pe_pixel_count_drawn_by_color_pipe));
	        gcmPRINT_XML("<pe_pixel_count_drawn_by_depth_pipe value=\"%u\"/>\n",
						 gcmCOUNTER(pe_pixel_count_drawn_by_depth_pipe));
        	gcmPRINT_XML("<ps_inst_counter value=\"%u\"/>\n",
						 gcmCOUNTER(ps_inst_counter));
	        gcmPRINT_XML("<rendered_pixel_counter value=\"%u\"/>\n",
						 gcmCOUNTER(rendered_pixel_counter));
        	gcmPRINT_XML("<vs_inst_counter value=\"%u\"/>\n",
						 gcmCOUNTER(vs_inst_counter));
	        gcmPRINT_XML("<rendered_vertice_counter value=\"%u\"/>\n",
						 gcmCOUNTER(rendered_vertice_counter));
        	gcmPRINT_XML("<vtx_branch_inst_counter value=\"%u\"/>\n",
					 gcmCOUNTER(vtx_branch_inst_counter));
	        gcmPRINT_XML("<vtx_texld_inst_counter value=\"%u\"/>\n",
						 gcmCOUNTER(vtx_texld_inst_counter));
        	gcmPRINT_XML("<pxl_branch_inst_counter value=\"%u\"/>\n",
						 gcmCOUNTER(pxl_branch_inst_counter));
	        gcmPRINT_XML("<pxl_texld_inst_counter value=\"%u\"/>\n",
						 gcmCOUNTER(pxl_texld_inst_counter));
        	gcmPRINT_XML("<pa_input_vtx_counter value=\"%u\"/>\n",
					 gcmCOUNTER(pa_input_vtx_counter));
	        gcmPRINT_XML("<pa_input_prim_counter value=\"%u\"/>\n",
						 gcmCOUNTER(pa_input_prim_counter));
        	gcmPRINT_XML("<pa_output_prim_counter value=\"%u\"/>\n",
					 gcmCOUNTER(pa_output_prim_counter));
	        gcmPRINT_XML("<pa_depth_clipped_counter value=\"%u\"/>\n",
						 gcmCOUNTER(pa_depth_clipped_counter));
        	gcmPRINT_XML("<pa_trivial_rejected_counter value=\"%u\"/>\n",
					 gcmCOUNTER(pa_trivial_rejected_counter));
	        gcmPRINT_XML("<pa_culled_counter value=\"%u\"/>\n",
						 gcmCOUNTER(pa_culled_counter));
        	gcmPRINT_XML("<se_culled_triangle_count value=\"%u\"/>\n",
						 gcmCOUNTER(se_culled_triangle_count));
	        gcmPRINT_XML("<se_culled_lines_count value=\"%u\"/>\n",
						 gcmCOUNTER(se_culled_lines_count));
        	gcmPRINT_XML("<ra_valid_pixel_count value=\"%u\"/>\n",
					 gcmCOUNTER(ra_valid_pixel_count));
	        gcmPRINT_XML("<ra_total_quad_count value=\"%u\"/>\n",
						 gcmCOUNTER(ra_total_quad_count));
        	gcmPRINT_XML("<ra_valid_quad_count_after_early_z value=\"%u\"/>\n",
					 gcmCOUNTER(ra_valid_quad_count_after_early_z));
	        gcmPRINT_XML("<ra_total_primitive_count value=\"%u\"/>\n",
						 gcmCOUNTER(ra_total_primitive_count));
        	gcmPRINT_XML("<ra_pipe_cache_miss_counter value=\"%u\"/>\n",
						 gcmCOUNTER(ra_pipe_cache_miss_counter));
	        gcmPRINT_XML("<ra_prefetch_cache_miss_counter value=\"%u\"/>\n",
						 gcmCOUNTER(ra_prefetch_cache_miss_counter));
        	gcmPRINT_XML("<ra_eez_culled_counter value=\"%u\"/>\n",
					 gcmCOUNTER(ra_eez_culled_counter));
	        gcmPRINT_XML("<tx_total_bilinear_requests value=\"%u\"/>\n",
						 gcmCOUNTER(tx_total_bilinear_requests));
        	gcmPRINT_XML("<tx_total_trilinear_requests value=\"%u\"/>\n",
					 gcmCOUNTER(tx_total_trilinear_requests));
	        gcmPRINT_XML("<tx_total_discarded_texture_requests value=\"%u\"/>\n",
						 gcmCOUNTER(tx_total_discarded_texture_requests));
        	gcmPRINT_XML("<tx_total_texture_requests value=\"%u\"/>\n",
						 gcmCOUNTER(tx_total_texture_requests));
    	    gcmPRINT_XML("<tx_mem_read_count value=\"%u\"/>\n",
						 gcmCOUNTER(tx_mem_read_count));
	        gcmPRINT_XML("<tx_mem_read_in_8B_count value=\"%u\"/>\n",
						 gcmCOUNTER(tx_mem_read_in_8B_count));
        	gcmPRINT_XML("<tx_cache_miss_count value=\"%u\"/>\n",
						 gcmCOUNTER(tx_cache_miss_count));
    	    gcmPRINT_XML("<tx_cache_hit_texel_count value=\"%u\"/>\n",
						 gcmCOUNTER(tx_cache_hit_texel_count));
	        gcmPRINT_XML("<tx_cache_miss_texel_count value=\"%u\"/>\n",
						 gcmCOUNTER(tx_cache_miss_texel_count));
        	gcmPRINT_XML("<mc_total_read_req_8B_from_pipeline value=\"%u\"/>\n",
						 gcmCOUNTER(mc_total_read_req_8B_from_pipeline));
    	    gcmPRINT_XML("<mc_total_read_req_8B_from_IP value=\"%u\"/>\n",
						 gcmCOUNTER(mc_total_read_req_8B_from_IP));
	        gcmPRINT_XML("<mc_total_write_req_8B_from_pipeline value=\"%u\"/>\n",
						 gcmCOUNTER(mc_total_write_req_8B_from_pipeline));
	        gcmPRINT_XML("<hi_axi_cycles_read_request_stalled value=\"%u\"/>\n",
						 gcmCOUNTER(hi_axi_cycles_read_request_stalled));
	        gcmPRINT_XML("<hi_axi_cycles_write_request_stalled value=\"%u\"/>\n",
						 gcmCOUNTER(hi_axi_cycles_write_request_stalled));
        	gcmPRINT_XML("<hi_axi_cycles_write_data_stalled value=\"%u\"/>\n",
						 gcmCOUNTER(hi_axi_cycles_write_data_stalled));

	    	gcmWRITE_STRING("</HWCounters>\n");
#endif
		}
	}
/*#endif*/

	/* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#else
/* Stubs for Profiler functions. */

/* Initialize the gcsProfiler. */
gceSTATUS
gcoPROFILER_Initialize(
    IN gcoHAL Hal
    )
{
    gcmHEADER();
    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}

/* Destroy the gcProfiler. */
gceSTATUS
gcoPROFILER_Destroy(
    IN gcoHAL Hal
    )
{
    gcmHEADER();
    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}

/* Write data to profile. */
gceSTATUS
gcoPROFILER_Write(
    IN gcoHAL Hal,
    IN gctSIZE_T ByteCount,
    IN gctCONST_POINTER Data
    )
{
    gcmHEADER_ARG("ByteCount=%lu Data=0x%x", ByteCount, Data);

    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}

/* Flush data out. */
gceSTATUS
gcoPROFILER_Flush(
    IN gcoHAL Hal
    )
{
    gcmHEADER();
    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}

/* Call to signal end of frame. */
gceSTATUS
gcoPROFILER_EndFrame(
    IN gcoHAL Hal
    )
{
    gcmHEADER();
    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}

gceSTATUS
gcoPROFILER_Count(
	IN gcoHAL Hal,
	IN gctUINT32 Enum,
	IN gctINT Value
	)
{
    gcmHEADER_ARG("Enum=%lu Value=%d", Enum, Value);

    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}

/* Profile input vertex shader. */
gceSTATUS
gcoPROFILER_ShaderVS(
    IN gcoHAL Hal,
    IN gctPOINTER Vs
    )
{
    gcmHEADER_ARG("Vs=0x%x", Vs);
    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}

/* Profile input fragment shader. */
gceSTATUS
gcoPROFILER_ShaderFS(
    IN gcoHAL Hal,
    IN gctPOINTER Fs
    )
{
    gcmHEADER_ARG("Fs=0x%x", Fs);
    gcmFOOTER_NO();
    return gcvSTATUS_INVALID_REQUEST;
}

#endif /* VIVANTE_PROFILER */
