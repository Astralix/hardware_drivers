//
//  wiTGnChanl.h -- TGn Channel Model
//  Copyright 2006 DSP Group, Inc. All rights reserved.
//

#ifndef __WITGNCHANL_H
#define __WITGNCHANL_H

#include "wiChanl.h"
#include "wiArray.h"
#include "wiMatrix.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//  Model Parameter Definitions
//
#define WITGNCHANL_DOWNLINK 0
#define WITGNCHANL_UPLINK   1

#define WITGNCHANL_MAX_ANTENNAS 4

#define WITGNCHANL_MAX_CLUSTERS   7
#define WITGNCHANL_MAX_PATHS     20

#define WITGNCHANL_FADING_BELL_SHAPE       0
#define WITGNCHANL_FADING_BELL_SHAPE_SPIKE 1

typedef struct // data used in wiTGnChanl_MIMO_Channel()
{
    wiCMat DummyMatrix;
    wiCMat FilterStatesOut;
    wiCMat FilterStatesIn;
    wiCMat R, C;
    wiCMat FadingMatrix;
    wiCMat OversampledFadingMatrix;
    wiRealArray_t pdp_linear, RiceVectorNLOS, FadingTime_s, OversampledFadingTime_s, HTime_s;
    wiComplexArray_t RiceVectorLOS, RicePhasor, EachH;

} wiTGnChanlMimoChannel_t;

//=================================================================================================
//  TGn CHANNEL PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiBoolean LOS;        // 0:NLOS, i.e. Distance_Tx_Rx>d_BP; 1:LOS, i.e. Distance_Tx_Rx<d_BP

    double SpacingTx;
    double SpacingRx; 
    double TxRxDistance;
    double CarrierFrequencyHz;
    double Fs; // Sample rate for core TGn channel model
    double fDoppler; // main Doppler frequency (6 Hz for 1.2km/h at 5.4 GHz)

    wiInt Connection;
    char Model; // "A" to "F"

    struct // ----- Fluorescent Light Doppler Model -----
    {
        wiBoolean Enabled;
        wiReal    xMean;   // I/C = X^2, X~N(xMean, xStdDev^2)
        wiReal    xStdDev;
        wiReal    PowerLineFrequencyHz;
    } Fluorescent;

    struct
    {
        int Hold;            // Hold=0 : Generate a new independent channel;
                             // Hold=1 : Use FilterStates as the current state; Used for CSI feedback mode
        double Gap_s;        // The amount of delay between the two transmissions, in seconds;
    }  CSI;

}   wiTGnChanl_Param_t;

//=================================================================================================
//  TGn CHANNEL PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    int NumberOfPaths;
    int NumberOfClusters; // The number of clusters is assumed to be equal for Tx and Rx

    int NumberOfTxAntennas;
    int NumberOfRxAntennas;

    double d_BP;

    struct
    {
        const double *PDPdB[2];
        const double *PowerPerAngle_dB[WITGNCHANL_MAX_CLUSTERS];
        const double *AoD_Tx_deg[WITGNCHANL_MAX_CLUSTERS];
        const double *AoA_Rx_deg[WITGNCHANL_MAX_CLUSTERS];
        const double *AS_Tx_deg[WITGNCHANL_MAX_CLUSTERS];
        const double *AS_Rx_deg[WITGNCHANL_MAX_CLUSTERS];

    } ModelData;

    wiInt FadingType;

    wiInt Connection;
    char Model; // "A" to "F"

    wiCMat H;
    wiCMat tempK, tempC, tempCT; // wiTGnChanl_InitMIMO()

    // 3-D complex matrices:
    wiCMat RTx[WITGNCHANL_MAX_PATHS];
    wiCMat RRx[WITGNCHANL_MAX_PATHS];

    wiComplex RiceMatrix[WITGNCHANL_MAX_ANTENNAS][WITGNCHANL_MAX_ANTENNAS];
    
    double K_factor_dB[WITGNCHANL_MAX_PATHS];
    double PathLoss_dB;
    double ShadowFading_dB;
    double ShadowFading_std_dB; // ShadowFading standard deviation
    double AngleAtMobile_rad[WITGNCHANL_MAX_PATHS];
    double ASAtMobile_rad[WITGNCHANL_MAX_PATHS];

    wiRealArray_t    SymbolTimes;
    wiComplexArray_t Rresample;
    wiComplexArray_t X[WICHANL_TXMAX];
    wiComplexArray_t Noise2Filter;
    wiComplexArray_t ModulationFunction;
    struct {
        wiCMat A, B;
    } NormalizationLaplacian;

    wiCMat FluorescentModulationMatrix;
    wiCMat CSI_FilterStates;

    wiTGnChanlMimoChannel_t MimoChannel;

}   wiTGnChanl_State_t;

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exportable Functions)
//=================================================================================================
wiStatus wiTGnChanl_Init();
wiStatus wiTGnChanl_Close();

wiStatus wiTGnChanl_CloseAllThreads(int FirstThread);
wiStatus wiTGnChanl_WriteConfig(wiMessageFn MsgFunc);
wiTGnChanl_State_t *wiTGnChanl_State(void);

wiStatus wiTGnChanl(wiInt Nx, wiComplex *X[], wiInt Nr, wiComplex *R[], wiInt N_Tx, wiInt N_Rx, wiReal Fs);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------
