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



#include "gc_gpe_precomp.h"

gctBOOL GC2D_Accelerator::ReadRegistry()
{
    DWORD status;
    HKEY HKey;
    gctBOOL valid = gcvFALSE;

    GC2D_ENTER();

    // Open the device key.
    status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                GC2D_REGISTRY_PATH,
                0,
                0,
                &HKey);
    if (status)
    {
        GC2D_LEAVE_ARG("%d", gcvFALSE);
        return gcvFALSE;
    }

    do {
        DWORD valueType, valueSize = sizeof(mEnableFlag);
        if (ERROR_SUCCESS != RegQueryValueEx(HKey,
                                             GC2D_ENABLE_VALNAME,
                                             gcvNULL,
                                             &valueType,
                                             (LPBYTE) &mEnableFlag,
                                             &valueSize))
        {
            GC2D_DebugTrace(GC2D_ALWAYS_OUT, ("ReadRegistry Error: Can not get entry GC2DAcceleratorEnable!"));
            valid = gcvFALSE;
            break;
        }

        // Query the sync mode.
        valueSize = sizeof(mSyncMode);
        if (ERROR_SUCCESS != RegQueryValueEx(HKey,
                                             GC2D_SYNCMODE_VALNAME,
                                             gcvNULL,
                                             &valueType,
                                             (LPBYTE) &mSyncMode,
                                             &valueSize))
        {
            GC2D_DebugTrace(GC2D_ALWAYS_OUT, ("ReadRegistry Error: Can not get entry GC2DAcceleratorSyncMode!"));
            valid = gcvFALSE;
            break;
        }

        // Query the min size.
        valueSize = sizeof(mMinPixelNum);
        if (ERROR_SUCCESS != RegQueryValueEx(HKey,
                                             GC2D_MIN_SIZE_VALNAME,
                                             gcvNULL,
                                             &valueType,
                                             (LPBYTE) &mMinPixelNum,
                                             &valueSize))
        {
            GC2D_DebugTrace(GC2D_ALWAYS_OUT, ("ReadRegistry Error: Can not get entry  GC2DAcceleratorMinSize!"));
            valid = gcvFALSE;
            break;
        }

        valid = gcvTRUE;

    } while (gcvFALSE);

    // Close the device key.
    RegCloseKey(HKey);

    // Return error status.
    GC2D_LEAVE_ARG("%d", valid);
    return valid;
}

gceSTATUS GC2D_Accelerator::InitEngine2D()
{
    gceSTATUS status = gcvSTATUS_OK;

    GC2D_DebugTrace(GC2D_GCHAL, "Initialize: enter\n");

    if (mOs || mHal || mEngine2D)
    {
        GC2D_DebugTrace(GC2D_GCHAL, "Initialize: re-construct\n");
        return gcvSTATUS_TRUE;
    }

    do {
        /* Construct the gcoOS object. */
        status = gcoOS_Construct(gcvNULL, &mOs);
        if (status != gcvSTATUS_OK)
        {
            GC2D_DebugTrace(GC2D_GCHAL, "Construct: Failed to construct OS object (status = %d)\n", status);
            break;
        }

        /* Construct the gcoHAL object. */
        status = gcoHAL_Construct(gcvNULL, mOs, &mHal);
        if (status != gcvSTATUS_OK)
        {
            GC2D_DebugTrace(GC2D_GCHAL, "Construct: Failed to construct HAL object (status = %d)\n", status);
            break;
        }

        status = gcoHAL_Get2DEngine(mHal, &mEngine2D);
        if (status != gcvSTATUS_OK)
        {
            GC2D_DebugTrace(GC2D_GCHAL, "Construct: Failed to get 2D engine object (status = %d)\n", status);
            break;
        }
    } while (gcvFALSE);

    if (status != gcvSTATUS_OK)
    {
        if (mHal != gcvNULL)
        {
            gcoHAL_Destroy(mHal);
        }

        if (mOs != gcvNULL)
        {
            gcoOS_Destroy(mOs);
        }

        mEngine2D = gcvNULL;
        mHal = gcvNULL;
        mOs = gcvNULL;
    }

    GC2D_DebugTrace(GC2D_GCHAL, "Initialize: leave(status = %d)\n", status);

    return status;
}


gceSTATUS GC2D_Accelerator::DenitEngine2D()
{
    gceSTATUS status = gcvSTATUS_OK;

    GC2D_DebugTrace(GC2D_GCHAL, "Deinitialize: enter\n");

    do {
        if (mHal != gcvNULL)
        {
            gcmVERIFY_OK(gcoHAL_Commit(mHal, gcvTRUE));
            gcmVERIFY_OK(gcoHAL_Destroy(mHal));
        }

        if (mOs != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_Destroy(mOs));
        }

        mEngine2D = gcvNULL;
        mHal = gcvNULL;
        mOs = gcvNULL;

    } while (gcvFALSE);

    GC2D_DebugTrace(GC2D_GCHAL, "Deinitialize: leave(status = %d)\n", status);

    return status;
}

