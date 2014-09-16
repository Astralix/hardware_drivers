//--< Phy11n.c >-----------------------------------------------------------------------------------
//=================================================================================================
//
//  Phy11n - 802.11n 20 MHz MIMO-OFDM PHY
//  Copyright 2006-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wiMath.h"
#include "wiParse.h"
#include "wiPHY.h"
#include "wiUtil.h"
#include "wiRnd.h"
#include "Phy11n.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static Phy11n_TxState_t TX[WISE_MAX_THREADS] = {{0}};
static Phy11n_RxState_t RX[WISE_MAX_THREADS] = {{0}};

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;

#ifndef min
#define min(a,b) (((a)<(b))? (a):(b))
#define max(a,b) (((a)>(b))? (a):(b))
#endif

// ================================================================================================
// FUNCTION  : Phy11n_InitializeThread()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_InitializeThread(unsigned int ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADS)) 
        return WI_ERROR_PARAMETER1;
    else
    {
        Phy11n_TxState_t *pTX = TX + ThreadIndex;
        Phy11n_RxState_t *pRX = RX + ThreadIndex;

//        XSTATUS( Phy11n_CloseThread(ThreadIndex) );

        memcpy(pTX, TX, sizeof(Phy11n_TxState_t));
        memcpy(pRX, RX, sizeof(Phy11n_RxState_t));

        XSTATUS( Phy11n_TX_FreeMemory(pTX, WI_TRUE) );
        XSTATUS( Phy11n_RX_FreeMemory(pRX, WI_TRUE) );

        pTX->LastAllocationSize[0] = -1; // force allocation on next TX/RX operation
        pRX->LastAllocationSize[0] = -1;

        pTX->ThreadIsInitialized = WI_TRUE;
        pRX->ThreadIsInitialized = WI_TRUE;
    }
    return WI_SUCCESS;
}
// end of NevadaPHY_InitializeThread()

// ================================================================================================
// FUNCTION  : Phy11n_CloseThread()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_CloseThread(unsigned int ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADS)) 
        return WI_ERROR_PARAMETER1;
    else
    {
        XSTATUS( Phy11n_TX_FreeMemory(TX + ThreadIndex, WI_FALSE) );
        XSTATUS( Phy11n_RX_FreeMemory(RX + ThreadIndex, WI_FALSE) );
        TX[ThreadIndex].ThreadIsInitialized = WI_FALSE;
        RX[ThreadIndex].ThreadIsInitialized = WI_FALSE;
    }
    return WI_SUCCESS;
}
// end of NevadaPHY_CloseThread()

// ================================================================================================
// FUNCTION : Phy11n_TxState()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
Phy11n_TxState_t* Phy11n_TxState(void)
{
    return TX + wiProcess_ThreadIndex();
}
// end of Phy11n_TxState()

// ================================================================================================
// FUNCTION : Phy11n_RxState()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
Phy11n_RxState_t* Phy11n_RxState(void)
{
    return RX + wiProcess_ThreadIndex();
}
// end of Phy11n_RxState()

// ================================================================================================
// FUNCTION  : Phy11n_TX_FreeMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_TX_FreeMemory(Phy11n_TxState_t *pTX, wiBoolean ClearPointerOnly)
{
    #define FREE(ptr)                                        \
    {                                                        \
        if (!ClearPointerOnly && (ptr != NULL)) wiFree(ptr); \
        ptr = NULL;                                          \
    }
    int i;

    WIFREE_ARRAY( pTX->a );
    WIFREE_ARRAY( pTX->b );
    WIFREE_ARRAY( pTX->c );

    for (i=0; i<PHY11N_TXMAX; i++)
    {
        FREE( pTX->stsp[i]    );
        FREE( pTX->stit[i]    );
        FREE( pTX->stmp[i]    );
        FREE( pTX->stSTBC[i]  );
        FREE( pTX->stSM[i]    );
        FREE( pTX->x[i]       );
    }
    pTX->Nx = 0;
    pTX->Nmp = 0;

    #undef FREE

    return WI_SUCCESS;
}
// end of Phy11n_TX_FreeMemory()

