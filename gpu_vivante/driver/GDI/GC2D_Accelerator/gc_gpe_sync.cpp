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

gctUINT GC2D_Accelerator::GC2DChangeSyncMode(gctUINT* const pMode)
{
    gctUINT oldMode = mSyncMode;

    GC2D_ENTER_ARG("pMode = 0x%08p", pMode);

    if (pMode)
    {
        GC2DWaitForNotBusy();
        mSyncMode = *pMode;
    }

    GC2D_LEAVE_ARG("oldMode = %d", oldMode);
    return oldMode;
}

int GC2D_Accelerator::UpdateBusyStatus(gctBOOL clear)
{
    int ret;

    GC2D_ENTER_ARG("clear = %d", clear);

    do {
        if (clear)
        {
            *mFinishFlag = mCommitCounter = 0;
            ret = 0;
            break;
        }

        if ((++mCommitCounter) == 0xFFFFFFFF)
        {
            gcmVERIFY_OK(FlushAndCommit(gcvFALSE, gcvTRUE));
            *mFinishFlag = mCommitCounter = 0;
            ret = 0;
            break;
        }

        gcsHAL_INTERFACE halInterface;
        halInterface.command = gcvHAL_WRITE_DATA;
        halInterface.u.WriteData.address = mFinishFlagPhys;
        halInterface.u.WriteData.data = mCommitCounter;

        // schedule an event to write the FinishFlag
        gcmVERIFY_OK(gcoHAL_ScheduleEvent(mHal, &halInterface));

        // commit command buffer without wait
        gcmVERIFY_OK(FlushAndCommit(gcvFALSE, gcvFALSE));

#if GC2D_GATHER_STATISTICS
        ++nASyncCommit;
#endif

        ret = mCommitCounter;

    } while (gcvFALSE);

    GC2D_LEAVE_ARG("ret = %d", ret);

    return ret;
}

int GC2D_Accelerator::GC2DIsBusy()
{
    int ret;

    GC2D_ENTER();

    if ((mSyncMode >= GC2D_SYNC_MODE_ASYNC) && mCommitCounter && GC2DIsValid())
    {
        ret = (*mFinishFlag) < mCommitCounter;
    }
    else
    {
        ret = 0;
    }

    GC2D_LEAVE_ARG("ret = %d", ret);

    return ret;
}

void GC2D_Accelerator::GC2DWaitForNotBusy()
{
    GC2D_ENTER();

    if ((mSyncMode >= GC2D_SYNC_MODE_ASYNC) && GC2DIsBusy())
    {
        gcmVERIFY_OK(FlushAndCommit(gcvFALSE, gcvTRUE));
        *mFinishFlag = mCommitCounter = 0;
#if GC2D_GATHER_STATISTICS
        ++nASyncCommitFailed;
#endif
    }

    GC2D_LEAVE();
}

gctUINT GC2D_Accelerator::GC2DPowerHandler(BOOL bOff)
{
    gctUINT ret;

    GC2D_ENTER_ARG("bOff = %d", bOff);
    if (bOff)
    {
        ret = 0;

        GC2DWaitForNotBusy();

        // Disable accelerator
        mPowerHandle = GC2DSetEnable(&ret);

        ret = mPowerHandle;
    }
    else
    {
        // Revert accelerator
        ret = GC2DSetEnable(&mPowerHandle);
    }

    GC2D_LEAVE_ARG("%d", ret);

    return ret;
}

gceSTATUS GC2D_Accelerator::FlushAndCommit(gctBOOL bFlush, gctBOOL bWait)
{
    gceSTATUS status;

    if (bFlush)
    {
        gcmVERIFY_OK(gco2D_Flush(mEngine2D));
    }

    status = gcoHAL_Commit(mHal, bWait);

    return status;
}
