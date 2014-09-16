//--< Verona.c >-----------------------------------------------------------------------------------
//=================================================================================================
//
//  Verona - Baseband Model
//  Copyright 2007-2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wiUtil.h"
#include "wiParse.h"
#include "Verona.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static Verona_TxState_t        TX[WISE_MAX_THREADS] = {{0}};  // transmitter state
static Verona_RxState_t        RX[WISE_MAX_THREADS] = {{0}};  // receiver state
static Verona_bRxState_t      bRX[WISE_MAX_THREADS] = {{0}};  // receiver state
static Verona_DvState_t        DV[WISE_MAX_THREADS] = {{0}};  // design verification parameters
static VeronaRegisters_t Register[WISE_MAX_THREADS] ={{{0}}}; // baseband configuration registers

static Verona_LookupTable_t *pLookupTable           = NULL;   // lookup tables (set at initialization)
static wiUWORD RegMap[1024]                         = {{0}};  // register map (Tx/Rx combined)

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;
#define PSTATUS(arg)                   \
{                                      \
    wiStatus_t pstatus = WICHK(arg);   \
    if (pstatus <= 0) return pstatus;  \
}                                      \
// -------------------------------------

// ================================================================================================
// FUNCTION  : Verona_InitializeThread()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_InitializeThread(unsigned int ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADS)) 
        return WI_ERROR_PARAMETER1;
    else
    {
        int i;

        XSTATUS( Verona_CloseThread(ThreadIndex) )

        memcpy(      TX + ThreadIndex,       TX+0, sizeof(Verona_TxState_t) );
        memcpy(      RX + ThreadIndex,       RX+0, sizeof(Verona_RxState_t) );
        memcpy(     bRX + ThreadIndex,      bRX+0, sizeof(Verona_bRxState_t));
        memcpy(      DV + ThreadIndex,       DV+0, sizeof(Verona_DvState_t) );
        memcpy(Register + ThreadIndex, Register+0, sizeof(VeronaRegisters_t));

        Verona_TX_FreeMemory ( TX + ThreadIndex, 1);
/*
        Verona_RX_FreeMemory ( RX + ThreadIndex, 1);
*/
        Verona_bRX_FreeMemory(bRX + ThreadIndex, 1);

        TX[ThreadIndex].LastAllocation.Fn = NULL; // force allocation on next TX

        for (i=0; i<VERONA_MAX_DV_TX; i++)
            DV[ThreadIndex].MAC_TX_D[i] = NULL;
    }    
    return WI_SUCCESS;
}
// end of Verona_InitializeThread()

// ================================================================================================
// FUNCTION  : Verona_CloseThread()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_CloseThread(unsigned int ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADS)) 
        return WI_ERROR_PARAMETER1;
    else
    {
        int i;
        XSTATUS( Verona_TX_FreeMemory ( TX + ThreadIndex, 0) )
/*
        XSTATUS( Verona_RX_FreeMemory ( RX + ThreadIndex, 0) )
*/
        XSTATUS( Verona_bRX_FreeMemory(bRX + ThreadIndex, 0) );

        for (i=0; i<VERONA_MAX_DV_TX; i++)
        {
            WIFREE( DV[ThreadIndex].MAC_TX_D[i] );
        }
        RX[ThreadIndex].Nr = 0;
    }
    return WI_SUCCESS;
}
// end of Verona_CloseThread()