// ================================================================================================
// FUNCTION  : Phy11n_TX_AllocateMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_TX_AllocateMemory(void)
{
    Phy11n_TxState_t *pTX = Phy11n_TxState();
    int i;

    // Determine if a new memory allocation is required. The objective is to avoid excessive
    // allocate-free cycles that can cause memory fragementation when running multithreaded.
    //
    if ((pTX->LastAllocationSize[0] != pTX->N_SYM    ) ||
        (pTX->LastAllocationSize[1] != pTX->MCS      ) ||
        (pTX->LastAllocationSize[2] != pTX->N_TX     ) ||
        (pTX->LastAllocationSize[3] != pTX->N_STS    ) ||
        (pTX->LastAllocationSize[4] != pTX->aPadFront) ||
        (pTX->LastAllocationSize[5] != pTX->aPadBack ) ||
        (pTX->LastAllocationSize[6] != pTX->N_HT_DLTF) ||
        (pTX->LastAllocationSize[7] != pTX->N_HT_ELTF)
        )
    {
        pTX->LastAllocationSize[0] = pTX->N_SYM;
        pTX->LastAllocationSize[1] = pTX->MCS;
        pTX->LastAllocationSize[2] = pTX->N_TX;
        pTX->LastAllocationSize[3] = pTX->N_STS;
        pTX->LastAllocationSize[4] = pTX->aPadFront;
        pTX->LastAllocationSize[5] = pTX->aPadBack;
        pTX->LastAllocationSize[6] = pTX->N_HT_DLTF;
        pTX->LastAllocationSize[7] = pTX->N_HT_ELTF;
    }
    else return WI_SUCCESS; // keep current memory allocation unchanged

    XSTATUS( Phy11n_TX_FreeMemory(pTX, WI_FALSE) );

    for (i=0; i<pTX->N_SS; i++)
    {
        pTX->Nsp[i] = (pTX->N_BPSC[i] * pTX->N_SYM * pTX->N_CBPS) / pTX->N_TBPS;
        WICALLOC( pTX->stsp[i], pTX->Nsp[i], int );
        WICALLOC( pTX->stit[i], pTX->Nsp[i], int );
    }
    pTX->Nmp = pTX->Nsp[0]/pTX->N_BPSC[0];
    for (i=0; i<pTX->N_SS;  i++) WICALLOC( pTX->stmp[i],   pTX->Nmp, wiComplex );
    for (i=0; i<pTX->N_STS; i++) WICALLOC( pTX->stSTBC[i], pTX->Nmp, wiComplex );
    for (i=0; i<pTX->N_TX;  i++) WICALLOC( pTX->stSM[i],   pTX->Nmp, wiComplex );

    pTX->Nx = 640 + 80*(pTX->N_SYM + pTX->N_HT_DLTF) + 1 + pTX->aPadFront + pTX->aPadBack;
    for (i=0; i<pTX->N_STS; i++) WICALLOC( pTX->x[i], pTX->Nx, wiComplex );

    return WI_SUCCESS;
}
// end of Phy11n_TX_AllocateMemory()

// ================================================================================================
// FUNCTION  : Phy11n_RX_FreeMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_RX_FreeMemory(Phy11n_RxState_t *pRX, wiBoolean ClearPointerOnly)
{
    #define FREE(ptr)                                        \
    {                                                        \
        if (!ClearPointerOnly && (ptr != NULL)) wiFree(ptr); \
        ptr = NULL;                                          \
    }
    int i;

    FREE( pRX->a );
    FREE( pRX->b );
    FREE( pRX->Lc );
    FREE( pRX->Lc_e );
    pRX->Nmp = 0;
    pRX->Na = pRX->Nb = pRX->Nc = 0;

    for (i=0; i<PHY11N_RXMAX; i++)
    {
        pRX->Nsp[i] = 0;
        FREE( pRX->stsp[i] );
        FREE( pRX->stit[i] );
        FREE( pRX->stmp[i] );
        FREE( pRX->stsp_e[i] );
        FREE( pRX->stit_e[i] );
    }
    pRX->Ny  = 0;
    for (i=0; i<PHY11N_RXMAX; i++) FREE( pRX->y[i] );
    
    for (i=0; i<52; i++) wiCMatFree(&(pRX->H52[i]));
    for (i=0; i<56; i++) wiCMatFree(&(pRX->H56[i]));
    for (i=0; i<56; i++) wiCMatFree(&(pRX->InitialH56[i]));

    wiCMatFree(&(pRX->Y52)); 
    wiCMatFree(&(pRX->Y56));

    WIFREE_ARRAY( pRX->PSDU );

    for (i=0; i<64; i++)
    {
        WIFREE_ARRAY( pRX->BCJR.alpha[i] );
        WIFREE_ARRAY( pRX->BCJR.beta[i] );
    }
    FREE( pRX->SOVA );

	if(pRX->ListSphereDecoder.d2_lsd) FREE(pRX->ListSphereDecoder.d2_lsd);
	if(pRX->ListSphereDecoder.b_lsd) FREE(pRX->ListSphereDecoder.b_lsd); 
    
	if(pRX->DemapperType == 9 || pRX->DemapperType == 14)
	{
		pRX->SingleTreeSearchSphereDecoder.sym  = 0;		
		FREE(pRX->SingleTreeSearchSphereDecoder.NCyclesPSTS);  
		FREE(pRX->SingleTreeSearchSphereDecoder.PH2PSTS);  
		FREE(pRX->SingleTreeSearchSphereDecoder.R2RatioPSTS);  
	}
    #undef FREE
    return WI_SUCCESS;
}
// end of Phy11n_RX_FreeMemory()


