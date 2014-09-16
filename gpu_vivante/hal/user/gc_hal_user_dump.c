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




#include "gc_hal_user_precomp.h"
#include "gc_hal_user_debug.h"

#if gcdDUMP_FRAMERATE
static gctINT32 totalFrames = -gcdDUMP_FRAMERATE;
static gctINT32 previousTime = 0;
static gctINT32 totalTicks = 0;
#endif

#if defined(ANDROID) && gcdDUMP_API
#include <android/log.h>
#include <linux/spinlock.h>
#include <pthread.h>
#include <sys/syscall.h>
#include "utils/Log.h"

static gctUINT32 prevThreadID = 0;

static gctPOINTER dump_mutex = gcvNULL;
#define gcmLOCKDUMP() \
{ \
    if (dump_mutex == gcvNULL) \
        gcoOS_CreateMutex(gcvNULL, &dump_mutex); \
    gcoOS_AcquireMutex(gcvNULL, dump_mutex, gcvINFINITE); \
}

#define gcmUNLOCKDUMP() \
  gcoOS_ReleaseMutex(gcvNULL, dump_mutex);

#define gcmGETTHREADID() \
    (gctUINT32)pthread_self()

#endif

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HAL

/* gcoDUMP object structure. */
struct _gcoDUMP
{
    /* The object. */
    gcsOBJECT       object;

    /* Pointer to open file and accumulated length. */
    gctFILE         file;
    gctSIZE_T       length;

    /* Frame information. */
    gctINT32        frameCount;
    gctUINT32       frameStart;
    gctUINT32       frameLength;
};

