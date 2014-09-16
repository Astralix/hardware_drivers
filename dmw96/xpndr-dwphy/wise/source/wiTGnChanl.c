//--< wiTgnChanl.c >-------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE TGn Channel Model
//  Copyright 2006-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wiTGnChanl.h"
#include "wiChanl.h"
#include "wiMath.h"
#include "wiRnd.h"
#include "wiUtil.h"
#include "wiParse.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiTGnChanl_State_t *LocalState[WISE_MAX_THREADS] = {NULL};
static wiTGnChanl_Param_t  Param = {0};

static const int TGnChanlFilterOrder = 7; // complex filter order (must be 7)
static const double pi = 3.14159265358979323846;

//=================================================================================================
//--  MACRO DEFINITIONS
//=================================================================================================
#ifndef min
#define min(a,b) (((a)<(b))? (a):(b))
#define max(a,b) (((a)>(b))? (a):(b))
#endif

#ifndef Inf
#define Inf 1e20
#endif

#ifndef log10
#define log10(x) (log(x)/log(10.0))
#endif

#ifndef SQR
#define SQR(x)   ((x)*(x))
#endif

#ifndef DEG2RAD
#define DEG2RAD(x)   ((x)*pi/180.0)
#endif

#ifndef RAD2DEG
#define RAD2DEG(x)   ((x)*180.0/pi)
#endif

#ifndef dB2Linear
#define dB2Linear(x) (((x) > -Inf)? (pow(10.0, 0.1*(x))) : 0)
#endif

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define STATUS(Status)  WICHK((Status))
#define XSTATUS(Status) if (STATUS(Status)<0) return WI_ERROR_RETURNED;

//=================================================================================================
//--  FUNCTION PROTOTYPES
//=================================================================================================
wiStatus wiTGnChanl_ParseLine(wiString text);

wiStatus wiTGnChanl_AllocateMemory(int NumTxAnt, int NumRxAnt);
wiStatus wiTGnChanl_FreeMemory(wiTGnChanl_State_t *pState);
wiStatus wiTGnChanl_MIMO_Channel(int *Np, wiCMat *H, double *ChannelTimes, int Length_EachH, double Ts);
wiStatus wiTGnChanl_IEEE_80211n_Cases();
wiStatus wiTGnChanl_InitFadingTime(wiCMat *FadingMatrixTime, wiCMat *FilterStatesOut, int FadingLength, wiCMat *FilterStatesIn);
wiStatus wiTGnChanl_InitMIMO(wiCMat *C, wiCMat *R);

wiStatus wiTGnChanl_Init_Rice(wiComplex RiceMatrix[][WITGNCHANL_MAX_ANTENNAS], int NumberOfTxAntennas, double SpacingTx, double AoD_LOS_Tx,
                                        int NumberOfRxAntennas, double SpacingRx, double AoA_LOS_Rx);
wiStatus wiTGnChanl_ComplexFilter(wiComplex *y, wiComplex *StateOut, int ord, const wiComplex *b, 
                                  const wiComplex *a, int length, wiComplex *x, wiComplex *StateIn);
wiStatus wiTGnChanl_ComplexTimeInterpolation( wiComplex Y[], int LT, double T[], wiComplex pValue[], int LX, double X[]);

wiStatus wiTGnChanl_Correlation(wiCMat *R, double Q[], double SigmaDeg[], int N, double spacing, int ClusterNumber, 
                                double AmplitudeCluster[], double PhiDeg[], double AS_deg[], double DeltaPhiDeg[]);
void     wiTGnChanl_StatLaplacian(double *composite_AoA_deg, double *composite_SigmaDeg, int NumClusters, 
                                         double *Q, double *AoA_deg, double *SigmaDeg);
wiStatus wiTGnChanl_NormalizationLaplacian(double Q[],   double SigmaDeg[], int ClusterNumber, 
                                            double AmplitudeCluster[], double AS_deg[], double DeltaPhiDeg[] );

void     wiTGnChanl_Rxx_Laplacian(double Rxxtmp[], int N, double d_norm[], double PhiDegk, double SigmaDegk, double DeltaPhiDegk);
void     wiTGnChanl_Rxy_Laplacian(double Rxytmp[], int N, double d_norm[], double PhiDegk, double SigmaDegk, double DeltaPhiDegk);
wiStatus wiTGnChanl_FluorescentLights(wiCMat *Matrix, double *Time_s);
wiStatus wiTGnChanl_Resample(int Nx, wiComplex *x, int Ny, wiComplex *y, wiReal up, wiReal down);
wiStatus wiTGnChanl_InitializeThread(void);

//=================================================================================================
//--  LOOKUP TABLES
//=================================================================================================
static const wiReal AS2sigma_Laplacian90[130][2] = {
        { 0.00000,  0},{ 0.10000,0.1},{ 0.20000,0.2},{ 0.30000,0.3},{ 0.40000,0.4},{ 0.50000,0.5},{ 0.60000,0.6},
        { 0.70000,0.7},{ 0.80000,0.8},{ 0.90000,0.9},{ 1.00000,  1},{ 2.00000,  2},{ 3.00000,  3},{ 4.00000,  4},
        { 5.00000,  5},{ 6.00000,  6},{ 6.99999,  7},{ 7.99993,  8},{ 8.99963,  9},{ 9.99859, 10},{10.99587, 11},
        {11.98992, 12},{12.97862, 13},{13.95940, 14},{14.92939, 15},{15.88563, 16},{16.82520, 17},{17.74541, 18},
        {18.64388, 19},{19.51858, 20},{20.36783, 21},{21.19037, 22},{21.98528, 23},{22.75194, 24},{23.49006, 25},
        {24.19958, 26},{24.88066, 27},{25.53363, 28},{26.15897, 29},{26.75730, 30},{27.32929, 31},{27.87573, 32},
        {28.39742, 33},{28.89522, 34},{29.37000, 35},{29.82265, 36},{30.25405, 37},{30.66510, 38},{31.05666, 39},
        {31.42957, 40},{31.78469, 41},{32.12281, 42},{32.44473, 43},{32.75120, 44},{33.04295, 45},{33.32069, 46},
        {33.58509, 47},{33.83679, 48},{34.07641, 49},{34.30455, 50},{34.52177, 51},{34.72859, 52},{34.92554, 53},
        {35.11310, 54},{35.29174, 55},{35.46189, 56},{35.62396, 57},{35.77837, 58},{35.92548, 59},{36.06566, 60},
        {36.19923, 61},{36.32654, 62},{36.44787, 63},{36.56352, 64},{36.67378, 65},{36.77889, 66},{36.87910, 67},
        {36.97466, 68},{37.06578, 69},{37.15267, 70},{37.23554, 71},{37.31458, 72},{37.38996, 73},{37.46186, 74},
        {37.53045, 75},{37.59587, 76},{37.65827, 77},{37.71779, 78},{37.77457, 79},{37.82873, 80},{37.88039, 81},
        {37.92966, 82},{37.97666, 83},{38.02147, 84},{38.06421, 85},{38.10497, 86},{38.14382, 87},{38.18087, 88},
        {38.21618, 89},{38.24983, 90},{38.28190, 91},{38.31246, 92},{38.34156, 93},{38.36928, 94},{38.39567, 95},
        {38.42079, 96},{38.44470, 97},{38.46744, 98},{38.48907, 99},{38.50962,100},{38.52916,101},{38.54772,102},
        {38.56534,103},{38.58205,104},{38.59791,105},{38.61294,106},{38.62718,107},{38.64066,108},{38.65341,109},
        {38.66546,110},{38.67684,111},{38.68757,112},{38.69770,113},{38.70722,114},{38.71618,115},{38.72460,116},
        {38.73249,117},{38.73988,118},{38.74679,119},{38.75323,120} };

static const wiReal AS2sigma_Laplacian180[130][2] = {
        { 0.00000,  0},{ 0.10000,0.1},{ 0.20000,0.2},{ 0.30000,0.3},{ 0.40000,0.4},{ 0.50000,0.5},{ 0.60000,0.6},
        { 0.70000,0.7},{ 0.80000,0.8},{ 0.90000,0.9},{ 1.00000,  1},{ 2.00000,  2},{ 3.00000,  3},{ 4.00000,  4},
        { 5.00000,  5},{ 6.00000,  6},{ 7.00000,  7},{ 8.00000,  8},{ 9.00000,  9},{10.00000, 10},{11.00000, 11},
        {12.00000, 12},{13.00000, 13},{13.99998, 14},{14.99995, 15},{15.99986, 16},{16.99966, 17},{17.99926, 18},
        {18.99851, 19},{19.99722, 20},{20.99511, 21},{21.99185, 22},{22.98702, 23},{23.98013, 24},{24.97065, 25},
        {25.95797, 26},{26.94143, 27},{27.92038, 28},{28.89409, 29},{29.86187, 30},{30.82301, 31},{31.77683, 32},
        {32.72264, 33},{33.65982, 34},{34.58776, 35},{35.50589, 36},{36.41371, 37},{37.31073, 38},{38.19654, 39},
        {39.07076, 40},{39.93305, 41},{40.78314, 42},{41.62078, 43},{42.44579, 44},{43.25801, 45},{44.05732, 46},
        {44.84365, 47},{45.61694, 48},{46.37718, 49},{47.12439, 50},{47.85860, 51},{48.57987, 52},{49.28829, 53},
        {49.98396, 54},{50.66699, 55},{51.33753, 56},{51.99570, 57},{52.64168, 58},{53.27563, 59},{53.89771, 60},
        {54.50813, 61},{55.10706, 62},{55.69469, 63},{56.27123, 64},{56.83687, 65},{57.39181, 66},{57.93625, 67},
        {58.47041, 68},{58.99448, 69},{59.50866, 70},{60.01316, 71},{60.50818, 72},{60.99391, 73},{61.47056, 74},
        {61.93832, 75},{62.39737, 76},{62.84791, 77},{63.29013, 78},{63.72421, 79},{64.15032, 80},{64.56865, 81},
        {64.97937, 82},{65.38265, 83},{65.77866, 84},{66.16756, 85},{66.54951, 86},{66.92466, 87},{67.29318, 88},
        {67.65521, 89},{68.01089, 90},{68.36038, 91},{68.70380, 92},{69.04130, 93},{69.37301, 94},{69.69906, 95},
        {70.01958, 96},{70.33469, 97},{70.64450, 98},{70.94915, 99},{71.24874,100},{71.54338,101},{71.83319,102},
        {72.11827,103},{72.39872,104},{72.67465,105},{72.94616,106},{73.21333,107},{73.47628,108},{73.73507,109},
        {73.98982,110},{74.24059,111},{74.48748,112},{74.73057,113},{74.96994,114},{75.20567,115},{75.43783,116},
        {75.66650,117},{75.89175,118},{76.11366,119},{76.33229,120} };