// ================================================================================================
// FUNCTION  : Phy11n_RX_AllocateMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_RX_AllocateMemory(void)
{
    Phy11n_TxState_t *pTX = Phy11n_TxState();
    Phy11n_RxState_t *pRX = Phy11n_RxState();
    int i;

    // Determine if a new memory allocation is required. The objective is to avoid excessive
    // allocate-free cycles that can cause memory fragementation when running multithreaded.
    //
    if ((pRX->LastAllocationSize[0] != pTX->N_SYM    ) ||
        (pRX->LastAllocationSize[1] != pTX->MCS      ) ||
        (pRX->LastAllocationSize[2] != pTX->N_TX     ) ||
        (pRX->LastAllocationSize[3] != pTX->N_STS    ) ||
        (pRX->LastAllocationSize[4] != pRX->N_RX     ) ||
        (pRX->LastAllocationSize[5] != pTX->Nx       ) ||
		(pRX->DemapperType ==9 && pRX->SingleTreeSearchSphereDecoder.NSTS != pRX->N_SYM*52) )
    {
        pRX->LastAllocationSize[0] = pTX->N_SYM;
        pRX->LastAllocationSize[1] = pTX->MCS;
        pRX->LastAllocationSize[2] = pTX->N_TX;
        pRX->LastAllocationSize[3] = pTX->N_STS;
        pRX->LastAllocationSize[4] = pRX->N_RX;
        pRX->LastAllocationSize[5] = pTX->Nx;
		pRX->SingleTreeSearchSphereDecoder.NSTS = pRX->N_SYM*52;
    }
    else return WI_SUCCESS; // keep current memory allocation unchanged

    XSTATUS( Phy11n_RX_FreeMemory(pRX, WI_FALSE) );

    ALLOCATE_WIARRAY( pRX->PSDU, 65536, wiUWORD );
 
    pRX->Na = pRX->Nb = pRX->N_SYM * pRX->N_DBPS;
    WICALLOC( pRX->a, pRX->Na, wiInt );
    WICALLOC( pRX->b, pRX->Nb, wiInt );

    pRX->Nc = pRX->N_SYM * pRX->N_CBPS;
    WICALLOC( pRX->Lc,   pRX->Nc, wiReal );
    WICALLOC( pRX->Lc_e, pRX->Nc, wiReal );

    for (i=0; i<pRX->N_SS; i++)
    {
        pRX->Nsp[i] = pRX->N_BPSC[i]*pRX->Nc/pRX->N_TBPS;

        WICALLOC( pRX->stsp[i],   pRX->Nsp[i], wiReal );
        WICALLOC( pRX->stit[i],   pRX->Nsp[i], wiReal );
        WICALLOC( pRX->stsp_e[i], pRX->Nsp[i], wiReal );
        WICALLOC( pRX->stit_e[i], pRX->Nsp[i], wiReal );
    }
    pRX->Nmp = pRX->Nsp[0]/pRX->N_BPSC[0];
    for (i=0; i<pRX->N_SS; i++) {
        WICALLOC( pRX->stmp[i], pRX->Nmp, wiComplex );
    }
    pRX->Ny = pTX->Nx - pTX->aPadBack - pTX->aPadFront + 16; // +16 is margin for symbol sync
    for (i=0; i<pRX->N_RX; i++) {
        WICALLOC( pRX->y[i], pRX->Ny, wiComplex );
    }
    for (i=0; i<52; i++) {      
        wiCMatReInit(pRX->H52+i, pRX->N_RX, pRX->N_TX);
    }
    for (i=0; i<56; i++) {      
        wiCMatReInit(&(pRX->H56[i]),        pRX->N_RX, pRX->N_TX);
        wiCMatReInit(&(pRX->InitialH56[i]), pRX->N_RX, pRX->N_TX); 
    }
    wiCMatReInit(&(pRX->Y52), pRX->N_RX, 52*pRX->N_SYM);
    wiCMatReInit(&(pRX->Y56), pRX->N_RX, 56*pRX->N_SYM);

	if(pRX->DemapperType == 9 || pRX->DemapperType == 14)
	{
		pRX->SingleTreeSearchSphereDecoder.sym = 0;
		
		if(pRX->SingleTreeSearchSphereDecoder.NCyclesPSTS)			
			wiFree(pRX->SingleTreeSearchSphereDecoder.NCyclesPSTS);
		WICALLOC(pRX->SingleTreeSearchSphereDecoder.NCyclesPSTS, pRX->SingleTreeSearchSphereDecoder.NSTS, wiReal);	
			
		if(pRX->SingleTreeSearchSphereDecoder.PH2PSTS)			
			wiFree(pRX->SingleTreeSearchSphereDecoder.PH2PSTS);
		WICALLOC(pRX->SingleTreeSearchSphereDecoder.PH2PSTS, pRX->SingleTreeSearchSphereDecoder.NSTS, wiReal);
			
		if(pRX->SingleTreeSearchSphereDecoder.R2RatioPSTS)			
			wiFree(pRX->SingleTreeSearchSphereDecoder.R2RatioPSTS);
		WICALLOC(pRX->SingleTreeSearchSphereDecoder.R2RatioPSTS, pRX->SingleTreeSearchSphereDecoder.NSTS, wiReal);
	}
    return WI_SUCCESS;
}
// end of Phy11n_RX_AllocateMemory()