// ================================================================================================
// FUNCTION  : Verona_ReadRegisterMap()
// ------------------------------------------------------------------------------------------------
// Purpose   : Read values from the RegMap array and load into the Register structure
// Parameters: RegMap -- pointer to the register map from which to read parameters
// ================================================================================================
wiStatus Verona_ReadRegisterMap(wiUWORD *RegMap)
{
    VeronaRegisters_t *pReg = Verona_Registers();

    if (wiProcess_ThreadIndex() != 0) return STATUS(WI_ERROR_INVALID_FOR_THREAD);

    //  Matlab Generated Code from Verona_GetRegisterMap.m(1)
    // ----------------------------------------------------------------------------------
    pReg->ID.word                     = wiUWORD_GetUnsignedField(RegMap+0x001, 7,0);
    pReg->PathSel.word                = wiUWORD_GetUnsignedField(RegMap+0x003, 1,0);
    pReg->RXMode.word                 = wiUWORD_GetUnsignedField(RegMap+0x004, 2,0);
    pReg->TestMode.word               = wiUWORD_GetUnsignedField(RegMap+0x005, 3,0);
    pReg->UpmixConj.word              = wiUWORD_GetUnsignedField(RegMap+0x006, 7,7);
    pReg->DownmixConj.word            = wiUWORD_GetUnsignedField(RegMap+0x006, 6,6);
    pReg->UpmixDown.word              = wiUWORD_GetUnsignedField(RegMap+0x006, 3,3);
    pReg->DownmixUp.word              = wiUWORD_GetUnsignedField(RegMap+0x006, 2,2);
    pReg->UpmixOff.word               = wiUWORD_GetUnsignedField(RegMap+0x006, 1,1);
    pReg->DownmixOff.word             = wiUWORD_GetUnsignedField(RegMap+0x006, 0,0);
    pReg->TxIQSrcR.word               = wiUWORD_GetUnsignedField(RegMap+0x007, 7,6);
    pReg->TxIQSrcI.word               = wiUWORD_GetUnsignedField(RegMap+0x007, 5,4);
    pReg->PrdDACSrcR.word             = wiUWORD_GetUnsignedField(RegMap+0x007, 3,2);
    pReg->PrdDACSrcI.word             = wiUWORD_GetUnsignedField(RegMap+0x007, 1,0);
    pReg->aTxExtend.word              = wiUWORD_GetUnsignedField(RegMap+0x008, 7,6);
    pReg->aTxDelay.word               = wiUWORD_GetUnsignedField(RegMap+0x008, 5,0);
    pReg->bTxExtend.word              = wiUWORD_GetUnsignedField(RegMap+0x009, 7,6);
    pReg->bTxDelay.word               = wiUWORD_GetUnsignedField(RegMap+0x009, 5,0);
    pReg->TxIQSrcRun.word             = wiUWORD_GetUnsignedField(RegMap+0x00A, 7,7);
    pReg->TxSmoothing.word            = wiUWORD_GetUnsignedField(RegMap+0x00A, 0,0);
    pReg->TX_SERVICE.word             = wiUWORD_GetUnsignedField(RegMap+0x00C, 7,0) * 256
                                      + wiUWORD_GetUnsignedField(RegMap+0x00D, 7,0);
    pReg->TX_PRBS.word                = wiUWORD_GetUnsignedField(RegMap+0x00E, 6,0);
    pReg->RX_SERVICE.word             = wiUWORD_GetUnsignedField(RegMap+0x014, 7,0) * 256
                                      + wiUWORD_GetUnsignedField(RegMap+0x015, 7,0);
    pReg->RSSI0.word                  = wiUWORD_GetUnsignedField(RegMap+0x016, 7,0);
    pReg->RSSI1.word                  = wiUWORD_GetUnsignedField(RegMap+0x017, 7,0);
    pReg->maxLENGTH.word              = wiUWORD_GetUnsignedField(RegMap+0x018, 7,0);
    pReg->NotSounding.word            = wiUWORD_GetUnsignedField(RegMap+0x019, 3,3);
    pReg->nReserved1.word             = wiUWORD_GetUnsignedField(RegMap+0x019, 2,2);
    pReg->Reserved0.word              = wiUWORD_GetUnsignedField(RegMap+0x019, 0,0);
    pReg->maxLENGTHnH.word            = wiUWORD_GetUnsignedField(RegMap+0x01A, 7,0);
    pReg->maxLENGTHnL.word            = wiUWORD_GetUnsignedField(RegMap+0x01B, 7,0);
    pReg->aPPDUMaxTimeH.word          = wiUWORD_GetUnsignedField(RegMap+0x01C, 7,0);
    pReg->aPPDUMaxTimeL.word          = wiUWORD_GetUnsignedField(RegMap+0x01D, 7,0);
    pReg->PilotPLLEn.word             = wiUWORD_GetUnsignedField(RegMap+0x01E, 6,6);
    pReg->aC.word                     = wiUWORD_GetUnsignedField(RegMap+0x01E, 5,3);
    pReg->bC.word                     = wiUWORD_GetUnsignedField(RegMap+0x01E, 2,0);
    pReg->cC.word                     = wiUWORD_GetUnsignedField(RegMap+0x01F, 2,0);
    pReg->Sa.word                     = wiUWORD_GetUnsignedField(RegMap+0x020, 5,3);
    pReg->Sb.word                     = wiUWORD_GetUnsignedField(RegMap+0x020, 2,0);
    pReg->Ca.word                     = wiUWORD_GetUnsignedField(RegMap+0x021, 5,3);
    pReg->Cb.word                     = wiUWORD_GetUnsignedField(RegMap+0x021, 2,0);
    pReg->sTxIQ.word                  = wiUWORD_GetUnsignedField(RegMap+0x024, 0,0);
    pReg->TxIQa11.word                = wiUWORD_GetSignedField  (RegMap+0x025, 4,0);
    pReg->TxIQa22.word                = wiUWORD_GetSignedField  (RegMap+0x026, 4,0);
    pReg->TxIQa12.word                = wiUWORD_GetSignedField  (RegMap+0x027, 4,0);
    pReg->sRxIQ.word                  = wiUWORD_GetUnsignedField(RegMap+0x028, 1,0);
    pReg->RxIQ0a11.word               = wiUWORD_GetSignedField  (RegMap+0x029, 4,0);
    pReg->RxIQ0a22.word               = wiUWORD_GetSignedField  (RegMap+0x02A, 4,0);
    pReg->RxIQ0a12.word               = wiUWORD_GetSignedField  (RegMap+0x02B, 4,0);
    pReg->RxIQ1a11.word               = wiUWORD_GetSignedField  (RegMap+0x02C, 4,0);
    pReg->RxIQ1a22.word               = wiUWORD_GetSignedField  (RegMap+0x02D, 4,0);
    pReg->RxIQ1a12.word               = wiUWORD_GetSignedField  (RegMap+0x02E, 4,0);
    pReg->ImgDetCoefRH.word           = wiUWORD_GetSignedField  (RegMap+0x030, 3,0);
    pReg->ImgDetCoefRL.word           = wiUWORD_GetUnsignedField(RegMap+0x031, 7,0);
    pReg->ImgDetCoefIH.word           = wiUWORD_GetSignedField  (RegMap+0x032, 3,0);
    pReg->ImgDetCoefIL.word           = wiUWORD_GetUnsignedField(RegMap+0x033, 7,0);
    pReg->ImgDetLength.word           = wiUWORD_GetUnsignedField(RegMap+0x034, 7,0);
    pReg->ImgDetEn.word               = wiUWORD_GetUnsignedField(RegMap+0x035, 7,7);
    pReg->ImgDetLengthH.word          = wiUWORD_GetUnsignedField(RegMap+0x035, 6,6);
    pReg->ImgDetPostShift.word        = wiUWORD_GetUnsignedField(RegMap+0x035, 5,3);
    pReg->ImgDetPreShift.word         = wiUWORD_GetUnsignedField(RegMap+0x035, 2,0);
    pReg->ImgDetDelay.word            = wiUWORD_GetUnsignedField(RegMap+0x036, 7,0);
    pReg->ImgDetAvgNum.word           = wiUWORD_GetUnsignedField(RegMap+0x037, 2,0);
    pReg->ImgDetPowerH.word           = wiUWORD_GetUnsignedField(RegMap+0x038, 7,0);
    pReg->ImgDetPowerL.word           = wiUWORD_GetUnsignedField(RegMap+0x039, 7,0);
    pReg->CalDone.word                = wiUWORD_GetUnsignedField(RegMap+0x03A, 7,7);
    pReg->CalResultType.word          = wiUWORD_GetUnsignedField(RegMap+0x03A, 6,6);
    pReg->ImgDetDone.word             = wiUWORD_GetUnsignedField(RegMap+0x03A, 0,0);
    pReg->CalRun.word                 = wiUWORD_GetUnsignedField(RegMap+0x03B, 7,7);
    pReg->CalPwrAdjust.word           = wiUWORD_GetUnsignedField(RegMap+0x03B, 6,6);
    pReg->CalMode.word                = wiUWORD_GetUnsignedField(RegMap+0x03B, 5,5);
    pReg->CalCoefUpdate.word          = wiUWORD_GetUnsignedField(RegMap+0x03B, 4,4);
    pReg->CalFineStep.word            = wiUWORD_GetUnsignedField(RegMap+0x03B, 3,3);
    pReg->CalCoarseStep.word          = wiUWORD_GetUnsignedField(RegMap+0x03B, 2,0);
    pReg->CalPwrRatio.word            = wiUWORD_GetUnsignedField(RegMap+0x03C, 7,3);
    pReg->CalIterationTh.word         = wiUWORD_GetUnsignedField(RegMap+0x03C, 1,0);
    pReg->CalDelay.word               = wiUWORD_GetUnsignedField(RegMap+0x03D, 6,0);
    pReg->CalGain.word                = wiUWORD_GetUnsignedField(RegMap+0x03E, 5,0);
    pReg->CalPhase.word               = wiUWORD_GetUnsignedField(RegMap+0x03F, 4,0);
    pReg->RefPwrDig.word              = wiUWORD_GetUnsignedField(RegMap+0x040, 6,0);
    pReg->InitAGain.word              = wiUWORD_GetUnsignedField(RegMap+0x041, 5,0);
    pReg->AbsPwrL.word                = wiUWORD_GetUnsignedField(RegMap+0x042, 6,0);
    pReg->AbsPwrH.word                = wiUWORD_GetUnsignedField(RegMap+0x043, 6,0);
    pReg->ThSigLarge.word             = wiUWORD_GetUnsignedField(RegMap+0x044, 6,0);
    pReg->ThSigSmall.word             = wiUWORD_GetUnsignedField(RegMap+0x045, 6,0);
    pReg->StepLgSigDet.word           = wiUWORD_GetUnsignedField(RegMap+0x046, 4,0);
    pReg->StepLarge.word              = wiUWORD_GetUnsignedField(RegMap+0x047, 4,0);
    pReg->StepSmall.word              = wiUWORD_GetUnsignedField(RegMap+0x048, 4,0);
    pReg->ThSwitchLNA.word            = wiUWORD_GetUnsignedField(RegMap+0x049, 5,0);
    pReg->StepLNA.word                = wiUWORD_GetUnsignedField(RegMap+0x04A, 4,0);
    pReg->ThSwitchLNA2.word           = wiUWORD_GetUnsignedField(RegMap+0x04B, 5,0);
    pReg->StepLNA2.word               = wiUWORD_GetUnsignedField(RegMap+0x04C, 4,0);
    pReg->Pwr100dBm.word              = wiUWORD_GetUnsignedField(RegMap+0x04D, 4,0);
    pReg->StepSmallPeak.word          = wiUWORD_GetUnsignedField(RegMap+0x04E, 4,0);
    pReg->FixedGain.word              = wiUWORD_GetUnsignedField(RegMap+0x050, 7,7);
    pReg->InitLNAGain.word            = wiUWORD_GetUnsignedField(RegMap+0x050, 6,6);
    pReg->InitLNAGain2.word           = wiUWORD_GetUnsignedField(RegMap+0x050, 5,5);
    pReg->UpdateOnLNA.word            = wiUWORD_GetUnsignedField(RegMap+0x050, 4,4);
    pReg->MaxUpdateCount.word         = wiUWORD_GetUnsignedField(RegMap+0x050, 2,0);
    pReg->ThStepUpRefPwr.word         = wiUWORD_GetUnsignedField(RegMap+0x051, 7,0);
    pReg->ThStepUp.word               = wiUWORD_GetUnsignedField(RegMap+0x052, 7,4);
    pReg->ThStepDown.word             = wiUWORD_GetUnsignedField(RegMap+0x052, 3,0);
    pReg->ThDFSUp.word                = wiUWORD_GetUnsignedField(RegMap+0x053, 6,0);
    pReg->ThDFSDn.word                = wiUWORD_GetUnsignedField(RegMap+0x054, 6,0);
    pReg->ThStepDownMsrPwr.word       = wiUWORD_GetUnsignedField(RegMap+0x055, 5,0);
    pReg->DFSOn.word                  = wiUWORD_GetUnsignedField(RegMap+0x057, 7,7);
    pReg->DFS2Candidates.word         = wiUWORD_GetUnsignedField(RegMap+0x057, 5,5);
    pReg->DFSSmooth.word              = wiUWORD_GetUnsignedField(RegMap+0x057, 4,4);
    pReg->DFSMACFilter.word           = wiUWORD_GetUnsignedField(RegMap+0x057, 3,3);
    pReg->DFSHdrFilter.word           = wiUWORD_GetUnsignedField(RegMap+0x057, 2,2);
    pReg->DFSIntDet.word              = wiUWORD_GetUnsignedField(RegMap+0x057, 0,0);
    pReg->DFSPattern.word             = wiUWORD_GetUnsignedField(RegMap+0x058, 5,4);
    pReg->DFSminPPB.word              = wiUWORD_GetUnsignedField(RegMap+0x058, 2,0);
    pReg->DFSdTPRF.word               = wiUWORD_GetUnsignedField(RegMap+0x059, 3,0);
    pReg->DFSmaxWidth.word            = wiUWORD_GetUnsignedField(RegMap+0x05A, 7,0);
    pReg->DFSmaxTPRF.word             = wiUWORD_GetUnsignedField(RegMap+0x05B, 7,0);
    pReg->DFSminTPRF.word             = wiUWORD_GetUnsignedField(RegMap+0x05C, 7,0);
    pReg->DFSminGap.word              = wiUWORD_GetUnsignedField(RegMap+0x05D, 6,0);
    pReg->DFSPulseSR.word             = wiUWORD_GetUnsignedField(RegMap+0x05E, 7,0);
    pReg->detRadar.word               = wiUWORD_GetUnsignedField(RegMap+0x05F, 7,7);
    pReg->detHeader.word              = wiUWORD_GetUnsignedField(RegMap+0x05F, 3,3);
    pReg->detPreamble.word            = wiUWORD_GetUnsignedField(RegMap+0x05F, 2,2);
    pReg->detStepDown.word            = wiUWORD_GetUnsignedField(RegMap+0x05F, 1,1);
    pReg->detStepUp.word              = wiUWORD_GetUnsignedField(RegMap+0x05F, 0,0);
    pReg->WaitAGC.word                = wiUWORD_GetUnsignedField(RegMap+0x060, 6,0);
    pReg->DelayAGC.word               = wiUWORD_GetUnsignedField(RegMap+0x061, 4,0);
    pReg->SigDetWindow.word           = wiUWORD_GetUnsignedField(RegMap+0x062, 7,0);
    pReg->SyncShift.word              = wiUWORD_GetUnsignedField(RegMap+0x063, 3,0);
    pReg->DelayRestart.word           = wiUWORD_GetUnsignedField(RegMap+0x064, 7,0);
    pReg->OFDMSwDDelay.word           = wiUWORD_GetUnsignedField(RegMap+0x065, 4,0);
    pReg->OFDMSwDTh.word              = wiUWORD_GetUnsignedField(RegMap+0x066, 7,0);
    pReg->CFOFilter.word              = wiUWORD_GetUnsignedField(RegMap+0x067, 7,7);
    pReg->UpdateGainMF.word           = wiUWORD_GetUnsignedField(RegMap+0x067, 6,6);
    pReg->ArmLgSigDet3.word           = wiUWORD_GetUnsignedField(RegMap+0x067, 5,5);
    pReg->LgSigAFEValid.word          = wiUWORD_GetUnsignedField(RegMap+0x067, 4,4);
    pReg->IdleFSEnB.word              = wiUWORD_GetUnsignedField(RegMap+0x067, 3,3);
    pReg->IdleSDEnB.word              = wiUWORD_GetUnsignedField(RegMap+0x067, 2,2);
    pReg->IdleDAEnB.word              = wiUWORD_GetUnsignedField(RegMap+0x067, 1,1);
    pReg->SDEnB17.word                = wiUWORD_GetUnsignedField(RegMap+0x067, 0,0);
    pReg->DelayCFO.word               = wiUWORD_GetUnsignedField(RegMap+0x068, 4,0);
    pReg->DelayRSSI.word              = wiUWORD_GetUnsignedField(RegMap+0x069, 5,0);
    pReg->DelayRIFS.word              = wiUWORD_GetUnsignedField(RegMap+0x06A, 7,0);
    pReg->OFDMSwDEn.word              = wiUWORD_GetUnsignedField(RegMap+0x06B, 7,7);
    pReg->OFDMSwDSave.word            = wiUWORD_GetUnsignedField(RegMap+0x06B, 6,6);
    pReg->OFDMSwDInit.word            = wiUWORD_GetUnsignedField(RegMap+0x06B, 5,5);
    pReg->UpdateGain11b.word          = wiUWORD_GetUnsignedField(RegMap+0x06B, 0,0);
    pReg->OFDMSwDThLow.word           = wiUWORD_GetUnsignedField(RegMap+0x06C, 6,0);
    pReg->SigDetTh1.word              = wiUWORD_GetUnsignedField(RegMap+0x070, 5,0);
    pReg->SigDetTh2H.word             = wiUWORD_GetUnsignedField(RegMap+0x071, 7,0);
    pReg->SigDetTh2L.word             = wiUWORD_GetUnsignedField(RegMap+0x072, 7,0);
    pReg->SigDetDelay.word            = wiUWORD_GetUnsignedField(RegMap+0x073, 7,0);
    pReg->SigDetFilter.word           = wiUWORD_GetUnsignedField(RegMap+0x074, 7,7);
    pReg->SyncFilter.word             = wiUWORD_GetUnsignedField(RegMap+0x078, 7,7);
    pReg->SyncMag.word                = wiUWORD_GetUnsignedField(RegMap+0x078, 4,0);
    pReg->SyncThSig.word              = wiUWORD_GetUnsignedField(RegMap+0x079, 7,4);
    pReg->SyncThExp.word              = wiUWORD_GetSignedField  (RegMap+0x079, 3,0);
    pReg->SyncDelay.word              = wiUWORD_GetUnsignedField(RegMap+0x07A, 4,0);
    pReg->MinShift.word               = wiUWORD_GetUnsignedField(RegMap+0x080, 3,0);
    pReg->ChEstShift0.word            = wiUWORD_GetUnsignedField(RegMap+0x081, 7,4);
    pReg->ChEstShift1.word            = wiUWORD_GetUnsignedField(RegMap+0x081, 3,0);
    pReg->ChTracking.word             = wiUWORD_GetUnsignedField(RegMap+0x082, 7,7);
    pReg->ChTrackGain1.word           = wiUWORD_GetUnsignedField(RegMap+0x082, 5,3);
    pReg->ChTrackGain2.word           = wiUWORD_GetUnsignedField(RegMap+0x082, 2,0);
    pReg->aC1.word                    = wiUWORD_GetUnsignedField(RegMap+0x084, 5,3);
    pReg->bC1.word                    = wiUWORD_GetUnsignedField(RegMap+0x084, 2,0);
    pReg->aS1.word                    = wiUWORD_GetUnsignedField(RegMap+0x085, 5,3);
    pReg->bS1.word                    = wiUWORD_GetUnsignedField(RegMap+0x085, 2,0);
    pReg->aC2.word                    = wiUWORD_GetUnsignedField(RegMap+0x086, 5,3);
    pReg->bC2.word                    = wiUWORD_GetUnsignedField(RegMap+0x086, 2,0);
    pReg->aS2.word                    = wiUWORD_GetUnsignedField(RegMap+0x087, 5,3);
    pReg->bS2.word                    = wiUWORD_GetUnsignedField(RegMap+0x087, 2,0);
    pReg->TxToneDiv.word              = wiUWORD_GetUnsignedField(RegMap+0x088, 7,0);
    pReg->TxWordRH.word               = wiUWORD_GetUnsignedField(RegMap+0x08C, 1,0);
    pReg->TxWordRL.word               = wiUWORD_GetUnsignedField(RegMap+0x08D, 7,0);
    pReg->TxWordIH.word               = wiUWORD_GetUnsignedField(RegMap+0x08E, 1,0);
    pReg->TxWordIL.word               = wiUWORD_GetUnsignedField(RegMap+0x08F, 7,0);
    pReg->DCXSelect.word              = wiUWORD_GetUnsignedField(RegMap+0x090, 7,7);
    pReg->DCXFastLoad.word            = wiUWORD_GetUnsignedField(RegMap+0x090, 6,6);
    pReg->DCXHoldAcc.word             = wiUWORD_GetUnsignedField(RegMap+0x090, 5,5);
    pReg->DCXShift.word               = wiUWORD_GetUnsignedField(RegMap+0x090, 2,0);
    pReg->DCXAcc0RH.word              = wiUWORD_GetSignedField  (RegMap+0x091, 4,0);
    pReg->DCXAcc0RL.word              = wiUWORD_GetUnsignedField(RegMap+0x092, 7,0);
    pReg->DCXAcc0IH.word              = wiUWORD_GetSignedField  (RegMap+0x093, 4,0);
    pReg->DCXAcc0IL.word              = wiUWORD_GetUnsignedField(RegMap+0x094, 7,0);
    pReg->DCXAcc1RH.word              = wiUWORD_GetSignedField  (RegMap+0x095, 4,0);
    pReg->DCXAcc1RL.word              = wiUWORD_GetUnsignedField(RegMap+0x096, 7,0);
    pReg->DCXAcc1IH.word              = wiUWORD_GetSignedField  (RegMap+0x097, 4,0);
    pReg->DCXAcc1IL.word              = wiUWORD_GetUnsignedField(RegMap+0x098, 7,0);
    pReg->INITdelay.word              = wiUWORD_GetUnsignedField(RegMap+0x0A0, 7,0);
    pReg->EDwindow.word               = wiUWORD_GetUnsignedField(RegMap+0x0A1, 7,6);
    pReg->CQwindow.word               = wiUWORD_GetUnsignedField(RegMap+0x0A1, 5,4);
    pReg->SSwindow.word               = wiUWORD_GetUnsignedField(RegMap+0x0A1, 2,0);
    pReg->CQthreshold.word            = wiUWORD_GetUnsignedField(RegMap+0x0A2, 5,0);
    pReg->EDthreshold.word            = wiUWORD_GetUnsignedField(RegMap+0x0A3, 5,0);
    pReg->bWaitAGC.word               = wiUWORD_GetUnsignedField(RegMap+0x0A4, 5,0);
    pReg->bAGCdelay.word              = wiUWORD_GetUnsignedField(RegMap+0x0A5, 5,0);
    pReg->bRefPwr.word                = wiUWORD_GetUnsignedField(RegMap+0x0A6, 5,0);
    pReg->bInitAGain.word             = wiUWORD_GetUnsignedField(RegMap+0x0A7, 5,0);
    pReg->bThSigLarge.word            = wiUWORD_GetUnsignedField(RegMap+0x0A8, 5,0);
    pReg->bStepLarge.word             = wiUWORD_GetUnsignedField(RegMap+0x0A9, 4,0);
    pReg->bThSwitchLNA.word           = wiUWORD_GetUnsignedField(RegMap+0x0AA, 5,0);
    pReg->bStepLNA.word               = wiUWORD_GetUnsignedField(RegMap+0x0AB, 4,0);
    pReg->bThSwitchLNA2.word          = wiUWORD_GetUnsignedField(RegMap+0x0AC, 5,0);
    pReg->bStepLNA2.word              = wiUWORD_GetUnsignedField(RegMap+0x0AD, 4,0);
    pReg->bPwr100dBm.word             = wiUWORD_GetUnsignedField(RegMap+0x0AE, 4,0);
    pReg->DAmpGainRange.word          = wiUWORD_GetUnsignedField(RegMap+0x0AF, 3,0);
    pReg->bMaxUpdateCount.word        = wiUWORD_GetUnsignedField(RegMap+0x0B0, 3,0);
    pReg->CSMode.word                 = wiUWORD_GetUnsignedField(RegMap+0x0B1, 5,4);
    pReg->bPwrStepDet.word            = wiUWORD_GetUnsignedField(RegMap+0x0B1, 3,3);
    pReg->bCFOQual.word               = wiUWORD_GetUnsignedField(RegMap+0x0B1, 2,2);
    pReg->bAGCBounded4.word           = wiUWORD_GetUnsignedField(RegMap+0x0B1, 1,1);
    pReg->bLgSigAFEValid.word         = wiUWORD_GetUnsignedField(RegMap+0x0B1, 0,0);
    pReg->bSwDEn.word                 = wiUWORD_GetUnsignedField(RegMap+0x0B2, 7,7);
    pReg->bSwDSave.word               = wiUWORD_GetUnsignedField(RegMap+0x0B2, 6,6);
    pReg->bSwDInit.word               = wiUWORD_GetUnsignedField(RegMap+0x0B2, 5,5);
    pReg->bSwDDelay.word              = wiUWORD_GetUnsignedField(RegMap+0x0B2, 4,0);
    pReg->bSwDTh.word                 = wiUWORD_GetUnsignedField(RegMap+0x0B3, 7,0);
    pReg->bThStepUpRefPwr.word        = wiUWORD_GetUnsignedField(RegMap+0x0B5, 7,0);
    pReg->bThStepUp.word              = wiUWORD_GetUnsignedField(RegMap+0x0B6, 7,4);
    pReg->bThStepDown.word            = wiUWORD_GetUnsignedField(RegMap+0x0B6, 3,0);
    pReg->bRestartDelay.word          = wiUWORD_GetUnsignedField(RegMap+0x0B7, 7,0);
    pReg->SymHDRCE.word               = wiUWORD_GetUnsignedField(RegMap+0x0B8, 5,0);
    pReg->CENSym1.word                = wiUWORD_GetUnsignedField(RegMap+0x0B9, 6,4);
    pReg->CENSym2.word                = wiUWORD_GetUnsignedField(RegMap+0x0B9, 2,0);
    pReg->hThreshold1.word            = wiUWORD_GetUnsignedField(RegMap+0x0BA, 6,4);
    pReg->hThreshold2.word            = wiUWORD_GetUnsignedField(RegMap+0x0BA, 2,0);
    pReg->hThreshold3.word            = wiUWORD_GetUnsignedField(RegMap+0x0BB, 2,0);
    pReg->SFDWindow.word              = wiUWORD_GetUnsignedField(RegMap+0x0BC, 7,0);
    pReg->AntSel.word                 = wiUWORD_GetUnsignedField(RegMap+0x0C0, 7,7);
    pReg->DW_PHYEnB.word              = wiUWORD_GetUnsignedField(RegMap+0x0C0, 6,6);
    pReg->TopState.word               = wiUWORD_GetUnsignedField(RegMap+0x0C0, 5,0);
    pReg->CFO.word                    = wiUWORD_GetUnsignedField(RegMap+0x0C1, 7,0);
    pReg->RxFaultCount.word           = wiUWORD_GetUnsignedField(RegMap+0x0C2, 7,0);
    pReg->RestartCount.word           = wiUWORD_GetUnsignedField(RegMap+0x0C3, 7,0);
    pReg->NuisanceCount.word          = wiUWORD_GetUnsignedField(RegMap+0x0C4, 7,0);
    pReg->sPHYEnB.word                = wiUWORD_GetUnsignedField(RegMap+0x0C5, 7,7);
    pReg->PHYEnB.word                 = wiUWORD_GetUnsignedField(RegMap+0x0C5, 6,6);
    pReg->NoStepUpAgg.word            = wiUWORD_GetUnsignedField(RegMap+0x0C5, 5,5);
    pReg->RSSIdB.word                 = wiUWORD_GetUnsignedField(RegMap+0x0C5, 4,4);
    pReg->UnsupportedGF.word          = wiUWORD_GetUnsignedField(RegMap+0x0C5, 3,3);
    pReg->ClrOnWake.word              = wiUWORD_GetUnsignedField(RegMap+0x0C5, 2,2);
    pReg->ClrOnHdr.word               = wiUWORD_GetUnsignedField(RegMap+0x0C5, 1,1);
    pReg->ClrNow.word                 = wiUWORD_GetUnsignedField(RegMap+0x0C5, 0,0);
    pReg->Filter64QAM.word            = wiUWORD_GetUnsignedField(RegMap+0x0C6, 7,7);
    pReg->Filter16QAM.word            = wiUWORD_GetUnsignedField(RegMap+0x0C6, 6,6);
    pReg->FilterQPSK.word             = wiUWORD_GetUnsignedField(RegMap+0x0C6, 5,5);
    pReg->FilterBPSK.word             = wiUWORD_GetUnsignedField(RegMap+0x0C6, 4,4);
    pReg->FilterUndecoded.word        = wiUWORD_GetUnsignedField(RegMap+0x0C6, 2,2);
    pReg->FilterAllTypes.word         = wiUWORD_GetUnsignedField(RegMap+0x0C6, 1,1);
    pReg->AddrFilter.word             = wiUWORD_GetUnsignedField(RegMap+0x0C6, 0,0);
    pReg->STA0.word                   = wiUWORD_GetUnsignedField(RegMap+0x0C7, 7,0);
    pReg->STA1.word                   = wiUWORD_GetUnsignedField(RegMap+0x0C8, 7,0);
    pReg->STA2.word                   = wiUWORD_GetUnsignedField(RegMap+0x0C9, 7,0);
    pReg->STA3.word                   = wiUWORD_GetUnsignedField(RegMap+0x0CA, 7,0);
    pReg->STA4.word                   = wiUWORD_GetUnsignedField(RegMap+0x0CB, 7,0);
    pReg->STA5.word                   = wiUWORD_GetUnsignedField(RegMap+0x0CC, 7,0);
    pReg->minRSSI.word                = wiUWORD_GetUnsignedField(RegMap+0x0CD, 7,0);
    pReg->StepUpRestart.word          = wiUWORD_GetUnsignedField(RegMap+0x0CE, 7,7);
    pReg->StepDownRestart.word        = wiUWORD_GetUnsignedField(RegMap+0x0CE, 6,6);
    pReg->NoStepAfterSync.word        = wiUWORD_GetUnsignedField(RegMap+0x0CE, 5,5);
    pReg->NoStepAfterHdr.word         = wiUWORD_GetUnsignedField(RegMap+0x0CE, 4,4);
    pReg->RxRIFS.word                 = wiUWORD_GetUnsignedField(RegMap+0x0CE, 3,3);
    pReg->ForceRestart.word           = wiUWORD_GetUnsignedField(RegMap+0x0CE, 2,2);
    pReg->KeepCCA_New.word            = wiUWORD_GetUnsignedField(RegMap+0x0CE, 1,1);
    pReg->KeepCCA_Lost.word           = wiUWORD_GetUnsignedField(RegMap+0x0CE, 0,0);
    pReg->PktTimePhase80.word         = wiUWORD_GetUnsignedField(RegMap+0x0CF, 6,0);
    pReg->WakeupTimeH.word            = wiUWORD_GetUnsignedField(RegMap+0x0D0, 7,0);
    pReg->WakeupTimeL.word            = wiUWORD_GetUnsignedField(RegMap+0x0D1, 7,0);
    pReg->DelayStdby.word             = wiUWORD_GetUnsignedField(RegMap+0x0D2, 7,0);
    pReg->DelayBGEnB.word             = wiUWORD_GetUnsignedField(RegMap+0x0D3, 7,0);
    pReg->DelayADC1.word              = wiUWORD_GetUnsignedField(RegMap+0x0D4, 7,0);
    pReg->DelayADC2.word              = wiUWORD_GetUnsignedField(RegMap+0x0D5, 7,0);
    pReg->DelayModem.word             = wiUWORD_GetUnsignedField(RegMap+0x0D6, 7,0);
    pReg->DelayDFE.word               = wiUWORD_GetUnsignedField(RegMap+0x0D7, 7,0);
    pReg->DelayRFSW.word              = wiUWORD_GetUnsignedField(RegMap+0x0D8, 7,4);
    pReg->TxRxTime.word               = wiUWORD_GetUnsignedField(RegMap+0x0D8, 3,0);
    pReg->DelayFCAL1.word             = wiUWORD_GetUnsignedField(RegMap+0x0D9, 7,4);
    pReg->DelayPA.word                = wiUWORD_GetUnsignedField(RegMap+0x0D9, 3,0);
    pReg->DelayFCAL2.word             = wiUWORD_GetUnsignedField(RegMap+0x0DA, 7,4);
    pReg->DelayState51.word           = wiUWORD_GetUnsignedField(RegMap+0x0DA, 3,0);
    pReg->DelayShutdown.word          = wiUWORD_GetUnsignedField(RegMap+0x0DB, 7,4);
    pReg->DelaySleep.word             = wiUWORD_GetUnsignedField(RegMap+0x0DB, 3,0);
    pReg->TimeExtend.word             = wiUWORD_GetUnsignedField(RegMap+0x0DC, 7,0);
    pReg->SquelchTime.word            = wiUWORD_GetUnsignedField(RegMap+0x0DD, 5,0);
    pReg->DelaySquelchRF.word         = wiUWORD_GetUnsignedField(RegMap+0x0DE, 7,4);
    pReg->DelayClearDCX.word          = wiUWORD_GetUnsignedField(RegMap+0x0DE, 3,0);
    pReg->RadioMC.word                = wiUWORD_GetUnsignedField(RegMap+0x0E0, 7,0);
    pReg->DACOPM.word                 = wiUWORD_GetUnsignedField(RegMap+0x0E1, 5,0);
    pReg->ADCAOPM.word                = wiUWORD_GetUnsignedField(RegMap+0x0E2, 7,0);
    pReg->ADCBOPM.word                = wiUWORD_GetUnsignedField(RegMap+0x0E3, 7,0);
    pReg->RFSW1.word                  = wiUWORD_GetUnsignedField(RegMap+0x0E4, 7,0);
    pReg->RFSW2.word                  = wiUWORD_GetUnsignedField(RegMap+0x0E5, 7,0);
    pReg->LNAEnB.word                 = wiUWORD_GetUnsignedField(RegMap+0x0E6, 7,6);
    pReg->ADCBCTRLR.word              = wiUWORD_GetUnsignedField(RegMap+0x0E6, 5,4);
    pReg->ADCBCTRL.word               = wiUWORD_GetUnsignedField(RegMap+0x0E6, 2,0);
    pReg->sPA50.word                  = wiUWORD_GetUnsignedField(RegMap+0x0E7, 7,4);
    pReg->sPA24.word                  = wiUWORD_GetUnsignedField(RegMap+0x0E7, 3,0);
    pReg->ADCFSCTRL.word              = wiUWORD_GetUnsignedField(RegMap+0x0E8, 7,6);
    pReg->ADCCHSEL.word               = wiUWORD_GetUnsignedField(RegMap+0x0E8, 5,5);
    pReg->ADCOE.word                  = wiUWORD_GetUnsignedField(RegMap+0x0E8, 4,4);
    pReg->LNAxxBOnSW2.word            = wiUWORD_GetUnsignedField(RegMap+0x0E8, 3,3);
    pReg->ADCOFCEN.word               = wiUWORD_GetUnsignedField(RegMap+0x0E8, 2,2);
    pReg->ADCDCREN.word               = wiUWORD_GetUnsignedField(RegMap+0x0E8, 1,1);
    pReg->ADCMUX0.word                = wiUWORD_GetUnsignedField(RegMap+0x0E8, 0,0);
    pReg->ADCARDY.word                = wiUWORD_GetUnsignedField(RegMap+0x0E9, 7,7);
    pReg->ADCBRDY.word                = wiUWORD_GetUnsignedField(RegMap+0x0E9, 6,6);
    pReg->PPSEL_MODE.word             = wiUWORD_GetUnsignedField(RegMap+0x0E9, 4,3);
    pReg->ADCFCAL2.word               = wiUWORD_GetUnsignedField(RegMap+0x0E9, 2,2);
    pReg->ADCFCAL1.word               = wiUWORD_GetUnsignedField(RegMap+0x0E9, 1,1);
    pReg->ADCFCAL.word                = wiUWORD_GetUnsignedField(RegMap+0x0E9, 0,0);
    pReg->VCMEN.word                  = wiUWORD_GetUnsignedField(RegMap+0x0EA, 7,6);
    pReg->DACIFS_CTRL.word            = wiUWORD_GetUnsignedField(RegMap+0x0EA, 2,0);
    pReg->DACFG_CTRL0.word            = wiUWORD_GetUnsignedField(RegMap+0x0EB, 7,0);
    pReg->DACFG_CTRL1.word            = wiUWORD_GetUnsignedField(RegMap+0x0EC, 7,0);
    pReg->RESETB.word                 = wiUWORD_GetUnsignedField(RegMap+0x0ED, 7,7);
    pReg->TxLNAGAIN2.word             = wiUWORD_GetUnsignedField(RegMap+0x0ED, 6,6);
    pReg->LgSigSelDPD.word            = wiUWORD_GetUnsignedField(RegMap+0x0ED, 5,5);
    pReg->SREdge.word                 = wiUWORD_GetUnsignedField(RegMap+0x0ED, 2,2);
    pReg->SRFreq.word                 = wiUWORD_GetUnsignedField(RegMap+0x0ED, 1,0);
    pReg->sLNA24A.word                = wiUWORD_GetUnsignedField(RegMap+0x0EE, 7,4);
    pReg->sLNA24B.word                = wiUWORD_GetUnsignedField(RegMap+0x0EE, 3,0);
    pReg->sLNA50A.word                = wiUWORD_GetUnsignedField(RegMap+0x0EF, 7,4);
    pReg->sLNA50B.word                = wiUWORD_GetUnsignedField(RegMap+0x0EF, 3,0);
    pReg->SelectCCA.word              = wiUWORD_GetUnsignedField(RegMap+0x0F0, 7,0);
    pReg->ThCCA_SigDet.word           = wiUWORD_GetUnsignedField(RegMap+0x0F1, 7,0);
    pReg->ThCCA_RSSI1.word            = wiUWORD_GetUnsignedField(RegMap+0x0F2, 7,0);
    pReg->ThCCA_RSSI2.word            = wiUWORD_GetUnsignedField(RegMap+0x0F3, 7,0);
    pReg->ThCCA_GF1.word              = wiUWORD_GetUnsignedField(RegMap+0x0F4, 7,0);
    pReg->ThCCA_GF2.word              = wiUWORD_GetUnsignedField(RegMap+0x0F5, 7,0);
    pReg->ExtendBusyGF.word           = wiUWORD_GetUnsignedField(RegMap+0x0F6, 7,0);
    pReg->ExtendRxError.word          = wiUWORD_GetUnsignedField(RegMap+0x0F7, 7,0);
    pReg->ExtendRxError1.word         = wiUWORD_GetUnsignedField(RegMap+0x0F8, 7,0);
    pReg->P1_coef_t0_r_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x200, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x201, 4,0) * 256;
    pReg->P1_coef_t0_i_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x202, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x203, 4,0) * 256;
    pReg->P1_coef_t1_r_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x204, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x205, 4,0) * 256;
    pReg->P1_coef_t1_i_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x206, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x207, 4,0) * 256;
    pReg->P1_coef_t2_r_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x208, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x209, 4,0) * 256;
    pReg->P1_coef_t2_i_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x20A, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x20B, 4,0) * 256;
    pReg->P1_coef_t3_r_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x20C, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x20D, 4,0) * 256;
    pReg->P1_coef_t3_i_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x20E, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x20F, 4,0) * 256;
    pReg->P1_coef_t4_r_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x210, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x211, 4,0) * 256;
    pReg->P1_coef_t4_i_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x212, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x213, 4,0) * 256;
    pReg->P3_coef_t0_r_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x214, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x215, 6,0) * 256;
    pReg->P3_coef_t0_i_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x216, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x217, 6,0) * 256;
    pReg->P3_coef_t1_r_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x218, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x219, 6,0) * 256;
    pReg->P3_coef_t1_i_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x21A, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x21B, 6,0) * 256;
    pReg->P3_coef_t2_r_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x21C, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x21D, 6,0) * 256;
    pReg->P3_coef_t2_i_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x21E, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x21F, 6,0) * 256;
    pReg->P5_coef_t0_r_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x220, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x221, 3,0) * 256;
    pReg->P5_coef_t0_i_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x222, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x223, 3,0) * 256;
    pReg->P5_coef_t1_r_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x224, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x225, 3,0) * 256;
    pReg->P5_coef_t1_i_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x226, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x227, 3,0) * 256;
    pReg->P5_coef_t2_r_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x228, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x229, 3,0) * 256;
    pReg->P5_coef_t2_i_s0.word        = wiUWORD_GetUnsignedField(RegMap+0x22A, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x22B, 3,0) * 256;
    pReg->P1_coef_t0_r_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x22C, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x22D, 4,0) * 256;
    pReg->P1_coef_t0_i_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x22E, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x22F, 4,0) * 256;
    pReg->P1_coef_t1_r_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x230, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x231, 4,0) * 256;
    pReg->P1_coef_t1_i_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x232, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x233, 4,0) * 256;
    pReg->P1_coef_t2_r_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x234, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x235, 4,0) * 256;
    pReg->P1_coef_t2_i_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x236, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x237, 4,0) * 256;
    pReg->P1_coef_t3_r_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x238, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x239, 4,0) * 256;
    pReg->P1_coef_t3_i_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x23A, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x23B, 4,0) * 256;
    pReg->P1_coef_t4_r_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x23C, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x23D, 4,0) * 256;
    pReg->P1_coef_t4_i_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x23E, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x23F, 4,0) * 256;
    pReg->P3_coef_t0_r_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x240, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x241, 6,0) * 256;
    pReg->P3_coef_t0_i_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x242, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x243, 6,0) * 256;
    pReg->P3_coef_t1_r_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x244, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x245, 6,0) * 256;
    pReg->P3_coef_t1_i_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x246, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x247, 6,0) * 256;
    pReg->P3_coef_t2_r_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x248, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x249, 6,0) * 256;
    pReg->P3_coef_t2_i_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x24A, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x24B, 6,0) * 256;
    pReg->P5_coef_t0_r_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x24C, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x24D, 3,0) * 256;
    pReg->P5_coef_t0_i_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x24E, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x24F, 3,0) * 256;
    pReg->P5_coef_t1_r_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x250, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x251, 3,0) * 256;
    pReg->P5_coef_t1_i_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x252, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x253, 3,0) * 256;
    pReg->P5_coef_t2_r_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x254, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x255, 3,0) * 256;
    pReg->P5_coef_t2_i_s1.word        = wiUWORD_GetUnsignedField(RegMap+0x256, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x257, 3,0) * 256;
    pReg->P1_coef_t0_r_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x258, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x259, 4,0) * 256;
    pReg->P1_coef_t0_i_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x25A, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x25B, 4,0) * 256;
    pReg->P1_coef_t1_r_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x25C, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x25D, 4,0) * 256;
    pReg->P1_coef_t1_i_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x25E, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x25F, 4,0) * 256;
    pReg->P1_coef_t2_r_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x260, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x261, 4,0) * 256;
    pReg->P1_coef_t2_i_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x262, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x263, 4,0) * 256;
    pReg->P1_coef_t3_r_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x264, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x265, 4,0) * 256;
    pReg->P1_coef_t3_i_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x266, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x267, 4,0) * 256;
    pReg->P1_coef_t4_r_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x268, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x269, 4,0) * 256;
    pReg->P1_coef_t4_i_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x26A, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x26B, 4,0) * 256;
    pReg->P3_coef_t0_r_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x26C, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x26D, 6,0) * 256;
    pReg->P3_coef_t0_i_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x26E, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x26F, 6,0) * 256;
    pReg->P3_coef_t1_r_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x270, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x271, 6,0) * 256;
    pReg->P3_coef_t1_i_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x272, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x273, 6,0) * 256;
    pReg->P3_coef_t2_r_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x274, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x275, 6,0) * 256;
    pReg->P3_coef_t2_i_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x276, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x277, 6,0) * 256;
    pReg->P5_coef_t0_r_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x278, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x279, 3,0) * 256;
    pReg->P5_coef_t0_i_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x27A, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x27B, 3,0) * 256;
    pReg->P5_coef_t1_r_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x27C, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x27D, 3,0) * 256;
    pReg->P5_coef_t1_i_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x27E, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x27F, 3,0) * 256;
    pReg->P5_coef_t2_r_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x280, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x281, 3,0) * 256;
    pReg->P5_coef_t2_i_s2.word        = wiUWORD_GetUnsignedField(RegMap+0x282, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x283, 3,0) * 256;
    pReg->P1_coef_t0_r_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x284, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x285, 4,0) * 256;
    pReg->P1_coef_t0_i_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x286, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x287, 4,0) * 256;
    pReg->P1_coef_t1_r_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x288, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x289, 4,0) * 256;
    pReg->P1_coef_t1_i_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x28A, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x28B, 4,0) * 256;
    pReg->P1_coef_t2_r_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x28C, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x28D, 4,0) * 256;
    pReg->P1_coef_t2_i_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x28E, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x28F, 4,0) * 256;
    pReg->P1_coef_t3_r_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x290, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x291, 4,0) * 256;
    pReg->P1_coef_t3_i_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x292, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x293, 4,0) * 256;
    pReg->P1_coef_t4_r_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x294, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x295, 4,0) * 256;
    pReg->P1_coef_t4_i_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x296, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x297, 4,0) * 256;
    pReg->P3_coef_t0_r_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x298, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x299, 6,0) * 256;
    pReg->P3_coef_t0_i_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x29A, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x29B, 6,0) * 256;
    pReg->P3_coef_t1_r_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x29C, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x29D, 6,0) * 256;
    pReg->P3_coef_t1_i_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x29E, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x29F, 6,0) * 256;
    pReg->P3_coef_t2_r_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x2A0, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x2A1, 6,0) * 256;
    pReg->P3_coef_t2_i_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x2A2, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x2A3, 6,0) * 256;
    pReg->P5_coef_t0_r_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x2A4, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x2A5, 3,0) * 256;
    pReg->P5_coef_t0_i_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x2A6, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x2A7, 3,0) * 256;
    pReg->P5_coef_t1_r_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x2A8, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x2A9, 3,0) * 256;
    pReg->P5_coef_t1_i_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x2AA, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x2AB, 3,0) * 256;
    pReg->P5_coef_t2_r_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x2AC, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x2AD, 3,0) * 256;
    pReg->P5_coef_t2_i_s3.word        = wiUWORD_GetUnsignedField(RegMap+0x2AE, 7,0)
                                      + wiUWORD_GetSignedField  (RegMap+0x2AF, 3,0) * 256;
    pReg->cross_pwr1.word             = wiUWORD_GetUnsignedField(RegMap+0x2B0, 5,0);
    pReg->cross_pwr2.word             = wiUWORD_GetUnsignedField(RegMap+0x2B1, 5,0);
    pReg->cross_pwr3.word             = wiUWORD_GetUnsignedField(RegMap+0x2B2, 5,0);
    pReg->Tx_Rate_OFDM_HT.word        = wiUWORD_GetUnsignedField(RegMap+0x2B3, 7,0);
    pReg->Tx_Rate_OFDM_LL.word        = wiUWORD_GetUnsignedField(RegMap+0x2B4, 7,0);
    pReg->Tx_Rate_OFDM_LM.word        = wiUWORD_GetUnsignedField(RegMap+0x2B5, 7,0);
    pReg->DPD_spare.word              = wiUWORD_GetUnsignedField(RegMap+0x2B6, 7,5);
    pReg->DPD_LOW_TMP_DIS.word        = wiUWORD_GetUnsignedField(RegMap+0x2B6, 4,4);
    pReg->DPD_PA_tmp.word             = wiUWORD_GetUnsignedField(RegMap+0x2B6, 3,3);
    pReg->DPD_LOW_RATE_DIS.word       = wiUWORD_GetUnsignedField(RegMap+0x2B6, 2,2);
    pReg->DPD_coef_interspersion.word = wiUWORD_GetUnsignedField(RegMap+0x2B6, 1,0);
    pReg->Cbc_coef_sel.word           = wiUWORD_GetUnsignedField(RegMap+0x2B7, 7,6);
    pReg->cnfg_eovr.word              = wiUWORD_GetUnsignedField(RegMap+0x2B7, 4,4);
    pReg->Coef_sel_vovr.word          = wiUWORD_GetUnsignedField(RegMap+0x2B7, 3,2);
    pReg->Vfb_byp_vovr.word           = wiUWORD_GetUnsignedField(RegMap+0x2B7, 1,1);
    pReg->Pa_tmp_ext.word             = wiUWORD_GetUnsignedField(RegMap+0x2B7, 0,0);
    pReg->BISTDone.word               = wiUWORD_GetUnsignedField(RegMap+0x300, 7,7);
    pReg->BISTADCSel.word             = wiUWORD_GetUnsignedField(RegMap+0x300, 6,6);
    pReg->BISTNoPrechg.word           = wiUWORD_GetUnsignedField(RegMap+0x300, 5,5);
    pReg->BISTFGSel.word              = wiUWORD_GetUnsignedField(RegMap+0x300, 4,4);
    pReg->BISTMode.word               = wiUWORD_GetUnsignedField(RegMap+0x300, 3,1);
    pReg->StartBIST.word              = wiUWORD_GetUnsignedField(RegMap+0x300, 0,0);
    pReg->BISTPipe.word               = wiUWORD_GetUnsignedField(RegMap+0x301, 7,4);
    pReg->BISTN_val.word              = wiUWORD_GetUnsignedField(RegMap+0x301, 3,0);
    pReg->BISTOfsR.word               = wiUWORD_GetUnsignedField(RegMap+0x302, 7,0);
    pReg->BISTOfsI.word               = wiUWORD_GetUnsignedField(RegMap+0x303, 7,0);
    pReg->BISTTh1.word                = wiUWORD_GetUnsignedField(RegMap+0x304, 7,0);
    pReg->BISTTh2.word                = wiUWORD_GetUnsignedField(RegMap+0x305, 7,0) * 256
                                      + wiUWORD_GetUnsignedField(RegMap+0x306, 7,0);
    pReg->BISTFG_CTRLI.word           = wiUWORD_GetUnsignedField(RegMap+0x307, 7,0);
    pReg->BISTFG_CTRLR.word           = wiUWORD_GetUnsignedField(RegMap+0x308, 7,0);
    pReg->BISTStatus.word             = wiUWORD_GetUnsignedField(RegMap+0x309, 7,0);
    pReg->BISTAccR.word               = wiUWORD_GetUnsignedField(RegMap+0x30A, 7,0) * 65536
                                      + wiUWORD_GetUnsignedField(RegMap+0x30B, 7,0) * 256
                                      + wiUWORD_GetUnsignedField(RegMap+0x30C, 7,0);
    pReg->BISTAccI.word               = wiUWORD_GetUnsignedField(RegMap+0x30D, 7,0) * 65536
                                      + wiUWORD_GetUnsignedField(RegMap+0x30E, 7,0) * 256
                                      + wiUWORD_GetUnsignedField(RegMap+0x30F, 7,0);
    pReg->BISTRangeLimH.word          = wiUWORD_GetUnsignedField(RegMap+0x310, 7,4);
    pReg->BISTRangeLimL.word          = wiUWORD_GetUnsignedField(RegMap+0x310, 3,0);
    pReg->DFSIntEOB.word              = wiUWORD_GetUnsignedField(RegMap+0x320, 1,1);
    pReg->DFSIntFIFO.word             = wiUWORD_GetUnsignedField(RegMap+0x320, 0,0);
    pReg->DFSThFIFO.word              = wiUWORD_GetUnsignedField(RegMap+0x321, 6,0);
    pReg->DFSDepthFIFO.word           = wiUWORD_GetUnsignedField(RegMap+0x322, 7,0);
    pReg->DFSReadFIFO.word            = wiUWORD_GetUnsignedField(RegMap+0x323, 7,0);
    pReg->DFSSyncFIFO.word            = wiUWORD_GetUnsignedField(RegMap+0x324, 0,0);
    pReg->RxHeader1.word              = wiUWORD_GetUnsignedField(RegMap+0x328, 7,0);
    pReg->RxHeader2.word              = wiUWORD_GetUnsignedField(RegMap+0x329, 7,0);
    pReg->RxHeader3.word              = wiUWORD_GetUnsignedField(RegMap+0x32A, 7,0);
    pReg->RxHeader4.word              = wiUWORD_GetUnsignedField(RegMap+0x32B, 7,0);
    pReg->RxHeader5.word              = wiUWORD_GetUnsignedField(RegMap+0x32C, 7,0);
    pReg->RxHeader6.word              = wiUWORD_GetUnsignedField(RegMap+0x32D, 7,0);
    pReg->RxHeader7.word              = wiUWORD_GetUnsignedField(RegMap+0x32E, 7,0);
    pReg->RxHeader8.word              = wiUWORD_GetUnsignedField(RegMap+0x32F, 7,0);
    pReg->TxHeader1.word              = wiUWORD_GetUnsignedField(RegMap+0x330, 7,0);
    pReg->TxHeader2.word              = wiUWORD_GetUnsignedField(RegMap+0x331, 7,0);
    pReg->TxHeader3.word              = wiUWORD_GetUnsignedField(RegMap+0x332, 7,0);
    pReg->TxHeader4.word              = wiUWORD_GetUnsignedField(RegMap+0x333, 7,0);
    pReg->TxHeader5.word              = wiUWORD_GetUnsignedField(RegMap+0x334, 7,0);
    pReg->TxHeader6.word              = wiUWORD_GetUnsignedField(RegMap+0x335, 7,0);
    pReg->TxHeader7.word              = wiUWORD_GetUnsignedField(RegMap+0x336, 7,0);
    pReg->TxHeader8.word              = wiUWORD_GetUnsignedField(RegMap+0x337, 7,0);
    pReg->TestSigEn.word              = wiUWORD_GetUnsignedField(RegMap+0x338, 0,0);
    pReg->ADCAI.word                  = wiUWORD_GetUnsignedField(RegMap+0x339, 7,0) * 4
                                      + wiUWORD_GetUnsignedField(RegMap+0x33D, 7,6);
    pReg->ADCAQ.word                  = wiUWORD_GetUnsignedField(RegMap+0x33A, 7,0) * 4
                                      + wiUWORD_GetUnsignedField(RegMap+0x33D, 5,4);
    pReg->ADCBI.word                  = wiUWORD_GetUnsignedField(RegMap+0x33B, 7,0) * 4
                                      + wiUWORD_GetUnsignedField(RegMap+0x33D, 3,2);
    pReg->ADCBQ.word                  = wiUWORD_GetUnsignedField(RegMap+0x33C, 7,0) * 4
                                      + wiUWORD_GetUnsignedField(RegMap+0x33D, 1,0);
    pReg->bHeaderValid.word           = wiUWORD_GetUnsignedField(RegMap+0x33E, 7,7);
    pReg->bSyncFound.word             = wiUWORD_GetUnsignedField(RegMap+0x33E, 6,6);
    pReg->bAGCDone.word               = wiUWORD_GetUnsignedField(RegMap+0x33E, 5,5);
    pReg->bSigDet.word                = wiUWORD_GetUnsignedField(RegMap+0x33E, 4,4);
    pReg->aHeaderValid.word           = wiUWORD_GetUnsignedField(RegMap+0x33E, 3,3);
    pReg->aSyncFound.word             = wiUWORD_GetUnsignedField(RegMap+0x33E, 2,2);
    pReg->aAGCDone.word               = wiUWORD_GetUnsignedField(RegMap+0x33E, 1,1);
    pReg->aSigDet.word                = wiUWORD_GetUnsignedField(RegMap+0x33E, 0,0);
    pReg->SelectSignal2.word          = wiUWORD_GetUnsignedField(RegMap+0x3F8, 7,4);
    pReg->SelectSignal1.word          = wiUWORD_GetUnsignedField(RegMap+0x3F8, 3,0);
    pReg->SelectTrigger2.word         = wiUWORD_GetUnsignedField(RegMap+0x3F9, 3,2);
    pReg->SelectTrigger1.word         = wiUWORD_GetUnsignedField(RegMap+0x3F9, 1,0);
    pReg->TriggerState1.word          = wiUWORD_GetUnsignedField(RegMap+0x3FA, 5,0);
    pReg->TriggerState2.word          = wiUWORD_GetUnsignedField(RegMap+0x3FB, 5,0);
    pReg->TriggerRSSI.word            = wiUWORD_GetUnsignedField(RegMap+0x3FC, 7,0);
    pReg->LgSigAFE.word               = wiUWORD_GetUnsignedField(RegMap+0x3FD, 7,7);
    pReg->Trigger2.word               = wiUWORD_GetUnsignedField(RegMap+0x3FD, 3,3);
    pReg->Trigger1.word               = wiUWORD_GetUnsignedField(RegMap+0x3FD, 2,2);
    pReg->Signal2.word                = wiUWORD_GetUnsignedField(RegMap+0x3FD, 1,1);
    pReg->Signal1.word                = wiUWORD_GetUnsignedField(RegMap+0x3FD, 0,0);
    pReg->ClearInt.word               = wiUWORD_GetUnsignedField(RegMap+0x3FE, 7,7);
    pReg->IntEvent2.word              = wiUWORD_GetUnsignedField(RegMap+0x3FE, 6,6);
    pReg->IntEvent1.word              = wiUWORD_GetUnsignedField(RegMap+0x3FE, 5,5);
    pReg->IntRxFault.word             = wiUWORD_GetUnsignedField(RegMap+0x3FE, 4,4);
    pReg->IntRxAbortTx.word           = wiUWORD_GetUnsignedField(RegMap+0x3FE, 3,3);
    pReg->IntTxSleep.word             = wiUWORD_GetUnsignedField(RegMap+0x3FE, 2,2);
    pReg->IntTxAbort.word             = wiUWORD_GetUnsignedField(RegMap+0x3FE, 1,1);
    pReg->IntTxFault.word             = wiUWORD_GetUnsignedField(RegMap+0x3FE, 0,0);
    pReg->Interrupt.word              = wiUWORD_GetUnsignedField(RegMap+0x3FF, 7,7);
    pReg->Event2.word                 = wiUWORD_GetUnsignedField(RegMap+0x3FF, 6,6);
    pReg->Event1.word                 = wiUWORD_GetUnsignedField(RegMap+0x3FF, 5,5);
    pReg->RxFault.word                = wiUWORD_GetUnsignedField(RegMap+0x3FF, 4,4);
    pReg->RxAbortTx.word              = wiUWORD_GetUnsignedField(RegMap+0x3FF, 3,3);
    pReg->TxSleep.word                = wiUWORD_GetUnsignedField(RegMap+0x3FF, 2,2);
    pReg->TxAbort.word                = wiUWORD_GetUnsignedField(RegMap+0x3FF, 1,1);
    pReg->TxFault.word                = wiUWORD_GetUnsignedField(RegMap+0x3FF, 0,0);
    // ----------------------------------------------------------------------------------
    //  END: Matlab Generated Code
    // ----------------------------------------------------------------------------------
    {
        wiWORD Zero = {0};
/*
        Verona_DCX(-1, Zero, NULL, 0, Verona_RxState(), pReg);
*/
    }
    pReg->ID.word = VERONA_PART_ID; // force the part ID field

    return WI_SUCCESS;
}
// end of Verona_ReadRegisterMap()