// ================================================================================================
// FUNCTION  : wiTGnChanl()
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level routine for the TGn channel model to be called from wiChanl.c
// Parameters: pChanlState -- pointer to state structure from wiChanl
//             Nx
// ------------------------------------------------------------------------------------------------
wiStatus wiTGnChanl(wiInt Nx, wiComplex *Xin[], wiInt Nr, wiComplex *R[], wiInt NumTx, wiInt NumRx, wiReal Fs)
{
    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    double *SymbolTimes, ChannelTimes[WITGNCHANL_MAX_PATHS];
    double upsample, downsample;
    int Np, n, k, Nxr, Nrr, iRx, iTx;
    wiComplex *Rresample, *X[WICHANL_TXMAX];

    if (wiProcess_ThreadIndex() != 0)
    {
        if (!pState) {
            XSTATUS( wiTGnChanl_InitializeThread() );
        }
        pState = wiTGnChanl_State();
    }

    upsample   = Param.Fs;
    downsample = Fs;

    // Allocating memory for the TGnChannel state arrays
    XSTATUS(wiTGnChanl_AllocateMemory(NumTx, NumRx));

    Nxr = (int)floor((Nx*upsample + 0.1)/downsample); //Length of the upsampled data sequence
    Nrr = (int)floor((Nr*upsample + 0.1)/downsample); //Length of the upsampled output sequence

    XSTATUS( wiTGnChanl_MIMO_Channel(&Np, &pState->H, ChannelTimes, Nrr, 1.0/Param.Fs) );// Np is the number of paths

    GET_WIARRAY( Rresample,   pState->Rresample,   Nrr, wiComplex );
    GET_WIARRAY( SymbolTimes, pState->SymbolTimes, Nrr, wiReal );

    for (n=0; n<Nrr; n++) SymbolTimes[n] = n / Param.Fs;
    
    // -------------------------------
    // Resample the transmit signal(s)
    // -------------------------------
    for (iTx = 0; iTx < NumTx; iTx++)
    {
        GET_WIARRAY( X[iTx], pState->X[iTx], Nxr, wiComplex );
        XSTATUS(wiTGnChanl_Resample(Nx, Xin[iTx], Nxr, X[iTx], upsample, downsample) );
    }
    // -----------------------
    // Clear the output arrays
    // -----------------------
    for (iRx = 0; iRx < NumRx; iRx++) 
        memset(R[iRx], 0, Nr * sizeof(wiComplex));

    // Compute the output signal (with resampling)
    // R[iRx] += H[iRx][iTx] * X[iTx]
    for (iRx = 0; iRx < NumRx; iRx++)
    {
        memset( Rresample, 0, Nrr * sizeof(wiComplex)); // clear Rresample

        for (iTx = 0; iTx < NumTx; iTx++)
        {
            wiComplex *x = X[iTx];

            for (k=0; k<Np; k++)
            {
                wiComplex *h = pState->H.val[k*NumTx*NumRx + iTx*NumRx + iRx];
                int kTick = (int)floor(ChannelTimes[k]*(Param.Fs) + 0.1);

                for (n=0; n<Nrr; n++)
                {   
                    double PathTime = SymbolTimes[n] - ChannelTimes[k];
                    
                    if ((PathTime < 0) || (PathTime > SymbolTimes[Nxr-1]))
                        ;
                    else
                    {
                        int m = n - kTick;
                        Rresample[n].Real += (x[m].Real * h[n].Real) - (x[m].Imag * h[n].Imag);
                        Rresample[n].Imag += (x[m].Real * h[n].Imag) + (x[m].Imag * h[n].Real);
                    }
                }
            }
        }
        wiTGnChanl_Resample(Nrr, Rresample, Nr, R[iRx], downsample, upsample);
    }
    return WI_SUCCESS;
}
// end of wiTGnChanl()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Init
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize the TGn Channle Model
// Parameters: 
// ================================================================================================
wiStatus wiTGnChanl_Init()
{
    wiTGnChanl_State_t *pState;
    
    if (LocalState[0]) return WI_ERROR_MODULE_ALREADY_INIT;

    // ---------------------------------------
    // Allocate memory for the state structure
    // ---------------------------------------
    pState = LocalState[0] = (wiTGnChanl_State_t *)wiCalloc(1, sizeof(wiTGnChanl_State_t));
    if (!LocalState[0]) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

   // ----------------
    // Default Settings
    // ----------------
    Param.Connection            = WITGNCHANL_DOWNLINK; // Direction of connection
    Param.Fs                    = 100e6;  // 100 MHz sample rate in core TGn channel
    Param.TxRxDistance          = 20.0;   // to be changed in wiTGnChanl_MIMO_Channel, based on pState->NLOS
    Param.CarrierFrequencyHz    = 5.25e9; // carrier frequency
    Param.SpacingTx             = 0.5;    // Antenna configuration at Tx
    Param.SpacingRx             = 0.5;    // Antenna configuration at Rx
    Param.Model                 = 'D';    // IEEE 802.11 case to be simulated
    Param.fDoppler              = 5.8333; // 1.2 km/h @ 5.25 GHz
    Param.Fluorescent.xMean     = 0.0203; // see IEEE 802.11-03/940r4 section 4.7.3
    Param.Fluorescent.xStdDev   = 0.0107; // see IEEE 802.11-03/940r4 section 4.7.3
    Param.Fluorescent.PowerLineFrequencyHz = 60.0;   // power line frequency

    XSTATUS(wiParse_AddMethod(wiTGnChanl_ParseLine));

    return WI_SUCCESS;
}
// end of wiTGnChanl_Init

// ================================================================================================
// FUNCTION  : wiTGnChanl_InitializeThread
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTGnChanl_InitializeThread(void)
{
    int ThreadIndex = wiProcess_ThreadIndex();

    if (LocalState[ThreadIndex] != NULL) return STATUS(WI_ERROR); // already allocated

    LocalState[ThreadIndex] = (wiTGnChanl_State_t *)wiCalloc(1, sizeof(wiTGnChanl_State_t));
    if (!LocalState[ThreadIndex]) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    return WI_SUCCESS;
}
// end of wiTGnChanl_InitializeThread()

// ================================================================================================
// FUNCTION  : wiTGnChanl_CloseAllThreads
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTGnChanl_CloseAllThreads(int FirstThread)
{
    int n;

    for (n = FirstThread; n < WISE_MAX_THREADS; n++)
    {
        XSTATUS( wiTGnChanl_FreeMemory(LocalState[n]) );
        WIFREE( LocalState[n] );
    }
    return WI_SUCCESS;
}
// end of wiTGnChanl_CloseAllThreads()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Close()
// ------------------------------------------------------------------------------------------------
// Purpose   : Clean-up memory and shut down this module
// ================================================================================================
wiStatus wiTGnChanl_Close(void)
{
    if (!wiTGnChanl_State()) return WI_ERROR_MODULE_NOT_INITIALIZED;
    
    XSTATUS( wiTGnChanl_CloseAllThreads(0) )
    XSTATUS(wiParse_RemoveMethod(wiTGnChanl_ParseLine));

    return WI_SUCCESS;
}
// end of wiChanl_Close()

// ================================================================================================
// FUNCTION  : wiTGnChanl_FreeMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTGnChanl_FreeMemory(wiTGnChanl_State_t *pState)
{
    int i;
    if (!pState) return WI_SUCCESS;

    if (pState->RRx)
    {
        for (i=0; i<WITGNCHANL_MAX_PATHS; i++)
        {
            wiCMatFree(&(pState->RTx[i]));
            wiCMatFree(&(pState->RRx[i]));
        }
    }
    if (pState->CSI_FilterStates.val) wiCMatFree(&pState->CSI_FilterStates);
    WIFREE_ARRAY( pState->Rresample );
    WIFREE_ARRAY (pState->SymbolTimes );
    for (i=0; i<WICHANL_TXMAX; i++)
    {
        WIFREE_ARRAY( pState->X[i] );
    }
    wiCMatFree( &pState->MimoChannel.C );
    wiCMatFree( &pState->MimoChannel.R );
    wiCMatFree( &pState->MimoChannel.DummyMatrix );
    wiCMatFree( &pState->MimoChannel.FadingMatrix );
    wiCMatFree( &pState->MimoChannel.FilterStatesIn );
    wiCMatFree( &pState->MimoChannel.FilterStatesOut );
    wiCMatFree( &pState->MimoChannel.OversampledFadingMatrix );
    WIFREE_ARRAY( pState->MimoChannel.FadingTime_s );
    WIFREE_ARRAY( pState->MimoChannel.RicePhasor );
    WIFREE_ARRAY( pState->MimoChannel.EachH );
    WIFREE_ARRAY( pState->MimoChannel.HTime_s );
    WIFREE_ARRAY( pState->MimoChannel.OversampledFadingTime_s );
    WIFREE_ARRAY( pState->MimoChannel.pdp_linear );
    WIFREE_ARRAY( pState->MimoChannel.RiceVectorLOS );
    WIFREE_ARRAY( pState->MimoChannel.RiceVectorNLOS );

    WIFREE_ARRAY( pState->Noise2Filter );
    WIFREE_ARRAY( pState->ModulationFunction );
    
    wiCMatFree( &pState->H );
    wiCMatFree( &pState->tempC );
    wiCMatFree( &pState->tempCT );
    wiCMatFree( &pState->tempK );
    wiCMatFree( &pState->NormalizationLaplacian.A );
    wiCMatFree( &pState->NormalizationLaplacian.B );
    wiCMatFree( &pState->FluorescentModulationMatrix );

    return WI_SUCCESS;
}
// end of wiTGnChanl_FreeMemory()
 
// ================================================================================================
// FUNCTION  : wiTGnChanl_AllocateMemory()
// ------------------------------------------------------------------------------------------------
// Purpose   : Allocate memory for elements for the local state structure
// ================================================================================================
wiStatus wiTGnChanl_AllocateMemory(int NumTxAnt, int NumRxAnt)
{
    wiTGnChanl_State_t *pState = wiTGnChanl_State();
    int i;
    
    pState->NumberOfTxAntennas = NumTxAnt;
    pState->NumberOfRxAntennas = NumRxAnt;

    // RRx and RTx: Array[NumPaths] of wiCMat matrices
    for (i=0; i<WITGNCHANL_MAX_PATHS; i++)
    {
        XSTATUS( wiCMatReInit(&(pState->RTx[i]), NumTxAnt, NumTxAnt) );
        XSTATUS( wiCMatReInit(&(pState->RRx[i]), NumRxAnt, NumRxAnt) );
    }

    return WI_SUCCESS;
}
// end of wiTGnChanl_AllocateMemory()
 
// ================================================================================================
// FUNCTION  : wiTGnChanl_State()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return a pointer to the internal state structure
// ================================================================================================
wiTGnChanl_State_t *wiTGnChanl_State(void)
{
    return LocalState[wiProcess_ThreadIndex()];
}
// end of wiTGnChanl_State()