// ================================================================================================
// FUNCTION  : Phy11n_LogExpAdd()
// ------------------------------------------------------------------------------------------------
//  Purpose  : Compute log(exp(a) + exp(b)) = max(a,b) + log(1 + exp(-fabs(a,b)))
// ================================================================================================
double Phy11n_LogExpAdd(double a, double b)
{
    double c = (a > b) ? a : b;
    c += log(1.0 + exp(-fabs(a - b)));
    
    return c;
}
// end of Phy11n_LogExpAdd()


// ================================================================================================
// FUNCTION  : Phy11n_RESET()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_RESET(void)
{
    Phy11n_TxState_t *pTX = Phy11n_TxState();
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    pTX->PilotShiftRegister     = 0x7F;
    pTX->ScramblerShiftRegister = 0x7F;
    pRX->N_RX = 1;

    XSTATUS( Phy11n_TX_FreeMemory(pTX, WI_FALSE) );
    XSTATUS( Phy11n_RX_FreeMemory(pRX, WI_FALSE) );
    
    return WI_SUCCESS;
}
// end of Phy11n_RESET()


// ================================================================================================
// FUNCTION  : Phy11n_WriteConfig()
// ------------------------------------------------------------------------------------------------
// Purpose   : Write the current configuration using the provided message function
// Parameters: MsgFunc -- Function to direct message output
// ================================================================================================
wiStatus Phy11n_WriteConfig(wiMessageFn MsgFunc)
{
    const char *DemapperName[]={"","Log-Map (ML)","MMSE Tree Search (Moon & Li)","MMSE (Single Inversion)",
        "MMSE (Multiple Inversions)","MMSE + MIMO-MRC","MIMO-QR","MIMO-QR + MIMO-MRC","List Sphere Decoder",
        "Sphere Decoder (STS)","Kbest-SD","SE-SD (ML)","LF-SD","Kbvt-SD","Fixed Point STS","" };

    const char *DecoderName[]={"Viterbi Decoder","Viterbi Decoder (Hard Demapping)",
                               "Soft-Output Viterbi Algorithm (SOVA)","BCJR"};

    Phy11n_TxState_t *pTX = Phy11n_TxState();
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    MsgFunc(" MCS                   = %d\n",pTX->MCS);
    MsgFunc(" Antennas (TX x RX)    = %d x %d\n",pTX->N_TX, pRX->N_RX);
    MsgFunc(" STBC                  = %d\n",pTX->STBC);
    if (pTX->ScramblerDisabled)  MsgFunc(" Data Scrambler        = DISABLED\n");
    MsgFunc(" FrameSync             : Enabled=%d, Delay=%d, SyncShift=%d\n",pRX->EnableFrameSync,pRX->FrameSyncDelay,pRX->SyncShift);
    MsgFunc(" CFO Estimation        : Method = %d\n",pRX->CFOEstimation);
    MsgFunc(" Demapper Type         = %d: %s\n",pRX->DemapperType,DemapperName[pRX->DemapperType]);
    switch (pRX->DemapperType)
    {
        case 8:
        {
            Phy11n_ListSphereDecoder_t *p = &(pRX->ListSphereDecoder);
    		MsgFunc(" List Sphere Decoding  : List size =%d, Radius Control Factor = %f FixedRadius =%f\n", 
                    p->NCand, p->alpha, p->FixedRadius);
            break;
        }
        case 9:
		case 14:
		{
			MsgFunc("                        :  ModifiedQR = %d  Sorted = %d   MMSE-QR = %d  ", pRX->SingleTreeSearchSphereDecoder.ModifiedQR, pRX->SingleTreeSearchSphereDecoder.Sorted,  pRX->SingleTreeSearchSphereDecoder.MMSE);
    		if(pRX->SingleTreeSearchSphereDecoder.MMSE)
				MsgFunc("(Unbiased = %d) \n", pRX->SingleTreeSearchSphereDecoder.Unbiased);
			else
				MsgFunc("\n");			
			MsgFunc("                        : LLR Clip = %d", pRX->SingleTreeSearchSphereDecoder.LLRClip);
			if(pRX->SingleTreeSearchSphereDecoder.LLRClip) 
				MsgFunc(" Level = %f\n", pRX->SingleTreeSearchSphereDecoder.Norm1? pRX->SingleTreeSearchSphereDecoder.LMax1: pRX->SingleTreeSearchSphereDecoder.LMax);
			else 
				MsgFunc(" \n");

			if(pRX->SingleTreeSearchSphereDecoder.Terminated)
			{
				if(pRX->SingleTreeSearchSphereDecoder.BlockSchedule)
					MsgFunc("                       : BlockSchedule (%f,  %f)\n", pRX->SingleTreeSearchSphereDecoder.MeanCycles, pRX->SingleTreeSearchSphereDecoder.MinCycles);
				else
					MsgFunc("                       : Ternimate after %f Cycles/Search \n", pRX->SingleTreeSearchSphereDecoder.MaxCycles);
			}

			if(pRX->SingleTreeSearchSphereDecoder.Hybrid)
				    MsgFunc("                       : Hybrid Solution with STS and MMSE \n");
			if(pRX->SingleTreeSearchSphereDecoder.Norm1 == 1)
				    MsgFunc("                       : Norm1 used in metric calculation %s\n", pRX->SingleTreeSearchSphereDecoder.ExactLLR? "(ExactLLR)":"");
			if(pRX->SingleTreeSearchSphereDecoder.Norm1 == 2)
				    MsgFunc("                       : Infite Norm used in metric calculation %s\n", pRX->SingleTreeSearchSphereDecoder.ExactLLR? "(ExactLLR)":"");
			
			MsgFunc("\n");
			if(pRX->DemapperType == 14)
			{
				MsgFunc("                       : Fixed Point Parameters \n");
				MsgFunc("                       : Scale Factor %d \n",  pRX->SingleTreeSearchSphereDecoderFxP.FLScale);
				MsgFunc("                       : Intgy  = %d \n", pRX->SingleTreeSearchSphereDecoderFxP.Intgy);
				MsgFunc("                       : IntgR  = %d \n", pRX->SingleTreeSearchSphereDecoderFxP.IntgR);
				MsgFunc("                       : Intgn  = %d \n", pRX->SingleTreeSearchSphereDecoderFxP.Intgn);
				MsgFunc("                       : Intgd  = %d \n", pRX->SingleTreeSearchSphereDecoderFxP.Intgd);
				MsgFunc("                       : Intgd2 = %d \n", pRX->SingleTreeSearchSphereDecoderFxP.Intgd2);
				MsgFunc("                       : IntgM  = %d \n", pRX->SingleTreeSearchSphereDecoderFxP.IntgM);
				MsgFunc("                       : Shiftd  = %d \n", pRX->SingleTreeSearchSphereDecoderFxP.Shiftd);
				MsgFunc("                       : Shiftd2  = %d \n", pRX->SingleTreeSearchSphereDecoderFxP.Shiftd2);
			}
            break;
        }
	case 10:
	    MsgFunc("  K-best SD:         K = %d\n", pRX->BFDemapper.K);
	    break;
	case 12:
	    MsgFunc("  List-Fixed SD:     K = %d\n", pRX->BFDemapper.K1);
	    break;
	case 13:
	    if (pRX->BFDemapper.mode == 3 || pRX->BFDemapper.mode == 4) {
		MsgFunc("  Complex SE SD:       K = %d\n", pRX->BFDemapper.K);
		MsgFunc("  Complex SE SD:    Mode = %d\n", pRX->BFDemapper.mode);
		MsgFunc("  Complex SE SD:    Norm = %d\n", pRX->BFDemapper.normtype);
		MsgFunc("  Complex SE SD:   FixPt = %d\n", pRX->BFDemapper.fixpt);
		MsgFunc("  Complex SE SD: DistLim = %d\n", pRX->BFDemapper.distlim);
	    } else {
		MsgFunc("  K-best VT SD:        K = %d\n", pRX->BFDemapper.K);
		MsgFunc("  K-best VT SD:    Mode = %d\n", pRX->BFDemapper.mode);
	    }
	    break;
	}
    MsgFunc(" Decoder Type          = %d: %s\n",pRX->DecoderType, DecoderName[pRX->DecoderType]);
	MsgFunc(" Chanl Tracking Type   = %d \n", pRX->ChanlTracking.Type);
	MsgFunc(" Iterative Decoding    : Method=%d, Iterations=%d\n",pRX->IterationMethod,pRX->TotalIterations);
	
    return WI_SUCCESS;
}
// end of Phy11n_WriteConfig()

