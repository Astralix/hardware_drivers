//--< Verona_TX_DPD.c >----------------------------------------------------------------------------
//=================================================================================================
//
//  Verona - Digital TX Predistortion (Nonlinear Equalizer)
//  Copyright 2010-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <string.h>
#include "wiUtil.h"
#include "wiParse.h"
#include "Verona.h"
#include "Verona_dpdBlocks.h"

//=================================================================================================
//--  LOCAL FUNCTION PROTOTPYES
//=================================================================================================
static int Verona_wiDataRateDecoder(int, int, int, int, int);
static int Verona_dpdgetFilterBank(int *);

// ================================================================================================
// FUNCTION : Verona_TX_DPD()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit Digital Predistortion
// Parameters: N      -- length of input/output arrays (model input)
//             xR, xI -- real/imaginary input
//             yR, yI -- real.imaginary output
// ================================================================================================
void Verona_TX_DPD(int N, wiWORD xR[], wiWORD xI[], wiWORD *yR, wiWORD *yI)
{
    int k;
	dpdUInt pr;
	int bank_selector, dpd_state;
	int inp_limit;

	dpdComplexInt *DPD_IN;
	dpdComplexInt *DPD_OUT;

    // VeronaRegisters_t *pReg = Verona_Registers();

	pr = Verona_get_dpdBlock(0)->VFP.dpdHigherOrder.dpd_in_precision;
	inp_limit = (1<<(pr-1))-1;


	bank_selector = Verona_dpdgetFilterBank(&dpd_state);			// Select filter bank
	// disable
	if (!dpd_state)
	{
		for (k=0; k<N; k++)
		{
			yR[k].word = 0;
			yI[k].word = 0;
		}
		return;
			
	}

	// transparaent mode
	if (dpd_state == 2)
	{
		memcpy(yR, xR, N*sizeof(wiWORD));
		memcpy(yI, xI, N*sizeof(wiWORD));
		return;
	}

	DPD_IN  = (dpdComplexInt *)wiCalloc(N, sizeof(dpdComplexInt));
	DPD_OUT = (dpdComplexInt *)wiCalloc(N, sizeof(dpdComplexInt));

	Verona_dpdBlock_CleanFilters(0);					// Clean filters
	Verona_dpdBlock_SetFilterCoeff(0, bank_selector);	// Set coefficients for the current packet

	// Convert to complex vector
    for (k=0; k<N; k++)
    {
		DPD_SET_COMPLEX_INT_PR(&DPD_IN[k], Verona_SATURATE(xR[k].word, inp_limit), 
                               Verona_SATURATE(xI[k].word, inp_limit), pr);
    }

	// Predistorion
	Verona_dpdVFP(0, DPD_IN, N, DPD_OUT);

    for (k=0; k<VERONA_DPD_DELAY; k++)
        yR[k].word = yI[k].word = 0;
    
    // Convert to real vectors
	for (k=VERONA_DPD_DELAY; k<N; k++)
	{
		yR[k].word = DPD_COMPLEX_REAL(DPD_OUT[k - VERONA_DPD_DELAY]);
		yI[k].word = DPD_COMPLEX_IMAG(DPD_OUT[k - VERONA_DPD_DELAY]);
	}

	wiFree(DPD_OUT);
	wiFree(DPD_IN);
}
// end of Verona_TX_DPD()

// ================================================================================================
// FUNCTION : Verona_TX_DPD_Init()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_TX_DPD_Init(void)
{
	if (Verona_dpdBlock_Init(0)) return WI_SUCCESS;
	else return WI_ERROR;
}
// end of Verona_TX_DPD_Init()

// ================================================================================================
// FUNCTION : Verona_TX_DPD_Close()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Verona_TX_DPD_Close(void)
{
	if (Verona_dpdBlock_Close(0)) return WI_SUCCESS;
	else return WI_ERROR;
}
// end of Verona_TX_DPD_Close()

// ================================================================================================
// ================================================================================================
static int Verona_wiDataRateDecoder(int codeword, int TxMode, int rgf_Tx_Rate_OFDM_HT, int rgf_Tx_Rate_OFDM_LL, int rgf_Tx_Rate_OFDM_LM)
{
	int HighRate = 0;
	int Tx_Rate_OFDM_L;

	switch (TxMode)
	{
	case 2: HighRate = 0; break;
	case 3: HighRate = (rgf_Tx_Rate_OFDM_HT>>codeword)&0x1; break;
	case 0:
		Tx_Rate_OFDM_L = (rgf_Tx_Rate_OFDM_LM<<8) | rgf_Tx_Rate_OFDM_LL;
		HighRate = (Tx_Rate_OFDM_L>>codeword)&0x1; break;
	}
	return HighRate;
}