/*******************************************************************************
**
**  gcoDUMP_Construct
**
**  Construct a new gcoDUMP object.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**      gcoOS Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcoDUMP * Dump
**          Pointer to a variable receiving the gcoDUMP object pointer.
*/
gceSTATUS
gcoDUMP_Construct(
    IN gcoOS Os,
    IN gcoHAL Hal,
    OUT gcoDUMP * Dump
    )
{
    gceSTATUS status;
    gcoDUMP dump;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Os=0x%x Dump=0x%x", Os, Dump);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Dump != gcvNULL);

    do
    {
        /* Allocate the gcoDUMP structure. */
        gcmERR_BREAK(gcoOS_Allocate(Os,
                                    sizeof(struct _gcoDUMP),
                                    &pointer));

        dump = pointer;

        /* Initialize the gcoDUMP object. */
        dump->object.type = gcvOBJ_DUMP;
        dump->file        = gcvNULL;

        /* Return pointer to the object. */
        *Dump = dump;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER_ARG("*Dump=0x%x", *Dump);
    return status;
}

/*******************************************************************************
**
**  gcoDUMP_Destroy
**
**  Destroy a gcoDUMP object created by gcDUMP_COnstruct.
**
**  INPUT:
**
**      gcoDUMP Dump
**          Pointer to a gcoDUMP object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoDUMP_Destroy(
    IN gcoDUMP Dump
    )
{
    gcmHEADER_ARG("Dump=0x%x", Dump);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Dump, gcvOBJ_DUMP);

    if (Dump->file != gcvNULL)
    {
        if (Dump->frameStart != 0)
        {
            gcoDUMP_FrameEnd(Dump);
        }

        /* Close any open file. */
        gcmVERIFY_OK(gcoDUMP_Control(Dump, gcvNULL));
    }

    if (gcPLS.hal->dump == Dump)
    {
        /* Remove current gcoDUMP object. */
        gcPLS.hal->dump = gcvNULL;
    }

    /* Free the gcoDUMP structure. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Dump));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoDUMP_Control
**
**  Control dumping.
**
**  INPUT:
**
**      gcoDUMP Dump
**          Pointer to a gcoDUMP object.
**
**      gctSTRING FileName
**          If 'FileName' is not gcvNULL, it points to the filename to be used for
**          capturing all data.  If 'FileName' is gcvNULL, turn off any current
**          capturing.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoDUMP_Control(
    IN gcoDUMP Dump,
    IN gctSTRING FileName
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsDUMP_FILE header;
    gctUINT32 pos;

    gcmHEADER_ARG("Dump=0x%x FileName=0x%x", Dump, FileName);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Dump, gcvOBJ_DUMP);

    do
    {
        if (FileName != gcvNULL)
        {
            /* Need to create a new dump file. */
            if (Dump->file == gcvNULL)
            {
                /* Create the dump file. */
                gcmERR_BREAK(gcoOS_Open(gcvNULL,
                                    FileName,
                                    gcvFILE_CREATE,
                                    &Dump->file));

                /* Write the file header. */
                header.signature   = gcvDUMP_FILE_SIGNATURE;
                header.length      = Dump->length     = 0;
                header.frames      = Dump->frameCount = 0;

                gcmERR_BREAK(gcoOS_Write(gcvNULL,
                                     Dump->file,
                                     sizeof(header),
                                     &header));

                /* Frame is not yet started. */
                Dump->frameStart = 0;
            }
        }
        else
        {
            /* Need to close any current dump file. */
            if (Dump->file != gcvNULL)
            {
                /* Close any open frame. */
                if (Dump->frameStart != 0)
                {
                    gcoDUMP_FrameEnd(Dump);
                    gcoDUMP_FrameBegin(Dump);
                }

                /* Get the current position. */
                gcmERR_BREAK(gcoOS_GetPos(gcvNULL, Dump->file, &pos));

                /* Seek to the beginnnig of the file. */
                gcmERR_BREAK(gcoOS_SetPos(gcvNULL, Dump->file, 0));

                /* Make sure we have the correct size. */
                gcmASSERT(pos == Dump->length + sizeof(header));

                /* Update the file header. */
                header.signature = gcvDUMP_FILE_SIGNATURE;
                header.length    = Dump->length;
                header.frames    = Dump->frameCount;

                gcmERR_BREAK(gcoOS_Write(gcvNULL,
                                     Dump->file,
                                     sizeof(header),
                                     &header));

                /* Seek to the end of the file. */
                gcmERR_BREAK(gcoOS_SetPos(gcvNULL, Dump->file, pos));

                /* Close the file. */
                gcmERR_BREAK(gcoOS_Close(gcvNULL, Dump->file));

                /* Mark the file as closed. */
                Dump->file = gcvNULL;
            }
        }
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoDUMP_IsEnabled
**
**  Test whether dumping is enabeld or not.
**
**  INPUT:
**
**      gcoDUMP Dump
**          Pointer to a gcoDUMP object.
**
**  OUTPUT:
**
**      gctBOOL * Enabled
**          Pointer to a variable receiving the dump status.
*/
gceSTATUS
gcoDUMP_IsEnabled(
    IN gcoDUMP Dump,
    OUT gctBOOL * Enabled
    )
{
    gcmHEADER_ARG("Dump=0x%x Enabled=0x%x", Dump, Enabled);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Dump, gcvOBJ_DUMP);
    gcmVERIFY_ARGUMENT(Enabled != gcvNULL);

    /* Return dump status. */
    *Enabled = (Dump->file != gcvNULL);

    /* Success. */
    gcmFOOTER_ARG("*Enabled=%d", *Enabled);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoDUMP_FrameBegin