gctBOOL GC2D_Accelerator::GC2DInitialize()
{
    gctBOOL ret = gcvTRUE;

    GC2D_ENTER();

    do {

        // get registery information for only once
        if (!(mInitFlag & GC2D_ACCELERATOR_INIT_REGISTERY))
        {
            ret = ReadRegistry();
            if (!ret)
                break;

            mInitFlag |= GC2D_ACCELERATOR_INIT_REGISTERY;
        }

        // construct GC2D engine and initialize
        if (!(mInitFlag & GC2D_ACCELERATOR_INIT_2D_ENGINE))
        {
            if (gcmNO_SUCCESS(InitEngine2D()))
            {
                ret = gcvFALSE;
                break;
            }

            mInitFlag |= GC2D_ACCELERATOR_INIT_2D_ENGINE;
        }

        if (!(mInitFlag & GC2D_ACCELERATOR_INIT_PARMS))
        {
            gctUINT32 maxWidth = ScreenWidth();
            gctUINT32 maxHeight = ScreenHeight();
            gctUINT32 videoPhyBase, videoMemSize;

            GetPhysicalVideoMemory((unsigned long *)&videoPhyBase, (unsigned long *)&videoMemSize);

            AlignWidthHeight(&maxWidth, &maxHeight);

            mParms = new GC2DParms;

            ret = mParms->Initialize(
                        this,
                        videoPhyBase, videoMemSize,
                        maxWidth, maxHeight);
            if (!ret)
            {
                mParms->Denitialize(this);
                break;
            }

            mInitFlag |= GC2D_ACCELERATOR_INIT_PARMS;
        }

        // alloc non-cached memory for FinishFlag
        if (!(mInitFlag & GC2D_ACCELERATOR_INIT_FINISH_FLAG))
        {
            mFinishFlag = (gctUINT32 *)AllocNonCacheMem(PAGE_SIZE);
            if (!mFinishFlag)
            {
                ret = gcvFALSE;
                break;
            }

            gcmASSERT(!BYTE_OFFSET(mFinishFlag));
            if (!LockPages(mFinishFlag, PAGE_SIZE, (PDWORD)&mFinishFlagPhys, LOCKFLAG_QUERY_ONLY))
            {
                ret = gcvFALSE;
                break;
            }

            mFinishFlagPhys <<= UserKInfo[KINX_PFN_SHIFT];
            *mFinishFlag = 0;

            mInitFlag |= GC2D_ACCELERATOR_INIT_FINISH_FLAG;
        }
    } while (gcvFALSE);

    if (!ret)
    {
        // roll back
        if ((mInitFlag & GC2D_ACCELERATOR_INIT_FINISH_FLAG) && mFinishFlag)
        {
            gcmVERIFY(FreeNonCacheMem(mFinishFlag));
            mFinishFlag = gcvNULL;
        }

        if ((mInitFlag & GC2D_ACCELERATOR_INIT_PARMS) && mParms)
        {
            mParms->Denitialize(this);
            delete mParms;
            mParms = gcvNULL;
        }

        if (mInitFlag & GC2D_ACCELERATOR_INIT_2D_ENGINE)
        {
            DenitEngine2D();
        }

        mInitFlag = 0;
    }

    GC2D_LEAVE_ARG("ret = %d", ret);

    return ret;
}

gctBOOL GC2D_Accelerator::GC2DDeinitialize()
{
    gctBOOL ret = gcvFALSE;

    GC2D_ENTER();

    do {
        if ((mInitFlag & GC2D_ACCELERATOR_INIT_FINISH_FLAG) && mFinishFlag)
        {
            gcmVERIFY(FreeNonCacheMem(mFinishFlag));
            mFinishFlag = gcvNULL;

            mInitFlag &= ~GC2D_ACCELERATOR_INIT_FINISH_FLAG;
        }

        if ((mInitFlag & GC2D_ACCELERATOR_INIT_PARMS) && mParms)
        {
            mParms->Denitialize(this);
            delete mParms;
            mParms = gcvNULL;

            mInitFlag &= ~GC2D_ACCELERATOR_INIT_PARMS;
        }

        if (mInitFlag & GC2D_ACCELERATOR_INIT_2D_ENGINE)
        {
            DenitEngine2D();

            mInitFlag &= ~GC2D_ACCELERATOR_INIT_2D_ENGINE;
        }

        ret = gcvTRUE;

    } while (gcvFALSE);

    GC2D_LEAVE_ARG("ret = %d", ret);

    return ret;
}