// ================================================================================================
// ================================================================================================
static int Verona_dpdgetFilterBank(int *dpd_state)
{
	int HighRate;
	int TxMode;
	int dpd_pa_tmp;
	int rgf_pa_tmp_ext;
	int pa_tmp_eff;
	int rgf_cross_pwr1;
	int rgf_cross_pwr2;
	int rgf_cross_pwr3;

	int rgf_Tx_Rate_OFDM_HT, rgf_Tx_Rate_OFDM_LL, rgf_Tx_Rate_OFDM_LM;
	int rgf_low_rate_dis;
	int dpd_coef_inte;
	int TxPwrLvl;

	int ctrl_coef_sel = 0;

	VeronaRegisters_t *pReg = Verona_Registers();
	Verona_TxState_t  *pTX  = Verona_TxState();

	// Extracting the values
	TxMode = pTX->TxMode;
	dpd_pa_tmp = pReg->DPD_PA_tmp.word;
	rgf_pa_tmp_ext = pReg->Pa_tmp_ext.word;
	rgf_cross_pwr1 = pReg->cross_pwr1.word;
	rgf_cross_pwr2 = pReg->cross_pwr2.word;
	rgf_cross_pwr3 = pReg->cross_pwr3.word;

	rgf_Tx_Rate_OFDM_HT = pReg->Tx_Rate_OFDM_HT.word;
	rgf_Tx_Rate_OFDM_LL = pReg->Tx_Rate_OFDM_LL.word;
	rgf_Tx_Rate_OFDM_LM = pReg->Tx_Rate_OFDM_LM.word;

	rgf_low_rate_dis = pReg->DPD_LOW_RATE_DIS.w1;
	dpd_coef_inte = pReg->DPD_coef_interspersion.w2;
    TxPwrLvl = pReg->TxHeader1.w6;
	
	HighRate = Verona_wiDataRateDecoder(pTX->RATE.word, TxMode, rgf_Tx_Rate_OFDM_HT, rgf_Tx_Rate_OFDM_LL, rgf_Tx_Rate_OFDM_LM);
	

	// Select HW/SW temperature sensor
	pa_tmp_eff = rgf_pa_tmp_ext ?  pTX -> LgSigAFE : dpd_pa_tmp;


	switch (dpd_coef_inte)
	{
	case 0:
		if (TxPwrLvl <= rgf_cross_pwr1)
			ctrl_coef_sel = 0;
		else if (TxPwrLvl <= rgf_cross_pwr2)
			ctrl_coef_sel = 1;
		else if (TxPwrLvl <= rgf_cross_pwr3)
			ctrl_coef_sel = 2;
		else
			ctrl_coef_sel = 3;
		break;

	case 1:
		if ((TxPwrLvl<=rgf_cross_pwr1) && (!HighRate))
			ctrl_coef_sel = 0;
		else if ((TxPwrLvl>rgf_cross_pwr1) && (!HighRate))
			ctrl_coef_sel = 1;
		else if ((TxPwrLvl<=rgf_cross_pwr2) && HighRate)
			ctrl_coef_sel = 2;
		else if ((TxPwrLvl>rgf_cross_pwr2) && HighRate)
			ctrl_coef_sel = 3;
		break;

	case 2:
		if ((TxPwrLvl<=rgf_cross_pwr1) && (pa_tmp_eff==0))
			ctrl_coef_sel = 0;
		else if ((TxPwrLvl>rgf_cross_pwr1) && (pa_tmp_eff==0))
			ctrl_coef_sel = 1;
		else if ((TxPwrLvl<=rgf_cross_pwr2) && pa_tmp_eff)
			ctrl_coef_sel = 2;
		else if ((TxPwrLvl>rgf_cross_pwr2) && pa_tmp_eff)
			ctrl_coef_sel = 3;
		break;

	case 3:
		if (TxMode==2)
			ctrl_coef_sel = 3;
		else if (TxPwrLvl<=rgf_cross_pwr1)
			ctrl_coef_sel = 0;
		else if (TxPwrLvl<=rgf_cross_pwr2)
			ctrl_coef_sel = 1;
		else
			ctrl_coef_sel = 2;
		break;
	}


	// 0 - disabled, 1 - normal, 2 - overwrite
	*dpd_state = 1;
	// case 1
	if (pReg->DPD_LOW_TMP_DIS.w1 && (pa_tmp_eff))
	{
		*dpd_state = 2;
		// return pReg->Cbc_coef_sel.word = ctrl_coef_sel;
	}
	// case 2 & 3
	if (rgf_low_rate_dis)
	{
		if (TxMode == 2)
		{
			*dpd_state = 2;
			// return pReg->Cbc_coef_sel.word = ctrl_coef_sel;
		}
		else
		{
			if (!HighRate)
			{
				*dpd_state = 2;
				// return pReg->Cbc_coef_sel.word = ctrl_coef_sel;
			}
		}		
	}

	if ((dpd_coef_inte != 0) && (TxPwrLvl <= rgf_cross_pwr3))
	{
		*dpd_state = 2;
		// return pReg->Cbc_coef_sel.word = ctrl_coef_sel;
	}

	// ctrl_coef_sel = 0;
	if (pReg->cnfg_eovr.w1) ctrl_coef_sel = pReg->Coef_sel_vovr.word;
	if (pReg->Vfb_byp_vovr.w1) *dpd_state = 2;
	
	return pReg->Cbc_coef_sel.word = ctrl_coef_sel;
}
//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