**
**  Mark the beginning of a frame.
**
**  INPUT:
**
**      gcoDUMP Dump
**          Pointer to a gcoDUMP object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoDUMP_FrameBegin(
    IN gcoDUMP Dump
    )
{
    gceSTATUS status;
    gcsDUMP_DATA header;

    gcmHEADER_ARG("Dump=0x%x", Dump);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Dump, gcvOBJ_DUMP);

    if ( (Dump->file == gcvNULL) || (Dump->frameStart != 0) )
    {
        /* There is no open dump file. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    do
    {
        /* Get the current position. */
        gcmERR_BREAK(gcoOS_GetPos(gcvNULL, Dump->file, &Dump->frameStart));

        /* Write the frame header. */
        header.type    = gcvTAG_FRAME;
        header.length  = Dump->frameLength = 0;
        header.address = 0;

        gcmERR_BREAK(gcoOS_Write(gcvNULL, Dump->file, sizeof(header), &header));

        /* Update the file length. */
        Dump->length += sizeof(header);
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoDUMP_FrameEnd
**
**  Mark the end of a frame.
**
**  INPUT:
**
**      gcoDUMP Dump
**          Pointer to a gcoDUMP object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoDUMP_FrameEnd(
    IN gcoDUMP Dump
    )
{
    gceSTATUS status;
    gcsDUMP_DATA header;
    gctUINT32 pos;

    gcmHEADER_ARG("Dump=0x%x", Dump);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Dump, gcvOBJ_DUMP);

    if (Dump->file == gcvNULL)
    {
        /* There is no open dump file. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    do
    {
        /* Get the current position. */
        gcmERR_BREAK(gcoOS_GetPos(gcvNULL, Dump->file, &pos));

        /* Seek to the beginning of the frame. */
        gcmERR_BREAK(gcoOS_SetPos(gcvNULL, Dump->file, Dump->frameStart));

        /* Make sure we got the right byte count. */
        gcmASSERT(pos - Dump->frameStart == Dump->frameLength + sizeof(header));

        /* Update the frame header. */
        header.type    = gcvTAG_FRAME;
        header.length  = Dump->frameLength;
        header.address = ++ Dump->frameCount;

        gcmERR_BREAK(gcoOS_Write(gcvNULL, Dump->file, sizeof(header), &header));

        /* Seek to the end of the file. */
        gcmERR_BREAK(gcoOS_SetPos(gcvNULL, Dump->file, pos));

        /* Mark the frame as ended. */
        Dump->frameStart = 0;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoDUMP_DumpData
**
**  Dump data the file.
**
**  INPUT:
**
**      gcoDUMP Dump
**          Pointer to a gcoDUMP object.
**
**      gceDUMP_TAG Type
**          Type of data.
**
**      gctUINT32 Address
**          Physical address to be used as a handle for the data.
**
**      gctSIZE_T ByteCount
**          Number of bytes to write.
**
**      gctCONST_POINTER Data
**          Pointer to the data to write.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoDUMP_DumpData(
    IN gcoDUMP Dump,
    IN gceDUMP_TAG Type,
    IN gctUINT32 Address,
    IN gctSIZE_T ByteCount,
    IN gctCONST_POINTER Data
    )
{
    gceSTATUS status;
    gcsDUMP_DATA header;

    gcmHEADER_ARG("Dump=0x%x Type=%d Address=%x ByteCount=%d Data=0x%x",
                    Dump, Type, Address, ByteCount, Data);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Dump, gcvOBJ_DUMP);
    gcmVERIFY_ARGUMENT(ByteCount > 0);
    gcmVERIFY_ARGUMENT(Data != gcvNULL);

    if (Dump->file == gcvNULL)
    {
        /* There is no open dump file. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    do
    {
        /* Write the data record. */
        header.type    = Type;
        header.length  = ByteCount;
        header.address = Address;

        gcmERR_BREAK(
            gcoOS_Write(gcvNULL, Dump->file, sizeof(header), &header));

        /* Write the data. */
        gcmERR_BREAK(gcoOS_Write(gcvNULL, Dump->file, ByteCount, Data));

        /* Update the frame length. */
        Dump->frameLength += sizeof(header) + ByteCount;

        /* Update the file length. */
        Dump->length += sizeof(header) + ByteCount;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoDUMP_AddSurface
**
**  Allocate a surface.
**
**  INPUT:
**
**      gcoDUMP Dump
**          Pointer to a gcoDUMP object.
**
**      gctINT32 Width, Height
**          Width and height of the surface.
**
**      gceSURF_FORMAT PixelFormat
**          Pixel format for the surface.
**
**      gctUINT32 Address
**          Physical address to be used as a handle for the surface.
**
**      gctSIZE_T ByteCount
**          Number of bytes inside the surface.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoDUMP_AddSurface(
    IN gcoDUMP Dump,
    IN gctINT32 Width,
    IN gctINT32 Height,
    IN gceSURF_FORMAT PixelFormat,
    IN gctUINT32 Address,
    IN gctSIZE_T ByteCount
    )
{
    gceSTATUS status;
    gcsDUMP_SURFACE surface;

    gcmHEADER_ARG("Dump=0x%x Width=%d Height=%d PixelFormat=%d Address=%x "
                  "ByteCount=%d",
                  Dump, Width, Height, PixelFormat, Address, ByteCount);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Dump, gcvOBJ_DUMP);
    gcmVERIFY_ARGUMENT(ByteCount > 0);

    if (Dump->file == gcvNULL)
    {
        /* There is no open dump file. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    do
    {
        /* Write the data record. */
        surface.type    = gcvTAG_SURFACE;
        surface.address = Address;
        surface.width   = (gctINT16) Width;
        surface.height  = (gctINT16) Height;
        surface.format  = PixelFormat;
        surface.length  = ByteCount;

        gcmERR_BREAK(
            gcoOS_Write(gcvNULL, Dump->file, sizeof(surface), &surface));

        /* Update the frame length. */
        Dump->frameLength += sizeof(surface);

        /* Update the file length. */
        Dump->length += sizeof(surface);
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoDUMP_Delete
**
**  Mark an address as deleted.
**
**  INPUT:
**
**      gcoDUMP Dump
**          Pointer to a gcoDUMP object.
**
**      gctUINT32 Address
**          Physical address to delete.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoDUMP_Delete(
    IN gcoDUMP Dump,
    IN gctUINT32 Address
    )
{
    gceSTATUS status;
    gcsDUMP_DATA header;

    gcmHEADER_ARG("Dump=0x%x Address=%x", Dump, Address);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Dump, gcvOBJ_DUMP);

    if (Dump->file == gcvNULL)
    {
        /* There is no open dump file. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    do
    {
        /* Write the delete record. */
        header.type    = gcvTAG_DELETE;
        header.length  = 0;
        header.address = Address;

        gcmERR_BREAK(
            gcoOS_Write(gcvNULL, Dump->file, sizeof(header), &header));

        /* Update the frame length. */
        Dump->frameLength += sizeof(header);

        /* Update the file length. */
        Dump->length += sizeof(header);
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
** New dump code.
*/

/* While dumping command buffers in kernel,
   we want to enable gcmDUMP only when called from gcvDUMP_BUFFER.
*/
#if gcdDUMP_COMMAND && !gcdDUMP
#undef gcmDUMP
#define gcmDUMP               gcfDump
#endif

#if gcdDUMP || gcdDUMP_COMMAND
gceSTATUS
gcfDump(
    IN gcoOS Os,
    IN gctCONST_STRING Message,
    ...
    )
{
#ifndef __QNXNTO__
    gctUINT offset = 0;
    gctARGUMENTS args;

#if gcdDUMP_IN_KERNEL
    gcsHAL_INTERFACE ioctl;

    gcmARGUMENTS_START(args, Message);
    gcmVERIFY_OK(gcoOS_PrintStrVSafe(ioctl.u.Debug.message,
                                     gcmSIZEOF(ioctl.u.Debug.message),
                                     &offset,
                                     Message, args));
    gcmARGUMENTS_END(args);

    ioctl.command     = gcvHAL_DEBUG;
    ioctl.u.Debug.set = gcvFALSE;
#if gcdDUMP
    ioctl.u.Debug.type = gcvMESSAGE_TEXT;
#else
    ioctl.u.Debug.type = gcvMESSAGE_DUMP;
#endif
    ioctl.u.Debug.messageSize = (gctUINT32)gcmSIZEOF(ioctl.u.Debug.message);

    gcmVERIFY_OK(gcoOS_DeviceControl(Os,
                                     IOCTL_GCHAL_INTERFACE,
                                     &ioctl, gcmSIZEOF(ioctl),
                                     &ioctl, gcmSIZEOF(ioctl)));
#else
    char buffer[80];

    gcmARGUMENTS_START(args, Message);
    gcmVERIFY_OK(gcoOS_PrintStrVSafe(buffer, gcmSIZEOF(buffer),
                                     &offset,
                                     Message, args));
    gcmARGUMENTS_END(args);

    gcoOS_Print("%s", buffer);
#endif
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcfDumpData(
    IN gcoOS Os,
    IN gctSTRING Tag,
    IN gctPOINTER Logical,
    IN gctSIZE_T Bytes
    )
{
#ifndef __QNXNTO__
    gctUINT32_PTR ptr = (gctUINT32_PTR) Logical;
    gctSIZE_T bytes   = gcmALIGN(Bytes, 4);

    while (bytes >= 16)
    {
        gcmDUMP(Os,
                "  0x%08X 0x%08X 0x%08X 0x%08X",
                ptr[0], ptr[1], ptr[2], ptr[3]);

        ptr   += 4;
        bytes -= 16;
    }

    switch (bytes)
    {
    case 12:
        gcmDUMP(Os, "  0x%08X 0x%08X 0x%08X", ptr[0], ptr[1], ptr[2]);
        break;

    case 8:
        gcmDUMP(Os, "  0x%08X 0x%08X", ptr[0], ptr[1]);
        break;

    case 4:
        gcmDUMP(Os, "  0x%08X", ptr[0]);
        break;
    }

    gcmDUMP(Os, "] -- %s", Tag);
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcfDumpBuffer(
    IN gcoOS Os,
    IN gctSTRING Tag,
    IN gctUINT32 Physical,
    IN gctPOINTER Logical,
    IN gctUINT32 Offset,
    IN gctSIZE_T Bytes
    )
{
#ifndef __QNXNTO__
    gctUINT32_PTR ptr = (gctUINT32_PTR) Logical + (Offset >> 2);
    gctSIZE_T bytes   = gcmALIGN(Bytes + (Offset & 3), 4);

#if gcdDUMP_COMMAND
    static gctPOINTER   dumpLock;

    if (dumpLock == gcvNULL)
    {
        /* TODO: Delete this mutex somehow. */
        gcmVERIFY_OK(gcoOS_CreateMutex(Os, &dumpLock));
    }

    /* Acquire mutex to avoid multiple threads dumping data
       at the same time. */
    gcmVERIFY_OK(gcoOS_AcquireMutex(Os, dumpLock, gcvINFINITE));

#endif
    gcmDUMP(Os, "@[%s 0x%08X 0x%08X", Tag, Physical + (Offset & ~3), bytes);

    while (bytes >= 16)
    {
        gcmDUMP(Os, "  0x%08X 0x%08X 0x%08X 0x%08X",
                ptr[0], ptr[1], ptr[2], ptr[3]);

        ptr   += 4;
        bytes -= 16;
    }

    switch (bytes)
    {
    case 12:
        gcmDUMP(Os, "  0x%08X 0x%08X 0x%08X", ptr[0], ptr[1], ptr[2]);
        break;

    case 8:
        gcmDUMP(Os, "  0x%08X 0x%08X", ptr[0], ptr[1]);
        break;

    case 4:
        gcmDUMP(Os, "  0x%08X", ptr[0]);
        break;
    }

    gcmDUMP(Os, "] -- %s", Tag);

#if gcdDUMP_COMMAND
    gcmVERIFY_OK(gcoOS_ReleaseMutex(Os, dumpLock));
#endif

#endif

    return gcvSTATUS_OK;
}
#else
gceSTATUS
gcfDump(
    IN gcoOS Os,
    IN gctCONST_STRING Message,
    ...
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcfDumpBuffer(
    IN gcoOS Os,
    IN gctSTRING Tag,
    IN gctUINT32 Physical,
    IN gctPOINTER Logical,
    IN gctUINT32 Offset,
    IN gctSIZE_T Bytes
    )
{
    return gcvSTATUS_OK;
}
#endif

gceSTATUS
gcfDumpFrameRate(
	void
	)
{
#if gcdDUMP_FRAMERATE
	gctINT32 averageFPS = 0;
	gctINT32 currentTime = gcoOS_GetTicks();
	if (totalFrames > 0) {
		gctINT32 timePassed = currentTime - previousTime;
		totalTicks += timePassed;
		averageFPS = 1000 / (totalTicks/totalFrames);
		gcoOS_Print("Average FPS is :%d for %d frames", averageFPS, totalFrames);
	}
	totalFrames ++;
	previousTime = currentTime;
#endif
    return gcvSTATUS_OK;
}


gceSTATUS
gcfDumpApi(
    IN gctCONST_STRING Message,
    ...
    )
{
#if gcdDUMP_API

    char buffer[256];
    gctUINT offset = 0;
    gctARGUMENTS args;

#ifdef ANDROID
    gctUINT32 threadID;

    if (!Message)
	return gcvSTATUS_OK;

    threadID = gcmGETTHREADID();
    if ((prevThreadID != 0) && (prevThreadID != threadID))
    {
        //LOGD("!!!!!!!!!!!!Terri: tid %d vs %d\n!!!!!!!!!!!!!!!\n", prevThreadID, threadID);
        //return gcvSTATUS_OK;
    }
    prevThreadID = threadID;

    if (strncmp(Message, "${", 2) == 0)
		gcmLOCKDUMP();

    gcmARGUMENTS_START(args, Message);
    gcmVERIFY_OK(gcoOS_PrintStrVSafe(buffer, gcmSIZEOF(buffer),
                                     &offset,
                                     Message, args));
    gcmARGUMENTS_END(args);
    gcoOS_Print(buffer);

    if (strstr(Message, "}"))
		gcmUNLOCKDUMP();

#else
    gcmARGUMENTS_START(args, Message);
    gcmVERIFY_OK(gcoOS_PrintStrVSafe(buffer, gcmSIZEOF(buffer),
                                     &offset,
                                     Message, args));
    gcmARGUMENTS_END(args);
    gcoOS_Print(buffer);
#endif

#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcfDumpArray(
    IN gctCONST_POINTER Data,
    IN gctUINT32 Size
    )
{
#if gcdDUMP_API
    const gctUINT32_PTR data = (gctUINT32_PTR) Data;

    if (Size > 0)
    {
        if (Data == gcvNULL)
        {
            gcfDumpApi("$$ <nil>");
        }
        else
        {
            gctUINT index;

            for (index = 0; index < Size;)
            {
                switch (Size - index)
                {
                case 1:
                    gcfDumpApi("$$ 0x%08X: 0x%08X", data + index, data[index]);
                    index += 1;
                    break;
                case 2:
                    gcfDumpApi("$$ 0x%08X: 0x%08X 0x%08X",
                               data + index, data[index], data[index + 1]);
                    index += 2;
                    break;
                case 3:
                    gcfDumpApi("$$ 0x%08X: 0x%08X 0x%08X 0x%08X",
                               data + index, data[index], data[index + 1],
                               data[index + 2]);
                    index += 3;
                    break;
                default:
                    gcfDumpApi("$$ 0x%08X: 0x%08X 0x%08X 0x%08X 0x%08X",
                               data + index, data[index], data[index + 1],
                               data[index + 2], data[index + 3]);
                    index += 4;
                    break;
                }
            }
        }

        gcfDumpApi("$$ **********");
    }
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcfDumpArrayToken(
    IN gctCONST_POINTER Data,
    IN gctUINT32 Termination
    )
{
#if gcdDUMP_API
    const gctUINT32_PTR data = (gctUINT32_PTR) Data;

    if (Data == gcvNULL)
    {
        gcfDumpApi("$$ <nil>");
    }
    else
    {
        gctUINT index;

        for (index = 0; data[index] != Termination; index += 2)
        {
            gcfDumpApi("$$ 0x%08X: 0x%08X 0x%08X",
                       data + index, data[index], data[index + 1]);
        }
        gcfDumpApi("$$ 0x%08X: 0x%08X", data + index, Termination);
    }

    gcfDumpApi("$$ **********");
#endif

    return gcvSTATUS_OK;
}

gceSTATUS
gcfDumpApiData(
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Size
    )
{
#if gcdDUMP_API
    const gctUINT8_PTR data = (gctUINT8_PTR) Data;
	gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Data=0x%x Size=%d", Data, Size);

    if (Data == gcvNULL)
    {
        gcfDumpApi("$$ <nil>");
    }
    else
    {
        gctSIZE_T index;


        if (Size == 0)
        {
            gcmERR_RETURN(gcoOS_StrLen(Data, &Size));
            Size += 1;
        }

        for (index = 0; index < Size;)
        {
            switch (Size - index)
            {
            case 1:
                gcfDumpApi("$$ 0x%08X: 0x%02X", data + index, data[index]);
                index += 1;
                break;

            case 2:
                gcfDumpApi("$$ 0x%08X: 0x%02X 0x%02X",
                           data + index, data[index], data[index + 1]);
                index += 2;
                break;

            case 3:
                gcfDumpApi("$$ 0x%08X: 0x%02X 0x%02X 0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2]);
                index += 3;
                break;

            case 4:
                gcfDumpApi("$$ 0x%08X: 0x%02X 0x%02X 0x%02X 0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2], data[index + 3]);
                index += 4;
                break;

            case 5:
                gcfDumpApi("$$ 0x%08X: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2], data[index + 3],
                           data[index + 4]);
                index += 5;
                break;

            case 6:
                gcfDumpApi("$$ 0x%08X: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
                           "0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2], data[index + 3],
                           data[index + 4], data[index + 5]);
                index += 6;
                break;

            case 7:
                gcfDumpApi("$$ 0x%08X: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
                           "0x%02X 0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2], data[index + 3],
                           data[index + 4], data[index + 5],
                           data[index + 6]);
                index += 7;
                break;

            default:
                gcfDumpApi("$$ 0x%08X: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X "
                           "0x%02X 0x%02X 0x%02X",
                           data + index, data[index], data[index + 1],
                           data[index + 2], data[index + 3],
                           data[index + 4], data[index + 5],
                           data[index + 6], data[index + 7]);
                index += 8;
                break;
            }
        }
    }

    gcfDumpApi("$$ **********");
    gcmFOOTER();
#endif

    return gcvSTATUS_OK;
}