// ================================================================================================
// FUNCTION  : Phy11n_ParseLine()
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text for information relevent to this module
// Parameters: text -- A line of text to examine
// ================================================================================================
wiStatus Phy11n_ParseLine(wiString text)
{
    #define PSTATUS(arg) if ((ParseStatus = WICHK(arg)) <= 0) return ParseStatus;

    if (strstr(text,"Phy11n") || strstr(text,"PHY_802_11n"))
    {
        wiStatus ParseStatus;
        char NewText[WIPARSE_MAX_LINE_LENGTH];
        Phy11n_TxState_t *pTX = Phy11n_TxState();
        Phy11n_RxState_t *pRX = Phy11n_RxState();

        if (strstr(text, "PHY_802_11n") == text)
        {
            sprintf(NewText, "Phy11n%s", strstr(text,"PHY_802_11n")+11);
            text = NewText;
        }

        PSTATUS(wiParse_Boolean(text,"Phy11n.ScramblerDisabled",  &(pTX->ScramblerDisabled)));
        PSTATUS(wiParse_Boolean(text,"Phy11n.FixedScramblerState",&(pTX->FixedScramblerState)));
        PSTATUS(wiParse_Int    (text,"Phy11n.IterationMethod",    &(pRX->IterationMethod), 0, 1));

        PSTATUS(wiParse_Int    (text,"Phy11n.aPadFront",          &(pTX->aPadFront), 0, 20000) );
        PSTATUS(wiParse_Int    (text,"Phy11n.aPadBack",           &(pTX->aPadBack),  0, 20000) );

        PSTATUS(wiParse_Int    (text,"Phy11n.MCS",                &(pTX->MCS), 0, 76));
        PSTATUS(wiParse_Int    (text,"Phy11n.N_TX",               &(pTX->N_TX), 0, 4));
        PSTATUS(wiParse_Int    (text,"Phy11n.NumTx",              &(pTX->N_TX), 0, 4));
        PSTATUS(wiParse_Int    (text,"Phy11n.N_RX",               &(pRX->N_RX), 0, 4));
        PSTATUS(wiParse_Int    (text,"Phy11n.STBC",               &(pTX->STBC), 0, 3));
        PSTATUS(wiParse_Int    (text,"Phy11n.DemapperType",       &(pRX->DemapperType), 0, 15)); 
        PSTATUS(wiParse_Int    (text,"Phy11n.DecoderType",        &(pRX->DecoderType), 0, 10));
        PSTATUS(wiParse_Int    (text,"Phy11n.TotalIterations",    &(pRX->TotalIterations), 0, 20));
        PSTATUS(wiParse_Boolean(text,"Phy11n.EnableFrameSync",    &(pRX->EnableFrameSync)));
        PSTATUS(wiParse_Int    (text,"Phy11n.FrameSyncDelay",     &(pRX->FrameSyncDelay),  0, 36));
        PSTATUS(wiParse_Int    (text,"Phy11n.SyncShift",          &(pRX->SyncShift), -16, 16));
        PSTATUS(wiParse_Int    (text,"Phy11n.CFOEstimation",      &(pRX->CFOEstimation), 0, 2));
        PSTATUS(wiParse_Boolean(text,"Phy11n.RandomFrameSyncStart",&(pRX->RandomFrameSyncStart)));
    
        PSTATUS(wiParse_Int    (text,"Phy11n.Beamforming.Mode", NULL, 0, 3));

        PSTATUS(wiParse_Int    (text,"Phy11n.ChanlTracking.Type",       &(pRX->ChanlTracking.Type), 0, 3));
        PSTATUS(wiParse_Real   (text,"Phy11n.ChanlTracking.SoftProbTh", &(pRX->ChanlTracking.SoftProbTh),  0.0, 1.0));

        PSTATUS(wiParse_Int    (text,"Phy11n.PhaseTrackingType",    &(pRX->PhaseTrackingType), 0, 3));
        PSTATUS(wiParse_Boolean(text,"Phy11n.dumpTX", &(pTX->dumpTX)));

        PSTATUS(wiParse_Real   (text,"Phy11n.DPLL.K1", &(pRX->DPLL.K1), 0, 1.0));
        PSTATUS(wiParse_Real   (text,"Phy11n.DPLL.K2", &(pRX->DPLL.K2), 0, 1.0));

        PSTATUS(wiParse_Int    (text,"Phy11n.ListSphereDecoder.NCand",       &(pRX->ListSphereDecoder.NCand),1, 65536)); 
		PSTATUS(wiParse_Real   (text,"Phy11n.ListSphereDecoder.alpha",       &(pRX->ListSphereDecoder.alpha), 0.0, 2000.0)); 
		PSTATUS(wiParse_Real   (text,"Phy11n.ListSphereDecoder.FixedRadius", &(pRX->ListSphereDecoder.FixedRadius), 0.0, 2000.0));
		PSTATUS(wiParse_Boolean(text,"Phy11n.SingleTreeSearchSphereDecoder.GetStatistics",&(pRX->SingleTreeSearchSphereDecoder.GetStatistics))); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoder.LLRClip",      &(pRX->SingleTreeSearchSphereDecoder.LLRClip),0, 1)); 
		PSTATUS(wiParse_Real   (text,"Phy11n.SingleTreeSearchSphereDecoder.LMax",         &(pRX->SingleTreeSearchSphereDecoder.LMax), 0.0, 1000000.0));
        PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoder.ModifiedQR",      &(pRX->SingleTreeSearchSphereDecoder.ModifiedQR),0, 1)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoder.Sorted",      &(pRX->SingleTreeSearchSphereDecoder.Sorted),0, 1)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoder.MMSE",      &(pRX->SingleTreeSearchSphereDecoder.MMSE),0, 1)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoder.Unbiased",      &(pRX->SingleTreeSearchSphereDecoder.Unbiased),0, 1)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoder.Terminated",      &(pRX->SingleTreeSearchSphereDecoder.Terminated),0, 1)); 
		PSTATUS(wiParse_Real   (text,"Phy11n.SingleTreeSearchSphereDecoder.MaxCycles",      &(pRX->SingleTreeSearchSphereDecoder.MaxCycles),0.0, 1000000.0)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoder.Hybrid",      &(pRX->SingleTreeSearchSphereDecoder.Hybrid),0, 1)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoder.BlockSchedule",      &(pRX->SingleTreeSearchSphereDecoder.BlockSchedule),0, 1)); 
		PSTATUS(wiParse_Real   (text,"Phy11n.SingleTreeSearchSphereDecoder.MeanCycles",      &(pRX->SingleTreeSearchSphereDecoder.MeanCycles),0.0, 1000000.0)); 
        PSTATUS(wiParse_Real   (text,"Phy11n.SingleTreeSearchSphereDecoder.MinCycles",      &(pRX->SingleTreeSearchSphereDecoder.MinCycles),0.0, 1000000.0)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoder.Norm1",      &(pRX->SingleTreeSearchSphereDecoder.Norm1),0, 2)); 
		PSTATUS(wiParse_Real   (text,"Phy11n.SingleTreeSearchSphereDecoder.LMax1",         &(pRX->SingleTreeSearchSphereDecoder.LMax1), 0.0, 1000000.0));
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoder.ExactLLR",      &(pRX->SingleTreeSearchSphereDecoder.ExactLLR),0, 1));
	
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoderFxP.Intgy",      &(pRX->SingleTreeSearchSphereDecoderFxP.Intgy),-32, 32)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoderFxP.IntgR",      &(pRX->SingleTreeSearchSphereDecoderFxP.IntgR),-32, 32)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoderFxP.Intgn",      &(pRX->SingleTreeSearchSphereDecoderFxP.Intgn),-32, 32)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoderFxP.Intgd",      &(pRX->SingleTreeSearchSphereDecoderFxP.Intgd),-32, 32)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoderFxP.Intgd2",      &(pRX->SingleTreeSearchSphereDecoderFxP.Intgd2),-32, 32)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoderFxP.IntgM",      &(pRX->SingleTreeSearchSphereDecoderFxP.IntgM),-32, 32)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoderFxP.FLScale",      &(pRX->SingleTreeSearchSphereDecoderFxP.FLScale),0, 4096 )); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoderFxP.Shiftd",      &(pRX->SingleTreeSearchSphereDecoderFxP.Shiftd),-32, 32)); 
		PSTATUS(wiParse_Int    (text,"Phy11n.SingleTreeSearchSphereDecoderFxP.Shiftd2",      &(pRX->SingleTreeSearchSphereDecoderFxP.Shiftd2),-32, 32)); 


		PSTATUS(wiParse_Int    (text,"Phy11n.BFDemapper.K",       &(pRX->BFDemapper.K), 2, 64));
		PSTATUS(wiParse_Int    (text,"Phy11n.BFDemapper.K1",      &(pRX->BFDemapper.K1), 2, 256));
		PSTATUS(wiParse_Int    (text,"Phy11n.BFDemapper.mode",    &(pRX->BFDemapper.mode), 0, 255));
		PSTATUS(wiParse_Int    (text,"Phy11n.BFDemapper.norm",    &(pRX->BFDemapper.normtype), 1, 2));
		PSTATUS(wiParse_Int    (text,"Phy11n.BFDemapper.fixpt",   &(pRX->BFDemapper.fixpt), 0, 12));
		PSTATUS(wiParse_Int    (text,"Phy11n.BFDemapper.distlim", &(pRX->BFDemapper.distlim), 0, 12));
		PSTATUS(wiParse_Int    (text,"Phy11n.BFDemapper.TestVecSymbols", &(pRX->BFDemapper.TestVecSymbols), 0, 65535));
		PSTATUS(wiParse_String (text,"Phy11n.BFDemapper.TestVecFile", pRX->BFDemapper.TestVecFile, PHY11N_BF_MAX_FILENAME));

        // Obsolete parameters
        PSTATUS(wiParse_Int    (text,"Phy11n.TX.>M", NULL, 1, 4));
        PSTATUS(wiParse_Int    (text,"Phy11n.CHTR_Itr_Init", NULL, 0, 1));

        // Obsolete names
        PSTATUS(wiParse_Int    (text,"Phy11n.PH_Tracking_Type",     &(pRX->PhaseTrackingType), 0, 3));
        PSTATUS(wiParse_Int    (text,"Phy11n.CFO_Ctrl",             &(pRX->CFOEstimation), 0, 2));
        PSTATUS(wiParse_Boolean(text,"Phy11n.FrameSync_Ctrl",       &(pRX->EnableFrameSync)));
        PSTATUS(wiParse_Int    (text,"Phy11n.FrameSync_Delay",      &(pRX->FrameSyncDelay),  0, 36));
        PSTATUS(wiParse_Int    (text,"Phy11n.Iteration_Method",     &(pRX->IterationMethod), 0, 1));
        PSTATUS(wiParse_Int    (text,"Phy11n.MIMO_Demapper_Type",   &(pRX->DemapperType), 0, 13)); 
        PSTATUS(wiParse_Int    (text,"Phy11n.Decoder_Type",         &(pRX->DecoderType), 0, 10));
        PSTATUS(wiParse_Int    (text,"Phy11n.itr_tot",              &(pRX->TotalIterations), 0, 20));
        PSTATUS(wiParse_Int    (text,"Phy11n.CHTR_Type",            &(pRX->ChanlTracking.Type), 0, 3));
        PSTATUS(wiParse_Real   (text,"Phy11n.CHTR_Soft_ProbTH",     &(pRX->ChanlTracking.SoftProbTh),  0.0, 1.0));

    }
    return WI_WARN_PARSE_FAILED;
}
// end of Phy11n_ParseLine()