// ================================================================================================
// FUNCTION  : wiTGnChanl_MIMO_Channel()
// ------------------------------------------------------------------------------------------------
// Purpose   : Core computation of MIMO channel
// Parameters: 
// ------------------------------------------------------------------------------------------------
wiStatus wiTGnChanl_MIMO_Channel(wiInt *Np, wiCMat *H, double *ChannelTimes, int Length_EachH, double SamplingTime_s)
{
    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    wiTGnChanlMimoChannel_t *p = &(pState->MimoChannel);

    const  double f_D_norm = 1/300.0; // normalized Doppler spread (tied to hard-coded filters)
    double SimulationLengthInCoherenceTimes;
    double SamplingRateHz, FadingSamplingFrequencyHz, FadingSamplingTime_s;
    double RunningTimeInstant_s = 0.0, FadingOffset_s = 0.0;   
    double SumPDP;
    int NumberOfIterations, FadingNumberOfIterations, OversampledFadingNumberOfIterations, NumberOfSamplesPerIteration, row, col;
    int LengthRunningTime_s;
    int k, i, j, ii, jj, NumAntRxTx;
    wiReal    *pdp_linear, *RiceVectorNLOS, *FadingTime_s, *OversampledFadingTime_s, *HTime_s;
    wiComplex *RiceVectorLOS, *RicePhasor, *EachH;

    Param.TxRxDistance = Param.LOS ? 1 : 100; // LOS vs. NLOS
    SimulationLengthInCoherenceTimes = 100.0; // Simulation length, in coherence times
    SamplingRateHz = 1/SamplingTime_s; // Sampling rate

    // Definition of time scales
    FadingSamplingFrequencyHz = Param.fDoppler / f_D_norm;
    FadingSamplingTime_s      = Param.fDoppler ? 1/FadingSamplingFrequencyHz : 1.0E+12;
    
    // This should be chosen carefully!! It determines the period of channel simulation
    FadingNumberOfIterations = (int)(Length_EachH * SamplingTime_s / FadingSamplingTime_s) + 2; //512;
    OversampledFadingNumberOfIterations = 10 * (FadingNumberOfIterations - 1) +1 ;
    //////////////////////////////////////
    
    LengthRunningTime_s = (int)(FadingNumberOfIterations * FadingSamplingTime_s / SamplingTime_s);

    //Length_EachH = (int)(LengthRunningTime_s);
    // Check for aliasing of the fluorescent effect
    // Highest harmonic                             = 10*PowerLineFrequencyHz
    // Nyquist frequency of the first interpolation = 1/2 *(10*FadingSamplingFrequencyHz)
    if ((10*Param.Fluorescent.PowerLineFrequencyHz) > (5*FadingSamplingFrequencyHz))
    {
        wiPrintf("Warning! Aliasing of the fluorescent effect!\n");
        wiPrintf("Highest harmonic falls beyond Nyquist frequency of the fading samples!\n");
    }
    
    XSTATUS(wiTGnChanl_IEEE_80211n_Cases()); // Get the specifications of the channel type

    NumberOfIterations = 1;//(int)((SimulationLengthInCoherenceTimes/f_D_Hz)/(FadingNumberOfIterations*FadingSamplingTime_s));
    NumberOfSamplesPerIteration = (int)((FadingNumberOfIterations*FadingSamplingTime_s)/SamplingTime_s);

    // Initialization of fading filter state________________________________
    // "Dummy" call in order to fill the filter states with realistic values.
    // Filters have a very long transient and steady state is achieved after 800-1000 filtered 
    // samples. Fading samples generated during the transient phase should not be used in the 
    // channel model, because their variance is lower than expected.
    row = pState->NumberOfTxAntennas * pState->NumberOfRxAntennas * pState->NumberOfPaths;
    XSTATUS( wiCMatReInit(&p->FilterStatesOut, row, TGnChanlFilterOrder) );
    XSTATUS( wiCMatReInit(&p->FilterStatesIn,  row, TGnChanlFilterOrder) ); // it will become all zeros

    if (!Param.CSI.Hold)
    {
        col = 1000;
        XSTATUS( wiCMatReInit(&p->DummyMatrix, row, col) );
        XSTATUS( wiTGnChanl_InitFadingTime(&p->DummyMatrix, &p->FilterStatesOut, col, &p->FilterStatesIn) );
    }
    else
    {
        col = (int)(Param.CSI.Gap_s / FadingSamplingTime_s); // The number of fading iterations corresponding to Gap_s seconds of delay
        if (col>0)
        {
            XSTATUS(wiCMatReInit(&p->DummyMatrix, row, col));
            XSTATUS(wiTGnChanl_InitFadingTime(&p->DummyMatrix, &p->FilterStatesOut, col, &pState->CSI_FilterStates) );
        }
        else
            XSTATUS( wiCMatCopy(&pState->CSI_FilterStates, &p->FilterStatesOut) );
    }

    // Computation of the correlation matrices, one for each tap
    XSTATUS( wiCMatReInit(&p->C, row, row) );
    XSTATUS( wiCMatReInit(&p->R, row, row) );
    XSTATUS( wiTGnChanl_InitMIMO(&p->C, &p->R) );

    // Memory Allocation
    NumAntRxTx = pState->NumberOfRxAntennas * pState->NumberOfTxAntennas;
    GET_WIARRAY( pdp_linear,     p->pdp_linear,     pState->NumberOfPaths,              wiReal );
    GET_WIARRAY( RiceVectorNLOS, p->RiceVectorNLOS, pState->NumberOfPaths * NumAntRxTx, wiReal );
    GET_WIARRAY( RiceVectorLOS,  p->RiceVectorLOS,  pState->NumberOfPaths * NumAntRxTx, wiComplex );

    // Computation of the power delay profile of the (LOS+NLOS) power
    // The PDP is defined as the time dispersion of the NLOS power. The addition
    // of the LOS component modifies the time dispersion of the total power.
    SumPDP = 0;
    for (k=0; k<pState->NumberOfPaths; k++)
    {
        pdp_linear[k] = dB2Linear(pState->ModelData.PDPdB[0][k]) * (1 + dB2Linear(pState->K_factor_dB[k]));
        SumPDP += pdp_linear[k];
    }
    for (k=0; k<pState->NumberOfPaths; k++) // Normalisation of the power delay profile of the (LOS+NLOS) power
        pdp_linear[k] /= SumPDP;

    for (k=0; k<pState->NumberOfPaths; k++)
    {
        double RiceFactor = dB2Linear(pState->K_factor_dB[k]);
        double c = sqrt(pdp_linear[k]) * sqrt(RiceFactor / (RiceFactor + 1));
        int n = k * NumAntRxTx;

        for (i=0; i<pState->NumberOfTxAntennas; i++)
        {
            for (j=0; j<pState->NumberOfRxAntennas; j++)
            {
                RiceVectorLOS[n] = wiComplexScaled(pState->RiceMatrix[j][i], c);
                RiceVectorNLOS[n] = sqrt(1 / (RiceFactor + 1));
                n++;
            }
        }
    }

    //Initialize the matrices
    XSTATUS( wiCMatReInit(H, row, Length_EachH*NumberOfIterations) );
    XSTATUS( wiCMatReInit(&p->FadingMatrix, row, FadingNumberOfIterations) );
    XSTATUS( wiCMatReInit(&p->OversampledFadingMatrix, row, OversampledFadingNumberOfIterations) );
    XSTATUS( wiCMatReInit(&p->DummyMatrix, row, FadingNumberOfIterations) );  

    GET_WIARRAY( FadingTime_s,            p->FadingTime_s,            FadingNumberOfIterations,            wiReal );
    GET_WIARRAY( OversampledFadingTime_s, p->OversampledFadingTime_s, OversampledFadingNumberOfIterations, wiReal );
    GET_WIARRAY( RicePhasor,              p->RicePhasor,              FadingNumberOfIterations,            wiComplex );
    GET_WIARRAY( EachH,                   p->EachH,                   Length_EachH,                        wiComplex );
    GET_WIARRAY( HTime_s,                 p->HTime_s,                 Length_EachH,                        wiReal );

    RunningTimeInstant_s = 0;
    FadingOffset_s = 0;

    for (i=0; i<NumberOfIterations; i++)
    {
        // Computation of the matrix of fading coefficients
        XSTATUS( wiTGnChanl_InitFadingTime(&p->DummyMatrix, &p->FilterStatesOut, FadingNumberOfIterations, &p->FilterStatesOut) );

        // Spatial correlation of the fading matrix
        XSTATUS( wiCMatMul(&p->FadingMatrix, &p->C, &p->DummyMatrix) );

        // Calculate the mean power of the channel coefficients and check if it matches the PathLoss + Shadowing gains
        // Normalization of the correlated fading processes
        for (ii = 0; ii < pState->NumberOfPaths; ii++)
        {
            double sqrt_pdp_linear = sqrt(pdp_linear[ii]);

            for (jj = 0; jj < NumAntRxTx; jj++)
                for (k = 0; k < FadingNumberOfIterations; k++)
                {
                    p->FadingMatrix.val[ii*NumAntRxTx+jj][k].Real *= sqrt_pdp_linear;
                    p->FadingMatrix.val[ii*NumAntRxTx+jj][k].Imag *= sqrt_pdp_linear;
                }
        }
        // Calculation of the Rice phasor
        // AoA/AoD hard-coded to 45 degrees; Time reference: FadingTime
        for (k = 0; k < FadingNumberOfIterations; k++)
        {
            FadingTime_s[k] = FadingOffset_s + k * FadingSamplingTime_s;
            RicePhasor[k] = wiComplexExp(2*pi*Param.fDoppler*cos(pi/4) * FadingTime_s[k]);
        }
        for (k = 0; k < OversampledFadingNumberOfIterations; k++)
            OversampledFadingTime_s[k] = FadingOffset_s + k*FadingSamplingTime_s / 10;

        for (k = 0; k < Length_EachH; k++)
            HTime_s[k] = RunningTimeInstant_s + k*SamplingTime_s;

        for (ii = 0; ii < row; ii++)
        {
            for (jj = 0; jj < FadingNumberOfIterations; jj++)
            {
                p->FadingMatrix.val[ii][jj].Real *= RiceVectorNLOS[ii];
                p->FadingMatrix.val[ii][jj].Imag *= RiceVectorNLOS[ii];

                p->FadingMatrix.val[ii][jj].Real += RiceVectorLOS[ii].Real * RicePhasor[jj].Real;
                p->FadingMatrix.val[ii][jj].Real -= RiceVectorLOS[ii].Imag * RicePhasor[jj].Imag;
                p->FadingMatrix.val[ii][jj].Imag += RiceVectorLOS[ii].Real * RicePhasor[jj].Imag;
                p->FadingMatrix.val[ii][jj].Imag += RiceVectorLOS[ii].Imag * RicePhasor[jj].Real;
            }
        }
        // ----------------------------------------------------------------------------------
        // Doppler Due to Fluorescent Lights : Oversample, Add Fluorescent Effect, Downsample
        // ----------------------------------------------------------------------------------
        if (Param.Fluorescent.Enabled)
        {
            for (k=0; k<row; k++) { // 10x oversampling
                wiTGnChanl_ComplexTimeInterpolation( p->OversampledFadingMatrix.val[k], FadingNumberOfIterations, 
                                                     FadingTime_s, p->FadingMatrix.val[k], 
                                                     OversampledFadingNumberOfIterations, OversampledFadingTime_s );
            }
            wiTGnChanl_FluorescentLights(&p->OversampledFadingMatrix, OversampledFadingTime_s);
    
            for (k=0; k<row; k++) { // downsampling
                wiTGnChanl_ComplexTimeInterpolation( p->FadingMatrix.val[k], OversampledFadingNumberOfIterations, 
                                                     OversampledFadingTime_s, p->OversampledFadingMatrix.val[k], 
                                                     FadingNumberOfIterations, FadingTime_s );
            }
        }
        // Large-scale fading << removed...not used for SNR type modeling with IEEE Channel Models >>

        // Deriving H at sampling rate 1/NumberOfHSamples;
        for (k=0; k<row; k++)
        {
            wiTGnChanl_ComplexTimeInterpolation(EachH, FadingNumberOfIterations, FadingTime_s, 
                                                p->FadingMatrix.val[k], Length_EachH, HTime_s);
            for (j=0; j<Length_EachH; j++)
                H->val[k][j + i*Length_EachH] = EachH[j];
        }
        // Update of book-keeping variables
        FadingOffset_s       = FadingTime_s[FadingNumberOfIterations-1]+FadingSamplingTime_s;
        RunningTimeInstant_s = LengthRunningTime_s*SamplingTime_s;
    }
    // ============================================================================
    // Structure of H:
    // t=0..NumTxAnt,   r=0..NumRxAnt,   p=0..NumPaths,   I=1..NumIterations
    // The channel coefficient between Tx Ant t and Rx Ant r in path p and iteration I is
    //   H->val[p*NumTxAnt*NumRxAnt + t*NumRxAnt + r][I]
    // ============================================================================*/
    XSTATUS( wiCMatReInit(&pState->CSI_FilterStates, p->FilterStatesOut.row, p->FilterStatesOut.col) );
    XSTATUS( wiCMatCopy(&p->FilterStatesOut, &pState->CSI_FilterStates) );

    for (i = 0; i < pState->NumberOfPaths; i++)
        ChannelTimes[i] = pState->ModelData.PDPdB[1][i];

    if (Np) *Np = pState->NumberOfPaths;
    return WI_SUCCESS;
}
// end of wiTGnChanl_MIMO_Channel()