// ================================================================================================
// FUNCTION  : Verona_WriteRegisterMap()
// ------------------------------------------------------------------------------------------------
// Purpose   : Write values from the Register structure into the RegMap array
// Parameters: RegMap -- pointer to the register map to which to parameters are written
// ================================================================================================
wiStatus Verona_WriteRegisterMap(wiUWORD *RegMap)
{
    VeronaRegisters_t *pReg = Verona_Registers();

    if (wiProcess_ThreadIndex() != 0) return STATUS(WI_ERROR_INVALID_FOR_THREAD);

    pReg->ID.word = VERONA_PART_ID; // force the part ID value
    memset( RegMap, 0, 1024 );

    //  Matlab Generated Code from Verona_GetRegisterMap.m
    // ----------------------------------------------------------------------------------
    wiUWORD_SetUnsignedField(RegMap+0x001, 7,0, pReg->ID.w8);
    wiUWORD_SetUnsignedField(RegMap+0x003, 1,0, pReg->PathSel.w2);
    wiUWORD_SetUnsignedField(RegMap+0x004, 2,0, pReg->RXMode.w3);
    wiUWORD_SetUnsignedField(RegMap+0x005, 3,0, pReg->TestMode.w4);
    wiUWORD_SetUnsignedField(RegMap+0x006, 7,7, pReg->UpmixConj.w1);
    wiUWORD_SetUnsignedField(RegMap+0x006, 6,6, pReg->DownmixConj.w1);
    wiUWORD_SetUnsignedField(RegMap+0x006, 3,3, pReg->UpmixDown.w1);
    wiUWORD_SetUnsignedField(RegMap+0x006, 2,2, pReg->DownmixUp.w1);
    wiUWORD_SetUnsignedField(RegMap+0x006, 1,1, pReg->UpmixOff.w1);
    wiUWORD_SetUnsignedField(RegMap+0x006, 0,0, pReg->DownmixOff.w1);
    wiUWORD_SetUnsignedField(RegMap+0x007, 7,6, pReg->TxIQSrcR.w2);
    wiUWORD_SetUnsignedField(RegMap+0x007, 5,4, pReg->TxIQSrcI.w2);
    wiUWORD_SetUnsignedField(RegMap+0x007, 3,2, pReg->PrdDACSrcR.w2);
    wiUWORD_SetUnsignedField(RegMap+0x007, 1,0, pReg->PrdDACSrcI.w2);
    wiUWORD_SetUnsignedField(RegMap+0x008, 7,6, pReg->aTxExtend.w2);
    wiUWORD_SetUnsignedField(RegMap+0x008, 5,0, pReg->aTxDelay.w6);
    wiUWORD_SetUnsignedField(RegMap+0x009, 7,6, pReg->bTxExtend.w2);
    wiUWORD_SetUnsignedField(RegMap+0x009, 5,0, pReg->bTxDelay.w6);
    wiUWORD_SetUnsignedField(RegMap+0x00A, 7,7, pReg->TxIQSrcRun.w1);
    wiUWORD_SetUnsignedField(RegMap+0x00A, 0,0, pReg->TxSmoothing.w1);
    wiUWORD_SetUnsignedField(RegMap+0x00C, 7,0,(pReg->TX_SERVICE.w16>>8) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x00D, 7,0,(pReg->TX_SERVICE.w16>>0) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x00E, 6,0, pReg->TX_PRBS.w7);
    wiUWORD_SetUnsignedField(RegMap+0x014, 7,0,(pReg->RX_SERVICE.w16>>8) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x015, 7,0,(pReg->RX_SERVICE.w16>>0) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x016, 7,0, pReg->RSSI0.w8);
    wiUWORD_SetUnsignedField(RegMap+0x017, 7,0, pReg->RSSI1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x018, 7,0, pReg->maxLENGTH.w8);
    wiUWORD_SetUnsignedField(RegMap+0x019, 3,3, pReg->NotSounding.w1);
    wiUWORD_SetUnsignedField(RegMap+0x019, 2,2, pReg->nReserved1.w1);
    wiUWORD_SetUnsignedField(RegMap+0x019, 0,0, pReg->Reserved0.w1);
    wiUWORD_SetUnsignedField(RegMap+0x01A, 7,0, pReg->maxLENGTHnH.w8);
    wiUWORD_SetUnsignedField(RegMap+0x01B, 7,0, pReg->maxLENGTHnL.w8);
    wiUWORD_SetUnsignedField(RegMap+0x01C, 7,0, pReg->aPPDUMaxTimeH.w8);
    wiUWORD_SetUnsignedField(RegMap+0x01D, 7,0, pReg->aPPDUMaxTimeL.w8);
    wiUWORD_SetUnsignedField(RegMap+0x01E, 6,6, pReg->PilotPLLEn.w1);
    wiUWORD_SetUnsignedField(RegMap+0x01E, 5,3, pReg->aC.w3);
    wiUWORD_SetUnsignedField(RegMap+0x01E, 2,0, pReg->bC.w3);
    wiUWORD_SetUnsignedField(RegMap+0x01F, 2,0, pReg->cC.w3);
    wiUWORD_SetUnsignedField(RegMap+0x020, 5,3, pReg->Sa.w3);
    wiUWORD_SetUnsignedField(RegMap+0x020, 2,0, pReg->Sb.w3);
    wiUWORD_SetUnsignedField(RegMap+0x021, 5,3, pReg->Ca.w3);
    wiUWORD_SetUnsignedField(RegMap+0x021, 2,0, pReg->Cb.w3);
    wiUWORD_SetUnsignedField(RegMap+0x024, 0,0, pReg->sTxIQ.w1);
    wiUWORD_SetSignedField  (RegMap+0x025, 4,0, pReg->TxIQa11.w5);
    wiUWORD_SetSignedField  (RegMap+0x026, 4,0, pReg->TxIQa22.w5);
    wiUWORD_SetSignedField  (RegMap+0x027, 4,0, pReg->TxIQa12.w5);
    wiUWORD_SetUnsignedField(RegMap+0x028, 1,0, pReg->sRxIQ.w2);
    wiUWORD_SetSignedField  (RegMap+0x029, 4,0, pReg->RxIQ0a11.w5);
    wiUWORD_SetSignedField  (RegMap+0x02A, 4,0, pReg->RxIQ0a22.w5);
    wiUWORD_SetSignedField  (RegMap+0x02B, 4,0, pReg->RxIQ0a12.w5);
    wiUWORD_SetSignedField  (RegMap+0x02C, 4,0, pReg->RxIQ1a11.w5);
    wiUWORD_SetSignedField  (RegMap+0x02D, 4,0, pReg->RxIQ1a22.w5);
    wiUWORD_SetSignedField  (RegMap+0x02E, 4,0, pReg->RxIQ1a12.w5);
    wiUWORD_SetSignedField  (RegMap+0x030, 3,0, pReg->ImgDetCoefRH.w4);
    wiUWORD_SetUnsignedField(RegMap+0x031, 7,0, pReg->ImgDetCoefRL.w8);
    wiUWORD_SetSignedField  (RegMap+0x032, 3,0, pReg->ImgDetCoefIH.w4);
    wiUWORD_SetUnsignedField(RegMap+0x033, 7,0, pReg->ImgDetCoefIL.w8);
    wiUWORD_SetUnsignedField(RegMap+0x034, 7,0, pReg->ImgDetLength.w8);
    wiUWORD_SetUnsignedField(RegMap+0x035, 7,7, pReg->ImgDetEn.w1);
    wiUWORD_SetUnsignedField(RegMap+0x035, 6,6, pReg->ImgDetLengthH.w1);
    wiUWORD_SetUnsignedField(RegMap+0x035, 5,3, pReg->ImgDetPostShift.w3);
    wiUWORD_SetUnsignedField(RegMap+0x035, 2,0, pReg->ImgDetPreShift.w3);
    wiUWORD_SetUnsignedField(RegMap+0x036, 7,0, pReg->ImgDetDelay.w8);
    wiUWORD_SetUnsignedField(RegMap+0x037, 2,0, pReg->ImgDetAvgNum.w3);
    wiUWORD_SetUnsignedField(RegMap+0x038, 7,0, pReg->ImgDetPowerH.w8);
    wiUWORD_SetUnsignedField(RegMap+0x039, 7,0, pReg->ImgDetPowerL.w8);
    wiUWORD_SetUnsignedField(RegMap+0x03A, 7,7, pReg->CalDone.w1);
    wiUWORD_SetUnsignedField(RegMap+0x03A, 6,6, pReg->CalResultType.w1);
    wiUWORD_SetUnsignedField(RegMap+0x03A, 0,0, pReg->ImgDetDone.w1);
    wiUWORD_SetUnsignedField(RegMap+0x03B, 7,7, pReg->CalRun.w1);
    wiUWORD_SetUnsignedField(RegMap+0x03B, 6,6, pReg->CalPwrAdjust.w1);
    wiUWORD_SetUnsignedField(RegMap+0x03B, 5,5, pReg->CalMode.w1);
    wiUWORD_SetUnsignedField(RegMap+0x03B, 4,4, pReg->CalCoefUpdate.w1);
    wiUWORD_SetUnsignedField(RegMap+0x03B, 3,3, pReg->CalFineStep.w1);
    wiUWORD_SetUnsignedField(RegMap+0x03B, 2,0, pReg->CalCoarseStep.w3);
    wiUWORD_SetUnsignedField(RegMap+0x03C, 7,3, pReg->CalPwrRatio.w5);
    wiUWORD_SetUnsignedField(RegMap+0x03C, 1,0, pReg->CalIterationTh.w2);
    wiUWORD_SetUnsignedField(RegMap+0x03D, 6,0, pReg->CalDelay.w7);
    wiUWORD_SetUnsignedField(RegMap+0x03E, 5,0, pReg->CalGain.w6);
    wiUWORD_SetUnsignedField(RegMap+0x03F, 4,0, pReg->CalPhase.w5);
    wiUWORD_SetUnsignedField(RegMap+0x040, 6,0, pReg->RefPwrDig.w7);
    wiUWORD_SetUnsignedField(RegMap+0x041, 5,0, pReg->InitAGain.w6);
    wiUWORD_SetUnsignedField(RegMap+0x042, 6,0, pReg->AbsPwrL.w7);
    wiUWORD_SetUnsignedField(RegMap+0x043, 6,0, pReg->AbsPwrH.w7);
    wiUWORD_SetUnsignedField(RegMap+0x044, 6,0, pReg->ThSigLarge.w7);
    wiUWORD_SetUnsignedField(RegMap+0x045, 6,0, pReg->ThSigSmall.w7);
    wiUWORD_SetUnsignedField(RegMap+0x046, 4,0, pReg->StepLgSigDet.w5);
    wiUWORD_SetUnsignedField(RegMap+0x047, 4,0, pReg->StepLarge.w5);
    wiUWORD_SetUnsignedField(RegMap+0x048, 4,0, pReg->StepSmall.w5);
    wiUWORD_SetUnsignedField(RegMap+0x049, 5,0, pReg->ThSwitchLNA.w6);
    wiUWORD_SetUnsignedField(RegMap+0x04A, 4,0, pReg->StepLNA.w5);
    wiUWORD_SetUnsignedField(RegMap+0x04B, 5,0, pReg->ThSwitchLNA2.w6);
    wiUWORD_SetUnsignedField(RegMap+0x04C, 4,0, pReg->StepLNA2.w5);
    wiUWORD_SetUnsignedField(RegMap+0x04D, 4,0, pReg->Pwr100dBm.w5);
    wiUWORD_SetUnsignedField(RegMap+0x04E, 4,0, pReg->StepSmallPeak.w5);
    wiUWORD_SetUnsignedField(RegMap+0x050, 7,7, pReg->FixedGain.w1);
    wiUWORD_SetUnsignedField(RegMap+0x050, 6,6, pReg->InitLNAGain.w1);
    wiUWORD_SetUnsignedField(RegMap+0x050, 5,5, pReg->InitLNAGain2.w1);
    wiUWORD_SetUnsignedField(RegMap+0x050, 4,4, pReg->UpdateOnLNA.w1);
    wiUWORD_SetUnsignedField(RegMap+0x050, 2,0, pReg->MaxUpdateCount.w3);
    wiUWORD_SetUnsignedField(RegMap+0x051, 7,0, pReg->ThStepUpRefPwr.w8);
    wiUWORD_SetUnsignedField(RegMap+0x052, 7,4, pReg->ThStepUp.w4);
    wiUWORD_SetUnsignedField(RegMap+0x052, 3,0, pReg->ThStepDown.w4);
    wiUWORD_SetUnsignedField(RegMap+0x053, 6,0, pReg->ThDFSUp.w7);
    wiUWORD_SetUnsignedField(RegMap+0x054, 6,0, pReg->ThDFSDn.w7);
    wiUWORD_SetUnsignedField(RegMap+0x055, 5,0, pReg->ThStepDownMsrPwr.w6);
    wiUWORD_SetUnsignedField(RegMap+0x057, 7,7, pReg->DFSOn.w1);
    wiUWORD_SetUnsignedField(RegMap+0x057, 5,5, pReg->DFS2Candidates.w1);
    wiUWORD_SetUnsignedField(RegMap+0x057, 4,4, pReg->DFSSmooth.w1);
    wiUWORD_SetUnsignedField(RegMap+0x057, 3,3, pReg->DFSMACFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0x057, 2,2, pReg->DFSHdrFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0x057, 0,0, pReg->DFSIntDet.w1);
    wiUWORD_SetUnsignedField(RegMap+0x058, 5,4, pReg->DFSPattern.w2);
    wiUWORD_SetUnsignedField(RegMap+0x058, 2,0, pReg->DFSminPPB.w3);
    wiUWORD_SetUnsignedField(RegMap+0x059, 3,0, pReg->DFSdTPRF.w4);
    wiUWORD_SetUnsignedField(RegMap+0x05A, 7,0, pReg->DFSmaxWidth.w8);
    wiUWORD_SetUnsignedField(RegMap+0x05B, 7,0, pReg->DFSmaxTPRF.w8);
    wiUWORD_SetUnsignedField(RegMap+0x05C, 7,0, pReg->DFSminTPRF.w8);
    wiUWORD_SetUnsignedField(RegMap+0x05D, 6,0, pReg->DFSminGap.w7);
    wiUWORD_SetUnsignedField(RegMap+0x05E, 7,0, pReg->DFSPulseSR.w8);
    wiUWORD_SetUnsignedField(RegMap+0x05F, 7,7, pReg->detRadar.w1);
    wiUWORD_SetUnsignedField(RegMap+0x05F, 3,3, pReg->detHeader.w1);
    wiUWORD_SetUnsignedField(RegMap+0x05F, 2,2, pReg->detPreamble.w1);
    wiUWORD_SetUnsignedField(RegMap+0x05F, 1,1, pReg->detStepDown.w1);
    wiUWORD_SetUnsignedField(RegMap+0x05F, 0,0, pReg->detStepUp.w1);
    wiUWORD_SetUnsignedField(RegMap+0x060, 6,0, pReg->WaitAGC.w7);
    wiUWORD_SetUnsignedField(RegMap+0x061, 4,0, pReg->DelayAGC.w5);
    wiUWORD_SetUnsignedField(RegMap+0x062, 7,0, pReg->SigDetWindow.w8);
    wiUWORD_SetUnsignedField(RegMap+0x063, 3,0, pReg->SyncShift.w4);
    wiUWORD_SetUnsignedField(RegMap+0x064, 7,0, pReg->DelayRestart.w8);
    wiUWORD_SetUnsignedField(RegMap+0x065, 4,0, pReg->OFDMSwDDelay.w5);
    wiUWORD_SetUnsignedField(RegMap+0x066, 7,0, pReg->OFDMSwDTh.w8);
    wiUWORD_SetUnsignedField(RegMap+0x067, 7,7, pReg->CFOFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0x067, 6,6, pReg->UpdateGainMF.w1);
    wiUWORD_SetUnsignedField(RegMap+0x067, 5,5, pReg->ArmLgSigDet3.w1);
    wiUWORD_SetUnsignedField(RegMap+0x067, 4,4, pReg->LgSigAFEValid.w1);
    wiUWORD_SetUnsignedField(RegMap+0x067, 3,3, pReg->IdleFSEnB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x067, 2,2, pReg->IdleSDEnB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x067, 1,1, pReg->IdleDAEnB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x067, 0,0, pReg->SDEnB17.w1);
    wiUWORD_SetUnsignedField(RegMap+0x068, 4,0, pReg->DelayCFO.w5);
    wiUWORD_SetUnsignedField(RegMap+0x069, 5,0, pReg->DelayRSSI.w6);
    wiUWORD_SetUnsignedField(RegMap+0x06A, 7,0, pReg->DelayRIFS.w8);
    wiUWORD_SetUnsignedField(RegMap+0x06B, 7,7, pReg->OFDMSwDEn.w1);
    wiUWORD_SetUnsignedField(RegMap+0x06B, 6,6, pReg->OFDMSwDSave.w1);
    wiUWORD_SetUnsignedField(RegMap+0x06B, 5,5, pReg->OFDMSwDInit.w1);
    wiUWORD_SetUnsignedField(RegMap+0x06B, 0,0, pReg->UpdateGain11b.w1);
    wiUWORD_SetUnsignedField(RegMap+0x06C, 6,0, pReg->OFDMSwDThLow.w7);
    wiUWORD_SetUnsignedField(RegMap+0x070, 5,0, pReg->SigDetTh1.w6);
    wiUWORD_SetUnsignedField(RegMap+0x071, 7,0, pReg->SigDetTh2H.w8);
    wiUWORD_SetUnsignedField(RegMap+0x072, 7,0, pReg->SigDetTh2L.w8);
    wiUWORD_SetUnsignedField(RegMap+0x073, 7,0, pReg->SigDetDelay.w8);
    wiUWORD_SetUnsignedField(RegMap+0x074, 7,7, pReg->SigDetFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0x078, 7,7, pReg->SyncFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0x078, 4,0, pReg->SyncMag.w5);
    wiUWORD_SetUnsignedField(RegMap+0x079, 7,4, pReg->SyncThSig.w4);
    wiUWORD_SetSignedField  (RegMap+0x079, 3,0, pReg->SyncThExp.w4);
    wiUWORD_SetUnsignedField(RegMap+0x07A, 4,0, pReg->SyncDelay.w5);
    wiUWORD_SetUnsignedField(RegMap+0x080, 3,0, pReg->MinShift.w4);
    wiUWORD_SetUnsignedField(RegMap+0x081, 7,4, pReg->ChEstShift0.w4);
    wiUWORD_SetUnsignedField(RegMap+0x081, 3,0, pReg->ChEstShift1.w4);
    wiUWORD_SetUnsignedField(RegMap+0x082, 7,7, pReg->ChTracking.w1);
    wiUWORD_SetUnsignedField(RegMap+0x082, 5,3, pReg->ChTrackGain1.w3);
    wiUWORD_SetUnsignedField(RegMap+0x082, 2,0, pReg->ChTrackGain2.w3);
    wiUWORD_SetUnsignedField(RegMap+0x084, 5,3, pReg->aC1.w3);
    wiUWORD_SetUnsignedField(RegMap+0x084, 2,0, pReg->bC1.w3);
    wiUWORD_SetUnsignedField(RegMap+0x085, 5,3, pReg->aS1.w3);
    wiUWORD_SetUnsignedField(RegMap+0x085, 2,0, pReg->bS1.w3);
    wiUWORD_SetUnsignedField(RegMap+0x086, 5,3, pReg->aC2.w3);
    wiUWORD_SetUnsignedField(RegMap+0x086, 2,0, pReg->bC2.w3);
    wiUWORD_SetUnsignedField(RegMap+0x087, 5,3, pReg->aS2.w3);
    wiUWORD_SetUnsignedField(RegMap+0x087, 2,0, pReg->bS2.w3);
    wiUWORD_SetUnsignedField(RegMap+0x088, 7,0, pReg->TxToneDiv.w8);
    wiUWORD_SetUnsignedField(RegMap+0x08C, 1,0, pReg->TxWordRH.w2);
    wiUWORD_SetUnsignedField(RegMap+0x08D, 7,0, pReg->TxWordRL.w8);
    wiUWORD_SetUnsignedField(RegMap+0x08E, 1,0, pReg->TxWordIH.w2);
    wiUWORD_SetUnsignedField(RegMap+0x08F, 7,0, pReg->TxWordIL.w8);
    wiUWORD_SetUnsignedField(RegMap+0x090, 7,7, pReg->DCXSelect.w1);
    wiUWORD_SetUnsignedField(RegMap+0x090, 6,6, pReg->DCXFastLoad.w1);
    wiUWORD_SetUnsignedField(RegMap+0x090, 5,5, pReg->DCXHoldAcc.w1);
    wiUWORD_SetUnsignedField(RegMap+0x090, 2,0, pReg->DCXShift.w3);
    wiUWORD_SetSignedField  (RegMap+0x091, 4,0, pReg->DCXAcc0RH.w5);
    wiUWORD_SetUnsignedField(RegMap+0x092, 7,0, pReg->DCXAcc0RL.w8);
    wiUWORD_SetSignedField  (RegMap+0x093, 4,0, pReg->DCXAcc0IH.w5);
    wiUWORD_SetUnsignedField(RegMap+0x094, 7,0, pReg->DCXAcc0IL.w8);
    wiUWORD_SetSignedField  (RegMap+0x095, 4,0, pReg->DCXAcc1RH.w5);
    wiUWORD_SetUnsignedField(RegMap+0x096, 7,0, pReg->DCXAcc1RL.w8);
    wiUWORD_SetSignedField  (RegMap+0x097, 4,0, pReg->DCXAcc1IH.w5);
    wiUWORD_SetUnsignedField(RegMap+0x098, 7,0, pReg->DCXAcc1IL.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0A0, 7,0, pReg->INITdelay.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0A1, 7,6, pReg->EDwindow.w2);
    wiUWORD_SetUnsignedField(RegMap+0x0A1, 5,4, pReg->CQwindow.w2);
    wiUWORD_SetUnsignedField(RegMap+0x0A1, 2,0, pReg->SSwindow.w3);
    wiUWORD_SetUnsignedField(RegMap+0x0A2, 5,0, pReg->CQthreshold.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0A3, 5,0, pReg->EDthreshold.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0A4, 5,0, pReg->bWaitAGC.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0A5, 5,0, pReg->bAGCdelay.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0A6, 5,0, pReg->bRefPwr.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0A7, 5,0, pReg->bInitAGain.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0A8, 5,0, pReg->bThSigLarge.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0A9, 4,0, pReg->bStepLarge.w5);
    wiUWORD_SetUnsignedField(RegMap+0x0AA, 5,0, pReg->bThSwitchLNA.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0AB, 4,0, pReg->bStepLNA.w5);
    wiUWORD_SetUnsignedField(RegMap+0x0AC, 5,0, pReg->bThSwitchLNA2.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0AD, 4,0, pReg->bStepLNA2.w5);
    wiUWORD_SetUnsignedField(RegMap+0x0AE, 4,0, pReg->bPwr100dBm.w5);
    wiUWORD_SetUnsignedField(RegMap+0x0AF, 3,0, pReg->DAmpGainRange.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0B0, 3,0, pReg->bMaxUpdateCount.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0B1, 5,4, pReg->CSMode.w2);
    wiUWORD_SetUnsignedField(RegMap+0x0B1, 3,3, pReg->bPwrStepDet.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0B1, 2,2, pReg->bCFOQual.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0B1, 1,1, pReg->bAGCBounded4.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0B1, 0,0, pReg->bLgSigAFEValid.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0B2, 7,7, pReg->bSwDEn.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0B2, 6,6, pReg->bSwDSave.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0B2, 5,5, pReg->bSwDInit.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0B2, 4,0, pReg->bSwDDelay.w5);
    wiUWORD_SetUnsignedField(RegMap+0x0B3, 7,0, pReg->bSwDTh.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0B5, 7,0, pReg->bThStepUpRefPwr.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0B6, 7,4, pReg->bThStepUp.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0B6, 3,0, pReg->bThStepDown.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0B7, 7,0, pReg->bRestartDelay.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0B8, 5,0, pReg->SymHDRCE.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0B9, 6,4, pReg->CENSym1.w3);
    wiUWORD_SetUnsignedField(RegMap+0x0B9, 2,0, pReg->CENSym2.w3);
    wiUWORD_SetUnsignedField(RegMap+0x0BA, 6,4, pReg->hThreshold1.w3);
    wiUWORD_SetUnsignedField(RegMap+0x0BA, 2,0, pReg->hThreshold2.w3);
    wiUWORD_SetUnsignedField(RegMap+0x0BB, 2,0, pReg->hThreshold3.w3);
    wiUWORD_SetUnsignedField(RegMap+0x0BC, 7,0, pReg->SFDWindow.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0C0, 7,7, pReg->AntSel.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C0, 6,6, pReg->DW_PHYEnB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C0, 5,0, pReg->TopState.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0C1, 7,0, pReg->CFO.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0C2, 7,0, pReg->RxFaultCount.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0C3, 7,0, pReg->RestartCount.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0C4, 7,0, pReg->NuisanceCount.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0C5, 7,7, pReg->sPHYEnB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C5, 6,6, pReg->PHYEnB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C5, 5,5, pReg->NoStepUpAgg.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C5, 4,4, pReg->RSSIdB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C5, 3,3, pReg->UnsupportedGF.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C5, 2,2, pReg->ClrOnWake.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C5, 1,1, pReg->ClrOnHdr.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C5, 0,0, pReg->ClrNow.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C6, 7,7, pReg->Filter64QAM.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C6, 6,6, pReg->Filter16QAM.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C6, 5,5, pReg->FilterQPSK.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C6, 4,4, pReg->FilterBPSK.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C6, 2,2, pReg->FilterUndecoded.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C6, 1,1, pReg->FilterAllTypes.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C6, 0,0, pReg->AddrFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0C7, 7,0, pReg->STA0.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0C8, 7,0, pReg->STA1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0C9, 7,0, pReg->STA2.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0CA, 7,0, pReg->STA3.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0CB, 7,0, pReg->STA4.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0CC, 7,0, pReg->STA5.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0CD, 7,0, pReg->minRSSI.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0CE, 7,7, pReg->StepUpRestart.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0CE, 6,6, pReg->StepDownRestart.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0CE, 5,5, pReg->NoStepAfterSync.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0CE, 4,4, pReg->NoStepAfterHdr.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0CE, 3,3, pReg->RxRIFS.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0CE, 2,2, pReg->ForceRestart.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0CE, 1,1, pReg->KeepCCA_New.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0CE, 0,0, pReg->KeepCCA_Lost.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0CF, 6,0, pReg->PktTimePhase80.w7);
    wiUWORD_SetUnsignedField(RegMap+0x0D0, 7,0, pReg->WakeupTimeH.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0D1, 7,0, pReg->WakeupTimeL.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0D2, 7,0, pReg->DelayStdby.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0D3, 7,0, pReg->DelayBGEnB.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0D4, 7,0, pReg->DelayADC1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0D5, 7,0, pReg->DelayADC2.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0D6, 7,0, pReg->DelayModem.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0D7, 7,0, pReg->DelayDFE.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0D8, 7,4, pReg->DelayRFSW.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0D8, 3,0, pReg->TxRxTime.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0D9, 7,4, pReg->DelayFCAL1.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0D9, 3,0, pReg->DelayPA.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0DA, 7,4, pReg->DelayFCAL2.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0DA, 3,0, pReg->DelayState51.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0DB, 7,4, pReg->DelayShutdown.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0DB, 3,0, pReg->DelaySleep.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0DC, 7,0, pReg->TimeExtend.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0DD, 5,0, pReg->SquelchTime.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0DE, 7,4, pReg->DelaySquelchRF.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0DE, 3,0, pReg->DelayClearDCX.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0E0, 7,0, pReg->RadioMC.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0E1, 5,0, pReg->DACOPM.w6);
    wiUWORD_SetUnsignedField(RegMap+0x0E2, 7,0, pReg->ADCAOPM.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0E3, 7,0, pReg->ADCBOPM.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0E4, 7,0, pReg->RFSW1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0E5, 7,0, pReg->RFSW2.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0E6, 7,6, pReg->LNAEnB.w2);
    wiUWORD_SetUnsignedField(RegMap+0x0E6, 5,4, pReg->ADCBCTRLR.w2);
    wiUWORD_SetUnsignedField(RegMap+0x0E6, 2,0, pReg->ADCBCTRL.w3);
    wiUWORD_SetUnsignedField(RegMap+0x0E7, 7,4, pReg->sPA50.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0E7, 3,0, pReg->sPA24.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0E8, 7,6, pReg->ADCFSCTRL.w2);
    wiUWORD_SetUnsignedField(RegMap+0x0E8, 5,5, pReg->ADCCHSEL.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0E8, 4,4, pReg->ADCOE.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0E8, 3,3, pReg->LNAxxBOnSW2.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0E8, 2,2, pReg->ADCOFCEN.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0E8, 1,1, pReg->ADCDCREN.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0E8, 0,0, pReg->ADCMUX0.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0E9, 7,7, pReg->ADCARDY.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0E9, 6,6, pReg->ADCBRDY.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0E9, 4,3, pReg->PPSEL_MODE.w2);
    wiUWORD_SetUnsignedField(RegMap+0x0E9, 2,2, pReg->ADCFCAL2.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0E9, 1,1, pReg->ADCFCAL1.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0E9, 0,0, pReg->ADCFCAL.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0EA, 7,6, pReg->VCMEN.w2);
    wiUWORD_SetUnsignedField(RegMap+0x0EA, 2,0, pReg->DACIFS_CTRL.w3);
    wiUWORD_SetUnsignedField(RegMap+0x0EB, 7,0, pReg->DACFG_CTRL0.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0EC, 7,0, pReg->DACFG_CTRL1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0ED, 7,7, pReg->RESETB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0ED, 6,6, pReg->TxLNAGAIN2.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0ED, 5,5, pReg->LgSigSelDPD.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0ED, 2,2, pReg->SREdge.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0ED, 1,0, pReg->SRFreq.w2);
    wiUWORD_SetUnsignedField(RegMap+0x0EE, 7,4, pReg->sLNA24A.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0EE, 3,0, pReg->sLNA24B.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0EF, 7,4, pReg->sLNA50A.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0EF, 3,0, pReg->sLNA50B.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0F0, 7,0, pReg->SelectCCA.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0F1, 7,0, pReg->ThCCA_SigDet.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0F2, 7,0, pReg->ThCCA_RSSI1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0F3, 7,0, pReg->ThCCA_RSSI2.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0F4, 7,0, pReg->ThCCA_GF1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0F5, 7,0, pReg->ThCCA_GF2.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0F6, 7,0, pReg->ExtendBusyGF.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0F7, 7,0, pReg->ExtendRxError.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0F8, 7,0, pReg->ExtendRxError1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x200, 7,0,(pReg->P1_coef_t0_r_s0.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x201, 4,0,(pReg->P1_coef_t0_r_s0.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x202, 7,0,(pReg->P1_coef_t0_i_s0.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x203, 4,0,(pReg->P1_coef_t0_i_s0.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x204, 7,0,(pReg->P1_coef_t1_r_s0.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x205, 4,0,(pReg->P1_coef_t1_r_s0.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x206, 7,0,(pReg->P1_coef_t1_i_s0.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x207, 4,0,(pReg->P1_coef_t1_i_s0.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x208, 7,0,(pReg->P1_coef_t2_r_s0.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x209, 4,0,(pReg->P1_coef_t2_r_s0.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x20A, 7,0,(pReg->P1_coef_t2_i_s0.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x20B, 4,0,(pReg->P1_coef_t2_i_s0.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x20C, 7,0,(pReg->P1_coef_t3_r_s0.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x20D, 4,0,(pReg->P1_coef_t3_r_s0.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x20E, 7,0,(pReg->P1_coef_t3_i_s0.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x20F, 4,0,(pReg->P1_coef_t3_i_s0.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x210, 7,0,(pReg->P1_coef_t4_r_s0.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x211, 4,0,(pReg->P1_coef_t4_r_s0.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x212, 7,0,(pReg->P1_coef_t4_i_s0.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x213, 4,0,(pReg->P1_coef_t4_i_s0.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x214, 7,0,(pReg->P3_coef_t0_r_s0.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x215, 6,0,(pReg->P3_coef_t0_r_s0.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x216, 7,0,(pReg->P3_coef_t0_i_s0.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x217, 6,0,(pReg->P3_coef_t0_i_s0.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x218, 7,0,(pReg->P3_coef_t1_r_s0.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x219, 6,0,(pReg->P3_coef_t1_r_s0.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x21A, 7,0,(pReg->P3_coef_t1_i_s0.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x21B, 6,0,(pReg->P3_coef_t1_i_s0.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x21C, 7,0,(pReg->P3_coef_t2_r_s0.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x21D, 6,0,(pReg->P3_coef_t2_r_s0.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x21E, 7,0,(pReg->P3_coef_t2_i_s0.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x21F, 6,0,(pReg->P3_coef_t2_i_s0.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x220, 7,0,(pReg->P5_coef_t0_r_s0.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x221, 3,0,(pReg->P5_coef_t0_r_s0.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x222, 7,0,(pReg->P5_coef_t0_i_s0.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x223, 3,0,(pReg->P5_coef_t0_i_s0.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x224, 7,0,(pReg->P5_coef_t1_r_s0.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x225, 3,0,(pReg->P5_coef_t1_r_s0.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x226, 7,0,(pReg->P5_coef_t1_i_s0.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x227, 3,0,(pReg->P5_coef_t1_i_s0.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x228, 7,0,(pReg->P5_coef_t2_r_s0.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x229, 3,0,(pReg->P5_coef_t2_r_s0.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x22A, 7,0,(pReg->P5_coef_t2_i_s0.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x22B, 3,0,(pReg->P5_coef_t2_i_s0.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x22C, 7,0,(pReg->P1_coef_t0_r_s1.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x22D, 4,0,(pReg->P1_coef_t0_r_s1.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x22E, 7,0,(pReg->P1_coef_t0_i_s1.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x22F, 4,0,(pReg->P1_coef_t0_i_s1.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x230, 7,0,(pReg->P1_coef_t1_r_s1.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x231, 4,0,(pReg->P1_coef_t1_r_s1.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x232, 7,0,(pReg->P1_coef_t1_i_s1.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x233, 4,0,(pReg->P1_coef_t1_i_s1.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x234, 7,0,(pReg->P1_coef_t2_r_s1.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x235, 4,0,(pReg->P1_coef_t2_r_s1.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x236, 7,0,(pReg->P1_coef_t2_i_s1.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x237, 4,0,(pReg->P1_coef_t2_i_s1.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x238, 7,0,(pReg->P1_coef_t3_r_s1.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x239, 4,0,(pReg->P1_coef_t3_r_s1.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x23A, 7,0,(pReg->P1_coef_t3_i_s1.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x23B, 4,0,(pReg->P1_coef_t3_i_s1.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x23C, 7,0,(pReg->P1_coef_t4_r_s1.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x23D, 4,0,(pReg->P1_coef_t4_r_s1.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x23E, 7,0,(pReg->P1_coef_t4_i_s1.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x23F, 4,0,(pReg->P1_coef_t4_i_s1.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x240, 7,0,(pReg->P3_coef_t0_r_s1.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x241, 6,0,(pReg->P3_coef_t0_r_s1.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x242, 7,0,(pReg->P3_coef_t0_i_s1.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x243, 6,0,(pReg->P3_coef_t0_i_s1.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x244, 7,0,(pReg->P3_coef_t1_r_s1.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x245, 6,0,(pReg->P3_coef_t1_r_s1.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x246, 7,0,(pReg->P3_coef_t1_i_s1.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x247, 6,0,(pReg->P3_coef_t1_i_s1.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x248, 7,0,(pReg->P3_coef_t2_r_s1.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x249, 6,0,(pReg->P3_coef_t2_r_s1.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x24A, 7,0,(pReg->P3_coef_t2_i_s1.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x24B, 6,0,(pReg->P3_coef_t2_i_s1.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x24C, 7,0,(pReg->P5_coef_t0_r_s1.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x24D, 3,0,(pReg->P5_coef_t0_r_s1.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x24E, 7,0,(pReg->P5_coef_t0_i_s1.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x24F, 3,0,(pReg->P5_coef_t0_i_s1.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x250, 7,0,(pReg->P5_coef_t1_r_s1.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x251, 3,0,(pReg->P5_coef_t1_r_s1.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x252, 7,0,(pReg->P5_coef_t1_i_s1.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x253, 3,0,(pReg->P5_coef_t1_i_s1.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x254, 7,0,(pReg->P5_coef_t2_r_s1.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x255, 3,0,(pReg->P5_coef_t2_r_s1.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x256, 7,0,(pReg->P5_coef_t2_i_s1.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x257, 3,0,(pReg->P5_coef_t2_i_s1.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x258, 7,0,(pReg->P1_coef_t0_r_s2.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x259, 4,0,(pReg->P1_coef_t0_r_s2.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x25A, 7,0,(pReg->P1_coef_t0_i_s2.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x25B, 4,0,(pReg->P1_coef_t0_i_s2.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x25C, 7,0,(pReg->P1_coef_t1_r_s2.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x25D, 4,0,(pReg->P1_coef_t1_r_s2.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x25E, 7,0,(pReg->P1_coef_t1_i_s2.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x25F, 4,0,(pReg->P1_coef_t1_i_s2.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x260, 7,0,(pReg->P1_coef_t2_r_s2.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x261, 4,0,(pReg->P1_coef_t2_r_s2.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x262, 7,0,(pReg->P1_coef_t2_i_s2.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x263, 4,0,(pReg->P1_coef_t2_i_s2.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x264, 7,0,(pReg->P1_coef_t3_r_s2.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x265, 4,0,(pReg->P1_coef_t3_r_s2.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x266, 7,0,(pReg->P1_coef_t3_i_s2.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x267, 4,0,(pReg->P1_coef_t3_i_s2.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x268, 7,0,(pReg->P1_coef_t4_r_s2.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x269, 4,0,(pReg->P1_coef_t4_r_s2.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x26A, 7,0,(pReg->P1_coef_t4_i_s2.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x26B, 4,0,(pReg->P1_coef_t4_i_s2.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x26C, 7,0,(pReg->P3_coef_t0_r_s2.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x26D, 6,0,(pReg->P3_coef_t0_r_s2.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x26E, 7,0,(pReg->P3_coef_t0_i_s2.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x26F, 6,0,(pReg->P3_coef_t0_i_s2.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x270, 7,0,(pReg->P3_coef_t1_r_s2.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x271, 6,0,(pReg->P3_coef_t1_r_s2.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x272, 7,0,(pReg->P3_coef_t1_i_s2.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x273, 6,0,(pReg->P3_coef_t1_i_s2.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x274, 7,0,(pReg->P3_coef_t2_r_s2.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x275, 6,0,(pReg->P3_coef_t2_r_s2.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x276, 7,0,(pReg->P3_coef_t2_i_s2.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x277, 6,0,(pReg->P3_coef_t2_i_s2.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x278, 7,0,(pReg->P5_coef_t0_r_s2.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x279, 3,0,(pReg->P5_coef_t0_r_s2.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x27A, 7,0,(pReg->P5_coef_t0_i_s2.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x27B, 3,0,(pReg->P5_coef_t0_i_s2.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x27C, 7,0,(pReg->P5_coef_t1_r_s2.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x27D, 3,0,(pReg->P5_coef_t1_r_s2.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x27E, 7,0,(pReg->P5_coef_t1_i_s2.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x27F, 3,0,(pReg->P5_coef_t1_i_s2.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x280, 7,0,(pReg->P5_coef_t2_r_s2.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x281, 3,0,(pReg->P5_coef_t2_r_s2.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x282, 7,0,(pReg->P5_coef_t2_i_s2.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x283, 3,0,(pReg->P5_coef_t2_i_s2.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x284, 7,0,(pReg->P1_coef_t0_r_s3.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x285, 4,0,(pReg->P1_coef_t0_r_s3.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x286, 7,0,(pReg->P1_coef_t0_i_s3.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x287, 4,0,(pReg->P1_coef_t0_i_s3.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x288, 7,0,(pReg->P1_coef_t1_r_s3.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x289, 4,0,(pReg->P1_coef_t1_r_s3.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x28A, 7,0,(pReg->P1_coef_t1_i_s3.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x28B, 4,0,(pReg->P1_coef_t1_i_s3.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x28C, 7,0,(pReg->P1_coef_t2_r_s3.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x28D, 4,0,(pReg->P1_coef_t2_r_s3.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x28E, 7,0,(pReg->P1_coef_t2_i_s3.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x28F, 4,0,(pReg->P1_coef_t2_i_s3.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x290, 7,0,(pReg->P1_coef_t3_r_s3.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x291, 4,0,(pReg->P1_coef_t3_r_s3.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x292, 7,0,(pReg->P1_coef_t3_i_s3.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x293, 4,0,(pReg->P1_coef_t3_i_s3.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x294, 7,0,(pReg->P1_coef_t4_r_s3.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x295, 4,0,(pReg->P1_coef_t4_r_s3.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x296, 7,0,(pReg->P1_coef_t4_i_s3.w13>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x297, 4,0,(pReg->P1_coef_t4_i_s3.w13>>8) & 0x1F);
    wiUWORD_SetUnsignedField(RegMap+0x298, 7,0,(pReg->P3_coef_t0_r_s3.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x299, 6,0,(pReg->P3_coef_t0_r_s3.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x29A, 7,0,(pReg->P3_coef_t0_i_s3.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x29B, 6,0,(pReg->P3_coef_t0_i_s3.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x29C, 7,0,(pReg->P3_coef_t1_r_s3.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x29D, 6,0,(pReg->P3_coef_t1_r_s3.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x29E, 7,0,(pReg->P3_coef_t1_i_s3.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x29F, 6,0,(pReg->P3_coef_t1_i_s3.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x2A0, 7,0,(pReg->P3_coef_t2_r_s3.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x2A1, 6,0,(pReg->P3_coef_t2_r_s3.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x2A2, 7,0,(pReg->P3_coef_t2_i_s3.w15>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x2A3, 6,0,(pReg->P3_coef_t2_i_s3.w15>>8) & 0x7F);
    wiUWORD_SetUnsignedField(RegMap+0x2A4, 7,0,(pReg->P5_coef_t0_r_s3.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x2A5, 3,0,(pReg->P5_coef_t0_r_s3.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x2A6, 7,0,(pReg->P5_coef_t0_i_s3.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x2A7, 3,0,(pReg->P5_coef_t0_i_s3.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x2A8, 7,0,(pReg->P5_coef_t1_r_s3.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x2A9, 3,0,(pReg->P5_coef_t1_r_s3.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x2AA, 7,0,(pReg->P5_coef_t1_i_s3.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x2AB, 3,0,(pReg->P5_coef_t1_i_s3.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x2AC, 7,0,(pReg->P5_coef_t2_r_s3.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x2AD, 3,0,(pReg->P5_coef_t2_r_s3.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x2AE, 7,0,(pReg->P5_coef_t2_i_s3.w12>>0) & 0xFF);
    wiUWORD_SetSignedField  (RegMap+0x2AF, 3,0,(pReg->P5_coef_t2_i_s3.w12>>8) & 0x0F);
    wiUWORD_SetUnsignedField(RegMap+0x2B0, 5,0, pReg->cross_pwr1.w6);
    wiUWORD_SetUnsignedField(RegMap+0x2B1, 5,0, pReg->cross_pwr2.w6);
    wiUWORD_SetUnsignedField(RegMap+0x2B2, 5,0, pReg->cross_pwr3.w6);
    wiUWORD_SetUnsignedField(RegMap+0x2B3, 7,0, pReg->Tx_Rate_OFDM_HT.w8);
    wiUWORD_SetUnsignedField(RegMap+0x2B4, 7,0, pReg->Tx_Rate_OFDM_LL.w8);
    wiUWORD_SetUnsignedField(RegMap+0x2B5, 7,0, pReg->Tx_Rate_OFDM_LM.w8);
    wiUWORD_SetUnsignedField(RegMap+0x2B6, 7,5, pReg->DPD_spare.w3);
    wiUWORD_SetUnsignedField(RegMap+0x2B6, 4,4, pReg->DPD_LOW_TMP_DIS.w1);
    wiUWORD_SetUnsignedField(RegMap+0x2B6, 3,3, pReg->DPD_PA_tmp.w1);
    wiUWORD_SetUnsignedField(RegMap+0x2B6, 2,2, pReg->DPD_LOW_RATE_DIS.w1);
    wiUWORD_SetUnsignedField(RegMap+0x2B6, 1,0, pReg->DPD_coef_interspersion.w2);
    wiUWORD_SetUnsignedField(RegMap+0x2B7, 7,6, pReg->Cbc_coef_sel.w2);
    wiUWORD_SetUnsignedField(RegMap+0x2B7, 4,4, pReg->cnfg_eovr.w1);
    wiUWORD_SetUnsignedField(RegMap+0x2B7, 3,2, pReg->Coef_sel_vovr.w2);
    wiUWORD_SetUnsignedField(RegMap+0x2B7, 1,1, pReg->Vfb_byp_vovr.w1);
    wiUWORD_SetUnsignedField(RegMap+0x2B7, 0,0, pReg->Pa_tmp_ext.w1);
    wiUWORD_SetUnsignedField(RegMap+0x300, 7,7, pReg->BISTDone.w1);
    wiUWORD_SetUnsignedField(RegMap+0x300, 6,6, pReg->BISTADCSel.w1);
    wiUWORD_SetUnsignedField(RegMap+0x300, 5,5, pReg->BISTNoPrechg.w1);
    wiUWORD_SetUnsignedField(RegMap+0x300, 4,4, pReg->BISTFGSel.w1);
    wiUWORD_SetUnsignedField(RegMap+0x300, 3,1, pReg->BISTMode.w3);
    wiUWORD_SetUnsignedField(RegMap+0x300, 0,0, pReg->StartBIST.w1);
    wiUWORD_SetUnsignedField(RegMap+0x301, 7,4, pReg->BISTPipe.w4);
    wiUWORD_SetUnsignedField(RegMap+0x301, 3,0, pReg->BISTN_val.w4);
    wiUWORD_SetUnsignedField(RegMap+0x302, 7,0, pReg->BISTOfsR.w8);
    wiUWORD_SetUnsignedField(RegMap+0x303, 7,0, pReg->BISTOfsI.w8);
    wiUWORD_SetUnsignedField(RegMap+0x304, 7,0, pReg->BISTTh1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x305, 7,0,(pReg->BISTTh2.w16>>8) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x306, 7,0,(pReg->BISTTh2.w16>>0) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x307, 7,0, pReg->BISTFG_CTRLI.w8);
    wiUWORD_SetUnsignedField(RegMap+0x308, 7,0, pReg->BISTFG_CTRLR.w8);
    wiUWORD_SetUnsignedField(RegMap+0x309, 7,0, pReg->BISTStatus.w8);
    wiUWORD_SetUnsignedField(RegMap+0x30A, 7,0,(pReg->BISTAccR.w24>>16) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x30B, 7,0,(pReg->BISTAccR.w24>>8) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x30C, 7,0,(pReg->BISTAccR.w24>>0) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x30D, 7,0,(pReg->BISTAccI.w24>>16) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x30E, 7,0,(pReg->BISTAccI.w24>>8) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x30F, 7,0,(pReg->BISTAccI.w24>>0) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x310, 7,4, pReg->BISTRangeLimH.w4);
    wiUWORD_SetUnsignedField(RegMap+0x310, 3,0, pReg->BISTRangeLimL.w4);
    wiUWORD_SetUnsignedField(RegMap+0x320, 1,1, pReg->DFSIntEOB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x320, 0,0, pReg->DFSIntFIFO.w1);
    wiUWORD_SetUnsignedField(RegMap+0x321, 6,0, pReg->DFSThFIFO.w7);
    wiUWORD_SetUnsignedField(RegMap+0x322, 7,0, pReg->DFSDepthFIFO.w8);
    wiUWORD_SetUnsignedField(RegMap+0x323, 7,0, pReg->DFSReadFIFO.w8);
    wiUWORD_SetUnsignedField(RegMap+0x324, 0,0, pReg->DFSSyncFIFO.w1);
    wiUWORD_SetUnsignedField(RegMap+0x328, 7,0, pReg->RxHeader1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x329, 7,0, pReg->RxHeader2.w8);
    wiUWORD_SetUnsignedField(RegMap+0x32A, 7,0, pReg->RxHeader3.w8);
    wiUWORD_SetUnsignedField(RegMap+0x32B, 7,0, pReg->RxHeader4.w8);
    wiUWORD_SetUnsignedField(RegMap+0x32C, 7,0, pReg->RxHeader5.w8);
    wiUWORD_SetUnsignedField(RegMap+0x32D, 7,0, pReg->RxHeader6.w8);
    wiUWORD_SetUnsignedField(RegMap+0x32E, 7,0, pReg->RxHeader7.w8);
    wiUWORD_SetUnsignedField(RegMap+0x32F, 7,0, pReg->RxHeader8.w8);
    wiUWORD_SetUnsignedField(RegMap+0x330, 7,0, pReg->TxHeader1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x331, 7,0, pReg->TxHeader2.w8);
    wiUWORD_SetUnsignedField(RegMap+0x332, 7,0, pReg->TxHeader3.w8);
    wiUWORD_SetUnsignedField(RegMap+0x333, 7,0, pReg->TxHeader4.w8);
    wiUWORD_SetUnsignedField(RegMap+0x334, 7,0, pReg->TxHeader5.w8);
    wiUWORD_SetUnsignedField(RegMap+0x335, 7,0, pReg->TxHeader6.w8);
    wiUWORD_SetUnsignedField(RegMap+0x336, 7,0, pReg->TxHeader7.w8);
    wiUWORD_SetUnsignedField(RegMap+0x337, 7,0, pReg->TxHeader8.w8);
    wiUWORD_SetUnsignedField(RegMap+0x338, 0,0, pReg->TestSigEn.w1);
    wiUWORD_SetUnsignedField(RegMap+0x339, 7,0,(pReg->ADCAI.w10>>2) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x33D, 7,6,(pReg->ADCAI.w10>>0) & 0x03);
    wiUWORD_SetUnsignedField(RegMap+0x33A, 7,0,(pReg->ADCAQ.w10>>2) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x33D, 5,4,(pReg->ADCAQ.w10>>0) & 0x03);
    wiUWORD_SetUnsignedField(RegMap+0x33B, 7,0,(pReg->ADCBI.w10>>2) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x33D, 3,2,(pReg->ADCBI.w10>>0) & 0x03);
    wiUWORD_SetUnsignedField(RegMap+0x33C, 7,0,(pReg->ADCBQ.w10>>2) & 0xFF);
    wiUWORD_SetUnsignedField(RegMap+0x33D, 1,0,(pReg->ADCBQ.w10>>0) & 0x03);
    wiUWORD_SetUnsignedField(RegMap+0x33E, 7,7, pReg->bHeaderValid.w1);
    wiUWORD_SetUnsignedField(RegMap+0x33E, 6,6, pReg->bSyncFound.w1);
    wiUWORD_SetUnsignedField(RegMap+0x33E, 5,5, pReg->bAGCDone.w1);
    wiUWORD_SetUnsignedField(RegMap+0x33E, 4,4, pReg->bSigDet.w1);
    wiUWORD_SetUnsignedField(RegMap+0x33E, 3,3, pReg->aHeaderValid.w1);
    wiUWORD_SetUnsignedField(RegMap+0x33E, 2,2, pReg->aSyncFound.w1);
    wiUWORD_SetUnsignedField(RegMap+0x33E, 1,1, pReg->aAGCDone.w1);
    wiUWORD_SetUnsignedField(RegMap+0x33E, 0,0, pReg->aSigDet.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3F8, 7,4, pReg->SelectSignal2.w4);
    wiUWORD_SetUnsignedField(RegMap+0x3F8, 3,0, pReg->SelectSignal1.w4);
    wiUWORD_SetUnsignedField(RegMap+0x3F9, 3,2, pReg->SelectTrigger2.w2);
    wiUWORD_SetUnsignedField(RegMap+0x3F9, 1,0, pReg->SelectTrigger1.w2);
    wiUWORD_SetUnsignedField(RegMap+0x3FA, 5,0, pReg->TriggerState1.w6);
    wiUWORD_SetUnsignedField(RegMap+0x3FB, 5,0, pReg->TriggerState2.w6);
    wiUWORD_SetUnsignedField(RegMap+0x3FC, 7,0, pReg->TriggerRSSI.w8);
    wiUWORD_SetUnsignedField(RegMap+0x3FD, 7,7, pReg->LgSigAFE.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FD, 3,3, pReg->Trigger2.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FD, 2,2, pReg->Trigger1.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FD, 1,1, pReg->Signal2.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FD, 0,0, pReg->Signal1.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FE, 7,7, pReg->ClearInt.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FE, 6,6, pReg->IntEvent2.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FE, 5,5, pReg->IntEvent1.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FE, 4,4, pReg->IntRxFault.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FE, 3,3, pReg->IntRxAbortTx.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FE, 2,2, pReg->IntTxSleep.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FE, 1,1, pReg->IntTxAbort.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FE, 0,0, pReg->IntTxFault.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FF, 7,7, pReg->Interrupt.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FF, 6,6, pReg->Event2.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FF, 5,5, pReg->Event1.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FF, 4,4, pReg->RxFault.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FF, 3,3, pReg->RxAbortTx.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FF, 2,2, pReg->TxSleep.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FF, 1,1, pReg->TxAbort.w1);
    wiUWORD_SetUnsignedField(RegMap+0x3FF, 0,0, pReg->TxFault.w1);
    // ----------------------------------------------------------------------------------
    //  END: Matlab Generated Code

    return WI_SUCCESS;
}
// end of Verona_WriteRegisterMap()

// ================================================================================================
// FUNCTION  : Verona_ReadRegister()
// ------------------------------------------------------------------------------------------------
// Purpose   : Update and return the internal register map
// Parameters: pRegMap -- pointer (by reference) to the register map
// ================================================================================================
wiUInt Verona_ReadRegister(wiUInt Addr)
{
    if (Addr < 1024)
    {
        STATUS( Verona_WriteRegisterMap(RegMap) );      // copy parameters to register map
        return (wiUInt)(RegMap[Addr].w8);
    }
    else return 0; // address not valid
}
// end of Verona_ReadRegister()

// ================================================================================================
// FUNCTION  : Verona_WriteRegister()
// ------------------------------------------------------------------------------------------------
// Purpose   : Write a register by address
// ================================================================================================
wiStatus Verona_WriteRegister(wiUInt Addr, wiUInt Data)
{
    if (wiProcess_ThreadIndex() != 0)  return STATUS(WI_ERROR_INVALID_FOR_THREAD);
    if (Addr > 0x3FF) return STATUS(WI_ERROR_PARAMETER1);
    if (Data > 0xFF)  return STATUS(WI_ERROR_PARAMETER2);

    if (Addr > 1) // registers 0 and 1 are read-only
    {
        XSTATUS(Verona_WriteRegisterMap(RegMap)); // copy parameters to register map
        RegMap[Addr].word = Data;                 // write data to register map
        XSTATUS(Verona_ReadRegisterMap(RegMap));  // load parameters from register map
    }
    return WI_SUCCESS;
}
// end of Verona_WriteRegister()

// ================================================================================================
// FUNCTION  : Verona_LoadRegisters()
// ------------------------------------------------------------------------------------------------
// Purpose   : Save a register map to a file
// Parameters: RegMap   -- pointer to the register map to which to parameters are written
//             Filename -- name of the file to write
// ================================================================================================
wiStatus Verona_LoadRegisters(wiUWORD *RegMap, wiString Filename)
{
    char buf[256], *cptr, *endptr;
    int addr, data;

    FILE *F = wiParse_OpenFile(Filename, "rt");
    if (!F) return STATUS(WI_ERROR_FILE_OPEN);

    if (wiProcess_ThreadIndex() != 0) return STATUS(WI_ERROR_INVALID_FOR_THREAD);

    while (!feof(F))
    {
        if (feof(F)) continue;

        fgets(buf, 255, F);
        if (strstr(buf,"//")) *strstr(buf,"//")='\0'; // remove trailing comments
        cptr = buf;
        while (*cptr==' ' || *cptr=='\t' || *cptr=='\r' || *cptr=='\n') cptr++;
        if (!*cptr) continue;
        cptr = strstr(buf,"Reg[");
        if (!cptr) {fclose(F); return STATUS(WI_ERROR);}
        cptr+=4;
        while (*cptr==' ') cptr++;
        addr = strtoul(cptr, &endptr, 10);
        while (*endptr==' ') endptr++;
        if (strstr(buf,"]=8'd") != endptr) {fclose(F); return STATUS(WI_ERROR);}
        cptr = endptr + 5;
        data = strtoul(cptr, &endptr, 10);
        if (*endptr==';') endptr++; // allow semicolon at end of assignment
        while (*endptr==' ' || *endptr=='\t' || *endptr=='\r' || *endptr=='\n') endptr++;
        if (*endptr != '\0') {fclose(F); return STATUS(WI_ERROR);}
        if (addr>1023) {fclose(F); return STATUS(WI_ERROR);}
        RegMap[addr].word = data;
    }
    fclose(F); 
    return WI_SUCCESS;
}
// end of Verona_LoadRegisters()

// ================================================================================================
// FUNCTION  : Verona_SaveRegisters()
// ------------------------------------------------------------------------------------------------
// Purpose   : Save a register map to a file
// Parameters: RegMap   -- pointer to the register map to which to parameters are written
//             Filename -- name of the file to write
// ================================================================================================
wiStatus Verona_SaveRegisters(wiUWORD *RegMap, wiString Filename)
{
    int i; time_t t;
    FILE *F;

    F = fopen(Filename, "wt"); time(&t);
    if (!F) return STATUS(WI_ERROR_FILE_OPEN);

    fprintf(F,"// ----------------------------------------------------------------------------\n");
    fprintf(F,"// Settings from WiSE::Verona: %s",ctime(&t));
    fprintf(F,"// ----------------------------------------------------------------------------\n");
    for (i=  0; i< 256; i++) fprintf(F,"Reg[%4d]=8'd%-3u //Reg[0x%03X]=0x%02X\n",i,RegMap[i].w8,i,RegMap[i].w8);
    for (i=512; i<1023; i++) fprintf(F,"Reg[%4d]=8'd%-3u //Reg[0x%03X]=0x%02X\n",i,RegMap[i].w8,i,RegMap[i].w8);
    fclose(F);
    return WI_SUCCESS;
}
// end of Verona_SaveRegisters()

// ================================================================================================
// FUNCTION  : Verona_GetRegisterMap()
// ------------------------------------------------------------------------------------------------
// Purpose   : Update and return the internal register map
// Parameters: pRegMap -- pointer (by reference) to the register map
// ================================================================================================
wiStatus Verona_GetRegisterMap(wiUWORD **pRegMap)
{
    XSTATUS( Verona_WriteRegisterMap(RegMap) );
    *pRegMap = RegMap;
    return WI_SUCCESS;
}
// end of Verona_GetRegisterMap()

// ================================================================================================
// FUNCTION  : Verona_SetRegisterMap()
// ------------------------------------------------------------------------------------------------
// Purpose   : Set the contents of the internal register map and update parameters
// Parameters: RegData -- array of register data values
//             NumRegs -- number of registers to copy       
// ================================================================================================
wiStatus Verona_SetRegisterMap(wiUInt RegData[], unsigned int NumRegs)
{
    unsigned int i;
    for (i=0; i<NumRegs; i++) 
        RegMap[i].word = RegData[i];

    XSTATUS( Verona_ReadRegisterMap(RegMap) );
    return WI_SUCCESS;
}
// end of Verona_SetRegisterMap()

// ================================================================================================
// FUNCTION : Verona_Init()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_Init(void)
{
	pLookupTable = (Verona_LookupTable_t *)wiMalloc(sizeof(Verona_LookupTable_t));
	if (!pLookupTable) return STATUS(WI_ERROR_MEMORY_ALLOCATION);	

    XSTATUS( Verona_TX_Init() );
/*
    XSTATUS( Verona_RX_Init() );
    */
    XSTATUS( Verona_b_Init()  );
    XSTATUS( Verona_TX_DPD_Init() );
    XSTATUS( Verona_RESET()   ); // power-on reset

    return WI_SUCCESS;
}
// end of Verona_Init()

// ================================================================================================
// FUNCTION : Verona_Close()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_Close(void)
{
    unsigned int i, n;

    for (n=0; n<WISE_MAX_THREADS; n++)
    {
        XSTATUS( Verona_TX_FreeMemory ( TX+n, 0)  );
/*
        XSTATUS( Verona_RX_FreeMemory ( RX+n, 0)  );
*/
        XSTATUS( Verona_bRX_FreeMemory(bRX+n, 0) );
        for (i=0; i<VERONA_MAX_DV_TX; i++) 
        {
            WIFREE( DV[n].MAC_TX_D[i] )
        }
    }

    if (wiProcess_ThreadIndex() == 0)
    {
        XSTATUS( Verona_TX_DPD_Close() );
	    WIFREE( pLookupTable );
    }
    Verona_RxState()->Nr = 0;

    return WI_SUCCESS;
}
// end of Verona_Close()

// ================================================================================================
// FUNCTION : Verona_DvCopyTxData()
// ------------------------------------------------------------------------------------------------
// Purpose  : Copy the current PHY_TX_D array to pDV->PHY_TX_RD[n]
// ================================================================================================
wiStatus Verona_DvCopyTxData(wiUInt n)
{
    Verona_TxState_t *pTX = Verona_TxState();
    Verona_DvState_t *pDV = Verona_DvState();

    if (n > VERONA_MAX_DV_TX) return WI_ERROR_PARAMETER1;

    pDV->N_PHY_TX_RD[n] = pTX->LENGTH.w16 + 8;
    WIFREE( pDV->MAC_TX_D[n] );

    pDV->MAC_TX_D[n] = (wiUWORD *)wiCalloc(pDV->N_PHY_TX_RD[n], sizeof(wiUWORD));
    if (!pDV->MAC_TX_D[n]) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    memcpy(pDV->MAC_TX_D[n], pTX->MAC_TX_D, pDV->N_PHY_TX_RD[n] * sizeof(wiUWORD));

    return WI_SUCCESS;
}
// end of Verona_DvCopyTxData()

// ================================================================================================
// FUNCTION : Verona_DvSetTxDataByte()
// ------------------------------------------------------------------------------------------------
// Purpose  : Set a specific byte values in the pDV->PHY_TX_RD[n] array. Generally used to create
//            an intentially bad header value.
// ================================================================================================
wiStatus Verona_DvSetTxDataByte(wiUInt n, wiUInt i, wiUInt X)
{
    Verona_DvState_t *pDV = Verona_DvState();

    if (n > VERONA_MAX_DV_TX)    return WI_ERROR_PARAMETER1;
    if (i > pDV->N_PHY_TX_RD[n]) return WI_ERROR_PARAMETER2;
    if (X > 255)                 return WI_ERROR_PARAMETER3;

    pDV->MAC_TX_D[n][i].word = X;

    return WI_SUCCESS;
}
// end of Verona_DvSetTxDataByte()

// ================================================================================================
// FUNCTION : Verona_DvCopyTxHeader()
// ------------------------------------------------------------------------------------------------
// Purpose  : Copy the PHY header from pDV->PHY_TX_RD[n] to the TxHeader registers
// ================================================================================================
wiStatus Verona_DvCopyTxHeader(wiUInt n)
{
    Verona_DvState_t  *pDV  = Verona_DvState();
    VeronaRegisters_t *pReg = Verona_Registers();

    if (n > VERONA_MAX_DV_TX)     return WI_ERROR_PARAMETER1;
    if (pDV->MAC_TX_D[n] == NULL) return WI_ERROR_PARAMETER1;
    if (pDV->N_PHY_TX_RD[n] < 8)  return WI_ERROR_PARAMETER1;

    pReg->TxHeader1.word = pDV->MAC_TX_D[n][0].w8;
    pReg->TxHeader2.word = pDV->MAC_TX_D[n][1].w8;
    pReg->TxHeader3.word = pDV->MAC_TX_D[n][2].w8;
    pReg->TxHeader4.word = pDV->MAC_TX_D[n][3].w8;
    pReg->TxHeader5.word = pDV->MAC_TX_D[n][4].w8;
    pReg->TxHeader6.word = pDV->MAC_TX_D[n][5].w8;
    pReg->TxHeader7.word = pDV->MAC_TX_D[n][6].w8;
    pReg->TxHeader8.word = pDV->MAC_TX_D[n][7].w8;

    return WI_SUCCESS;
}
// end of Verona_DvCopyTxHeader()

// ================================================================================================
// FUNCTION : Verona_RESET()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_RESET(void)
{
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Default Register Values
    //
    //  The array below contains values for the reset state of baseband programmable
    //  registers. This array is typically generated from the Excel file containing the
    //  register map.
    //  
    wiUInt DefaultRegisters[1024] = {
          0,  8,  0,  3,  1,  0,  3,  0, 20, 20,  1,  0,  0,  0,127,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        255,  5,255,255, 38,248,  3,  0, 17,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          6,121,  4,183, 80,  8,  0,  0,  0,  0,  0,  0,235, 63, 32, 30, 60, 37,  0, 56, 72, 68, 20, 12,
          8, 13, 14, 63,  0,  9, 10,  0,114, 51, 72, 30, 18,  0,  0, 61, 36,  4, 70,195,128, 25,  0,  0,
         12, 15,144,  4, 20, 28, 60,176, 20, 28,172,  0,  0,  0,  0,  0,  4,  0, 17,162,  0,  0,  0,  0,
        132, 76, 16,  0,  0,  0,  0,  0,  4,  0,  0,  0, 21, 31, 21, 31,  2,  0,  0,  0,  2,  0,  2,  0,
        194,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 66, 35, 21, 26, 22, 50, 32, 37,
         40, 10, 28, 14, 63,  0,  8, 12,  7, 31, 90, 45,  0, 32,102, 66, 16, 53, 17,  5,144,  0,  0,  0,
          0,  0,  0,  0,  0,192, 48,255,255,255,255,255,255, 30,128, 64,  0,220, 20,  8,  8,  8,  2,  1,
        103, 67, 67,100, 33, 24,  1,  0, 30, 11,  7,  7,165, 60, 52,  0,215,  6,  0,128,128,133,  0,  0,
        127, 15, 45, 37, 32, 24,200,200,169,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,127,  0,  0,  0
    };
    Verona_TX_FreeMemory(Verona_TxState(), 1);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Set Default Register Values
    //
    XSTATUS( Verona_SetRegisterMap(DefaultRegisters, sizeof(DefaultRegisters)/sizeof(wiUWORD)) );

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Reset Individual Modules (as needed by the 'C' model)
    //  
    {
        wiWORD Zero = {0};
/*
        Verona_DCX(-1, Zero, NULL, 0, Verona_RxState(), Verona_Registers());
*/
    }
    return WI_SUCCESS;
}
// end of Verona_RESET()

// ================================================================================================
// FUNCTION : Verona_DvState
// ------------------------------------------------------------------------------------------------
// ================================================================================================
Verona_DvState_t* Verona_DvState(void)
{
    return DV + wiProcess_ThreadIndex();
}
// end of Verona_DvState()

// ================================================================================================
// FUNCTION : Verona_TxState
// ------------------------------------------------------------------------------------------------
// ================================================================================================
Verona_TxState_t* Verona_TxState(void)
{
    int ThreadIndex = wiProcess_ThreadIndex();
    Verona_TxState_t *pTX = &(TX[ThreadIndex]);

//wiPrintf("Verona_TxState [%d] = @x%08X [@x%08X, @x%08X, @x%08X] @x%08X\n",ThreadIndex,pTX,TX+0,TX+1,TX+2,TX+3);

    return pTX;
}
// end of Verona_TxState()

// ================================================================================================
// FUNCTION : Verona_RxState
// ------------------------------------------------------------------------------------------------
// ================================================================================================
Verona_RxState_t* Verona_RxState(void)
{
    return RX + wiProcess_ThreadIndex();
}
// end of Verona_RxState()

// ================================================================================================
// FUNCTION : Verona_bRxState()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
Verona_bRxState_t * Verona_bRxState(void)
{
    return bRX + wiProcess_ThreadIndex();
}
// end of Verona_bRxState()

// ================================================================================================
// FUNCTION : Verona_LookupTable()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
Verona_LookupTable_t* Verona_LookupTable(void)
{
    return pLookupTable;
}
// end of Verona_LookupTable()

// ================================================================================================
// FUNCTION : Verona_Registers
// ------------------------------------------------------------------------------------------------
// ================================================================================================
VeronaRegisters_t* Verona_Registers(void)
{
    return Register + wiProcess_ThreadIndex();
}
// end of Verona_Registers()

// ================================================================================================
// FUNCTION : Verona_GetArrayByName
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_GetArrayByName(wiString Name, unsigned int n, void **ptr)
{
         if (!strcmp(Name, "TX.uReal"))          *ptr = (void *)Verona_TxState()->uReal;
    else if (!strcmp(Name, "TX.uImag"))          *ptr = (void *)Verona_TxState()->uImag;
    else if (!strcmp(Name, "RX.tReal"))          *ptr = (void *)Verona_RxState()->tReal[n];
    else if (!strcmp(Name, "RX.tImag"))          *ptr = (void *)Verona_RxState()->tImag[n];
    else if (!strcmp(Name, "RX.uReal"))          *ptr = (void *)Verona_RxState()->uReal[n];
    else if (!strcmp(Name, "RX.uImag"))          *ptr = (void *)Verona_RxState()->uImag[n];
    else if (!strcmp(Name, "RX.vReal"))          *ptr = (void *)Verona_RxState()->vReal[n];
    else if (!strcmp(Name, "RX.vImag"))          *ptr = (void *)Verona_RxState()->vImag[n];
    else if (!strcmp(Name, "RX.wReal"))          *ptr = (void *)Verona_RxState()->wReal[n];
    else if (!strcmp(Name, "RX.wImag"))          *ptr = (void *)Verona_RxState()->wImag[n];
    else if (!strcmp(Name, "RX.xReal"))          *ptr = (void *)Verona_RxState()->xReal[n];
    else if (!strcmp(Name, "RX.xImag"))          *ptr = (void *)Verona_RxState()->xImag[n];
    else if (!strcmp(Name, "RX.yReal"))          *ptr = (void *)Verona_RxState()->yReal[n];
    else if (!strcmp(Name, "RX.yImag"))          *ptr = (void *)Verona_RxState()->yImag[n];
    else if (!strcmp(Name, "RX.zReal"))          *ptr = (void *)Verona_RxState()->zReal[n];
    else if (!strcmp(Name, "RX.zImag"))          *ptr = (void *)Verona_RxState()->zImag[n];
    else if (!strcmp(Name, "RX.a"))              *ptr = (void *)Verona_RxState()->a;
    else if (!strcmp(Name, "RX.b"))              *ptr = (void *)Verona_RxState()->b;
    else if (!strcmp(Name, "RX.Lp"))             *ptr = (void *)Verona_RxState()->Lp;
    else if (!strcmp(Name, "RX.Lc"))             *ptr = (void *)Verona_RxState()->Lc;
    else if (!strcmp(Name, "RX.cA"))             *ptr = (void *)Verona_RxState()->cA;
    else if (!strcmp(Name, "RX.cB"))             *ptr = (void *)Verona_RxState()->cB;
    else if (!strcmp(Name, "RX.AGain"))          *ptr = (void *)Verona_RxState()->AGain;
    else if (!strcmp(Name, "RX.DGain"))          *ptr = (void *)Verona_RxState()->DGain;
    else if (!strcmp(Name, "RX.LNAGain"))        *ptr = (void *)Verona_RxState()->LNAGain;
    else if (!strcmp(Name, "RX.DFEState"))       *ptr = (void *)Verona_RxState()->DFEState;
    else if (!strcmp(Name, "RX.DFEFlags"))       *ptr = (void *)Verona_RxState()->DFEFlags;
    else if (!strcmp(Name, "RX.AdvDly"))         *ptr = (void *)Verona_RxState()->AdvDly;
    else if (!strcmp(Name, "RX.aSigOut"))        *ptr = (void *)Verona_RxState()->aSigOut;
    else if (!strcmp(Name, "RX.bSigOut"))        *ptr = (void *)Verona_RxState()->bSigOut;
    else if (!strcmp(Name, "RX.traceRxClock"))   *ptr = (void *)Verona_RxState()->traceRxClock;
    else if (!strcmp(Name, "RX.traceRxState"))   *ptr = (void *)Verona_RxState()->traceRxState;
    else if (!strcmp(Name, "RX.traceRxControl")) *ptr = (void *)Verona_RxState()->traceRxControl;
    else if (!strcmp(Name, "RX.traceDFE"))       *ptr = (void *)Verona_RxState()->traceDFE;
    else if (!strcmp(Name, "RX.traceAGC"))       *ptr = (void *)Verona_RxState()->traceAGC;
    else if (!strcmp(Name, "RX.traceRSSI"))      *ptr = (void *)Verona_RxState()->traceRSSI;
    else if (!strcmp(Name, "RX.traceDFS"))       *ptr = (void *)Verona_RxState()->traceDFS;
    else if (!strcmp(Name, "RX.traceSigDet"))    *ptr = (void *)Verona_RxState()->traceSigDet;
    else if (!strcmp(Name, "RX.traceFrameSync")) *ptr = (void *)Verona_RxState()->traceFrameSync;
    else if (!strcmp(Name, "RX.traceRadioIO"))   *ptr = (void *)Verona_RxState()->traceRadioIO;
    else if (!strcmp(Name, "RX.traceDataConv"))  *ptr = (void *)Verona_RxState()->traceDataConv;
    else if (!strcmp(Name, "RX.traceRX2"))       *ptr = (void *)Verona_RxState()->traceRX2;
    else if (!strcmp(Name, "RX.traceRx"))        *ptr = (void *)Verona_RxState()->traceRx;
    else if (!strcmp(Name, "RX.traceImgDet"))    *ptr = (void *)Verona_RxState()->traceImgDet;
    else if (!strcmp(Name, "RX.traceIQCal"))     *ptr = (void *)Verona_RxState()->traceIQCal;
    else if (!strcmp(Name, "RX.PHY_RX_D"))       *ptr = (void *)Verona_RxState()->PHY_RX_D;
    else if (!strcmp(Name, "bRX.xReal"))         *ptr = (void *)Verona_bRxState()->xReal;
    else if (!strcmp(Name, "bRX.xImag"))         *ptr = (void *)Verona_bRxState()->xImag;
    else if (!strcmp(Name, "bRX.uReal"))         *ptr = (void *)Verona_bRxState()->uReal;
    else if (!strcmp(Name, "bRX.uImag"))         *ptr = (void *)Verona_bRxState()->uImag;
    else if (!strcmp(Name, "bRX.traceCCK"))      *ptr = (void *)Verona_bRxState()->traceCCK;
    else if (!strcmp(Name, "bRX.traceState"))    *ptr = (void *)Verona_bRxState()->traceState;
    else if (!strcmp(Name, "bRX.traceControl"))  *ptr = (void *)Verona_bRxState()->traceControl;
    else if (!strcmp(Name, "bRX.traceFrontReg")) *ptr = (void *)Verona_bRxState()->traceFrontReg;
    else if (!strcmp(Name, "bRX.traceDPLL"))     *ptr = (void *)Verona_bRxState()->traceDPLL;
    else if (!strcmp(Name, "bRX.traceBarker"))   *ptr = (void *)Verona_bRxState()->traceBarker;
    else if (!strcmp(Name, "bRX.traceDSSS"))     *ptr = (void *)Verona_bRxState()->traceDSSS;
    else if (!strcmp(Name, "bRX.traceCCKInput")) *ptr = (void *)Verona_bRxState()->traceCCKInput;
    else if (!strcmp(Name, "bRX.traceCCKTVMF"))  *ptr = (void *)Verona_bRxState()->traceCCKTVMF;
    else if (!strcmp(Name, "bRX.traceCCKFWT"))   *ptr = (void *)Verona_bRxState()->traceCCKFWT;
    else if (!strcmp(Name, "bRX.traceResample")) *ptr = (void *)Verona_bRxState()->traceResample;
    else return WI_ERROR_PARAMETER1;

    return WI_SUCCESS;
}
// end of Verona_GetArrayByName()

// ================================================================================================
// FUNCTION : Verona_InvertArrayBit
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_InvertArrayBit(wiString Name, unsigned int n, unsigned int k, unsigned int m)
{
    void *ptr;
    wiUInt *pU;

    if (n > 1)                               return STATUS(WI_ERROR_PARAMETER2);
    if (k > (unsigned)Verona_RxState()->N80) return STATUS(WI_ERROR_PARAMETER3);
    if (m > 31)                              return STATUS(WI_ERROR_PARAMETER4);

    XSTATUS( Verona_GetArrayByName(Name, n, &ptr) );

    if (ptr == NULL) return STATUS(WI_ERROR);

    pU = (wiUInt *)ptr;
    pU[k] = pU[k] ^ (1 << m); // invert the mth bit
    return WI_SUCCESS;
}
// end of Verona_InvertArrayBit()

// ================================================================================================
// FUNCTION : Verona_SetArrayWord
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_SetArrayWord(wiString Name, unsigned int n, unsigned int k, unsigned int x)
{
    void *ptr;
    wiUInt *pU;

    if (n > 1)                               return STATUS(WI_ERROR_PARAMETER2);
    if (k > (unsigned)Verona_RxState()->N80) return STATUS(WI_ERROR_PARAMETER3);

    XSTATUS( Verona_GetArrayByName(Name, n, &ptr) );

    if (ptr == NULL) return STATUS(WI_ERROR);

    pU = (wiUInt *)ptr;
    pU[k] = x;
    return WI_SUCCESS;
}
// end of Verona_SetArrayWord()

// ================================================================================================
// FUNCTION : Verona_WriteConfig()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_WriteConfig(wiMessageFn MsgFunc)
{
    const char *RXModeStr[] = {"undefined","802.11a (OFDM)","802.11b (DSSS/CCK)","802.11g (DSSS/CCK + OFDM)",
                               "802.11n (OFDM) *** unsupported ***","802.11a/n (OFDM)","undefined","802.11g/n (DSSS/CCK + OFDM)"};

    Verona_TxState_t  *pTX = Verona_TxState();
    Verona_RxState_t  *pRX = Verona_RxState();
    VeronaRegisters_t *p   = Verona_Registers();

    MsgFunc(" REG: PathSel          = %d\n",p->PathSel.w2);
    MsgFunc(" REG: RXMode           = %d, %s\n",p->RXMode.w3,RXModeStr[p->RXMode.w3]);
    MsgFunc(" REG: Transmit         = TX_PRBS   = 0x%02X, TX_SERVICE  = 0x%04X\n",p->TX_PRBS,p->TX_SERVICE);
    MsgFunc(" REG: Packet Filters   : AddrFilter  =%3d, Reserved0   =%3d, maxLENGTH   =%3d\n",p->AddrFilter,p->Reserved0,p->maxLENGTH.w8);
    MsgFunc(" REG: Restart Control  : StepUpRstart=%3d, StepDnRstart=%3d, NoStep>Sync =%3d, NoStep>Hdr  =%3d\n",p->StepUpRestart,p->StepDownRestart,p->NoStepAfterSync,p->NoStepAfterHdr);
    MsgFunc(" REG: RX GainUpdate    : SquelchTime =%3d, DelaySqlchRF=%3d, DelayClrDCX =%3d, RFSW1/SW2 =%02X,%02X\n",p->SquelchTime,p->DelaySquelchRF,p->DelayClearDCX,p->RFSW1,p->RFSW2); 
    MsgFunc(" REG: AGC              : WaitAGC     =%3d, DelayAGC    =%3d, DelayCFO    =%3d, MaxUpdateCnt=%3d\n",p->WaitAGC,p->DelayAGC,p->DelayCFO.w5,p->MaxUpdateCount.w3);
    MsgFunc("                       : AbsPwrL     =%3d, AbsPwrH     =%3d, RefPwrDig   =%3d, InitAGain   =%3d\n",p->AbsPwrL,p->AbsPwrH,p->RefPwrDig,p->InitAGain);
    MsgFunc("                       : UpdateOnLNA =%3d, Pwr100dBm   =%3d, LgSigAFEVald=%3d, ArmLgSigDet3=%3d\n",p->UpdateOnLNA,p->Pwr100dBm,p->LgSigAFEValid,p->ArmLgSigDet3);
    MsgFunc("                       : StepSmall   =%3d, StepLarge   =%3d, StepLgSigDet=%3d, DelayRSSI   =%3d\n",p->StepSmall,p->StepLarge,p->StepLgSigDet,p->DelayRSSI);
    MsgFunc("                       : ThSigSmall  =%3d, ThSigLarge  =%3d, ThStepUp    =%3d, ThStepDown  =%3d\n",p->ThSigSmall,p->ThSigLarge,p->ThStepUp,p->ThStepDown);
    MsgFunc("                       : UpdateGainMF=%3d\n",p->UpdateGainMF.b0);
    MsgFunc(" REG: LNA Configuration: StepLNA     =%3d, StepLNA2    =%3d, ThSwitchLNA =%3d, ThSwitchLNA2=%3d\n",p->StepLNA,p->StepLNA2,p->ThSwitchLNA,p->ThSwitchLNA2);
    MsgFunc(" REG: Signal Detect    : SigDetTh1   =%3d, SigDetTh2   =%3d, SigDetFilter=%3d, SigDetWindow=%3d\n",p->SigDetTh1,p->SigDetTh2H.w8<<8|p->SigDetTh2L.w8,p->SigDetFilter,p->SigDetWindow);
    MsgFunc(" REG: Frame Sync       : SyncMag     =%3d, SyncThSig   =%3d, SyncThExp   =%3d, SyncDelay   =%3d\n",p->SyncMag,p->SyncThSig,p->SyncThExp,p->SyncDelay);
    MsgFunc(" REG: Frame Sync       : CFOFilter   =%3d, SyncFilter  =%3d, IdleFSEnB   =%3d, SyncShift   =%3d\n",p->CFOFilter.b0,p->SyncFilter,p->IdleFSEnB,p->SyncShift);
    MsgFunc(" REG: DCX              : DCXSelect   =%3d, DCXShift    =%3d, DCXHoldAcc  =%3d, DCXFastLoad =%3d\n",p->DCXSelect,p->DCXShift.word,p->DCXHoldAcc,p->DCXFastLoad);
    MsgFunc(" REG: Prd/Misc         : DACSrcR     =%3d, DACSrcI     =%3d, MinShift    =%3d, DelayRestart=%3d\n",p->PrdDACSrcR,p->PrdDACSrcI,p->MinShift,p->DelayRestart);
    MsgFunc(" REG: Chanl Estimation : ChTracking  =%3d, ChTrackGain1=%3d, ChTrackGain2=%3d \n",p->ChTracking.b0, p->ChTrackGain1.w3, p->ChTrackGain2.w3);
    MsgFunc(" REG: DFS Radar Detect : DFSOn       =%3d, ThUp=%d, ThDn=%d, minPPB=%d, Pattern=%d\n",p->DFSOn,p->ThDFSUp,p->ThDFSDn,p->DFSminPPB,p->DFSPattern); 
    MsgFunc(" REG: Mixers           : TxUpmix     =%3s%sRxDownmix   =%3s%s\n",
            p->UpmixOff.b0  ? "OFF":p->UpmixDown.b0? "-10":"+10", p->UpmixConj.b0? "*,":", ",
            p->DownmixOff.b0? "OFF":p->DownmixUp.b0? "-10":"+10", p->UpmixConj.b0? "*":"");
    MsgFunc(" REG: PilotTone PLL    : PilotPLLEn  =%3d, aC=%d, bC=%d, cC=%d\n",p->PilotPLLEn.b0,p->aC.w3,p->bC.w3,p->cC.w3);
    MsgFunc(" REG: Digital PLL      : Carrier Tracking Ca=%d, Cb=%d,  Sample Time Tracking Sa=%d, Sb=%d\n",p->Ca,p->Cb,p->Sa,p->Sb);
    MsgFunc(" REG: OFDM SwD         : Enable      =%3d, Init=%d, Save=%d, Th=%3d, Delay=%2d\n",p->OFDMSwDEn,p->OFDMSwDInit,p->OFDMSwDSave,p->OFDMSwDTh,p->OFDMSwDDelay);
    MsgFunc(" REG: DSSS SwD         : Enable      =%3d, Init=%d, Save=%d, Th=%3d, Delay=%2d\n",p->bSwDEn,p->bSwDInit,p->bSwDSave,p->bSwDTh,p->bSwDDelay);
    MsgFunc(" REG: b DPLL           : aC=%d,%d, bC=%d,%d, aS=%d,%d, bS=%d,%d\n",p->aC1, p->aC2, p->bC1, p->bC2, p->aS1, p->aS2, p->bS1, p->bS2);
    MsgFunc(" REG: b FrontCtrl      : INITdelay   =%3d, SFDWindow   =%3d, SymHDRCE    =%3d, SSwindow    =%3d\n",p->INITdelay,p->SFDWindow,p->SymHDRCE,p->SSwindow);
    MsgFunc(" REG: b Carrier Sense  : CQwindow    =%3d, CQThreshold =%3d, CSMode      =%3d, bPwrStepDet =%3d\n",p->CQwindow,p->CQthreshold,p->CSMode,p->bPwrStepDet);
    MsgFunc(" REG: b AGC/ED         : EDwindow    =%3d, EDThreshold =%3d, ThSigLarge  =%3d, bStepLarge  =%3d\n",p->EDwindow,p->EDthreshold,p->bThSigLarge,p->bStepLarge);
    MsgFunc(" REG: b AGC            : bWaitAGC    =%3d, bAGCDelay   =%3d, bMaxUpdateCt=%3d, LgSigAFEVald=%3d\n",p->bWaitAGC, p->bAGCdelay, p->bMaxUpdateCount,p->bLgSigAFEValid);
    MsgFunc("                       : bInitAGain  =%3d, bRefPwr     =%3d, DAmpGainRang=%3d, Pwr100dBm   =%3d\n",p->bInitAGain,p->bRefPwr,p->DAmpGainRange,p->Pwr100dBm);
    MsgFunc(" REG: bLNA Config      : bStepLNA    =%3d, bStepLNA2   =%3d, bThSwitchLNA=%3d, bThSwtchLNA2=%3d\n",p->bStepLNA,p->bStepLNA2,p->bThSwitchLNA,p->bThSwitchLNA2);
    MsgFunc(" REG: bChanEst         : CENSym1,2   =%d,%d, hThreshold1 =%3d, hThreshold2 =%3d, hThreshold3 =%3d\n",p->CENSym1,p->CENSym2,p->hThreshold1,p->hThreshold2,p->hThreshold3);

    MsgFunc(" Clock Phase           : RX.Downmix  =%3d, RX.Downsamp =%3d, RX.ClockGen =%3d, bResamplePhase=%d\n",pRX->DownmixPhase,pRX->DownsamPhase,pRX->clock.InitialPhase,pTX->bResamplePhase);
    MsgFunc(" TX.Pad Front,Back     : aPadFront =%5d, aPadBack    =%3d, bPadFront  =%4d, bPackBack   =%3d\n",pTX->aPadFront,pTX->aPadBack,pTX->bPadFront,pTX->bPadBack);
    MsgFunc(" Data Converters - DAC : Range = %1.1fVpp, nEff = %d, I/Q Mismatch = %3.2f dB\n",pTX->DAC.Vpp,pTX->DAC.nEff,pTX->DAC.IQdB);
    MsgFunc(" Data Converters - ADC : Range = %1.1fVpp, nEff = %d, Offset = %d, Rounding = %d\n",pRX->ADC.Vpp,pRX->ADC.nEff,pRX->ADC.Offset,pRX->ADC.Rounding);
    if (pRX->NonRTL.Enabled)
    {
        MsgFunc(" >>> ===================================================================== <<<\n");
        MsgFunc(" >>> NON-RTL PARAMETERS: MaxSamplesCFO = %2d\n",pRX->NonRTL.MaxSamplesCFO);
        MsgFunc(" >>>                     DisableCFO = %d, BypassPLL = %d\n",pRX->NonRTL.DisableCFO,pRX->NonRTL.DisablePLL);
        MsgFunc(" >>>                     MojaveFrontCtrlEOP = %d\n",pRX->NonRTL.MojaveFrontCtrlEOP);
        MsgFunc(" >>> ===================================================================== <<<\n");
    }
    return WI_SUCCESS;
}
// end of Verona_WriteConfig()

// ================================================================================================
// FUNCTION : Verona_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_ParseLine(wiString text)
{
    if (strstr(text, "Verona"))
    {
        wiStatus status;
        char buf[256]; wiUWORD X, Y, Z;
        int dummy;

        Verona_TxState_t  *pTX = Verona_TxState();
        Verona_RxState_t  *pRX = Verona_RxState();
        Verona_DvState_t  *pDV = Verona_DvState();
        VeronaRegisters_t *pReg = Verona_Registers();

        //  Matlab Generated Code from Verona_GetRegisterMap(16)
        // ------------------------------------------------------------------------------
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ID",              &(pReg->ID.word),               0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.PathSel",         &(pReg->PathSel.word),          0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RXMode",          &(pReg->RXMode.word),           0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TestMode",        &(pReg->TestMode.word),         0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.UpmixConj",       &(pReg->UpmixConj.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DownmixConj",     &(pReg->DownmixConj.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.UpmixDown",       &(pReg->UpmixDown.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DownmixUp",       &(pReg->DownmixUp.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.UpmixOff",        &(pReg->UpmixOff.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DownmixOff",      &(pReg->DownmixOff.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxIQSrcR",        &(pReg->TxIQSrcR.word),         0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxIQSrcI",        &(pReg->TxIQSrcI.word),         0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.PrdDACSrcR",      &(pReg->PrdDACSrcR.word),       0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.PrdDACSrcI",      &(pReg->PrdDACSrcI.word),       0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aTxExtend",       &(pReg->aTxExtend.word),        0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aTxDelay",        &(pReg->aTxDelay.word),         0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bTxExtend",       &(pReg->bTxExtend.word),        0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bTxDelay",        &(pReg->bTxDelay.word),         0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxIQSrcRun",      &(pReg->TxIQSrcRun.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxSmoothing",     &(pReg->TxSmoothing.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TX_SERVICE",      &(pReg->TX_SERVICE.word),       0, 65535));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TX_PRBS",         &(pReg->TX_PRBS.word),          0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RX_SERVICE",      &(pReg->RX_SERVICE.word),       0, 65535));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RSSI0",           &(pReg->RSSI0.word),            0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RSSI1",           &(pReg->RSSI1.word),            0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.maxLENGTH",       &(pReg->maxLENGTH.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.NotSounding",     &(pReg->NotSounding.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.nReserved1",      &(pReg->nReserved1.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Reserved0",       &(pReg->Reserved0.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.maxLENGTHnH",     &(pReg->maxLENGTHnH.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.maxLENGTHnL",     &(pReg->maxLENGTHnL.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aPPDUMaxTimeH",   &(pReg->aPPDUMaxTimeH.word),    0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aPPDUMaxTimeL",   &(pReg->aPPDUMaxTimeL.word),    0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.PilotPLLEn",      &(pReg->PilotPLLEn.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aC",              &(pReg->aC.word),               0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bC",              &(pReg->bC.word),               0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.cC",              &(pReg->cC.word),               0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Sa",              &(pReg->Sa.word),               0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Sb",              &(pReg->Sb.word),               0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Ca",              &(pReg->Ca.word),               0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Cb",              &(pReg->Cb.word),               0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.sTxIQ",           &(pReg->sTxIQ.word),            0, 1));
        PSTATUS(wiParse_Int    (text,"Verona.Register.TxIQa11",         &(pReg->TxIQa11.word),         -16, 15));
        PSTATUS(wiParse_Int    (text,"Verona.Register.TxIQa22",         &(pReg->TxIQa22.word),         -16, 15));
        PSTATUS(wiParse_Int    (text,"Verona.Register.TxIQa12",         &(pReg->TxIQa12.word),         -16, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.sRxIQ",           &(pReg->sRxIQ.word),            0, 3));
        PSTATUS(wiParse_Int    (text,"Verona.Register.RxIQ0a11",        &(pReg->RxIQ0a11.word),        -16, 15));
        PSTATUS(wiParse_Int    (text,"Verona.Register.RxIQ0a22",        &(pReg->RxIQ0a22.word),        -16, 15));
        PSTATUS(wiParse_Int    (text,"Verona.Register.RxIQ0a12",        &(pReg->RxIQ0a12.word),        -16, 15));
        PSTATUS(wiParse_Int    (text,"Verona.Register.RxIQ1a11",        &(pReg->RxIQ1a11.word),        -16, 15));
        PSTATUS(wiParse_Int    (text,"Verona.Register.RxIQ1a22",        &(pReg->RxIQ1a22.word),        -16, 15));
        PSTATUS(wiParse_Int    (text,"Verona.Register.RxIQ1a12",        &(pReg->RxIQ1a12.word),        -16, 15));
        PSTATUS(wiParse_Int    (text,"Verona.Register.ImgDetCoefRH",    &(pReg->ImgDetCoefRH.word),    -8, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetCoefRL",    &(pReg->ImgDetCoefRL.word),     0, 255));
        PSTATUS(wiParse_Int    (text,"Verona.Register.ImgDetCoefIH",    &(pReg->ImgDetCoefIH.word),    -8, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetCoefIL",    &(pReg->ImgDetCoefIL.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetLength",    &(pReg->ImgDetLength.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetEn",        &(pReg->ImgDetEn.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetLengthH",   &(pReg->ImgDetLengthH.word),    0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetPostShift", &(pReg->ImgDetPostShift.word),  0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetPreShift",  &(pReg->ImgDetPreShift.word),   0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetDelay",     &(pReg->ImgDetDelay.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetAvgNum",    &(pReg->ImgDetAvgNum.word),     0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetPowerH",    &(pReg->ImgDetPowerH.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetPowerL",    &(pReg->ImgDetPowerL.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalDone",         &(pReg->CalDone.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalResultType",   &(pReg->CalResultType.word),    0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ImgDetDone",      &(pReg->ImgDetDone.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalRun",          &(pReg->CalRun.word),           0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalPwrAdjust",    &(pReg->CalPwrAdjust.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalMode",         &(pReg->CalMode.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalCoefUpdate",   &(pReg->CalCoefUpdate.word),    0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalFineStep",     &(pReg->CalFineStep.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalCoarseStep",   &(pReg->CalCoarseStep.word),    0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalPwrRatio",     &(pReg->CalPwrRatio.word),      0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalIterationTh",  &(pReg->CalIterationTh.word),   0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalDelay",        &(pReg->CalDelay.word),         0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalGain",         &(pReg->CalGain.word),          0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CalPhase",        &(pReg->CalPhase.word),         0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RefPwrDig",       &(pReg->RefPwrDig.word),        0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.InitAGain",       &(pReg->InitAGain.word),        0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.AbsPwrL",         &(pReg->AbsPwrL.word),          0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.AbsPwrH",         &(pReg->AbsPwrH.word),          0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThSigLarge",      &(pReg->ThSigLarge.word),       0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThSigSmall",      &(pReg->ThSigSmall.word),       0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.StepLgSigDet",    &(pReg->StepLgSigDet.word),     0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.StepLarge",       &(pReg->StepLarge.word),        0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.StepSmall",       &(pReg->StepSmall.word),        0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThSwitchLNA",     &(pReg->ThSwitchLNA.word),      0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.StepLNA",         &(pReg->StepLNA.word),          0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThSwitchLNA2",    &(pReg->ThSwitchLNA2.word),     0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.StepLNA2",        &(pReg->StepLNA2.word),         0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Pwr100dBm",       &(pReg->Pwr100dBm.word),        0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.StepSmallPeak",   &(pReg->StepSmallPeak.word),    0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.FixedGain",       &(pReg->FixedGain.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.InitLNAGain",     &(pReg->InitLNAGain.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.InitLNAGain2",    &(pReg->InitLNAGain2.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.UpdateOnLNA",     &(pReg->UpdateOnLNA.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.MaxUpdateCount",  &(pReg->MaxUpdateCount.word),   0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThStepUpRefPwr",  &(pReg->ThStepUpRefPwr.word),   0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThStepUp",        &(pReg->ThStepUp.word),         0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThStepDown",      &(pReg->ThStepDown.word),       0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThDFSUp",         &(pReg->ThDFSUp.word),          0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThDFSDn",         &(pReg->ThDFSDn.word),          0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThStepDownMsrPwr",&(pReg->ThStepDownMsrPwr.word), 0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSOn",           &(pReg->DFSOn.word),            0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFS2Candidates",  &(pReg->DFS2Candidates.word),   0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSSmooth",       &(pReg->DFSSmooth.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSMACFilter",    &(pReg->DFSMACFilter.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSHdrFilter",    &(pReg->DFSHdrFilter.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSIntDet",       &(pReg->DFSIntDet.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSPattern",      &(pReg->DFSPattern.word),       0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSminPPB",       &(pReg->DFSminPPB.word),        0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSdTPRF",        &(pReg->DFSdTPRF.word),         0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSmaxWidth",     &(pReg->DFSmaxWidth.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSmaxTPRF",      &(pReg->DFSmaxTPRF.word),       0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSminTPRF",      &(pReg->DFSminTPRF.word),       0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSminGap",       &(pReg->DFSminGap.word),        0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSPulseSR",      &(pReg->DFSPulseSR.word),       0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.detRadar",        &(pReg->detRadar.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.detHeader",       &(pReg->detHeader.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.detPreamble",     &(pReg->detPreamble.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.detStepDown",     &(pReg->detStepDown.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.detStepUp",       &(pReg->detStepUp.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.WaitAGC",         &(pReg->WaitAGC.word),          0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayAGC",        &(pReg->DelayAGC.word),         0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SigDetWindow",    &(pReg->SigDetWindow.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SyncShift",       &(pReg->SyncShift.word),        0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayRestart",    &(pReg->DelayRestart.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.OFDMSwDDelay",    &(pReg->OFDMSwDDelay.word),     0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.OFDMSwDTh",       &(pReg->OFDMSwDTh.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CFOFilter",       &(pReg->CFOFilter.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.UpdateGainMF",    &(pReg->UpdateGainMF.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ArmLgSigDet3",    &(pReg->ArmLgSigDet3.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.LgSigAFEValid",   &(pReg->LgSigAFEValid.word),    0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.IdleFSEnB",       &(pReg->IdleFSEnB.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.IdleSDEnB",       &(pReg->IdleSDEnB.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.IdleDAEnB",       &(pReg->IdleDAEnB.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SDEnB17",         &(pReg->SDEnB17.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayCFO",        &(pReg->DelayCFO.word),         0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayRSSI",       &(pReg->DelayRSSI.word),        0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayRIFS",       &(pReg->DelayRIFS.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.OFDMSwDEn",       &(pReg->OFDMSwDEn.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.OFDMSwDSave",     &(pReg->OFDMSwDSave.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.OFDMSwDInit",     &(pReg->OFDMSwDInit.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.UpdateGain11b",   &(pReg->UpdateGain11b.word),    0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.OFDMSwDThLow",    &(pReg->OFDMSwDThLow.word),     0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SigDetTh1",       &(pReg->SigDetTh1.word),        0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SigDetTh2H",      &(pReg->SigDetTh2H.word),       0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SigDetTh2L",      &(pReg->SigDetTh2L.word),       0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SigDetDelay",     &(pReg->SigDetDelay.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SigDetFilter",    &(pReg->SigDetFilter.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SyncFilter",      &(pReg->SyncFilter.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SyncMag",         &(pReg->SyncMag.word),          0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SyncThSig",       &(pReg->SyncThSig.word),        0, 15));
        PSTATUS(wiParse_Int    (text,"Verona.Register.SyncThExp",       &(pReg->SyncThExp.word),       -8, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SyncDelay",       &(pReg->SyncDelay.word),        0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.MinShift",        &(pReg->MinShift.word),         0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ChEstShift0",     &(pReg->ChEstShift0.word),      0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ChEstShift1",     &(pReg->ChEstShift1.word),      0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ChTracking",      &(pReg->ChTracking.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ChTrackGain1",    &(pReg->ChTrackGain1.word),     0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ChTrackGain2",    &(pReg->ChTrackGain2.word),     0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aC1",             &(pReg->aC1.word),              0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bC1",             &(pReg->bC1.word),              0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aS1",             &(pReg->aS1.word),              0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bS1",             &(pReg->bS1.word),              0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aC2",             &(pReg->aC2.word),              0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bC2",             &(pReg->bC2.word),              0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aS2",             &(pReg->aS2.word),              0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bS2",             &(pReg->bS2.word),              0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxToneDiv",       &(pReg->TxToneDiv.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxWordRH",        &(pReg->TxWordRH.word),         0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxWordRL",        &(pReg->TxWordRL.word),         0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxWordIH",        &(pReg->TxWordIH.word),         0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxWordIL",        &(pReg->TxWordIL.word),         0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DCXSelect",       &(pReg->DCXSelect.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DCXFastLoad",     &(pReg->DCXFastLoad.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DCXHoldAcc",      &(pReg->DCXHoldAcc.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DCXShift",        &(pReg->DCXShift.word),         0, 7));
        PSTATUS(wiParse_Int    (text,"Verona.Register.DCXAcc0RH",       &(pReg->DCXAcc0RH.word),       -16, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DCXAcc0RL",       &(pReg->DCXAcc0RL.word),        0, 255));
        PSTATUS(wiParse_Int    (text,"Verona.Register.DCXAcc0IH",       &(pReg->DCXAcc0IH.word),       -16, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DCXAcc0IL",       &(pReg->DCXAcc0IL.word),        0, 255));
        PSTATUS(wiParse_Int    (text,"Verona.Register.DCXAcc1RH",       &(pReg->DCXAcc1RH.word),       -16, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DCXAcc1RL",       &(pReg->DCXAcc1RL.word),        0, 255));
        PSTATUS(wiParse_Int    (text,"Verona.Register.DCXAcc1IH",       &(pReg->DCXAcc1IH.word),       -16, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DCXAcc1IL",       &(pReg->DCXAcc1IL.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.INITdelay",       &(pReg->INITdelay.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.EDwindow",        &(pReg->EDwindow.word),         0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CQwindow",        &(pReg->CQwindow.word),         0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SSwindow",        &(pReg->SSwindow.word),         0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CQthreshold",     &(pReg->CQthreshold.word),      0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.EDthreshold",     &(pReg->EDthreshold.word),      0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bWaitAGC",        &(pReg->bWaitAGC.word),         0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bAGCdelay",       &(pReg->bAGCdelay.word),        0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bRefPwr",         &(pReg->bRefPwr.word),          0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bInitAGain",      &(pReg->bInitAGain.word),       0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bThSigLarge",     &(pReg->bThSigLarge.word),      0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bStepLarge",      &(pReg->bStepLarge.word),       0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bThSwitchLNA",    &(pReg->bThSwitchLNA.word),     0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bStepLNA",        &(pReg->bStepLNA.word),         0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bThSwitchLNA2",   &(pReg->bThSwitchLNA2.word),    0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bStepLNA2",       &(pReg->bStepLNA2.word),        0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bPwr100dBm",      &(pReg->bPwr100dBm.word),       0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DAmpGainRange",   &(pReg->DAmpGainRange.word),    0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bMaxUpdateCount", &(pReg->bMaxUpdateCount.word),  0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CSMode",          &(pReg->CSMode.word),           0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bPwrStepDet",     &(pReg->bPwrStepDet.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bCFOQual",        &(pReg->bCFOQual.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bAGCBounded4",    &(pReg->bAGCBounded4.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bLgSigAFEValid",  &(pReg->bLgSigAFEValid.word),   0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bSwDEn",          &(pReg->bSwDEn.word),           0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bSwDSave",        &(pReg->bSwDSave.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bSwDInit",        &(pReg->bSwDInit.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bSwDDelay",       &(pReg->bSwDDelay.word),        0, 31));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bSwDTh",          &(pReg->bSwDTh.word),           0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bThStepUpRefPwr", &(pReg->bThStepUpRefPwr.word),  0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bThStepUp",       &(pReg->bThStepUp.word),        0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bThStepDown",     &(pReg->bThStepDown.word),      0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bRestartDelay",   &(pReg->bRestartDelay.word),    0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SymHDRCE",        &(pReg->SymHDRCE.word),         0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CENSym1",         &(pReg->CENSym1.word),          0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CENSym2",         &(pReg->CENSym2.word),          0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.hThreshold1",     &(pReg->hThreshold1.word),      0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.hThreshold2",     &(pReg->hThreshold2.word),      0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.hThreshold3",     &(pReg->hThreshold3.word),      0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SFDWindow",       &(pReg->SFDWindow.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.AntSel",          &(pReg->AntSel.word),           0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DW_PHYEnB",       &(pReg->DW_PHYEnB.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TopState",        &(pReg->TopState.word),         0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.CFO",             &(pReg->CFO.word),              0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxFaultCount",    &(pReg->RxFaultCount.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RestartCount",    &(pReg->RestartCount.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.NuisanceCount",   &(pReg->NuisanceCount.word),    0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.sPHYEnB",         &(pReg->sPHYEnB.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.PHYEnB",          &(pReg->PHYEnB.word),           0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.NoStepUpAgg",     &(pReg->NoStepUpAgg.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RSSIdB",          &(pReg->RSSIdB.word),           0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.UnsupportedGF",   &(pReg->UnsupportedGF.word),    0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ClrOnWake",       &(pReg->ClrOnWake.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ClrOnHdr",        &(pReg->ClrOnHdr.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ClrNow",          &(pReg->ClrNow.word),           0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Filter64QAM",     &(pReg->Filter64QAM.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Filter16QAM",     &(pReg->Filter16QAM.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.FilterQPSK",      &(pReg->FilterQPSK.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.FilterBPSK",      &(pReg->FilterBPSK.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.FilterUndecoded", &(pReg->FilterUndecoded.word),  0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.FilterAllTypes",  &(pReg->FilterAllTypes.word),   0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.AddrFilter",      &(pReg->AddrFilter.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.STA0",            &(pReg->STA0.word),             0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.STA1",            &(pReg->STA1.word),             0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.STA2",            &(pReg->STA2.word),             0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.STA3",            &(pReg->STA3.word),             0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.STA4",            &(pReg->STA4.word),             0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.STA5",            &(pReg->STA5.word),             0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.minRSSI",         &(pReg->minRSSI.word),          0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.StepUpRestart",   &(pReg->StepUpRestart.word),    0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.StepDownRestart", &(pReg->StepDownRestart.word),  0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.NoStepAfterSync", &(pReg->NoStepAfterSync.word),  0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.NoStepAfterHdr",  &(pReg->NoStepAfterHdr.word),   0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxRIFS",          &(pReg->RxRIFS.word),           0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ForceRestart",    &(pReg->ForceRestart.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.KeepCCA_New",     &(pReg->KeepCCA_New.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.KeepCCA_Lost",    &(pReg->KeepCCA_Lost.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.PktTimePhase80",  &(pReg->PktTimePhase80.word),   0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.WakeupTimeH",     &(pReg->WakeupTimeH.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.WakeupTimeL",     &(pReg->WakeupTimeL.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayStdby",      &(pReg->DelayStdby.word),       0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayBGEnB",      &(pReg->DelayBGEnB.word),       0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayADC1",       &(pReg->DelayADC1.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayADC2",       &(pReg->DelayADC2.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayModem",      &(pReg->DelayModem.word),       0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayDFE",        &(pReg->DelayDFE.word),         0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayRFSW",       &(pReg->DelayRFSW.word),        0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxRxTime",        &(pReg->TxRxTime.word),         0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayFCAL1",      &(pReg->DelayFCAL1.word),       0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayPA",         &(pReg->DelayPA.word),          0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayFCAL2",      &(pReg->DelayFCAL2.word),       0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayState51",    &(pReg->DelayState51.word),     0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayShutdown",   &(pReg->DelayShutdown.word),    0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelaySleep",      &(pReg->DelaySleep.word),       0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TimeExtend",      &(pReg->TimeExtend.word),       0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SquelchTime",     &(pReg->SquelchTime.word),      0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelaySquelchRF",  &(pReg->DelaySquelchRF.word),   0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DelayClearDCX",   &(pReg->DelayClearDCX.word),    0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RadioMC",         &(pReg->RadioMC.word),          0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DACOPM",          &(pReg->DACOPM.word),           0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCAOPM",         &(pReg->ADCAOPM.word),          0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCBOPM",         &(pReg->ADCBOPM.word),          0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RFSW1",           &(pReg->RFSW1.word),            0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RFSW2",           &(pReg->RFSW2.word),            0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.LNAEnB",          &(pReg->LNAEnB.word),           0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCBCTRLR",       &(pReg->ADCBCTRLR.word),        0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCBCTRL",        &(pReg->ADCBCTRL.word),         0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.sPA50",           &(pReg->sPA50.word),            0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.sPA24",           &(pReg->sPA24.word),            0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCFSCTRL",       &(pReg->ADCFSCTRL.word),        0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCCHSEL",        &(pReg->ADCCHSEL.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCOE",           &(pReg->ADCOE.word),            0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.LNAxxBOnSW2",     &(pReg->LNAxxBOnSW2.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCOFCEN",        &(pReg->ADCOFCEN.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCDCREN",        &(pReg->ADCDCREN.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCMUX0",         &(pReg->ADCMUX0.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCARDY",         &(pReg->ADCARDY.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCBRDY",         &(pReg->ADCBRDY.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.PPSEL_MODE",      &(pReg->PPSEL_MODE.word),       0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCFCAL2",        &(pReg->ADCFCAL2.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCFCAL1",        &(pReg->ADCFCAL1.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCFCAL",         &(pReg->ADCFCAL.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.VCMEN",           &(pReg->VCMEN.word),            0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DACIFS_CTRL",     &(pReg->DACIFS_CTRL.word),      0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DACFG_CTRL0",     &(pReg->DACFG_CTRL0.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DACFG_CTRL1",     &(pReg->DACFG_CTRL1.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RESETB",          &(pReg->RESETB.word),           0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxLNAGAIN2",      &(pReg->TxLNAGAIN2.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SREdge",          &(pReg->SREdge.word),           0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SRFreq",          &(pReg->SRFreq.word),           0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.sLNA24A",         &(pReg->sLNA24A.word),          0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.sLNA24B",         &(pReg->sLNA24B.word),          0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.sLNA50A",         &(pReg->sLNA50A.word),          0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.sLNA50B",         &(pReg->sLNA50B.word),          0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SelectCCA",       &(pReg->SelectCCA.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThCCA_SigDet",    &(pReg->ThCCA_SigDet.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThCCA_RSSI1",     &(pReg->ThCCA_RSSI1.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThCCA_RSSI2",     &(pReg->ThCCA_RSSI2.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThCCA_GF1",       &(pReg->ThCCA_GF1.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ThCCA_GF2",       &(pReg->ThCCA_GF2.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ExtendBusyGF",    &(pReg->ExtendBusyGF.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ExtendRxError",   &(pReg->ExtendRxError.word),    0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ExtendRxError1",  &(pReg->ExtendRxError1.word),   0, 255));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t0_r_s0", &(pReg->P1_coef_t0_r_s0.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t0_i_s0", &(pReg->P1_coef_t0_i_s0.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t1_r_s0", &(pReg->P1_coef_t1_r_s0.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t1_i_s0", &(pReg->P1_coef_t1_i_s0.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t2_r_s0", &(pReg->P1_coef_t2_r_s0.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t2_i_s0", &(pReg->P1_coef_t2_i_s0.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t3_r_s0", &(pReg->P1_coef_t3_r_s0.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t3_i_s0", &(pReg->P1_coef_t3_i_s0.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t4_r_s0", &(pReg->P1_coef_t4_r_s0.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t4_i_s0", &(pReg->P1_coef_t4_i_s0.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t0_r_s0", &(pReg->P3_coef_t0_r_s0.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t0_i_s0", &(pReg->P3_coef_t0_i_s0.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t1_r_s0", &(pReg->P3_coef_t1_r_s0.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t1_i_s0", &(pReg->P3_coef_t1_i_s0.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t2_r_s0", &(pReg->P3_coef_t2_r_s0.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t2_i_s0", &(pReg->P3_coef_t2_i_s0.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t0_r_s0", &(pReg->P5_coef_t0_r_s0.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t0_i_s0", &(pReg->P5_coef_t0_i_s0.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t1_r_s0", &(pReg->P5_coef_t1_r_s0.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t1_i_s0", &(pReg->P5_coef_t1_i_s0.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t2_r_s0", &(pReg->P5_coef_t2_r_s0.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t2_i_s0", &(pReg->P5_coef_t2_i_s0.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t0_r_s1", &(pReg->P1_coef_t0_r_s1.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t0_i_s1", &(pReg->P1_coef_t0_i_s1.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t1_r_s1", &(pReg->P1_coef_t1_r_s1.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t1_i_s1", &(pReg->P1_coef_t1_i_s1.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t2_r_s1", &(pReg->P1_coef_t2_r_s1.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t2_i_s1", &(pReg->P1_coef_t2_i_s1.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t3_r_s1", &(pReg->P1_coef_t3_r_s1.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t3_i_s1", &(pReg->P1_coef_t3_i_s1.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t4_r_s1", &(pReg->P1_coef_t4_r_s1.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t4_i_s1", &(pReg->P1_coef_t4_i_s1.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t0_r_s1", &(pReg->P3_coef_t0_r_s1.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t0_i_s1", &(pReg->P3_coef_t0_i_s1.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t1_r_s1", &(pReg->P3_coef_t1_r_s1.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t1_i_s1", &(pReg->P3_coef_t1_i_s1.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t2_r_s1", &(pReg->P3_coef_t2_r_s1.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t2_i_s1", &(pReg->P3_coef_t2_i_s1.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t0_r_s1", &(pReg->P5_coef_t0_r_s1.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t0_i_s1", &(pReg->P5_coef_t0_i_s1.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t1_r_s1", &(pReg->P5_coef_t1_r_s1.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t1_i_s1", &(pReg->P5_coef_t1_i_s1.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t2_r_s1", &(pReg->P5_coef_t2_r_s1.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t2_i_s1", &(pReg->P5_coef_t2_i_s1.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t0_r_s2", &(pReg->P1_coef_t0_r_s2.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t0_i_s2", &(pReg->P1_coef_t0_i_s2.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t1_r_s2", &(pReg->P1_coef_t1_r_s2.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t1_i_s2", &(pReg->P1_coef_t1_i_s2.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t2_r_s2", &(pReg->P1_coef_t2_r_s2.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t2_i_s2", &(pReg->P1_coef_t2_i_s2.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t3_r_s2", &(pReg->P1_coef_t3_r_s2.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t3_i_s2", &(pReg->P1_coef_t3_i_s2.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t4_r_s2", &(pReg->P1_coef_t4_r_s2.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t4_i_s2", &(pReg->P1_coef_t4_i_s2.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t0_r_s2", &(pReg->P3_coef_t0_r_s2.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t0_i_s2", &(pReg->P3_coef_t0_i_s2.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t1_r_s2", &(pReg->P3_coef_t1_r_s2.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t1_i_s2", &(pReg->P3_coef_t1_i_s2.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t2_r_s2", &(pReg->P3_coef_t2_r_s2.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t2_i_s2", &(pReg->P3_coef_t2_i_s2.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t0_r_s2", &(pReg->P5_coef_t0_r_s2.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t0_i_s2", &(pReg->P5_coef_t0_i_s2.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t1_r_s2", &(pReg->P5_coef_t1_r_s2.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t1_i_s2", &(pReg->P5_coef_t1_i_s2.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t2_r_s2", &(pReg->P5_coef_t2_r_s2.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t2_i_s2", &(pReg->P5_coef_t2_i_s2.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t0_r_s3", &(pReg->P1_coef_t0_r_s3.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t0_i_s3", &(pReg->P1_coef_t0_i_s3.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t1_r_s3", &(pReg->P1_coef_t1_r_s3.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t1_i_s3", &(pReg->P1_coef_t1_i_s3.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t2_r_s3", &(pReg->P1_coef_t2_r_s3.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t2_i_s3", &(pReg->P1_coef_t2_i_s3.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t3_r_s3", &(pReg->P1_coef_t3_r_s3.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t3_i_s3", &(pReg->P1_coef_t3_i_s3.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t4_r_s3", &(pReg->P1_coef_t4_r_s3.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P1_coef_t4_i_s3", &(pReg->P1_coef_t4_i_s3.word), -4096, 4095));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t0_r_s3", &(pReg->P3_coef_t0_r_s3.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t0_i_s3", &(pReg->P3_coef_t0_i_s3.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t1_r_s3", &(pReg->P3_coef_t1_r_s3.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t1_i_s3", &(pReg->P3_coef_t1_i_s3.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t2_r_s3", &(pReg->P3_coef_t2_r_s3.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P3_coef_t2_i_s3", &(pReg->P3_coef_t2_i_s3.word), -16384, 16383));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t0_r_s3", &(pReg->P5_coef_t0_r_s3.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t0_i_s3", &(pReg->P5_coef_t0_i_s3.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t1_r_s3", &(pReg->P5_coef_t1_r_s3.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t1_i_s3", &(pReg->P5_coef_t1_i_s3.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t2_r_s3", &(pReg->P5_coef_t2_r_s3.word), -2048, 2047));
        PSTATUS(wiParse_Int    (text,"Verona.Register.P5_coef_t2_i_s3", &(pReg->P5_coef_t2_i_s3.word), -2048, 2047));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.cross_pwr1",      &(pReg->cross_pwr1.word),       0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.cross_pwr2",      &(pReg->cross_pwr2.word),       0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.cross_pwr3",      &(pReg->cross_pwr3.word),       0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Tx_Rate_OFDM_HT", &(pReg->Tx_Rate_OFDM_HT.word),  0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Tx_Rate_OFDM_LL", &(pReg->Tx_Rate_OFDM_LL.word),  0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Tx_Rate_OFDM_LM", &(pReg->Tx_Rate_OFDM_LM.word),  0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DPD_spare",       &(pReg->DPD_spare.word),        0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DPD_LOW_TMP_DIS", &(pReg->DPD_LOW_TMP_DIS.word),  0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DPD_PA_tmp",      &(pReg->DPD_PA_tmp.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DPD_LOW_RATE_DIS",&(pReg->DPD_LOW_RATE_DIS.word), 0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DPD_coef_interspersion",&(pReg->DPD_coef_interspersion.word), 0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Cbc_coef_sel",    &(pReg->Cbc_coef_sel.word),     0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.cnfg_eovr",       &(pReg->cnfg_eovr.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Coef_sel_vovr",   &(pReg->Coef_sel_vovr.word),    0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Vfb_byp_vovr",    &(pReg->Vfb_byp_vovr.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Pa_tmp_ext",      &(pReg->Pa_tmp_ext.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTDone",        &(pReg->BISTDone.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTADCSel",      &(pReg->BISTADCSel.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTNoPrechg",    &(pReg->BISTNoPrechg.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTFGSel",       &(pReg->BISTFGSel.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTMode",        &(pReg->BISTMode.word),         0, 7));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.StartBIST",       &(pReg->StartBIST.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTPipe",        &(pReg->BISTPipe.word),         0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTN_val",       &(pReg->BISTN_val.word),        0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTOfsR",        &(pReg->BISTOfsR.word),         0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTOfsI",        &(pReg->BISTOfsI.word),         0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTTh1",         &(pReg->BISTTh1.word),          0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTTh2",         &(pReg->BISTTh2.word),          0, 65535));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTFG_CTRLI",    &(pReg->BISTFG_CTRLI.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTFG_CTRLR",    &(pReg->BISTFG_CTRLR.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTStatus",      &(pReg->BISTStatus.word),       0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTAccR",        &(pReg->BISTAccR.word),         0, 16777215));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTAccI",        &(pReg->BISTAccI.word),         0, 16777215));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTRangeLimH",   &(pReg->BISTRangeLimH.word),    0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.BISTRangeLimL",   &(pReg->BISTRangeLimL.word),    0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSIntEOB",       &(pReg->DFSIntEOB.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSIntFIFO",      &(pReg->DFSIntFIFO.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSThFIFO",       &(pReg->DFSThFIFO.word),        0, 127));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSDepthFIFO",    &(pReg->DFSDepthFIFO.word),     0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSReadFIFO",     &(pReg->DFSReadFIFO.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.DFSSyncFIFO",     &(pReg->DFSSyncFIFO.word),      0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxHeader1",       &(pReg->RxHeader1.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxHeader2",       &(pReg->RxHeader2.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxHeader3",       &(pReg->RxHeader3.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxHeader4",       &(pReg->RxHeader4.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxHeader5",       &(pReg->RxHeader5.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxHeader6",       &(pReg->RxHeader6.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxHeader7",       &(pReg->RxHeader7.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxHeader8",       &(pReg->RxHeader8.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxHeader1",       &(pReg->TxHeader1.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxHeader2",       &(pReg->TxHeader2.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxHeader3",       &(pReg->TxHeader3.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxHeader4",       &(pReg->TxHeader4.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxHeader5",       &(pReg->TxHeader5.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxHeader6",       &(pReg->TxHeader6.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxHeader7",       &(pReg->TxHeader7.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxHeader8",       &(pReg->TxHeader8.word),        0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TestSigEn",       &(pReg->TestSigEn.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCAI",           &(pReg->ADCAI.word),            0, 1023));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCAQ",           &(pReg->ADCAQ.word),            0, 1023));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCBI",           &(pReg->ADCBI.word),            0, 1023));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ADCBQ",           &(pReg->ADCBQ.word),            0, 1023));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bHeaderValid",    &(pReg->bHeaderValid.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bSyncFound",      &(pReg->bSyncFound.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bAGCDone",        &(pReg->bAGCDone.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.bSigDet",         &(pReg->bSigDet.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aHeaderValid",    &(pReg->aHeaderValid.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aSyncFound",      &(pReg->aSyncFound.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aAGCDone",        &(pReg->aAGCDone.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.aSigDet",         &(pReg->aSigDet.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SelectSignal2",   &(pReg->SelectSignal2.word),    0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SelectSignal1",   &(pReg->SelectSignal1.word),    0, 15));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SelectTrigger2",  &(pReg->SelectTrigger2.word),   0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.SelectTrigger1",  &(pReg->SelectTrigger1.word),   0, 3));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TriggerState1",   &(pReg->TriggerState1.word),    0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TriggerState2",   &(pReg->TriggerState2.word),    0, 63));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TriggerRSSI",     &(pReg->TriggerRSSI.word),      0, 255));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.LgSigAFE",        &(pReg->LgSigAFE.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Trigger2",        &(pReg->Trigger2.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Trigger1",        &(pReg->Trigger1.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Signal2",         &(pReg->Signal2.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Signal1",         &(pReg->Signal1.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.ClearInt",        &(pReg->ClearInt.word),         0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.IntEvent2",       &(pReg->IntEvent2.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.IntEvent1",       &(pReg->IntEvent1.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.IntRxFault",      &(pReg->IntRxFault.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.IntRxAbortTx",    &(pReg->IntRxAbortTx.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.IntTxSleep",      &(pReg->IntTxSleep.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.IntTxAbort",      &(pReg->IntTxAbort.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.IntTxFault",      &(pReg->IntTxFault.word),       0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Interrupt",       &(pReg->Interrupt.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Event2",          &(pReg->Event2.word),           0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.Event1",          &(pReg->Event1.word),           0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxFault",         &(pReg->RxFault.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.RxAbortTx",       &(pReg->RxAbortTx.word),        0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxSleep",         &(pReg->TxSleep.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxAbort",         &(pReg->TxAbort.word),          0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.TxFault",         &(pReg->TxFault.word),          0, 1));
        // ------------------------------------------------------------------------------
        //  END: Matlab Generated Code

        PSTATUS(wiParse_Boolean(text,"Verona.EnableModelIO",           &(pRX->EnableModelIO)));
        PSTATUS(wiParse_Boolean(text,"Verona.EnableIQCal",             &(pRX->EnableIQCal)));
        PSTATUS(wiParse_UInt   (text,"Verona.aPadFront",               &(pTX->aPadFront), 0,2000000));
        PSTATUS(wiParse_UInt   (text,"Verona.aPadBack",                &(pTX->aPadBack),  0, 400000));
        PSTATUS(wiParse_UInt   (text,"Verona.bPadFront",               &(pTX->bPadFront), 0,  20000));
        PSTATUS(wiParse_UInt   (text,"Verona.bPadBack",                &(pTX->bPadBack),  0, 220000));
        PSTATUS(wiParse_Int    (text,"Verona.RX.InitialPhase",         &(pRX->clock.InitialPhase), -1, 80));
        PSTATUS(wiParse_Int    (text,"Verona.RX.DownsamPhase",         &(pRX->DownsamPhase),     0,  1));
        PSTATUS(wiParse_Int    (text,"Verona.RX.DownmixPhase",         &(pRX->DownmixPhase),     0,  3));
        PSTATUS(wiParse_Boolean(text,"Verona.TX.SetPBCC",              &(pTX->SetPBCC)));

        PSTATUS(wiParse_Int    (text,"Verona.ADC.Offset",              &(pRX->ADC.Offset), -256, 256));
        PSTATUS(wiParse_Int    (text,"Verona.ADC.nEff",                &(pRX->ADC.nEff), 2, 10));
        PSTATUS(wiParse_Real   (text,"Verona.ADC.Vpp",                 &(pRX->ADC.Vpp),  0.5, 3.0));
        PSTATUS(wiParse_Int    (text,"Verona.DAC.nEff",                &(pTX->DAC.nEff), 2, 10));
        PSTATUS(wiParse_Real   (text,"Verona.DAC.Vpp",                 &(pTX->DAC.Vpp),  0.5, 3.0));
        PSTATUS(wiParse_Real   (text,"Verona.DAC.IQdB",                &(pTX->DAC.IQdB),-6.0, 6.0));
        PSTATUS(wiParse_Int    (text,"Verona.TX.bResamplePhase",       &(pTX->bResamplePhase),  -1,19));
        PSTATUS(wiParse_Int    (text,"Verona.TX.UpmixPhase",           &(pTX->UpmixPhase), 0, 7));
        PSTATUS(wiParse_Int    (text,"Verona.TX.TxTonePhase",          &(pTX->TxTonePhase), 0, 159));

		PSTATUS(wiParse_Boolean(text,"Verona.RX.Nuisance.Enabled",   &(pRX->Nuisance.Enabled)));
		PSTATUS(wiParse_UInt   (text,"Verona.RX.Nuisance.kNuisance", &(pRX->Nuisance.kNuisance), 0, 1<<20));

        PSTATUS(wiParse_Boolean(text,"Verona.RX.NonRTL.Enabled",       &(pRX->NonRTL.Enabled)));
        PSTATUS(wiParse_UInt   (text,"Verona.RX.NonRTL.MaxSamplesCFO", &(pRX->NonRTL.MaxSamplesCFO), 0, 160));
        PSTATUS(wiParse_Boolean(text,"Verona.RX.NonRTL.DisableCFO",    &(pRX->NonRTL.DisableCFO)));
        PSTATUS(wiParse_Boolean(text,"Verona.RX.NonRTL.DisablePLL",    &(pRX->NonRTL.DisablePLL)));
        PSTATUS(wiParse_Boolean(text,"Verona.RX.NonRTL.MojaveFrontCtrlEOP",&(pRX->NonRTL.MojaveFrontCtrlEOP)));

        // legacy versions following block
        PSTATUS(wiParse_UInt   (text,"Verona.RX.TX.k1",                &(pRX->TX.k1[0]),      0, 1<<20));
        PSTATUS(wiParse_UInt   (text,"Verona.RX.TX.k2",                &(pRX->TX.k2[0]),      0, 1<<20));
        PSTATUS(wiParse_UInt   (text,"Verona.RX.TX.kTxDone",           &(pRX->TX.kTxDone[0]), 0, 1<<20));
        PSTATUS(wiParse_UInt   (text,"Verona.RX.TX.kTxFault",          &(pRX->TX.kTxFault),   0, 1<<20));

        PSTATUS(wiParse_Boolean  (text,"Verona.RX.TX.Enabled",         &(pRX->TX.Enabled)));
        PSTATUS(wiParse_Boolean  (text,"Verona.RX.TX.Usek2Delay",      &(pRX->TX.Usek2Delay)));
        PSTATUS(wiParse_UInt     (text,"Verona.RX.TX.k2Delay",         &(pRX->TX.k2Delay), 0,  8000));
        PSTATUS(wiParse_UIntArray(text,"Verona.RX.TX.k1",                pRX->TX.k1,      3, 0, 1<<20));
        PSTATUS(wiParse_UIntArray(text,"Verona.RX.TX.k2",                pRX->TX.k2,      3, 0, 1<<20));
        PSTATUS(wiParse_UIntArray(text,"Verona.RX.TX.kTxDone",           pRX->TX.kTxDone, 3, 0, 1<<20));

        PSTATUS(wiParse_Boolean(text,"Verona.TX.NoTxHeaderUpdate",     &(pTX->NoTxHeaderUpdate)                ));
        PSTATUS(wiParse_Boolean(text,"Verona.TX.UseForcedbSFD",        &(pTX->UseForcedbSFD)                   ));
        PSTATUS(wiParse_Boolean(text,"Verona.TX.UseForcedbSIGNAL",     &(pTX->UseForcedbSIGNAL)                ));
        PSTATUS(wiParse_Boolean(text,"Verona.TX.UseForcedbSERVICE",    &(pTX->UseForcedbSERVICE)               ));
        PSTATUS(wiParse_Boolean(text,"Verona.TX.UseForcedbLENGTH",     &(pTX->UseForcedbLENGTH)                ));
        PSTATUS(wiParse_Boolean(text,"Verona.TX.UseForcedbCRC",        &(pTX->UseForcedbCRC)                   ));
        PSTATUS(wiParse_Boolean(text,"Verona.TX.UseForcedSIGNAL",      &(pTX->UseForcedSIGNAL)                 ));
        PSTATUS(wiParse_Boolean(text,"Verona.TX.UseForcedHTSIG",       &(pTX->UseForcedHTSIG)                  ));
        PSTATUS(wiParse_Boolean(text,"Verona.TX.UseForcedHTSIG_CRC",   &(pTX->UseForcedHTSIG_CRC)              ));

        PSTATUS(wiParse_UInt   (text,"Verona.TX.ForcedbSFD",           &(pTX->ForcedbSFD.word),     0,    65535));
        PSTATUS(wiParse_UInt   (text,"Verona.TX.ForcedbSIGNAL",        &(pTX->ForcedbSIGNAL.word),  0,      255));
        PSTATUS(wiParse_UInt   (text,"Verona.TX.ForcedbSERVICE",       &(pTX->ForcedbSERVICE.word), 0,      255));
        PSTATUS(wiParse_UInt   (text,"Verona.TX.ForcedbLENGTH",        &(pTX->ForcedbLENGTH.word),  0,    65535));
        PSTATUS(wiParse_UInt   (text,"Verona.TX.ForcedbCRC",           &(pTX->ForcedbCRC.word),     0,    65535));
        PSTATUS(wiParse_UInt   (text,"Verona.TX.ForcedSIGNAL",         &(pTX->ForcedSIGNAL.word),   0, 16777215));
        PSTATUS(wiParse_UInt   (text,"Verona.TX.ForcedHTSIG1",         &(pTX->ForcedHTSIG1.word),   0, 16777215));
        PSTATUS(wiParse_UInt   (text,"Verona.TX.ForcedHTSIG2",         &(pTX->ForcedHTSIG2.word),   0,   262143));
        PSTATUS(wiParse_Boolean(text,"Verona.RX.MAC_NOFILTER",         &(pRX->MAC_NOFILTER)                    ));

        PSTATUS(wiParse_Boolean(text,"Verona.TX.Checksum.Enable",      &(pTX->Checksum.Enable)) );

        PSTATUS(wiParse_UIntArray(text,"Verona.TX.Trace.kMarker", pTX->Trace.kMarker, 5, 0, 0x7FFFFFFF));
        PSTATUS(wiParse_UIntArray(text,"Verona.RX.Trace.kMarker", pRX->Trace.kMarker, 5, 0, 0x7FFFFFFF));
        PSTATUS(wiParse_UIntArray(text,"Verona.RX.Trace.kStart",  pRX->Trace.kStart,  9, 0, 0x7FFFFFFF));
        PSTATUS(wiParse_UIntArray(text,"Verona.RX.Trace.kEnd",    pRX->Trace.kEnd,    9, 0, 0x7FFFFFFF));

        PSTATUS(wiParse_Boolean  (text,"Verona.TX.LgSigAFE",           &(pTX->LgSigAFE)));
        PSTATUS(wiParse_Boolean  (text,"Verona.TX.FixedTimingShift",   &(pTX->FixedTimingShift)));
        PSTATUS(wiParse_Real     (text,"Verona.TX.TimingShift",        &(pTX->TimingShift), -1.0, 1.0));

        PSTATUS(wiParse_Boolean  (text,"Verona.DV.ExpectPacketFail",   &(pDV->ExpectPacketFail)) );
        PSTATUS(wiParse_Int      (text,"Verona.DV.VerilogChecker",     &(pDV->VerilogChecker),    0, 0xFFFF));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.NumCheckRegisters",  &(pDV->NumCheckRegisters), 0, 512));
        PSTATUS(wiParse_UIntArray(text,"Verona.DV.CheckRegAddr",         pDV->CheckRegAddr, 512, 0, 1023));
        PSTATUS(wiParse_Boolean  (text,"Verona.DV.FixedTxTimingLimits",&(pDV->FixedTxTimingLimits)));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.aTxStart",           &(pDV->aTxStart),      0,  800));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.bTxStart",           &(pDV->bTxStart),      0,  800));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.aTxEnd",             &(pDV->aTxEnd),        0,  800));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.bTxEnd",             &(pDV->bTxEnd),        0,  800));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.aTxStartTolerance",  &(pDV->aTxStartTolerance), 0, 80));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.bTxStartTolerance",  &(pDV->bTxStartTolerance), 0, 80));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.aTxEndTolerance",    &(pDV->aTxEndTolerance),   0, 80));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.bTxEndTolerance",    &(pDV->bTxEndTolerance),   0, 80));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.TxStartMinT80",      &(pDV->TxStartMinT80), 0, 8000));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.TxStartMaxT80",      &(pDV->TxStartMaxT80), 0, 8000));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.TxEndMinT80",        &(pDV->TxEndMinT80),   0, 8000));
        PSTATUS(wiParse_UInt     (text,"Verona.DV.TxEndMaxT80",        &(pDV->TxEndMaxT80),   0, 8000));
        PSTATUS(wiParse_Boolean  (text,"Verona.DV.LongSimulation",     &(pDV->LongSimulation)));
        PSTATUS(wiParse_Boolean  (text,"Verona.DV.BreakCheck",         &(pDV->BreakCheck)));
        PSTATUS(wiParse_Int      (text,"Verona.DV.TestLimit",          &(pDV->TestLimit),-2147483647 ,2147483647));
        PSTATUS(wiParse_Int      (text,"Verona.DV.DelayT80",           &(pDV->DelayT80), -8000000, 8000000));
        PSTATUS(wiParse_Boolean  (text,"Verona.DV.NoRegMapUpdateOnTx", &(pDV->NoRegMapUpdateOnTx)));

        PSTATUS(wiParse_Boolean(text,"Verona.TraceKeyOp",     NULL)); // parameter removed
        PSTATUS(wiParse_Boolean(text,"Verona.EnableTraceCCK", NULL)); // parameter removed

        // -----------------------
        // Simulation Result Trace
        // -----------------------
        status = WICHK(wiParse_Boolean(text,"Verona.EnableTrace", &(pRX->EnableTrace)));
        if (status==WI_SUCCESS)
        {
/*
            XSTATUS(Verona_RX_AllocateMemory());  // make sure trace arrays are allocated
*/
            return WI_SUCCESS;
        }
        else if (status<0) return status;

        // -----------
        // Array Sizes
        // ----------- 
        status = STATUS(wiParse_UInt(text, "Verona.aLength", &(pRX->aLength), 1, 16+8*65535+6+287));
        if (status == WI_SUCCESS) {
/*
            XSTATUS(Verona_RX_AllocateMemory());
*/
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_UInt(text, "Verona.rLength", &(pRX->rLength), 1, 1000000));
        if (status == WI_SUCCESS) {
/*
            XSTATUS(Verona_RX_AllocateMemory());
*/
            return WI_SUCCESS;
        }
        // --------------
        // PLL parameters
        // --------------
        status = STATUS(wiParse_UInt(text,"Verona.DPLL.Sa_Sb",&(X.word), 0, 63));
        if (status == WI_SUCCESS) {
            pReg->Sa.word = (X.w6 >> 3) & 0x7;
            pReg->Sb.word = (X.w6 >> 0) & 0x7;
            return WI_SUCCESS;
        }
        XSTATUS(status = wiParse_UInt(text,"Verona.DPLL.Ca_Cb",&(X.word), 0, 63));
        if (status == WI_SUCCESS) {
            pReg->Ca.word = (X.w6 >> 3) & 0x7;
            pReg->Cb.word = (X.w6 >> 0) & 0x7;
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Command(text,"Verona_RESET()"));
        if (status == WI_SUCCESS) {
            STATUS(Verona_RESET());
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Function(text,"Verona_SaveRegisters(%255s)",&buf));
        if (status == WI_SUCCESS) {
            XSTATUS(Verona_WriteRegisterMap(RegMap));
            XSTATUS(Verona_SaveRegisters(RegMap, buf));
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Function(text,"Verona_LoadRegisters(%255s)",&buf));
        if (status == WI_SUCCESS) {
            XSTATUS(Verona_WriteRegisterMap(RegMap));
            XSTATUS(Verona_LoadRegisters(RegMap, buf));
            XSTATUS(Verona_ReadRegisterMap(RegMap));
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Function(text,"Verona_WriteRegister(%d,%d)",&(X.word), &(Y.word)));
        if (status == WI_SUCCESS)
        {
            XSTATUS(Verona_WriteRegister(X.word, Y.word));
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Function(text,"Verona_DvCopyTxData(%d)", &(X.word)));
        if (status == WI_SUCCESS)
        {
            XSTATUS( Verona_DvCopyTxData(X.word) );
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Function(text,"Verona_DvSetTxDataByte(%d, %d, %d)", &dummy, &(X.word), &(Y.word)));
        if (status == WI_SUCCESS)
        {
            XSTATUS( Verona_DvSetTxDataByte((wiUInt)dummy, X.word, Y.word) );
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Function(text,"Verona_DvCopyTxHeader(%d)", &(X.word)));
        if (status == WI_SUCCESS)
        {
            XSTATUS( Verona_DvCopyTxHeader(X.word) );
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Command(text,"Verona_WriteDvRegMap()"));
        if (status == WI_SUCCESS)
        {
            Verona_DvState_t *pDV = Verona_DvState();
            XSTATUS( Verona_WriteRegisterMap( pDV->RegMap ) );
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Function(text,"Verona_InvertArrayBit(%255s, %d, %d, %d)", &buf, &(X.word), &(Y.word), &(Z.word)));
        if (status == WI_SUCCESS)
        {
            XSTATUS( Verona_InvertArrayBit(buf, X.word, Y.word, Z.word) );
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Function(text,"Verona_SetArrayWord(%255s, %d, %d, %d)", &buf, &(X.word), &(Y.word), &(Z.word)));
        if (status == WI_SUCCESS)
        {
            XSTATUS( Verona_SetArrayWord(buf, X.word, Y.word, Z.word) );
            return WI_SUCCESS;
        }
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Aliases for backward compatibility
        //
        PSTATUS(wiParse_UInt   (text,"Verona.Register.FixedLNAGain",    &(pReg->InitLNAGain.word),     0, 1));
        PSTATUS(wiParse_UInt   (text,"Verona.Register.FixedLNA2Gain",   &(pReg->InitLNAGain2.word),    0, 1));
        //
        //  End Temporary aliases for backward compatibility

    } // if (strstr(...
    return WI_WARN_PARSE_FAILED;
}
// end of Verona_ParseLine()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