// ================================================================================================
// FUNCTION  : Phy11n()
// ------------------------------------------------------------------------------------------------
// Purpose   : Access function for PHY method
// ================================================================================================
wiStatus Phy11n(int Command, void *pData)
{
    static int AddMethodCount = 0;

    switch (Command & 0xFFFF0000)
    {
        case WIPHY_WRITE_CONFIG:
        {
            wiMessageFn MsgFunc = (wiMessageFn)*((wiFunction *)pData);
            XSTATUS(Phy11n_WriteConfig(MsgFunc));
            break;
        }

        case WIPHY_ADD_METHOD:
        {
            Phy11n_TxState_t *pTX = Phy11n_TxState();
            Phy11n_RxState_t *pRX = Phy11n_RxState();
            wiPhyMethod_t *method = (wiPhyMethod_t *)pData;
            method->TxFn = Phy11n_TX;
            method->RxFn = Phy11n_RX;

            if (AddMethodCount == 0)
            {
                XSTATUS(wiParse_AddMethod(Phy11n_ParseLine));
                XSTATUS(Phy11n_RESET());

                pRX->SyncShift =  -4; // default parameters
                pTX->N_TX      =   1;
            }
            TX[0].ThreadIsInitialized = RX[0].ThreadIsInitialized = WI_TRUE;
            AddMethodCount++;
            break;
        }

        case WIPHY_REMOVE_METHOD:
        {
            if (AddMethodCount == 1)
            {
                int i;
                for (i=0; i<WISE_MAX_THREADS; i++)
                {
                    XSTATUS( Phy11n_TX_FreeMemory(Phy11n_TxState()+i, WI_FALSE) );
                    XSTATUS( Phy11n_RX_FreeMemory(Phy11n_RxState()+i, WI_FALSE) );
                }
                XSTATUS( wiParse_RemoveMethod(Phy11n_ParseLine) );
            }
            AddMethodCount--;
            break;
        }
        case WIPHY_RESET: 
            XSTATUS(Phy11n_RESET()); 
            break;

        case WIPHY_SET_DATA: 
        {
            if ((Command & 0xFFFF) == WIPHY_RX_NUM_PATHS)
            {
                Phy11n_RxState()->N_RX = *(int *)pData;
            }
            else return WI_WARN_METHOD_UNHANDLED_PARAMETER;
            break;
        }
        case WIPHY_GET_DATA: 
        {
            if ((Command & 0xFFFF) == WIPHY_TX_DATARATE_BPS)
            {
                const wiReal Mbps[8] = {6.5, 13, 19.5, 26, 39, 52, 58.5, 65};
                int MCS = Phy11n_TxState()->MCS;

                if (MCS < 32)
                    *((wiReal *)pData) = 1.0e6 * Mbps[MCS%8] * (1.0 + floor(MCS/8.0));
            }
            else return WI_WARN_METHOD_UNHANDLED_PARAMETER;
            break;
        }
        case WIPHY_CLOSE_ALL_THREADS:
        {
            unsigned int n;
            for (n=1; n<WISE_MAX_THREADS; n++)
                XSTATUS( Phy11n_CloseThread(n) );
            break;
        }
        case WIPHY_START_TEST:
        {
            int NumberOfThreads = *(int *)pData;
            switch (Phy11n_RxState()->DemapperType) 
            {
		        case 10: case 11: case 12: case 13:
		            XSTATUS(Phy11n_BFDemapper_Start(NumberOfThreads)); 
                    break;
            }
	        break;
        }
        case WIPHY_END_TEST:
        {
            wiMessageFn MsgFunc = (wiMessageFn)*((wiFunction *)pData);
  
            switch (Phy11n_RxState()->DemapperType) 
            {
		        case 10: case 11: case 12: case 13:
		            XSTATUS(Phy11n_BFDemapper_End(MsgFunc)); 
                    break;
            }
            break;
        }
        default: return WI_WARN_METHOD_UNHANDLED_COMMAND;
    }
    return WI_SUCCESS;
}
// end of Phy11n()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