// ================================================================================================
// FUNCTION  : wiTGnChanl_InitFadingTime()
// ------------------------------------------------------------------------------------------------
// Purpose   : 
// row = pState->NumberOfTxAntennas * pState->NumberOfRxAntennas * pState->NumberOfPaths
// col = FadingLength
// *FadingMatrixTime is a rowXcol Complex Matrix
// *FilterStatesOut and *FilterStatesIn are rowXFilter_Order Complex Matrices
// ------------------------------------------------------------------------------------------------
wiStatus wiTGnChanl_InitFadingTime(wiCMat *FadingMatrixTime,  wiCMat *FilterStatesOut,
                                   int FadingLength, wiCMat *FilterStatesIn)
{
    const wiComplex numB[] = { { 2.785150513156437e-004, 0.0}, {-1.289546865642764e-003, 0.0},
                               { 2.616769929393532e-003, 0.0}, {-3.041340177530218e-003, 0.0},
                               { 2.204942394725852e-003, 0.0}, {-9.996063557790929e-004, 0.0},
                               { 2.558709319878001e-004, 0.0}, {-2.518824257145505e-005, 0.0} };
    const wiComplex denB[] = { { 1.000000000000000e+000, 0.0}, {-5.945307133332568e+000, 0.0},
                               { 1.481117656568614e+001, 0.0}, {-1.985278212976179e+001, 0.0},
                               { 1.520727030904915e+001, 0.0}, {-6.437156952794267e+000, 0.0},
                               { 1.279595585941577e+000, 0.0}, {-6.279622049460144e-002, 0.0} };

    const wiComplex numS[] = { { 7.346653645411014e-002,  0.0},                    
                               {-2.692961866120396e-001, -3.573876752780644e-002},
                               { 5.135568284321728e-001,  1.080031147197969e-001}, 
                               {-5.941696436704196e-001, -1.808579163219880e-001},
                               { 4.551285303843579e-001,  1.740226068499579e-001}, 
                               {-2.280789945309740e-001, -1.111510216362428e-001},
                               { 7.305274345216785e-002,  4.063385976397657e-002}, 
                               {-1.281829045132903e-002, -9.874529878635624e-003} };
    const wiComplex denS[] = { { 1.000000000000000e+000,  0.0},                    
                               {-4.803186161814090e+000, -6.564741337993197e-001},
                               { 1.083699602445439e+001,  2.655291948209460e+000}, 
                               {-1.461669273797422e+001, -5.099798708358494e+000},
                               { 1.254720870662511e+001,  5.723905805435745e+000}, 
                               {-6.793962089799931e+000, -3.887940726966912e+000},
                               { 2.107762007599925e+000,  1.503178356895452e+000}, 
                               {-2.777250532557400e-001, -2.392692168502482e-001} };

    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    int row = pState->NumberOfTxAntennas * pState->NumberOfRxAntennas * pState->NumberOfPaths;
    int k;

    wiComplex *Noise2Filter;
    GET_WIARRAY( Noise2Filter, pState->Noise2Filter, FadingLength, wiComplex );

    switch (pState->FadingType)
    {
        case WITGNCHANL_FADING_BELL_SHAPE:
        {
            for (k=0; k<row; k++)
            {
                wiRnd_ComplexNormal(FadingLength, Noise2Filter, 1);
                if (!FadingMatrixTime->val[k]) return STATUS(WI_ERROR_PARAMETER1);

                wiTGnChanl_ComplexFilter( FadingMatrixTime->val[k], FilterStatesOut->val[k], // Outputs
                                          TGnChanlFilterOrder, numB, denB, FadingLength, Noise2Filter, 
                                          FilterStatesIn->val[k]);
            }
            break;
        }
        case WITGNCHANL_FADING_BELL_SHAPE_SPIKE:
        {
            int SpikePos = 3; //tap with Bell + Spike shape
            int NumAnt   = pState->NumberOfTxAntennas * pState->NumberOfRxAntennas;

            for (k=0; k<row; k++)
            {
                wiRnd_ComplexNormal(FadingLength, Noise2Filter, 1.0);
                
                if ( (k >= (SpikePos-1)*NumAnt) && (k < SpikePos*NumAnt) ) // Bell-shaped fading
                    wiTGnChanl_ComplexFilter( FadingMatrixTime->val[k], FilterStatesOut->val[k], // Outputs
                                              TGnChanlFilterOrder, numS, denS, FadingLength, Noise2Filter, 
                                              FilterStatesIn->val[k]);
                else               // Bell+Spike-shaped fading
                    wiTGnChanl_ComplexFilter( FadingMatrixTime->val[k], FilterStatesOut->val[k], // Outputs
                                              TGnChanlFilterOrder, numB, denB, FadingLength, Noise2Filter, 
                                              FilterStatesIn->val[k]);
            }
            break;
        }
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of wiTGnChanl_InitFadingTime()

// ================================================================================================
// FUNCTION  : wiTGnChanl_InitMIMO()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize the MIMO Channel
// row = pState->NumberOfTxAntennas * pState->NumberOfRxAntennas * pState->NumberOfPaths
// *C and *R are rowXrow Complex Matrices
// ================================================================================================
wiStatus wiTGnChanl_InitMIMO(wiCMat *C, wiCMat *R)
{
    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    int N = pState->NumberOfTxAntennas * pState->NumberOfRxAntennas; // number of antenna connections
    int i;

    XSTATUS( wiCMatReInit(&pState->tempK,  N, N) );
    XSTATUS( wiCMatReInit(&pState->tempC,  N, N) );
    XSTATUS( wiCMatReInit(&pState->tempCT, N, N) );
    XSTATUS( wiCMatSetZero(C) );
    XSTATUS( wiCMatSetZero(R) );

    for (i=0; i<pState->NumberOfPaths; i++)
    {
        XSTATUS( wiCMatKron(&pState->tempK, pState->RTx + i, pState->RRx + i) );
        XSTATUS( wiCMatCholesky (&pState->tempC,  &pState->tempK) );
        XSTATUS( wiCMatConjTrans(&pState->tempCT, &pState->tempC) ); // Conjugate transpose of tempC
        XSTATUS( wiCMatSetSubMtx(R, &pState->tempK,  i*N, i*N, (i+1)*N - 1, (i+1)*N - 1) );
        XSTATUS( wiCMatSetSubMtx(C, &pState->tempCT, i*N, i*N, (i+1)*N - 1, (i+1)*N - 1) );
    }
    return WI_SUCCESS;
}
// end of wiTGnChanl_InitMIMO()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Type_A()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiTGnChanl_Type_A()
{
    #define NumPathsA 1
    #define NumClustersA 1
    
    // Variable definitions and initializations
    static const double PDP_dB[2][NumPathsA] = {   {0} ,  //Average power [dB]
                                      {0} }, //Relative delay (ns)

          PowerPerAngle_dB[NumClustersA][NumPathsA] = {{0}},
          // Tx
          AoD_Tx_deg[NumClustersA][NumPathsA] = {{45}},
          AS_Tx_deg[NumClustersA][NumPathsA] = {{40}},
          // Rx
          AoA_Rx_deg[NumClustersA][NumPathsA] = {{45}},
          AS_Rx_deg[NumClustersA][NumPathsA] = {{40}};
    
    int i;
    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    pState->NumberOfPaths    = NumPathsA;
    pState->NumberOfClusters = NumClustersA;

    pState->d_BP = 5;
    pState->FadingType = WITGNCHANL_FADING_BELL_SHAPE;

    for (i=0; i<pState->NumberOfPaths; i++)
        pState->K_factor_dB[i] = -100;

    for (i=0; i<pState->NumberOfClusters; i++)
    {
        pState->ModelData.PowerPerAngle_dB[i] = PowerPerAngle_dB[i];
        pState->ModelData.AoD_Tx_deg[i] = AoD_Tx_deg[i];
        pState->ModelData.AoA_Rx_deg[i] = AoA_Rx_deg[i];
        pState->ModelData.AS_Tx_deg[i]  = AS_Tx_deg[i];
        pState->ModelData.AS_Rx_deg[i]  = AS_Rx_deg[i];
    }   
    pState->ModelData.PDPdB[0] = PDP_dB[0];
    pState->ModelData.PDPdB[1] = PDP_dB[1];
    
    if (Param.TxRxDistance < pState->d_BP) // LOS conditions
        pState->K_factor_dB[0] = 0; 
}
// end of wiTGnChanl_Type_A()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Type_B()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiTGnChanl_Type_B()
{
    #define NumPathsB 9
    #define NumClustersB 2
    
    // Variable definitions and initializations
    static const double PDP_dB[2][NumPathsB] = { {0, -5.4287, -2.5162, -5.8905, -9.1603, -12.5105, -15.6126, -18.7147, -21.8168} , //Average power [dB]
                                          {0, 10e-9,   20e-9,   30e-9,   40e-9,   50e-9,    60e-9,    70e-9,    80e-9} }; //Relative delay (ns)

    static const double PowerPerAngle_dB[NumClustersB][NumPathsB] = {{   0,   -5.4287,  -10.8574,  -16.2860,  -21.7147,      -Inf,      -Inf,      -Inf,      -Inf},
                                                              {-Inf,      -Inf,   -3.2042,   -6.3063,   -9.4084,  -12.5105,  -15.6126,  -18.7147,  -21.8168} };

           // Tx
    static const double AoD_Tx_deg[NumClustersB][NumPathsB] = {{225.1084, 225.1084, 225.1084, 225.1084, 225.1084, -Inf, -Inf, -Inf, -Inf },
                                                  {-Inf, -Inf, 106.5545, 106.5545, 106.5545, 106.5545, 106.5545, 106.5545, 106.5545} };
    static const double AS_Tx_deg[NumClustersB][NumPathsB] = {{14.4490,   14.4490,   14.4490,   14.4490,   14.4490,      -Inf,      -Inf,      -Inf,      -Inf},
                                                 {-Inf,      -Inf,   25.4311,   25.4311,   25.4311,   25.4311,   25.4311,   25.4311,   25.4311 } };
           // Rx
    static const double AoA_Rx_deg[NumClustersB][NumPathsB] = {{4.3943, 4.3943, 4.3943, 4.3943, 4.3943, -Inf, -Inf, -Inf, -Inf},
                                                  {-Inf, -Inf, 118.4327, 118.4327, 118.4327, 118.4327, 118.4327, 118.4327, 118.4327}};
    static const double AS_Rx_deg[NumClustersB][NumPathsB] = {{14.4699, 14.4699, 14.4699, 14.4699, 14.4699, -Inf, -Inf, -Inf, -Inf},
                                                 {-Inf, -Inf, 25.2566, 25.2566, 25.2566, 25.2566, 25.2566, 25.2566, 25.2566}};
    
    int i;
    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    pState->NumberOfPaths    = NumPathsB;
    pState->NumberOfClusters = NumClustersB;

    pState->d_BP = 5;
    pState->FadingType = WITGNCHANL_FADING_BELL_SHAPE;

    for (i=0; i<pState->NumberOfPaths; i++)
        pState->K_factor_dB[i] = -100;

    for (i=0; i<pState->NumberOfClusters; i++)
    {
        pState->ModelData.PowerPerAngle_dB[i] = PowerPerAngle_dB[i];
        pState->ModelData.AoD_Tx_deg[i] = AoD_Tx_deg[i];
        pState->ModelData.AoA_Rx_deg[i] = AoA_Rx_deg[i];
        pState->ModelData.AS_Tx_deg[i]  = AS_Tx_deg[i];
        pState->ModelData.AS_Rx_deg[i]  = AS_Rx_deg[i];
    }   
    pState->ModelData.PDPdB[0] = PDP_dB[0];
    pState->ModelData.PDPdB[1] = PDP_dB[1];

    if (Param.TxRxDistance < pState->d_BP) // LOS conditions
        pState->K_factor_dB[0] = 0;
}
// end of wiTGnChanl_Type_B()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Type_C()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiTGnChanl_Type_C()
{
    #define NumPathsC 14
    #define NumClustersC 2
    
    // Variable definitions and initializations
    static const double PDP_dB[2][NumPathsC] = {   {0, -2.1715, -4.3429, -6.5144, -8.6859, -10.8574, -4.3899, -6.5614, -8.7329, -10.9043, -13.7147, -15.8862, -18.0577, -20.2291} , //Average power [dB]
                                                {0, 10e-9,   20e-9,   30e-9,   40e-9,   50e-9,    60e-9,   70e-9,   80e-9,   90e-9,    110e-9,   140e-9,   170e-9,   200e-9} }, //Relative delay (ns)

            PowerPerAngle_dB[NumClustersC][NumPathsC] = {{   0, -2.1715, -4.3429, -6.5144, -8.6859, -10.8574, -13.0288, -15.2003, -17.3718, -19.5433,     -Inf,     -Inf,     -Inf,     -Inf},
                                                                            {-Inf,    -Inf,    -Inf,    -Inf,    -Inf,     -Inf,  -5.0288,  -7.2003,  -9.3718, -11.5433, -13.7147, -15.8862, -18.0577, -20.2291}},

            // Tx
            AoD_Tx_deg[NumClustersC][NumPathsC] = {{13.5312, 13.5312, 13.5312, 13.5312, 13.5312, 13.5312, 13.5312, 13.5312, 13.5312, 13.5312, -Inf, -Inf, -Inf, -Inf },
                                                                {-Inf, -Inf, -Inf, -Inf, -Inf, -Inf, 56.4329, 56.4329, 56.4329, 56.4329, 56.4329, 56.4329, 56.4329, 56.4329} },
            AS_Tx_deg[NumClustersC][NumPathsC] = {{24.7897,   24.7897,   24.7897,   24.7897,   24.7897,   24.7897,   24.7897,   24.7897,   24.7897,   24.7897,   -Inf,      -Inf,      -Inf,      -Inf},
                                               {-Inf,     -Inf,    -Inf,     -Inf,    -Inf,     -Inf,    22.5729,   22.5729,   22.5729,   22.5729,   22.5729,   22.5729,   22.5729,   22.5729} },
            // Rx
            AoA_Rx_deg[NumClustersC][NumPathsC] = {{290.3715, 290.3715, 290.3715, 290.3715, 290.3715, 290.3715, 290.3715, 290.3715, 290.3715, 290.3715, -Inf, -Inf, -Inf, -Inf},
                                                                {-Inf, -Inf, -Inf, -Inf, -Inf, -Inf, 332.3754, 332.3754, 332.3754, 332.3754, 332.3754, 332.3754, 332.3754, 332.3754}},
            AS_Rx_deg[NumClustersC][NumPathsC] = {{24.6949, 24.6949, 24.6949, 24.6949, 24.6949, 24.6949, 24.6949, 24.6949, 24.6949, 24.6949, -Inf, -Inf, -Inf, -Inf},
                                               {-Inf, -Inf, -Inf, -Inf, -Inf, -Inf, 22.4530, 22.4530, 22.4530, 22.4530, 22.4530, 22.4530, 22.4530, 22.4530}};
    
    int i;
    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    pState->NumberOfPaths    = NumPathsC;
    pState->NumberOfClusters = NumClustersC;

    pState->d_BP = 5;
    pState->FadingType = WITGNCHANL_FADING_BELL_SHAPE;

    for (i=0; i<pState->NumberOfPaths; i++)
        pState->K_factor_dB[i] = -100;

    for (i=0; i<pState->NumberOfClusters; i++)
    {
        pState->ModelData.PowerPerAngle_dB[i] = PowerPerAngle_dB[i];
        pState->ModelData.AoD_Tx_deg[i] = AoD_Tx_deg[i];
        pState->ModelData.AoA_Rx_deg[i] = AoA_Rx_deg[i];
        pState->ModelData.AS_Tx_deg[i]  = AS_Tx_deg[i];
        pState->ModelData.AS_Rx_deg[i]  = AS_Rx_deg[i];
    }   
    pState->ModelData.PDPdB[0] = PDP_dB[0];
    pState->ModelData.PDPdB[1] = PDP_dB[1];

    if (Param.TxRxDistance < pState->d_BP) // LOS conditions
        pState->K_factor_dB[0] = 0;
}
// end of wiTGnChanl_Type_C()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Type_D()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiTGnChanl_Type_D()
{
    #define NumPathsD 18
    #define NumClustersD 3
    
    // Variable definitions and initializations
    static const double PDP_dB[2][NumPathsD] = {   {0,  -0.9,  -1.7,  -2.6,  -3.5,  -4.3,  -5.2,  -6.1,  -6.9,  -7.8,  -4.7,  -7.3,  -9.9,  -12.5,  -13.7,  -18,  -22.4,  -26.7} , //Average power [dB]
                                                {0,  1e-008,  2e-008,  3e-008,  4e-008,  5e-008,  6e-008,  7e-008,  8e-008,  9e-008,  1.1e-007,  1.4e-007,  1.7e-007,  2e-007,  2.4e-007,  2.9e-007,  3.4e-007,  3.9e-007} }, //Relative delay (ns)

            PowerPerAngle_dB[NumClustersD][NumPathsD] = {{0,  -0.9,  -1.7,  -2.6,  -3.5,  -4.3,  -5.2,  -6.1,  -6.9,  -7.8,  -9.0712,  -11.1991,  -13.7954,  -16.3918,  -19.371,  -23.2017,  -Inf,  -Inf},
                                                                        {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -6.6756,  -9.5729,  -12.1754,  -14.7779,  -17.4358,  -21.9928,  -25.5807,  -Inf}, 
                                                                        {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -18.8433,  -23.2381,  -25.2463,  -26.7} },

            // Tx
            AoD_Tx_deg[NumClustersD][NumPathsD] = {{332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  332.1027,  -Inf,  -Inf},
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  49.384,  49.384,  49.384,  49.384,  49.384,  49.384,  49.384,  -Inf}, 
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  275.9769,  275.9769,  275.9769,  275.9769} },
            AS_Tx_deg[NumClustersD][NumPathsD] = {{27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  27.4412,  -Inf,  -Inf},
                                               {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  32.143,  32.143,  32.143,  32.143,  32.143,  32.143,  32.143,  -Inf},
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  36.8825,  36.8825,  36.8825,  36.8825}},
            // Rx
            AoA_Rx_deg[NumClustersD][NumPathsD] = {{158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  158.9318,  -Inf,  -Inf},
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  320.2865,  320.2865,  320.2865,  320.2865,  320.2865,  320.2865,  320.2865,  -Inf},
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  276.1246,  276.1246,  276.1246,  276.1246}},
            AS_Rx_deg[NumClustersD][NumPathsD] = {{27.758,  27.758,  27.758,  27.758,  27.758,  27.758,  27.758,  27.758,  27.758,  27.758,  27.758,  27.758,  27.758,  27.758,  27.758,  27.758,  -Inf,  -Inf},
                                               {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  31.4672,  31.4672,  31.4672,  31.4672,  31.4672,  31.4672,  31.4672,  -Inf},
                                              {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  37.4179,  37.4179,  37.4179,  37.4179}};
    
    int i;
    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    pState->NumberOfPaths    = NumPathsD;
    pState->NumberOfClusters = NumClustersD;

    pState->d_BP = 10;
    pState->FadingType = WITGNCHANL_FADING_BELL_SHAPE;

    for (i=0; i<pState->NumberOfPaths; i++)
        pState->K_factor_dB[i] = -100;

    for (i=0; i<pState->NumberOfClusters; i++)
    {
        pState->ModelData.PowerPerAngle_dB[i] = PowerPerAngle_dB[i];
        pState->ModelData.AoD_Tx_deg[i] = AoD_Tx_deg[i];
        pState->ModelData.AoA_Rx_deg[i] = AoA_Rx_deg[i];
        pState->ModelData.AS_Tx_deg[i]  = AS_Tx_deg[i];
        pState->ModelData.AS_Rx_deg[i]  = AS_Rx_deg[i];
    }   
    pState->ModelData.PDPdB[0] = PDP_dB[0];
    pState->ModelData.PDPdB[1] = PDP_dB[1];

    if (Param.TxRxDistance < pState->d_BP) // LOS conditions
        pState->K_factor_dB[0] = 3;
}
// end of wiTGnChanl_Type_D()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Type_E()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiTGnChanl_Type_E()
{
    #define NumPathsE 18
    #define NumClustersE 4
    
    // Variable definitions and initializations
    static const double PDP_dB[2][NumPathsE] = {   {-2.5,  -3,  -3.5,  -3.9,  0,  -1.3,  -2.6,  -3.9,  -3.4,  -5.6,  -7.7,  -9.9,  -12.1,  -14.3,  -15.4,  -18.4,  -20.7,  -24.6} , //Average power [dB]
                                                {0,  1e-008,  2e-008,  3e-008,  5e-008,  8e-008,  1.1e-007,  1.4e-007,  1.8e-007,  2.3e-007,  2.8e-007,  3.3e-007,  3.8e-007,  4.3e-007,  4.9e-007,  5.6e-007,  6.4e-007,  7.3e-007} }, //Relative delay (ns)

            PowerPerAngle_dB[NumClustersE][NumPathsE] = { {-2.6,  -3,  -3.5,  -3.9,  -4.5644,  -5.6552,  -6.9752,  -8.2952,  -9.8222,  -11.7855,  -13.9855,  -16.1855,  -18.3855,  -20.5855,  -22.9852,  -Inf,  -Inf,  -Inf},
                                                                            {-Inf,  -Inf,  -Inf,  -Inf,  -1.8681,  -3.2849,  -4.5734,  -5.8619,  -7.192,  -9.9304,  -10.3438,  -14.3537,  -14.7671,  -18.777,  -19.9822,  -22.4464,  -Inf,  -Inf },
                                                                            {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -7.9045,  -9.6852,  -14.2606,  -13.8128,  -18.6038,  -18.1924,  -22.8346,  -Inf,  -Inf,  -Inf },
                                                                            {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -20.6734,  -20.5744,  -20.7,  -24.6} },

            // Tx
            AoD_Tx_deg[NumClustersE][NumPathsE] = { {105.6434,  105.6434,  105.6434,  105.6434,  105.6434,  105.6434,  105.6434,  105.6434,  105.6434,  105.6434,  105.6434,  105.6434,  105.6434,  105.6434,  105.6434,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  293.1199,  293.1199,  293.1199,  293.1199,  293.1199,  293.1199,  293.1199,  293.1199,  293.1199,  293.1199,  293.1199,  293.1199,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  61.972,  61.972,  61.972,  61.972,  61.972,  61.972,  61.972,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  275.764,  275.764,  275.764,  275.764} },
            AS_Tx_deg[NumClustersE][NumPathsE] = { {36.1176,  36.1176,  36.1176,  36.1176,  36.1176,  36.1176,  36.1176,  36.1176,  36.1176,  36.1176,  36.1176,  36.1176,  36.1176,  36.1176,  36.1176,  -Inf,  -Inf,  -Inf },
                                               {-Inf,  -Inf,  -Inf,  -Inf,  42.5299,  42.5299,  42.5299,  42.5299,  42.5299,  42.5299,  42.5299,  42.5299,  42.5299,  42.5299,  42.5299,  42.5299,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  38.0096,  38.0096,  38.0096,  38.0096,  38.0096,  38.0096,  38.0096,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  38.7026,  38.7026,  38.7026,  38.7026} },
            // Rx
            AoA_Rx_deg[NumClustersE][NumPathsE] = { {163.7475,  163.7475,  163.7475,  163.7475,  163.7475,  163.7475,  163.7475,  163.7475,  163.7475,  163.7475,  163.7475,  163.7475,  163.7475,  163.7475,  163.7475,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  251.8792,  251.8792,  251.8792,  251.8792,  251.8792,  251.8792,  251.8792,  251.8792,  251.8792,  251.8792,  251.8792,  251.8792,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  80.024,  80.024,  80.024,  80.024,  80.024,  80.024,  80.024,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  182,  182,  182,  182} },
            AS_Rx_deg[NumClustersE][NumPathsE] = { {35.8768,  35.8768,  35.8768,  35.8768,  35.8768,  35.8768,  35.8768,  35.8768,  35.8768,  35.8768,  35.8768,  35.8768,  35.8768,  35.8768,  35.8768,  -Inf,  -Inf,  -Inf },
                                               {-Inf,  -Inf,  -Inf,  -Inf,  41.6812,  41.6812,  41.6812,  41.6812,  41.6812,  41.6812,  41.6812,  41.6812,  41.6812,  41.6812,  41.6812,  41.6812,  -Inf,  -Inf },
                                              {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  37.4221,  37.4221,  37.4221,  37.4221,  37.4221,  37.4221,  37.4221,  -Inf,  -Inf,  -Inf },
                                              {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  40.3685,  40.3685,  40.3685,  40.3685} };
    
    int i;
    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    pState->NumberOfPaths    = NumPathsE;
    pState->NumberOfClusters = NumClustersE;

    pState->d_BP = 20;
    pState->FadingType = WITGNCHANL_FADING_BELL_SHAPE;

    for (i=0; i<pState->NumberOfPaths; i++)
        pState->K_factor_dB[i] = -100;

    for (i=0; i<pState->NumberOfClusters; i++)
    {
        pState->ModelData.PowerPerAngle_dB[i] = PowerPerAngle_dB[i];
        pState->ModelData.AoD_Tx_deg[i] = AoD_Tx_deg[i];
        pState->ModelData.AoA_Rx_deg[i] = AoA_Rx_deg[i];
        pState->ModelData.AS_Tx_deg[i]  = AS_Tx_deg[i];
        pState->ModelData.AS_Rx_deg[i]  = AS_Rx_deg[i];
    }   
    pState->ModelData.PDPdB[0] = PDP_dB[0];
    pState->ModelData.PDPdB[1] = PDP_dB[1];

    // Rice
    if (Param.TxRxDistance < pState->d_BP)  // LOS conditions
        pState->K_factor_dB[0] = 6;
}
// end of wiTGnChanl_Type_E()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Type_F()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiTGnChanl_Type_F()
{
    #define NumPathsF 18
    #define NumClustersF 6
    
    // Variable definitions and initializations
    static const double PDP_dB[2][NumPathsF] = { {-3.3,  -3.6,  -3.9,  -4.2,  0,  -0.9,  -1.7,  -2.6,  -1.5,  -3,  -4.4,  -5.9,  -5.3,  -7.9,  -9.4,  -13.2,  -16.3,  -21.2 }, //Average power [dB]
                                                {0,  1e-008,  2e-008,  3e-008,  5e-008,  8e-008,  1.1e-007,  1.4e-007,  1.8e-007,  2.3e-007,  2.8e-007,  3.3e-007,  4e-007,  4.9e-007,  6e-007,  7.3e-007,  8.8e-007,  1.05e-006} }, //Relative delay (ns)

            PowerPerAngle_dB[NumClustersF][NumPathsF] = { {-3.3,  -3.6,  -3.9,  -4.2,  -4.6474,  -5.3931,  -6.2931,  -7.1931,  -8.2371,  -9.5793,  -11.0793,  -12.5793,  -14.3586,  -16.7311,  -19.9788,  -Inf,  -Inf,  -Inf },
                                                                            {-Inf,  -Inf,  -Inf,  -Inf,  -1.8242,  -2.8069,  -3.5528,  -4.4528,  -5.3076,  -7.4276,  -7.0894,  -10.3048,  -10.4441,  -13.8374,  -15.7621,  -19.9403,  -Inf,  -Inf },
                                                                            {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -5.796,  -6.7737,  -10.4758,  -9.6417,  -14.1072,  -12.7523,  -18.5033,  -Inf,  -Inf,  -Inf },
                                                                            {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -8.8825,  -13.3195,  -18.7334,  -Inf,  -Inf,  -Inf },
                                                                            {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -12.9472,  -14.2338,  -Inf,  -Inf },
                                                                            {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -16.3,  -21.2} },

            // Tx
            AoD_Tx_deg[NumClustersF][NumPathsF] = { {56.2139,  56.2139,  56.2139,  56.2139,  56.2139,  56.2139,  56.2139,  56.2139,  56.2139,  56.2139,  56.2139,  56.2139,  56.2139,  56.2139,  56.2139,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  183.7089,  183.7089,  183.7089,  183.7089,  183.7089,  183.7089,  183.7089,  183.7089,  183.7089,  183.7089,  183.7089,  183.7089,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  153.0836,  153.0836,  153.0836,  153.0836,  153.0836,  153.0836,  153.0836,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  112.5317,  112.5317,  112.5317,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  291.0921,  291.0921,  -Inf,  -Inf },
                                                 {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  62.379,  62.379} },
            AS_Tx_deg[NumClustersF][NumPathsF] = { {41.6936,  41.6936,  41.6936,  41.6936,  41.6936,  41.6936,  41.6936,  41.6936,  41.6936,  41.6936,  41.6936,  41.6936,  41.6936,  41.6936,  41.6936,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  55.2669,  55.2669,  55.2669,  55.2669,  55.2669,  55.2669,  55.2669,  55.2669,  55.2669,  55.2669,  55.2669,  55.2669,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  47.4867,  47.4867,  47.4867,  47.4867,  47.4867,  47.4867,  47.4867,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  27.2136,  27.2136,  27.2136,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  33.0126,  33.0126,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  38.0482,  38.0482} },

            // Rx
            AoA_Rx_deg[NumClustersF][NumPathsF] = { {315.1048,  315.1048,  315.1048,  315.1048,  315.1048,  315.1048,  315.1048,  315.1048,  315.1048,  315.1048,  315.1048,  315.1048,  315.1048,  315.1048,  315.1048,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  180.409,  180.409,  180.409,  180.409,  180.409,  180.409,  180.409,  180.409,  180.409,  180.409,  180.409,  180.409,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  74.7062,  74.7062,  74.7062,  74.7062,  74.7062,  74.7062,  74.7062,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  251.5763,  251.5763,  251.5763,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  68.5751,  68.5751,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  246.2344,  246.2344} },

            AS_Rx_deg[NumClustersF][NumPathsF] = { {48.0084,  48.0084,  48.0084,  48.0084,  48.0084,  48.0084,  48.0084,  48.0084,  48.0084,  48.0084,  48.0084,  48.0084,  48.0084,  48.0084,  48.0084,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  55.0823,  55.0823,  55.0823,  55.0823,  55.0823,  55.0823,  55.0823,  55.0823,  55.0823,  55.0823,  55.0823,  55.0823,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  42.0885,  42.0885,  42.0885,  42.0885,  42.0885,  42.0885,  42.0885,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  28.6161,  28.6161,  28.6161,  -Inf,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  30.7745,  30.7745,  -Inf,  -Inf },
                                                                {-Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  -Inf,  38.2914,  38.2914} };

    int i;
    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    pState->NumberOfPaths    = NumPathsF;
    pState->NumberOfClusters = NumClustersF;

    pState->d_BP = 30;
    pState->FadingType = WITGNCHANL_FADING_BELL_SHAPE_SPIKE;

    for (i=0; i<pState->NumberOfPaths; i++)
        pState->K_factor_dB[i] = -100;

    for (i=0; i<pState->NumberOfClusters; i++)
    {
        pState->ModelData.PowerPerAngle_dB[i] = PowerPerAngle_dB[i];
        pState->ModelData.AoD_Tx_deg[i] = AoD_Tx_deg[i];
        pState->ModelData.AoA_Rx_deg[i] = AoA_Rx_deg[i];
        pState->ModelData.AS_Tx_deg[i]  = AS_Tx_deg[i];
        pState->ModelData.AS_Rx_deg[i]  = AS_Rx_deg[i];
    }   
    pState->ModelData.PDPdB[0] = PDP_dB[0];
    pState->ModelData.PDPdB[1] = PDP_dB[1];

    if (Param.TxRxDistance < pState->d_BP) // Rice LOS conditions
        pState->K_factor_dB[0] = 6;
}
// end of wiTGnChanl_Type_F()

// ================================================================================================
// FUNCTION  : wiTGnChanl_IEEE_80211n_Cases()
// ------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------
wiStatus wiTGnChanl_IEEE_80211n_Cases()
{
    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    double AngleAtTx[WITGNCHANL_MAX_PATHS], ASAtTx[WITGNCHANL_MAX_PATHS];
    double AngleAtRx[WITGNCHANL_MAX_PATHS], ASAtRx[WITGNCHANL_MAX_PATHS];
    double AS_Tx[WITGNCHANL_MAX_CLUSTERS],  AoD_Tx[WITGNCHANL_MAX_CLUSTERS];
    double AS_Rx[WITGNCHANL_MAX_CLUSTERS],  AoA_Rx[WITGNCHANL_MAX_CLUSTERS];
    double SigmaDeg[WITGNCHANL_MAX_CLUSTERS], Q[WITGNCHANL_MAX_CLUSTERS];
    double DeltaPhi[WITGNCHANL_MAX_CLUSTERS], PowerPerAngle[WITGNCHANL_MAX_CLUSTERS];

    int i, k, length;

    switch (Param.Model)
    {
        case 'A': wiTGnChanl_Type_A(); break; // Flat fading with 0 ns rms delay spread (one tap at 0 ns delay model)
        case 'B': wiTGnChanl_Type_B(); break; // Typical residential environment, 15 ns rms delay spread
        case 'C': wiTGnChanl_Type_C(); break; // Typical residential or small office environment, 30 ns rms delay spread
        case 'D': wiTGnChanl_Type_D(); break; // Medbo model A - Typical office environment, 50 ns rms delay spread
        case 'E': wiTGnChanl_Type_E(); break; // Medbo model B - Typical large open space and office environments, 100 ns rms delay spread
        case 'F': wiTGnChanl_Type_F(); break; // Medbo model C - Large open space (indoor and outdoor), 150 ns rms delay spread
        default : return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    
    for (i=0; i < pState->NumberOfPaths; i++)
    {
        length = 0;
        for (k=0; k < pState->NumberOfClusters; k++)
        {   
            if (pState->ModelData.PowerPerAngle_dB[k][i] > -Inf)
            {
                PowerPerAngle[length] = dB2Linear(pState->ModelData.PowerPerAngle_dB[k][i]);
                AS_Rx[length]  = pState->ModelData.AS_Rx_deg[k][i];
                AS_Tx[length]  = pState->ModelData.AS_Tx_deg[k][i];
                AoA_Rx[length] = pState->ModelData.AoA_Rx_deg[k][i];
                AoD_Tx[length] = pState->ModelData.AoD_Tx_deg[k][i];
                DeltaPhi[length] = 180;
                length++;
            }
        }
           
        //----- Rx -----
        if (length > 0)
        {
            // Normalisation of the multimodal Laplacian PAS
            XSTATUS( wiTGnChanl_NormalizationLaplacian(Q, SigmaDeg, length, PowerPerAngle, AS_Rx, DeltaPhi) );

            // Computation of the composite Azimuth Spread (AS)
            if (length > 1)
                wiTGnChanl_StatLaplacian( AngleAtRx + i, ASAtRx + i, length, Q, AoA_Rx, SigmaDeg);
            else
            {
                AngleAtRx[i] = AoA_Rx[0];
                ASAtRx[i]    = SigmaDeg[0];
            }
        }
        else
        {
            AngleAtRx[i] = -Inf;
            ASAtRx[i] = -Inf;
        }

        //----- Tx -----
        if (length > 0)
        {
            // Normalisation of the multimodal Laplacian PAS
            XSTATUS( wiTGnChanl_NormalizationLaplacian(Q, SigmaDeg, length, PowerPerAngle, AS_Tx, DeltaPhi) );

            // Computation of the composite Azimuth Spread (AS)
            if (length > 1)
                wiTGnChanl_StatLaplacian( &(AngleAtTx[i]), &(ASAtTx[i]), length, Q, AoD_Tx, SigmaDeg);
            else
            {
                AngleAtTx[i] = AoD_Tx[0];
                ASAtTx[i] = SigmaDeg[0];
            }
        }
        else
        {
            AngleAtTx[i] = -Inf;
            ASAtTx[i] = -Inf;
        }
        
        switch (Param.Connection)
        {
            case WITGNCHANL_DOWNLINK: // Rx is mobile
                pState->AngleAtMobile_rad[i] = DEG2RAD(AngleAtRx[i]);
                pState->ASAtMobile_rad[i]    = DEG2RAD(ASAtRx[i]   );
                break;

            case WITGNCHANL_UPLINK: // Tx is mobile
                pState->AngleAtMobile_rad[i] = DEG2RAD(AngleAtTx[i]);
                pState->ASAtMobile_rad[i]    = DEG2RAD(ASAtTx[i]   );
                break;
            
            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
        //----- Derivation of the correlation -----
        if (length > 0)
        {
            XSTATUS( wiTGnChanl_Correlation(pState->RTx + i, Q, SigmaDeg, pState->NumberOfTxAntennas, 
                                            Param.SpacingTx, length, PowerPerAngle, AoD_Tx, AS_Tx, DeltaPhi) ); // TX

            XSTATUS( wiTGnChanl_Correlation(pState->RRx + i, Q, SigmaDeg, pState->NumberOfRxAntennas, 
                                            Param.SpacingRx, length, PowerPerAngle, AoA_Rx, AS_Rx, DeltaPhi) ); // RX
        }
    }
    // Computation of the Rice steering matrix
    // AoD and AoA of the LOS component are hard-coded to 45 degrees
    XSTATUS( wiTGnChanl_Init_Rice(pState->RiceMatrix, pState->NumberOfTxAntennas, Param.SpacingTx, pi/4,
                                  pState->NumberOfRxAntennas, Param.SpacingRx, pi/4) );

    // Path loss computation (two-slope model with breakpoint)
    // [Barrett] PathLoss_dB appears to be unused
    if (Param.TxRxDistance > pState->d_BP)
        pState->PathLoss_dB = 20*log10(4*pi*Param.CarrierFrequencyHz/3e8) 
                            + 20*log10(pState->d_BP) + 35*log10(Param.TxRxDistance/pState->d_BP);
    else
        pState->PathLoss_dB = 20*log10(4*pi*Param.CarrierFrequencyHz/3e8) + 20*log10(Param.TxRxDistance);

    return WI_SUCCESS;
}
// end of wiTGnChanl_IEEE_802_11_Cases()

// ================================================================================================
// FUNCTION  : wiTGnChanl_StatLaplacian()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiTGnChanl_StatLaplacian(double *composite_AoA_deg, double *composite_SigmaDeg, int NumClusters, 
                               double *Q, double *AoA_deg, double *SigmaDeg)
{
    double AoA[WITGNCHANL_MAX_CLUSTERS], sig[WITGNCHANL_MAX_CLUSTERS];
    double z, compAoA = 0.0, comp_sqr_sigma = 0.0;
    int k;
    
    const double dphi = pi;

    for (k=0; k<NumClusters; k++)
    {
        AoA[k]  = DEG2RAD(AoA_deg[k]);
        sig[k]  = DEG2RAD(SigmaDeg[k]);

        compAoA += AoA[k] * Q[k]*(1 - exp(-sqrt(2.0)*(dphi/sig[k])) );
    }
    for (k=0; k<NumClusters; k++)
    {
        z =  sqrt(2.0)*(AoA[k] - compAoA) / sig[k];
        comp_sqr_sigma += 0.25 * Q[k] * SQR(sig[k]) * (SQR(z)-2*z+2);

        z =  sqrt(2.0)*(AoA[k] - compAoA - dphi) / sig[k];
        comp_sqr_sigma -= 0.25 * Q[k] * SQR(sig[k]) * exp(-sqrt(2.0)*(dphi/sig[k]) ) * (SQR(z)-2*z+2);

        z = -sqrt(2.0)*(AoA[k] - compAoA + dphi) / sig[k];
        comp_sqr_sigma -= 0.25 * Q[k] * SQR(sig[k]) * exp (-sqrt(2.0)*(dphi/sig[k]) ) * (SQR(z)-2*z+2);

        z = -sqrt(2.0)*(AoA[k] - compAoA) / sig[k];
        comp_sqr_sigma += 0.25 * Q[k] * SQR(sig[k]) * (SQR(z)-2*z+2);
    }
    *composite_AoA_deg   = RAD2DEG( compAoA );
    *composite_SigmaDeg = RAD2DEG( sqrt(comp_sqr_sigma) );
}
// end of wiTGnChanl_StatLaplacian()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Init_Rice()
// ------------------------------------------------------------------------------------------------
// Inputs
//
// * Variable NumberOfTxAntennas, number of antenna elements of the Uniform Linear Array (ULA) at Tx
// * Variable Spacing_Tx_norm, normalized spacing of the antenna elements of the ULA at Tx (in wavelengths)
// * Variable AoD_LOS_Tx_rad, angle of departure of the LOS component at Tx
// * Variable NumberOfRxAntennas, number of antenna elements of the ULA at Rx
// * Variable Spacing_Rx_norm, normalized spacing of the antenna elements of the ULA at Rx (in wavelengths)
// * Variable AoA_LOS_Rx_rad, angle of arrival of the LOS component at Rx
//
// Output
//
// A 2-D complex matrix *RiceMatrix of size NumberOfRxAntennas x NumberOfTxAntennas whose
// elements are the product of the phasors related to the elements of two ULAs, one at Tx
// and one at Rx, under incidence angles AoD_LOS_Tx_rad and AoA_LOS_Rx_rad respectively.
// ================================================================================================
wiStatus wiTGnChanl_Init_Rice(wiComplex RiceMatrix[][WITGNCHANL_MAX_ANTENNAS], // Output
                    int NumberOfTxAntennas, double SpacingTx, double AoD_LOS_Tx,
                    int NumberOfRxAntennas, double SpacingRx, double AoA_LOS_Rx)
{
    wiComplex VectorTx[WICHANL_TXMAX], VectorRx[WICHANL_RXMAX];
    int i, j;

    for (i=0; i<NumberOfTxAntennas; i++)
        VectorTx[i] = wiComplexExp( 2*pi*(SpacingTx*i)*sin(AoD_LOS_Tx) );

    for (i=0; i<NumberOfRxAntennas; i++)
        VectorRx[i] = wiComplexExp( 2*pi*(SpacingRx*i)*sin(AoA_LOS_Rx) );
    
    // RiceMatrix = vector_Rx * vector_Tx.'
    for (i=0; i<NumberOfRxAntennas; i++)
        for (j=0; j<NumberOfTxAntennas; j++)
            RiceMatrix[i][j] = wiComplexMultiply(VectorRx[i], VectorTx[j]);
    
    return WI_SUCCESS;
}
// end of wiTGnChanl_Init_Rice()

// ================================================================================================
// FUNCTION  : wiTGnChanl_ComplexFilter()
// ------------------------------------------------------------------------------------------------
// Purpose   : y=conv(f,x), where the filter f = (\sigma b_i) / (\sigma a_i)
// Parameters: x and y are wiComplex[length]
//             StateOut and StateIn are wiComplex[ord]
//             b and a are wiComplex[ord+1]
// ================================================================================================
wiStatus wiTGnChanl_ComplexFilter(wiComplex *y, wiComplex *StateOut, int ord, const wiComplex *b, 
                                  const wiComplex *a, int length, wiComplex *x, wiComplex *StateIn)
{
    int i, k;
    
    for (i=0; i<ord; i++) StateOut[i] = StateIn[i]; // Initializing StateOut

    for (k=0; k<length; k++) // y[k] = StateOut[0]+b[0]*x[k]
    {   
        y[k].Real = StateOut[0].Real + (x[k].Real * b[0].Real) - (x[k].Imag * b[0].Imag);
        y[k].Imag = StateOut[0].Imag + (x[k].Imag * b[0].Real) + (x[k].Real * b[0].Imag);
        
        for (i=0; i<ord; i++) // StateOut[i] = b[i+1]*x[k] + StateOut[i+1] - a[i+1]*y[k]
        {  
            if (i<ord-1) 
                StateOut[i] = StateOut[i+1];
            else
                StateOut[i].Real = StateOut[i].Imag = 0;

            StateOut[i].Real += b[i+1].Real * x[k].Real - a[i+1].Real * y[k].Real;
            StateOut[i].Real -= b[i+1].Imag * x[k].Imag - a[i+1].Imag * y[k].Imag;

            StateOut[i].Imag += b[i+1].Imag * x[k].Real - a[i+1].Imag * y[k].Real;
            StateOut[i].Imag += b[i+1].Real * x[k].Imag - a[i+1].Real * y[k].Imag;
        }
    }
    return WI_SUCCESS;
} 
// end of wiTGnChanl_ComplexFilter()

// ================================================================================================
// FUNCTION  : wiTGnChanl_ComplexTimeInterpolation()
// ------------------------------------------------------------------------------------------------
// Purpose   : Given the points (T,Value), and 1st coordinates X, linearly interpolates 
//             the point(X,Y) and returns Y.
// Parameters:   The time vectors LT and X should be sorted (increasing)
// ================================================================================================
wiStatus wiTGnChanl_ComplexTimeInterpolation(wiComplex Y[], int LT, double T[], wiComplex Value[], int LX, double X[])
{
    int i = 0, j = 0;
    double lambda;

    while ((i < LX) && (j  <LT-1))
    {
        if (X[i] < T[0])
        {
            Y[i] = Value[0];
            i++;
        }
        else 
        {
            if (X[i] >= T[LT-1])
            {
                Y[i] = Value[LT-1];
                i++;
            }
            else
                if ((X[i] >= T[0]) && (X[i] < T[LT-1]))
                {   
                    if ((X[i] >= T[j]) && (X[i] < T[j+1])) // Found the interval that contains X[i]
                    { 
                        lambda = (X[i] - T[j]) / (T[j+1] - T[j]);
                        Y[i].Real = (1-lambda) * Value[j].Real + lambda * Value[j+1].Real;
                        Y[i].Imag = (1-lambda) * Value[j].Imag + lambda * Value[j+1].Imag;
                        i++;
                    }
                    else j++;
                }
        }
    }
    return WI_SUCCESS;
}
// end of wiTGnChanl_ComplexTimeInterpolation()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Correlation()
// ------------------------------------------------------------------------------------------------
// Purpose   : Find the correlation matrix
// Derives the correlation matrix R from the description of the environment, namely the 
// antenna element spacing and the way the waves impinge (number of clusters, PAS type, 
// AS and mean angle of incidence). The generated correlation coefficients are either
//   (complex) field correlation coefficients (type = 0) or (real positive) power 
//  correlation coefficients (type = 1).
//
//   Inputs
//  
//    * Variable N, number of antenna elements of the ULA
//    * Variable spacing, spacing of the antenna elements of the ULA
//    * Variable ClusterNumber, number of impinging clusters
//    * Vector AmplitudeCluster, containing the amplitude ofthe ClusterNumber impinging 
//    clusters
//    * Variable PAS_type, defines the nature of the PAS, isotropic uniform (1),
//    isotropic Gaussian (2), isotropic Laplacian (3) or directive Laplacian (6) according 
//    to 3GPP-3GPP2 SCM AHG radiation pattern
//    * Vector PhiDeg, containing the AoAs of the number_cluster clusters
//    * Vector AS_deg, containing the ASs of the number_cluster clusters
//    * Vector DeltaPhiDeg, containing the constraints limits of the truncated PAS, 
//    if applicable (Gaussian and Laplacian case)
//    * Variable type, defines whether correlation properties should be computed 
//    in field (0) or in power (1)
//    
//   Outputs
//      
//    * 2-D Hermitian matrix R of size M x M, whose elements are the correlation 
//    coefficients of the corresponding elements of the ULA impinged by the described PAS
//    * Vector Q of ClusterNumber elements, giving the normalization coefficients to be 
//    applied to the clusters in order to have the PAS fulfilling the definition of a pdf
//    * Vector SigmaDeg of ClusterNumber of elements, giving the standard deviations of
//    the clusters, derived from their description. There is not necessarily equality 
//    between the given AS and the standard deviation of the modelling PAS.
// ================================================================================================
wiStatus wiTGnChanl_Correlation(wiCMat *R, double Q[], double SigmaDeg[], int N, double spacing, 
                                int ClusterNumber, double AmplitudeCluster[], 
                                double PhiDeg[], double AS_deg[], double DeltaPhiDeg[])
{
    int j=0, k=0;  //use j for indexing antennas and k for indexing clusters
    wiReal Rxx[WITGNCHANL_MAX_ANTENNAS];
    wiReal Rxy[WITGNCHANL_MAX_ANTENNAS];
    wiReal Rxxtmp[WITGNCHANL_MAX_ANTENNAS];
    wiReal Rxytmp[WITGNCHANL_MAX_ANTENNAS];
    wiReal dNorm[WITGNCHANL_MAX_ANTENNAS];
    wiComplex tmp;
    
    // Normalized Antenna Distance
    for (j=0; j<N; j++) dNorm[j] = j * spacing;

    // Normalization
    XSTATUS( wiTGnChanl_NormalizationLaplacian(Q, SigmaDeg, ClusterNumber, AmplitudeCluster, AS_deg, DeltaPhiDeg) );
    
    if (N > 1)
    {
        // Complex correlation computation for Isotropic Laplacian distribution
        for (j=0; j<N; j++)
        {
            Rxx[j] = j0(2*pi*dNorm[j]);
            Rxy[j] = 0; 
        }
        for (k=0; k<ClusterNumber; k++) //add the effects of the k'th cluster
        {   
            wiTGnChanl_Rxx_Laplacian(Rxxtmp, N, dNorm, PhiDeg[k], SigmaDeg[k], DeltaPhiDeg[k]);
            wiTGnChanl_Rxy_Laplacian(Rxytmp, N, dNorm, PhiDeg[k], SigmaDeg[k], DeltaPhiDeg[k]);
                
            //Now,  Rxx = Rxx + Q(k).*Rxxtmp; and Rxy = Rxy + Q(k).*Rxytmp; :
            for (j=0; j<N; j++)
            {
                Rxx[j] += Q[k] * Rxxtmp[j];
                Rxy[j] += Q[k] * Rxytmp[j];
            }
        }
        
        for (j=0; j<N; j++)
        {
            tmp.Real = Rxx[j];
            tmp.Imag = Rxy[j];
        
            // We implement R = toeplitz(tmp) as follows:
            for (k=0; k<N-j; k++)
            {
                // Since the matrix should be Hermitian
                // NOTE: the main diagonal (i.e. for j=0) is set twice, the second time is the correct one.
                R->val[k+j][k] = wiComplexConj(tmp);
                R->val[k][k+j] = tmp;
            }
        }
    }
    else
    {
        R->val[0][0].Real = 1;
        R->val[0][0].Imag = 0;
    }
    return WI_SUCCESS;
}
// end of wiTGnChanl_Correlation()

// ================================================================================================
// FUNCTION  : wiTGnChanl_NormalizationLaplacian()
// ------------------------------------------------------------------------------------------------
// Purpose   : Calculate the normalization parameters for the Laplacian AS
// ================================================================================================
wiStatus wiTGnChanl_NormalizationLaplacian(double Q[], double SigmaDeg[], int ClusterNumber, 
                                            double AmplitudeCluster[], double AS_deg[], double DeltaPhiDeg[])
{
    int j, k;
    double integral = 0.0;
    const wiReal (*sigma)[2];
    wiCMat *pA = &wiTGnChanl_State()->NormalizationLaplacian.A;
    wiCMat *pB = &wiTGnChanl_State()->NormalizationLaplacian.B;

    for (k=0; k<ClusterNumber; k++)
    {
        switch ((int)(DeltaPhiDeg[k]))
        {
            case  90: sigma = AS2sigma_Laplacian90;  break;
            case 180: sigma = AS2sigma_Laplacian180; break;
            default : return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
        // Find the smallest j where in temp_mat[j][1]>=AS_deg[k], and compute SigmaDeg
        for (j=0; ((j<130) && (sigma[j][0] < AS_deg[k])); j++) 
            ;
        SigmaDeg[k] = ((sigma[j][0]-AS_deg[k])*sigma[j-1][1] + (AS_deg[k]-sigma[j-1][0])*sigma[j][1])/(sigma[j][0]-sigma[j-1][0]);
    }
    // Computation of Q
    if (ClusterNumber == 1)
        Q[0] = 1/(1 - exp((-sqrt(2.0)*DeltaPhiDeg[0])/SigmaDeg[0]) );
    else
    {
        XSTATUS( wiCMatReInit(pA, ClusterNumber, ClusterNumber) );
        for (k=0; k<ClusterNumber; k++)
        {
            if (k < ClusterNumber-1)
            {
                pA->val[k][0  ].Real =  1 / (SigmaDeg[0  ]*(pi/180)*AmplitudeCluster[0  ]);
                pA->val[k][k+1].Real = -1 / (SigmaDeg[k+1]*(pi/180)*AmplitudeCluster[k+1]); 
            }
            pA->val[ClusterNumber-1][k].Real = 1 - exp((-sqrt(2.0)*DeltaPhiDeg[k])/SigmaDeg[k]);
        }

        XSTATUS( wiCMatReInit(pB, ClusterNumber, ClusterNumber) ); // Computed B = Inv(A)
        XSTATUS( wiCMatInv(pB, pA) );

        // Integrate to verify the result...
        for (k=0; k<ClusterNumber; k++)
        {
            Q[k] = pB->val[k][ClusterNumber-1].Real;
            integral += (1 - exp( (-sqrt(2.0)*DeltaPhiDeg[k]) / SigmaDeg[k] ) ) * Q[k];
        }
        if ((integral - 1) > 1e-10)
        {
            wiPrintf("Normalization of the Laplacian distribution failed...\n");
            return STATUS(WI_ERROR);
        }
    }
    return WI_SUCCESS;
}
// end of wiTGnChanl_NormalizationLaplacian()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Rxx_Laplacian()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiTGnChanl_Rxx_Laplacian(double Rxxtmp[], int N, double dNorm[], double PhiDegk, 
                              double SigmaDegk, double DeltaPhiDegk)
{
    double tmp_xx;
    double DeltaPhi = DEG2RAD(DeltaPhiDegk);
    double sigma     = DEG2RAD(SigmaDegk);
    double phi       = DEG2RAD(PhiDegk);
    int m, j;

    for (j=0; j<N; j++)
    {
        Rxxtmp[j] = 0;
        for (m=1; m<21; m++)
        {
            int n = 2*m;
            double x = dNorm[j]*2*pi;

            tmp_xx = 4*jn(n,x) * cos(n*phi) 
                * (sqrt(2.0)/sigma + exp(-sqrt(2.0)*(DeltaPhi/sigma))*(2*m*sin(n*DeltaPhi) - sqrt(2.0) * cos(n*DeltaPhi)/sigma))
                / (4*sqrt(2.0) * sigma*m*m + sqrt(8.0)/sigma);
            Rxxtmp[j] += tmp_xx;
        }
    }
}
// end of wiTGnChanl_Rxx_Laplacian()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Rxy_Laplacian()
// ------------------------------------------------------------------------------------------------
// Purpose   : Computes the correlation of the real and imaginary parts of the signals in 
//             the case of a laplacian Power Azimuth Spectrum (PAS) at spacings given by 
//             dNorm. The PAS is characterised by the Angle Of Arrival (AOA) PhiDegk 
//             and by its standard deviation SigmaDegk.
// ================================================================================================
void wiTGnChanl_Rxy_Laplacian(double Rxytmp[], int N, double dNorm[], double PhiDegk, double SigmaDegk, 
                              double DeltaPhiDegk)
{
    double tmp_xy;
    double DeltaPhi = DEG2RAD(DeltaPhiDegk);
    double sigma    = DEG2RAD(SigmaDegk);
    double phi      = DEG2RAD(PhiDegk);
    int m, j;

    for (j=0; j<N; j++)
    {   
        double x = dNorm[j]*2*pi;
        Rxytmp[j] = 0;
        for (m=0; m<21; m++)
        {
            int n = 2*m + 1;
            tmp_xy = 4*jn(n,x) * sin(n*phi) 
                * (sqrt(2.0)/sigma + exp(-sqrt(2.0)*DeltaPhi/sigma)*(n*sin(n*DeltaPhi) - sqrt(2.0)*cos(n*DeltaPhi)/sigma)) 
                / ( sqrt(2.0)*sigma*( SQR(sqrt(2.0)/sigma) + SQR(n) ) );
        
            Rxytmp[j] += tmp_xy;
        }
    }
}
// end of wiTGnChanl_Rxy_Laplacian()

// ================================================================================================
// FUNCTION  : wiTGnChanl_FluorescentLights()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTGnChanl_FluorescentLights(wiCMat *pMatrix, double *Time_s)
{
    wiTGnChanl_State_t *pState = wiTGnChanl_State();

    const int Taps_D[3] = {12, 14, 16};
    const int Taps_E[3] = { 3,  5,  7};
    const int *Taps;
    const double Amplitudes[] = {0, -15, -20}; // dB

    double  ModulationMatrixEnergy = 0, FadingMatrixEnergy = 0;
    double  RealInterfererToCarrierRatio, sqrtNormalizationConstant;
    wiComplex *ModulationFunction;
    int i, j, k, index;

    wiReal Random_Phase = 2.0*pi * wiRnd_UniformSample();
    wiReal xRandom = wiRnd_NormalSample(Param.Fluorescent.xStdDev);
    wiReal RandomInterfererToCarrierRatio = SQR(Param.Fluorescent.xMean + xRandom);

    XSTATUS( wiCMatReInit(&pState->FluorescentModulationMatrix, pMatrix->row, pMatrix->col) );

    // Computation of the modulation function
    GET_WIARRAY( ModulationFunction, pState->ModulationFunction, pMatrix->col, wiComplex );

    for (k=0; k<3; k++)
    {
        double A = pow(10.0, Amplitudes[k]/20);
        for (i=0; i<pMatrix->col; i++)
        {
            double a = 4*pi*(2*(k-1)+1) * Param.Fluorescent.PowerLineFrequencyHz*Time_s[i] + Random_Phase;
            ModulationFunction[i].Real += A * cos(a);
            ModulationFunction[i].Imag += A * sin(a);
        }
    }

    // Filling of the ModulationMatrix, later to be added to the FadingMatrix
    //
    Taps = (Param.Model == 'D')? Taps_D : Taps_E;
    for (i=0; i<pState->NumberOfTxAntennas; i++)
        for (j=0; j<pState->NumberOfRxAntennas; j++)
            for (k=0; k<3; k++)
                for (index=0; index<pMatrix->col; index++)
                {
                    int n = ((Taps[k]-1) * pState->NumberOfTxAntennas * pState->NumberOfRxAntennas) + (i*pState->NumberOfRxAntennas) + j;
                    pState->FluorescentModulationMatrix.val[n][index] = ModulationFunction[index];
                }

    // Multiplication of the modulation matrix and the fading matrix
    // g(t) = g(t) * c(t)
    //
    for (i=0; i<pMatrix->row; i++)
        for (j=0; j<pMatrix->col; j++)
        {
            pState->FluorescentModulationMatrix.val[i][j] = wiComplexMultiply(
                pState->FluorescentModulationMatrix.val[i][j], pMatrix->val[i][j] );
            
            // Calculation of the energy of both matrices
            ModulationMatrixEnergy += wiComplexNormSquared(pState->FluorescentModulationMatrix.val[i][j]);
            FadingMatrixEnergy     += wiComplexNormSquared(pMatrix->val[i][j]);
        }

    // Comparison and calculation of the normalization constant alpha
    // NormalizationConstant * (I/C)_real = (I/C)_drawn
    //
    RealInterfererToCarrierRatio = ModulationMatrixEnergy / FadingMatrixEnergy;
    sqrtNormalizationConstant    = sqrt(RandomInterfererToCarrierRatio / RealInterfererToCarrierRatio);
  
    // Final adaptation of the fading matrix
    // c(t) = c(t) + (alpha * c(t) *g(t))
    //
    for (i=0; i<pMatrix->row; i++) {
        for (j=0; j<pMatrix->col; j++)
        {
            pMatrix->val[i][j].Real += sqrtNormalizationConstant * pState->FluorescentModulationMatrix.val[i][j].Real;
            pMatrix->val[i][j].Imag += sqrtNormalizationConstant * pState->FluorescentModulationMatrix.val[i][j].Imag;
        }
    }
    return WI_SUCCESS;
}
// end of wiTGnChanl_FluorescentLights()

// ================================================================================================
// FUNCTION  : wiTGnChanl_Resample
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTGnChanl_Resample(int Nx, wiComplex *x, int Ny, wiComplex *y, wiReal FsOut, wiReal FsIn)
{
    double T = FsIn/FsOut;
    double t, s, *w;
    int i, j, k, r, n, halflength;
    wiChanl_LookupTable_t *pLUT = wiChanl_LookupTable();

    if (!x) return WI_ERROR_PARAMETER2;
    if (!y) return WI_ERROR_PARAMETER4;

    halflength = (WICHANL_INTERPOLATOR_TAPS-1)/2;

    for (k=0; k<Ny; k++)
    {
        t = k * T;            // new time at sample index k
        t = min(t, Nx + 1);
        j = (int)floor(t + 0.5);           // closest input sample index
        s = t - j;                       // phase shift
        n = (int)floor(0.5+(s+0.5)*(WICHANL_INTERPOLATOR_STEPS-1)); // select filter
        w = pLUT->wInterpolate[n];       // w points to filter taps
        
        y[k].Real = y[k].Imag = 0.0;

        for (i = max(0, j-halflength); i <= min(Nx-1, j+halflength); i++)
        {
            r = j - i + halflength; 
            y[k].Real += w[r] * x[i].Real;
            y[k].Imag += w[r] * x[i].Imag;
        }
    }
    return WI_SUCCESS;
}
// end of wiTGnChanl_Resample()

// ================================================================================================
// FUNCTION  : wiTGnChanl_WriteConfig
// ------------------------------------------------------------------------------------------------
// Purpose   : Output the model configuration using the message handling function
// Parameters: MsgFunc -- output function (similar to printf)
// ================================================================================================
wiStatus wiTGnChanl_WriteConfig(wiMessageFn MsgFunc)
{
    char EnabledStr[2][16] = {"Disabled","Enabled"};

    switch (Param.Model)
    {
        case 'A' : MsgFunc(" IEEE TGn Channel Model: A -- Flat Fading\n"); break;
        case 'B' : MsgFunc(" IEEE TGn Channel Model: B -- Trms = 15 ns (Residential and Small Office)\n"); break;
        case 'C' : MsgFunc(" IEEE TGn Channel Model: C -- Trms = 30 ns (Residential and Small Office)\n"); break;
        case 'D' : MsgFunc(" IEEE TGn Channel Model: D -- Trms = 50 ns (Office)\n"); break;
        case 'E' : MsgFunc(" IEEE TGn Channel Model: E -- Trms = 100 ns (Large Office)\n"); break;
        case 'F' : MsgFunc(" IEEE TGn Channel Model: F -- Trms = 150 ns (Large Open Space; Doppler From Vehicle at 25 MPH)\n"); break;
        default  : MsgFunc(" IEEE TGn Channel Model Not Defined ********************\n");
    }
    MsgFunc(" Line-of-Sight         : %s\n",EnabledStr[Param.LOS]);
    MsgFunc(" Antenna Spacing       : TX = %1.1f, RX = %1.1f Wavelenghts\n", Param.SpacingTx, Param.SpacingRx);
    MsgFunc(" Main Doppler Component: %1.2f Hz\n",Param.fDoppler);

    if (Param.Fluorescent.Enabled)
        MsgFunc(" Fluorescent Doppler   : PowerLine %1.0f Hz, xMean=%1.4f, xStdDev=%1.4f\n",
                Param.Fluorescent.PowerLineFrequencyHz, Param.Fluorescent.xMean, Param.Fluorescent.xStdDev);
    else MsgFunc(" Fluorescent Doppler   : Disabled\n");
    
    return WI_SUCCESS;
}
// end of wiTGnChanl_WriteConfig()

// ================================================================================================
// FUNCTION  : wiTGnChanl_ParseLine
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text and retrieve parameters relevent to this module
// Parameters: text -- single line of text to examine
// ================================================================================================
wiStatus wiTGnChanl_ParseLine(wiString text)
{
     #define PSTATUS(arg)    if ((pstatus=WICHK(arg))<=0) return pstatus;

    if (strstr(text, "wiTGnChanl"))
    {
        wiStatus status, pstatus;
        char buf[256];

        PSTATUS( wiParse_Real   (text,"wiTGnChanl.SpacingTx",           &(Param.SpacingTx),  0, 100));
        PSTATUS( wiParse_Real   (text,"wiTGnChanl.SpacingRx",           &(Param.SpacingRx),  0, 100));
        PSTATUS( wiParse_Real   (text,"wiTGnChanl.CSI.Gap_s",           &(Param.CSI.Gap_s),  0, 100));
        PSTATUS( wiParse_Boolean(text,"wiTGnChanl.LOS",                 &(Param.LOS)));
        PSTATUS( wiParse_Real   (text,"wiTGnChanl.fDoppler",            &(Param.fDoppler),   0, 1000));
        PSTATUS( wiParse_Boolean(text,"wiTGnChanl.Fluorescent.Enabled", &(Param.Fluorescent.Enabled)));
        PSTATUS( wiParse_Real   (text,"wiTGnChanl.Fluorescent.xMean",   &(Param.Fluorescent.xMean  ), 0.0, 1.0));
        PSTATUS( wiParse_Real   (text,"wiTGnChanl.Fluorescent.xStdDev", &(Param.Fluorescent.xStdDev), 0.0, 1.0));
        PSTATUS( wiParse_Real   (text,"wiTGnChanl.Fluorescent.PowerLineFrequencyHz",&(Param.Fluorescent.PowerLineFrequencyHz), 49.0, 61.0));

        XSTATUS( status = wiParse_String(text,"wiTGnChanl.Model", buf, 10) );
        if (status == WI_SUCCESS)
        {
            if (strlen(buf)==1 && toupper(buf[0])>='A' && toupper(buf[0])<='G')
            {
                Param.Model = (char)toupper(buf[0]);
                return WI_SUCCESS;
            }
            else return STATUS(WI_ERROR_RANGE);
        }

        // deprecated names
//        PSTATUS( wiParse_Real   (text,"wiTGnChanl.Fluorescent.PowerLineFrequency_Hz",&(Param.Fluorescent.PowerLineFrequencyHz), 49.0, 61.0));
    }
    return WI_WARN_PARSE_FAILED;
}
// end of wiTGnChanl_ParseLine()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
